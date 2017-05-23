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

GSList *
processor_scan(void)
{
    Processor *processor;
    FILE *cpuinfo;
    gchar buffer[128];

    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo)
    return NULL;

    processor = g_new0(Processor, 1);
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);

        if (tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);

            get_str("Processor", processor->model_name);
            if (!processor->model_name)
                get_str("model name", processor->model_name);
            get_str("Features", processor->flags);
            get_float("BogoMIPS", processor->bogomips);

            get_str("Hardware", processor->has_fpu);
        }
        g_strfreev(tmp);
    }

    processor->cpu_mhz = 0.0f;

    fclose(cpuinfo);

    return g_slist_append(NULL, processor);
}

gchar *
processor_get_info(GSList *processors)
{
        Processor *processor = (Processor *)processors->data;
        
    return g_strdup_printf("[Processor]\n"
                           "Name=%s\n"
                           "Features=%s\n"
                   "BogoMips=%.2f\n"
                   "Endianesss="
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                       "Little Endian"
#else
                       "Big Endian"
#endif
                       "\n"
                   "Hardware=%s\n",
                   processor->model_name,
                   processor->flags,
                   processor->bogomips,
                   processor->has_fpu);
}
