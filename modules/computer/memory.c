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
#include "computer.h"

MemoryInfo *
computer_get_memory(void)
{
    MemoryInfo *mi;
    FILE *procmem;
    gchar buffer[128];

    procmem = fopen("/proc/meminfo", "r");
    if (!procmem)
        return NULL;
    mi = g_new0(MemoryInfo, 1);

    while (fgets(buffer, 128, procmem)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);
        if (tmp[1] == NULL) {
            g_strfreev(tmp);
            continue;
        }
        tmp[0] = g_strstrip(tmp[0]);
        tmp[1] = g_strstrip(tmp[1]);

        get_int("MemTotal", mi->total);
        get_int("MemFree", mi->free);
        get_int("Cached", mi->cached);

        g_strfreev(tmp);
    }
    fclose(procmem);

    mi->used = mi->total - mi->free;

    mi->total  /= 1000;
    mi->cached /= 1000;
    mi->used   /= 1000;
    mi->free   /= 1000;

    mi->used -= mi->cached;
    mi->ratio = 1 - (gdouble) mi->used / mi->total;

    return mi;
}
