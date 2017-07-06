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
            (  CHECK_FOR("cpu")
            || CHECK_FOR("clock")
            || CHECK_FOR("revision") ) ) {

            /* single proc/core may not have "processor : n" */
            processor = g_new0(Processor, 1);
            processor->id = 0;

            if (rep_pname)
                processor->model_name = g_strdup(rep_pname);
        }

        if (processor) {
            get_str("cpu", processor->model_name);
            get_str("revision", processor->revision);
            get_float("clock", processor->cpu_mhz);
            get_float("BogoMIPS", processor->bogomips);
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
        if (processor->model_name) {
            dproc = processor;
        } else if (dproc) {
            REDUP(model_name);
            REDUP(revision);
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
        STRIFNULL(model_name, "POWER Processor");
        UNKIFNULL(revision);

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

    }

    return procs;
}


gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_cpufreq, *tmp_topology, *ret;

    tmp_topology = g_strdup_printf(
                    "[Topology]\n"
                    "ID=%d\n"
                    "Socket=%s\n"
                    "Core=%s\n",
                   processor->id,
                   processor->package_id,
                   processor->core_id);

    if (processor->cpukhz_min || processor->cpukhz_max || processor->cpukhz_cur) {
        tmp_cpufreq = g_strdup_printf(
                    "[Frequency Scaling]\n"
                    "Minimum=%d kHz\n"
                    "Maximum=%d kHz\n"
                    "Current=%d kHz\n"
                    "Transition Latency=%d ns\n"
                    "Governor=%s\n"
                    "Driver=%s\n",
                   processor->cpukhz_min,
                   processor->cpukhz_max,
                   processor->cpukhz_cur,
                   processor->transition_latency,
                   processor->scaling_governor,
                   processor->scaling_driver);
    } else {
        tmp_cpufreq = g_strdup_printf(
                    "[Frequency Scaling]\n"
                    "Driver=%s\n",
                   processor->scaling_driver);
    }

    ret = g_strdup_printf("[Processor]\n"
                           "Model=%s\n"
                           "Revision=%s\n"
                   "Frequency=%.2f MHz\n"
                   "BogoMips=%.2f\n"
                   "Byte Order="
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                       "Little Endian"
#else
                       "Big Endian"
#endif
                       "\n"
                       "%s" /* topology */
                       "%s" /* frequency scaling */
                       "%s",/* empty */
                   processor->model_name,
                   processor->revision,
                   processor->cpu_mhz,
                   processor->bogomips,
                   tmp_topology,
                   tmp_cpufreq,
                    "");
    g_free(tmp_cpufreq);
    g_free(tmp_topology);
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
