/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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
    Processor *processor;
    FILE *cpuinfo;
    gchar buffer[128];
    long long hz = 0;

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
        return NULL;

    processor = g_new0(Processor, 1);
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);

        if (tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);

            get_str("cpu model", processor->model_name);
            get_float("BogoMIPS", processor->bogomips);
            get_str("platform string", processor->strmodel);
            get_str("cycle frequency [Hz]", processor->cycle_frequency_hz_str);

        }
        g_strfreev(tmp);
    }

    fclose(cpuinfo);

    gchar *tmp = g_strconcat("Alpha ", processor->model_name, NULL);
    g_free(processor->model_name);
    processor->model_name = tmp;

    if (processor->cycle_frequency_hz_str) {
        hz = atoll(processor->cycle_frequency_hz_str);
        processor->cpu_mhz = hz;
        processor->cpu_mhz /= 1000000;
    } else
        processor->cpu_mhz = 0.0f;

    return g_slist_append(NULL, processor);
}

gchar *processor_name(GSList * processors) {
    return processor_name_default(processors);
}

gchar *processor_describe(GSList * processors) {
    return processor_describe_default(processors);
}

gchar *
processor_get_info(GSList *processors)
{
    Processor *processor = (Processor *)processors->data;

    return g_strdup_printf("[%s]\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%.2f %s\n" /* frequency */
                        "%s=%.2f\n"    /* bogomips */
                        "%s=%s\n",     /* byte order */
                    _("Processor"),
                    _("Model"), processor->model_name,
                    _("Platform String"), processor->strmodel,
                    _("Frequency"), processor->cpu_mhz, _("MHz"),
                    _("BogoMips"), processor->bogomips,
                    _("Byte Order"), byte_order_str()
                    );
}
