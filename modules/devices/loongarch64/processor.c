/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 L. A. F. Pereira <l@tia.mat.br>
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
    Processor *processor;
    FILE *cpuinfo;
    gchar buffer[128];

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
        return NULL;

    processor = g_new0(Processor, 1);
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);

        if (tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);

            get_str("system type", processor->vendor_id);
            get_str("CPU Family", processor->family);
            get_str("Model Name", processor->model_name);
            get_int("CPU Revision", processor->revision);
            get_float("CPU MHz", processor->cpu_mhz);
            get_float("BogoMIPS", processor->bogomips);
            get_str("Features", processor->features);
        }
        g_strfreev(tmp);
    }

    fclose(cpuinfo);

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
                        "%s=%s\n"      /* vendor */
                        "%s=%s\n"      /* family */
                        "%s=%s\n"      /* name */
                        "%s=%d\n"      /* revision */
                        "%s=%.2f %s\n" /* frequency */
                        "%s=%.2f\n"    /* bogoMIPS */
                        "%s=%s\n"      /* features */
                        "%s=%s\n",     /* byte order */
                    _("Processor"),
                    _("System Type"), processor->vendor_id,
                    _("Family"), processor->family,
                    _("Model"), processor->model_name,
                    _("Revision"), processor->revision,
                    _("Frequency"), processor->cpu_mhz, _("MHz"),
                    _("BogoMIPS"), processor->bogomips,
                    _("Features"), processor->features,
                    _("Byte Order"), byte_order_str()
                   );
}
