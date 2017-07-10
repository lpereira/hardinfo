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

gchar *byte_order_str() {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    return _("Little Endian");
#else
    return _("Big Endian");
#endif
}

#ifndef PROC_CPUINFO
#define PROC_CPUINFO "/proc/cpuinfo"
#endif

GSList *
processor_scan(void)
{
    Processor *processor;
    FILE *cpuinfo;
    gchar buffer[128];
    gchar *toss = NULL;

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
        return NULL;

    processor = g_new0(Processor, 1);
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);

        if (tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);

            get_str("archrev", toss); /* ignore this or "arch" will catch it below */

            get_str("vendor", processor->vendor_id);
            get_str("arch", processor->arch);
            get_str("family", processor->family);
            get_int("model", processor->model);
            get_int("revision", processor->revision);
            get_float("BogoMIPS", processor->bogomips);
            get_float("cpu MHz", processor->cpu_mhz);

        }
        g_strfreev(tmp);
    }

    processor->model_name = _("IA64 Processor");

    fclose(cpuinfo);

    return g_slist_append(NULL, processor);
}

gchar *
processor_get_info(GSList *processors)
{
    Processor *processor = (Processor *)processors->data;

    return g_strdup_printf("[%s]\n"
                        "%s=%s\n"      /* name */
                        "%s=%s\n"      /* vendor */
                        "%s=%s\n"      /* arch */
                        "%s=%s\n"      /* family */
                        "%s=%d\n"      /* model no. */
                        "%s=%d\n"      /* revision */
                        "%s=%.2f %s\n" /* frequency */
                        "%s=%.2f\n"    /* bogomips */
                        "%s=%s\n",     /* byte order */
                    _("Processor"),
                    _("Name"), processor->model_name,
                    _("Vendor"), processor->vendor_id,
                    _("Architecture"), processor->arch,
                    _("Family"), processor->family,
                    _("Model"), processor->model,
                    _("Revision"), processor->revision,
                    _("Frequency"), processor->cpu_mhz, _("MHz"),
                    _("BogoMips"), processor->bogomips,
                    _("Byte Order"), byte_order_str()
                              );
}
