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

LoadInfo *
computer_get_loadinfo(void)
{
    LoadInfo *li = g_new0(LoadInfo, 1);
    FILE *procloadavg;

    procloadavg = fopen("/proc/loadavg", "r");
    (void)fscanf(procloadavg, "%f %f %f", &(li->load1), &(li->load5),
	   &(li->load15));
    fclose(procloadavg);

    return li;
}

gchar *
computer_get_formatted_loadavg()
{
    LoadInfo *li;
    gchar *tmp;

    li = computer_get_loadinfo();

    tmp =
	g_strdup_printf("%.2f, %.2f, %.2f", li->load1, li->load5,
			li->load15);

    g_free(li);
    return tmp;
}
