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
#include "cpu_util.h"

GSList *
processor_scan(void)
{
    GSList *procs = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[128];
    gchar *rep_pname = NULL;
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
            (  CHECK_FOR("vendor")
            || CHECK_FOR("arch")
            || CHECK_FOR("family") ) ) {

            /* single proc/core may not have "processor : n" */
            processor = g_new0(Processor, 1);
            processor->id = 0;

            if (rep_pname)
                processor->model_name = g_strdup(rep_pname);
        }

        if (processor) {
            get_str("vendor", processor->vendor_id);
            get_str("archrev", processor->archrev);
            get_str("arch", processor->arch);
            get_str("family", processor->family);
            get_str("features", processor->features);
            get_int("model", processor->model);
            get_int("revision", processor->revision);
            get_float("BogoMIPS", processor->bogomips);
            get_float("cpu MHz", processor->cpu_mhz);
            get_int("cpu regs", processor->cpu_regs);
        }
        g_strfreev(tmp);
    }

    if (processor)
        procs = g_slist_append(procs, processor);

    g_free(rep_pname);
    fclose(cpuinfo);

    /* TODO: redup */

    /* data not from /proc/cpuinfo */
    for (pi = procs; pi; pi = pi->next) {
        processor = (Processor *) pi->data;

        /* strings can't be null or segfault later */
        STRIFNULL(processor->model_name, _("IA64 Processor") );
        UNKIFNULL(processor->vendor_id);
        STRIFNULL(processor->arch, "IA-64");
        STRIFNULL(processor->archrev, "0");
        UNKIFNULL(processor->family);
        UNKIFNULL(processor->features);

        /* topo & freq */
        processor->cpufreq = cpufreq_new(processor->id);
        processor->cputopo = cputopo_new(processor->id);

        if (processor->cpufreq->cpukhz_max)
            processor->cpu_mhz = processor->cpufreq->cpukhz_max / 1000;

    }

    return procs;
}

gchar *processor_name(GSList * processors) {
    return processor_name_default(processors);
}

gchar *processor_describe(GSList * processors) {
    return processor_describe_default(processors);
}

gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_cpufreq, *tmp_topology, *ret;

    tmp_topology = cputopo_section_str(processor->cputopo);
    tmp_cpufreq = cpufreq_section_str(processor->cpufreq);

    ret = g_strdup_printf("[%s]\n"
                        "%s=%s\n"      /* name */
                        "%s=%s\n"      /* vendor */
                        "%s=%s\n"      /* arch */
                        "%s=%s\n"      /* archrev */
                        "%s=%s\n"      /* family */
                        "%s=%d\n"      /* model no. */
                        "%s=%d\n"      /* revision */
                        "%s=%.2f %s\n" /* frequency */
                        "%s=%.2f\n"    /* bogomips */
                        "%s=%s\n"      /* byte order */
                        "%s=%d\n"      /* regs */
                        "%s=%s\n"      /* features */
                        "%s" /* topology */
                        "%s" /* frequency scaling */
                        "%s",/* empty */
                    _("Processor"),
                    _("Name"), processor->model_name,
                    _("Vendor"), processor->vendor_id,
                    _("Architecture"), processor->arch,
                    _("Architecture Revision"), processor->archrev,
                    _("Family"), processor->family,
                    _("Model"), processor->model,
                    _("Revision"), processor->revision,
                    _("Frequency"), processor->cpu_mhz, _("MHz"),
                    _("BogoMips"), processor->bogomips,
                    _("Byte Order"), byte_order_str(),
                    _("CPU regs"), processor->cpu_regs,
                    _("Features"), processor->features,
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
