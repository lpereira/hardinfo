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
#include "string.h"
#include "cpu_util.h"

GSList *
processor_scan(void)
{
    GSList *procs = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[128];
    gchar *vendor_id = NULL;
    gint   num_procs = 0;
    gfloat bogomips = 0.0f;
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

        get_str("vendor_id", vendor_id);
        get_int("# processors", num_procs);
        get_int("bogomips per cpu", bogomips);

        if ( CHECK_FOR("processor") ) {
            /* finish previous */
            if (processor) {
                procs = g_slist_append(procs, processor);
            }

            /* start next */
            processor = g_new0(Processor, 1);
            if (strlen(tmp[0]) >= 10)
                processor->id = atol(tmp[0] + 10); /* processor n: ... */
            else
                processor->id = 0; /* idk */
            processor->proc_str = g_strdup(tmp[1]);

            if (vendor_id)
                processor->model_name = g_strdup(vendor_id);
            if (bogomips)
                processor->bogomips = bogomips;

            g_strfreev(tmp);
            continue;
        }

        g_strfreev(tmp);
    }

    if (processor)
        procs = g_slist_append(procs, processor);

    g_free(vendor_id);
    fclose(cpuinfo);

    /* data not from /proc/cpuinfo */
    for (pi = procs; pi; pi = pi->next) {
        processor = (Processor *) pi->data;

        /* strings can't be null or segfault later */
        STRIFNULL(processor->model_name, _("S390 Processor") );
        UNKIFNULL(processor->proc_str);

        /* topo & freq */
        processor->cpufreq = cpufreq_new(processor->id);
        processor->cputopo = cputopo_new(processor->id);

        if (processor->cpufreq->cpukhz_max)
            processor->cpu_mhz = processor->cpufreq->cpukhz_max / 1000;
        else
            processor->cpu_mhz = 0.0f;

    }

    return procs;
}

gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_cpufreq, *tmp_topology, *ret;
    tmp_topology = cputopo_section_str(processor->cputopo);
    tmp_cpufreq = cpufreq_section_str(processor->cpufreq);

    ret = g_strdup_printf("[%s]\n"
                        "%s=%s\n"      /* model */
                        "%s=%s\n"      /* proc string */
                        "%s=%.2f\n"    /* bogomips */
                        "%s=%s\n"      /* byte order */
                        "%s"  /* topology */
                        "%s", /* frequency scaling */
                        _("Processor"),
                        _("Model"), processor->model_name,
                        _("ID String"), processor->proc_str,
                        _("BogoMips"), processor->bogomips,
                        _("Byte Order"), byte_order_str(),
                        tmp_topology,
                        tmp_cpufreq
                        );
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

