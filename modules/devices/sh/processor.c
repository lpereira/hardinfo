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

            get_str("machine", processor->vendor_id);
            get_str("cpu type", processor->model_name);
            get_str("cpu family", processor->family);
            get_float("cpu clock", processor->cpu_mhz);
            get_float("bus clock", processor->bus_mhz);
            get_float("module clock", processor->mod_mhz);
            get_float("bogomips", processor->bogomips);
        }
        g_strfreev(tmp);
    }

    fclose(cpuinfo);

    STRIFNULL(model_name, _("SuperH Processor"));
    UNKIFNULL(vendor_id);

    return g_slist_append(NULL, processor);
}

gchar *
processor_get_info(GSList *processors)
{
    Processor *processor = (Processor *)processors->data;

    return g_strdup_printf("[%s]\n"
                        "%s=%s\n"      /* cpu type */
                        "%s=%s\n"      /* machine */
                        "%s=%s\n"      /* family */
                        "%s=%.2f %s\n" /* frequency */
                        "%s=%.2f %s\n" /* bus frequency */
                        "%s=%.2f %s\n" /* module frequency */
                        "%s=%.2f\n"    /* bogomips */
                        "%s=%s\n",     /* byte order */
                    _("Processor"),
                    _("Name"), processor->model_name,
                    _("Machine"), processor->vendor_id,
                    _("Family"), processor->family,
                    _("Frequency"), processor->cpu_mhz, _("MHz"),
                    _("Bus Frequency"), processor->bus_mhz, _("MHz"),
                    _("Module Frequency"), processor->mod_mhz, _("MHz"),
                    _("BogoMips"), processor->bogomips,
                    _("Byte Order"), byte_order_str()
                   );
}
