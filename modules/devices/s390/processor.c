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

#define PROC_CPUINFO "/proc/cpuinfo"

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

            get_str("vendor_id", processor->vendor_id);
            get_float("# processors", processor->cache_size);
            get_int("bogomips per cpu", processor->bogomips);

        }
        g_strfreev(tmp);
    }

    processor->cpu_mhz = 0.0f;

    processor->model_name = g_strconcat("S390 ", processor->vendor_id, NULL);
    g_free(processor->vendor_id);

    fclose(cpuinfo);

    return g_slist_append(NULL, processor);
}

gchar *
processor_get_info(GSList *processors)
{
    Processor *processor = (Processor *)processors->data;

    return g_strdup_printf("[%s]\n"
                        "%s=%s\n"      /* model */
                        "%s=%d\n"      /* proc count */
                        "%s=%.2f\n"    /* bogomips */
                        "%s=%s\n",     /* byte order */
                        _("Processor"),
                        _("Model"), processor->model_name,
                        _("Processors"), processor->cache_size,
                        _("BogoMips"), processor->bogomips,
                        _("Byte Order"), byte_order_str()
                        );
}
