/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include "hardinfo.h"
#include "devices.h"

#include "arm_data.h"
#include "arm_data.c"

enum {
    ARM_A32 = 0,
    ARM_A64 = 1,
    ARM_A32_ON_A64 = 2,
};

static const gchar *arm_mode_str[] = {
    "A32",
    "A64",
    "A32 on A64",
};

static gchar* get_cpu_str(const gchar* file, gint cpuid) {
    gchar *tmp0 = NULL;
    gchar *tmp1 = NULL;
    tmp0 = g_strdup_printf("/sys/devices/system/cpu/cpu%d/%s", cpuid, file);
    g_file_get_contents(tmp0, &tmp1, NULL, NULL);
    g_free(tmp0);
    return tmp1;
}

static gint get_cpu_int(const char* item, int cpuid) {
    gchar *fc = NULL;
    int ret = 0;
    fc = get_cpu_str(item, cpuid);
    if (fc) {
        ret = atol(fc);
        g_free(fc);
    }
    return ret;
}

int processor_has_flag(gchar * strflags, gchar * strflag)
{
    gchar **flags;
    gint ret = 0;
    if (strflags == NULL || strflag == NULL)
        return 0;
    flags = g_strsplit(strflags, " ", 0);
    ret = g_strv_contains((const gchar * const *)flags, strflag);
    g_strfreev(flags);
    return ret;
}

#define PROC_CPUINFO "/proc/cpuinfo"

GSList *
processor_scan(void)
{
    GSList *procs = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[128];
    gchar *rep_pname = NULL;
    gchar *tmpfreq_str = NULL;
    GSList *pi = NULL;

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
    return NULL;

#define CHECK_FOR(k) (g_str_has_prefix(tmp[0], k))
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);
        if (tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);
        } else {
            g_strfreev(tmp);
            continue;
        }

        get_str("Processor", rep_pname);

        if ( CHECK_FOR("processor") ) {
            /* finish previous */
            if (processor) {
                procs = g_slist_append(procs, processor);
            }

            /* start next */
            processor = g_new0(Processor, 1);
            processor->id = atol(tmp[1]);

            if (rep_pname)
                processor->model_name = g_strdup(rep_pname);

            g_strfreev(tmp);
            continue;
        }

        if (!processor &&
            (  CHECK_FOR("model name")
            || CHECK_FOR("Features")
            || CHECK_FOR("BogoMIPS") ) ) {

            /* single proc/core may not have "processor : n" */
            processor = g_new0(Processor, 1);
            processor->id = 0;

            if (rep_pname)
                processor->model_name = g_strdup(rep_pname);
        }

        if (processor) {
            get_str("model name", processor->model_name);
            get_str("Features", processor->flags);
            get_float("BogoMIPS", processor->bogomips);

            get_str("CPU implementer", processor->cpu_implementer);
            get_str("CPU architecture", processor->cpu_architecture);
            get_str("CPU variant", processor->cpu_variant);
            get_str("CPU part", processor->cpu_part);
            get_str("CPU revision", processor->cpu_revision);
        }
        g_strfreev(tmp);
    }

    if (processor)
        procs = g_slist_append(procs, processor);

    g_free(rep_pname);
    fclose(cpuinfo);

    /* re-duplicate missing data for /proc/cpuinfo variant that de-duplicated it */
#define REDUP(f) if (dproc->f && !processor->f) processor->f = g_strdup(dproc->f);
    Processor *dproc;
    GSList *l;
    l = procs = g_slist_reverse(procs);
    while (l) {
        processor = l->data;
        if (processor->flags) {
            dproc = processor;
        } else if (dproc) {
            REDUP(flags);
            REDUP(cpu_implementer);
            REDUP(cpu_architecture);
            REDUP(cpu_variant);
            REDUP(cpu_part);
            REDUP(cpu_revision);
        }
        l = g_slist_next(l);
    }
    procs = g_slist_reverse(procs);

    /* data not from /proc/cpuinfo */
    for (pi = procs; pi; pi = pi->next) {
        processor = (Processor *) pi->data;

        /* strings can't be null or segfault later */
#define STRIFNULL(f,cs) if (processor->f == NULL) processor->f = g_strdup(cs);
#define UNKIFNULL(f) STRIFNULL(f, "(Unknown)")
#define EMPIFNULL(f) STRIFNULL(f, "")
        STRIFNULL(model_name, "ARM Processor");
        EMPIFNULL(flags);
        UNKIFNULL(cpu_implementer);
        UNKIFNULL(cpu_architecture);
        UNKIFNULL(cpu_variant);
        UNKIFNULL(cpu_part);
        UNKIFNULL(cpu_revision);

        processor->decoded_name = arm_decoded_name(
            processor->cpu_implementer, processor->cpu_part,
            processor->cpu_variant, processor->cpu_revision,
            processor->cpu_architecture, processor->model_name);
        UNKIFNULL(decoded_name);

        /* topo */
        processor->package_id = get_cpu_str("topology/physical_package_id", processor->id);
        processor->core_id = get_cpu_str("topology/core_id", processor->id);
        UNKIFNULL(package_id);
        UNKIFNULL(core_id);

        /* freq */
        processor->scaling_driver = get_cpu_str("cpufreq/scaling_driver", processor->id);
        processor->scaling_governor = get_cpu_str("cpufreq/scaling_governor", processor->id);
        UNKIFNULL(scaling_driver);
        UNKIFNULL(scaling_governor);
        processor->transition_latency = get_cpu_int("cpufreq/cpuinfo_transition_latency", processor->id);
        processor->cpukhz_cur = get_cpu_int("cpufreq/scaling_cur_freq", processor->id);
        processor->cpukhz_min = get_cpu_int("cpufreq/scaling_min_freq", processor->id);
        processor->cpukhz_max = get_cpu_int("cpufreq/scaling_max_freq", processor->id);
        if (processor->cpukhz_max)
            processor->cpu_mhz = processor->cpukhz_max / 1000;
        else
            processor->cpu_mhz = 0.0f;

        /* mode */
        processor->mode = ARM_A32;
        if ( processor_has_flag(processor->flags, "pmull")
             || processor_has_flag(processor->flags, "crc32") ) {
#ifdef __aarch64__
                processor->mode = ARM_A64;
#else
                processor->mode = ARM_A32_ON_A64;
#endif
        }
    }

    return procs;
}

gchar *processor_get_capabilities_from_flags(gchar * strflags)
{
    gchar **flags, **old;
    gchar *tmp = NULL;
    gint j = 0;

    flags = g_strsplit(strflags, " ", 0);
    old = flags;

    while (flags[j]) {
        const gchar *meaning = arm_flag_meaning( flags[j] );

        if (meaning) {
            tmp = h_strdup_cprintf("%s=%s\n", tmp, flags[j], meaning);
        } else {
            tmp = h_strdup_cprintf("%s=\n", tmp, flags[j]);
        }
        j++;
    }
    if (tmp == NULL || g_strcmp0(tmp, "") == 0)
        tmp = g_strdup_printf("%s=%s\n", "empty", _("Empty List"));

    g_strfreev(old);
    return tmp;
}

gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_flags, *tmp_imp, *tmp_part, *tmp_arch, *ret;
    tmp_flags = processor_get_capabilities_from_flags(processor->flags);
    tmp_imp = (char*)arm_implementer(processor->cpu_implementer);
    tmp_part = (char*)arm_part(processor->cpu_implementer, processor->cpu_part);
    tmp_arch = (char*)arm_arch_more(processor->cpu_architecture);

    ret = g_strdup_printf("[Processor]\n"
                           "Linux Name=%s\n"
                           "Decoded Name=%s\n"
                           "Mode=%s\n"
                   "BogoMips=%.2f\n"
                   "Endianesss="
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                       "Little Endian"
#else
                       "Big Endian"
#endif
                       "\n"
                       "[Topology]\n"
                       "ID=%d\n"
                       "Socket=%s\n"
                       "Core=%s\n"
                       "[Frequency Scaling]\n"
                       "Minimum=%d kHz\n"
                       "Maximum=%d kHz\n"
                       "Current=%d kHz\n"
                       "Transition Latency=%d ns\n"
                       "Governor=%s\n"
                       "Driver=%s\n"
                       "[ARM]\n"
                       "Implementer=[%s] %s\n"
                       "Part=[%s] %s\n"
                       "Architecture=[%s] %s\n"
                       "Variant=%s\n"
                       "Revision=%s\n"
                       "[Capabilities]\n"
                       "%s"
                       "%s",
                   processor->model_name,
                   processor->decoded_name,
                   arm_mode_str[processor->mode],
                   processor->bogomips,
                   processor->id,
                   processor->package_id,
                   processor->core_id,
                   processor->cpukhz_min,
                   processor->cpukhz_max,
                   processor->cpukhz_cur,
                   processor->transition_latency,
                   processor->scaling_governor,
                   processor->scaling_driver,
                   processor->cpu_implementer, (tmp_imp) ? tmp_imp : "",
                   processor->cpu_part, (tmp_part) ? tmp_part : "",
                   processor->cpu_architecture, (tmp_arch) ? tmp_arch : "",
                   processor->cpu_variant,
                   processor->cpu_revision,
                   tmp_flags,
                    "");
    g_free(tmp_flags);

    return ret;
}

gchar *processor_get_info(GSList * processors)
{
    Processor *processor;

    if (g_slist_length(processors) > 1) {
    gchar *ret, *tmp, *hashkey;
    GSList *l;

    tmp = g_strdup("");

    for (l = processors; l; l = l->next) {
        processor = (Processor *) l->data;

        tmp = g_strdup_printf(_("%s$CPU%d$%s=%.2fMHz\n"),
                  tmp, processor->id,
                  processor->model_name,
                  processor->cpu_mhz);

        hashkey = g_strdup_printf("CPU%d", processor->id);
        moreinfo_add_with_prefix("DEV", hashkey,
                processor_get_detailed_info(processor));
           g_free(hashkey);
    }

    ret = g_strdup_printf("[$ShellParam$]\n"
                  "ViewType=1\n"
                  "[Processors]\n"
                  "%s", tmp);
    g_free(tmp);

    return ret;
    }

    processor = (Processor *) processors->data;
    return processor_get_detailed_info(processor);
}
