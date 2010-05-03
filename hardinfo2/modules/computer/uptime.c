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

UptimeInfo *
computer_get_uptime(void)
{
    UptimeInfo *ui = g_new0(UptimeInfo, 1);
    FILE *procuptime;
    gulong minutes;

    if ((procuptime = fopen("/proc/uptime", "r")) != NULL) {
	(void)fscanf(procuptime, "%lu", &minutes);
	ui->minutes = minutes / 60;
	fclose(procuptime);
    } else {
	return NULL;
    }

    ui->hours = ui->minutes / 60;
    ui->minutes %= 60;
    ui->days = ui->hours / 24;
    ui->hours %= 24;

    return ui;
}

gchar *
computer_get_formatted_uptime()
{
    UptimeInfo *ui;
    gchar *tmp;

    ui = computer_get_uptime();

    /* FIXME: Use ngettext */
#define plural(x) ((x > 1) ? "s" : "")

    if (ui->days < 1) {
	if (ui->hours < 1) {
	    tmp =
		g_strdup_printf("%d minute%s", ui->minutes,
				plural(ui->minutes));
	} else {
	    tmp =
		g_strdup_printf("%d hour%s, %d minute%s", ui->hours,
				plural(ui->hours), ui->minutes,
				plural(ui->minutes));
	}
    } else {
	tmp =
	    g_strdup_printf("%d day%s, %d hour%s and %d minute%s",
			    ui->days, plural(ui->days), ui->hours,
			    plural(ui->hours), ui->minutes,
			    plural(ui->minutes));
    }

    g_free(ui);
    return tmp;
}
