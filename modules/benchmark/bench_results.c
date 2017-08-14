/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    This file:
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "cpu_util.h"

typedef struct {
    char *board;
    int memory_kiB;
    char *cpu_name;
    char *cpu_desc;
    char *cpu_config;
    int threads;
    char *mid;
} simple_machine;

typedef struct {
    char *name;
    float result;
    int threads;
    simple_machine *machine;
    int legacy; /* an old benchmark.conf result */
} bench_result;

/* "2x 1400.00 MHz + 2x 800.00 MHz" -> 4400.0 */
static float cpu_config_val(char *str) {
    char *c = str;
    int t;
    float f, r = 0.0;
    if (str != NULL) {
        if (strchr(str, 'x')) {
            while (c != NULL && sscanf(c, "%dx %f", &t, &f) ) {
                r += f * t;
                c = strchr(c+1, '+');
            }
        } else {
            sscanf(c, "%f", &r);
        }
    }
    return r;
}

static int cpu_config_cmp(char *str0, char *str1) {
    float r0, r1;
    r0 = cpu_config_val(str0);
    r1 = cpu_config_val(str1);
    if (r0 == r1) return 0;
    if (r0 < r1) return -1;
    return 1;
}

static int cpu_config_is_close(char *str0, char *str1) {
    float r0, r1, r1n;
    r0 = cpu_config_val(str0);
    r1 = cpu_config_val(str1);
    r1n = r1 * .9;
    if (r0 > r1n && r0 < r1)
        return 1;
    return 0;
}

static gen_machine_id(simple_machine *m) {
    char *s;
    if (m) {
        if (m->mid != NULL)
            free(m->mid);
        m->mid = g_strdup_printf("%s;%s;%.2f",
            m->board, m->cpu_name, cpu_config_val(m->cpu_config) );
        s = m->mid;
        while (*s != 0) {
            if (!isalnum(*s)) {
                if (*s != ';'
                    && *s != '('
                    && *s != '('
                    && *s != ')')
                *s = '_';
            }
            s++;
        }
    }
}

simple_machine *simple_machine_new() {
    simple_machine *m = NULL;
    m = malloc(sizeof(simple_machine));
    if (m)
        memset(m, 0, sizeof(simple_machine));
    return m;
}

simple_machine *simple_machine_this() {
    simple_machine *m = NULL;
    char *tmp;
    m = simple_machine_new();
    if (m) {
        m->board = module_call_method("devices::getMotherboard");
        m->cpu_name = module_call_method("devices::getProcessorName");
        m->cpu_desc = module_call_method("devices::getProcessorDesc");
        m->cpu_config = module_call_method("devices::getProcessorFrequencyDesc");
        tmp = module_call_method("devices::getMemoryTotal");
        m->memory_kiB = atoi(tmp);
        free(tmp);
        tmp = module_call_method("devices::getProcessorCount");
        m->threads = atoi(tmp);
        free(tmp);
        gen_machine_id(m);
    }
    return m;
}

void simple_machine_free(simple_machine *s) {
    if (s) {
        free(s->board);
        free(s->cpu_name);
        free(s->cpu_desc);
        free(s->cpu_config);
        free(s->mid);
    }
}

void bench_result_free(bench_result *s) {
    if (s) {
        free(s->name);
        simple_machine_free(s->machine);
    }
}

bench_result *bench_result_this_machine(const char *bench_name, float result, int threads) {
    bench_result *b = NULL;

    b = malloc(sizeof(bench_result));
    if (b) {
        memset(b, 0, sizeof(bench_result));
        b->machine = simple_machine_this();
        b->name = strdup(bench_name);
        b->result = result;
        b->threads = threads;
        b->legacy = 0;
    }
    return b;
}

/* -1 for none */
static int nx_prefix(const char *str) {
    char *s, *x;
    if (str != NULL) {
        s = (char*)str;
        x = strchr(str, 'x');
        if (x && x-s >= 1) {
            while(s != x) {
                if (!isdigit(*s))
                    return -1;
                s++;
            }
            *x = 0;
            return atoi(str);
        }
    }
    return -1;
}

bench_result *bench_result_benchmarkconf(const char *section, const char *key, char **values) {
    bench_result *b = NULL;
    char *s0, *s1, *s2;
    int nx = 0, vl = 0;
    float n, m;

    vl = g_strv_length(values);

    b = malloc(sizeof(bench_result));
    if (b) {
        memset(b, 0, sizeof(bench_result));
        b->machine = simple_machine_new();
        b->name = strdup(section);

        if (vl >= 8) {
            b->machine->mid = strdup(key);
            b->result = atof(values[0]);
            b->threads = atoi(values[1]);
            b->machine->board = strdup(values[2]);
            b->machine->cpu_name = strdup(values[3]);
            b->machine->cpu_desc = strdup(values[4]);
            b->machine->cpu_config = strdup(values[5]);
            b->machine->memory_kiB = atoi(values[6]);
            b->machine->threads = atoi(values[7]);
            b->legacy = 0;
        } else if (vl >= 2) {
            b->result = atof(values[0]);
            b->legacy = 1;

            /* old old format has prefix before cpu name (ex: 4x Pentium...) */
            nx = nx_prefix(key);
            if (nx > 0) {
                b->machine->cpu_name = strdup(strchr(key, 'x') + 1);
                b->machine->threads = nx;
                b->threads = nx;
            } else {
                b->machine->cpu_name = strdup(key);
                b->machine->threads = 1;
                b->threads = 1;
            }

            b->machine->cpu_config = strdup(values[1]);
            /* new old format has cpu_config string with nx prefix */
            nx = nx_prefix(values[1]);
            if (nx > 0) {
                b->machine->threads = nx;
                b->threads = nx;
            }

            /* If the clock rate in the id string is more than the
             * config string, use that. Older hardinfo used current cpu freq
             * instead of max freq.
             * "...@ 2.00GHz" -> 2000.0 */
            s0 = b->machine->cpu_name;
            s2 = strstr(s0, "Hz");
            if (s2 && s2 > s0 + 2) {
                m = 1; /* assume M */
                if (*(s2-1) == 'G')
                    m = 1000;
                s1 = s2 - 2;
                while (s1 > s0) {
                    if (!( isdigit(*s1) || *s1 == '.' || *s1 == ' '))
                        break;
                    s1--;
                }

                if (s1 > s0) {
                    n = atof(s1+1);
                    n *= m;

                    s1 = g_strdup_printf("%dx %.2f %s", b->threads, n, _("MHz"));
                    if ( cpu_config_cmp(b->machine->cpu_config, s1) == -1
                         && !cpu_config_is_close(b->machine->cpu_config, s1) ) {
                        free(b->machine->cpu_config);
                        b->machine->cpu_config = s1;
                    } else {
                        free(s1);
                    }
                }
            }
        }
        UNKIFNULL(b->machine->board);
        UNKIFNULL(b->machine->cpu_desc);
        gen_machine_id(b->machine);
    }
    return b;
}

char *bench_result_benchmarkconf_line(bench_result *b) {
    return g_strdup_printf("%s=%.2f|%d|%s|%s|%s|%s|%d|%d\n",
            b->machine->mid, b->result, b->threads,
            b->machine->board, b->machine->cpu_name,
            b->machine->cpu_desc, b->machine->cpu_config,
            b->machine->memory_kiB, b->machine->threads );
}

char *bench_result_more_info(bench_result *b) {
    return g_strdup_printf("[%s]\n"
        /* threads */   "%s=%d\n"
        /* legacy */    "%s=%s\n"
                        "[%s]\n"
        /* board */     "%s=%s\n"
        /* cpu   */     "%s=%s\n"
        /* cpudesc */   "%s=%s\n"
        /* cpucfg */    "%s=%s\n"
        /* threads */   "%s=%d\n"
        /* mem */       "%s=%d %s\n",
                        _("Benchmark Result"),
                        _("Threads"), b->threads,
                        b->legacy ? _("Note") : "#Note",
                        b->legacy ? _("This result is from an old version of HardInfo. Results might not be comparable to current version. Some details are missing.") : "",
                        _("Machine"),
                        _("Board"), b->machine->board,
                        _("CPU Name"), b->machine->cpu_name,
                        _("CPU Description"), b->machine->cpu_desc,
                        _("CPU Config"), b->machine->cpu_config,
                        _("Threads Available"), b->machine->threads,
                        _("Memory"), b->machine->memory_kiB, _("kiB")
                        );
}

char *bench_result_more_info_complete(bench_result *b) {
    return g_strdup_printf("[%s]\n"
        /* bench name */"%s=%s\n"
        /* result */    "%s=%0.2f\n"
        /* threads */   "%s=%d\n"
        /* legacy */    "%s=%s\n"
                        "[%s]\n"
        /* board */     "%s=%s\n"
        /* cpu   */     "%s=%s\n"
        /* cpudesc */   "%s=%s\n"
        /* cpucfg */    "%s=%s\n"
        /* threads */   "%s=%d\n"
        /* mem */       "%s=%d %s\n"
                        "[%s]\n"
        /* mid */       "%s=%s\n"
        /* cfg_val */   "%s=%.2f\n",
                        _("Benchmark Result"),
                        _("Benchmark"), b->name,
                        _("Result"), b->result,
                        _("Threads"), b->threads,
                        b->legacy ? _("Note") : "#Note",
                        b->legacy ? _("This result is from an old version of Hardinfo.") : "",
                        _("Machine"),
                        _("Board"), b->machine->board,
                        _("CPU Name"), b->machine->cpu_name,
                        _("CPU Description"), b->machine->cpu_desc,
                        _("CPU Config"), b->machine->cpu_config,
                        _("Threads Available"), b->machine->threads,
                        _("Memory"), b->machine->memory_kiB, _("kiB"),
                        _("Handles"),
                        _("mid"), b->machine->mid,
                        _("cfg_val"), cpu_config_val(b->machine->cpu_config)
                        );
}
