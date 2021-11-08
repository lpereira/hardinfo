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

#include <string.h>

#include "hardinfo.h"
#include "computer.h"

static gboolean
computer_get_loadinfo(LoadInfo *li)
{
    FILE *procloadavg;
    char buf[64];
    int ret;

    procloadavg = fopen("/proc/loadavg", "r");
    if (!procloadavg)
        return FALSE;

    if (!fgets(buf, sizeof(buf), procloadavg)) {
        fclose(procloadavg);
        return FALSE;
    }

    ret = sscanf(buf, "%f %f %f", &li->load1, &li->load5, &li->load15);
    if (ret != 3) {
        size_t len = strlen(buf);
        size_t i;

        for (i = 0; i < len; i++) {
            if (buf[i] == '.')
                buf[i] = ',';
        }

        ret = sscanf(buf, "%f %f %f", &li->load1, &li->load5, &li->load15);
    }

    fclose(procloadavg);

    return ret == 3;
}

gchar *
computer_get_formatted_loadavg()
{
    LoadInfo li;

    if (!computer_get_loadinfo(&li))
        return g_strdup(_("Couldn't obtain load average"));

    return g_strdup_printf("%.2f, %.2f, %.2f", li.load1, li.load5,
        li.load15);
}
