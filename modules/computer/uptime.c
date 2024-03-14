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
#include "computer.h"

UptimeInfo *
computer_get_uptime(void)
{
    UptimeInfo *ui = g_new0(UptimeInfo, 1);
    FILE *procuptime;
    gulong minutes;

    if ((procuptime = fopen("/proc/uptime", "r")) != NULL) {
        int c=fscanf(procuptime, "%lu", &minutes);
        ui->minutes = minutes / 60;
        fclose(procuptime);
    } else {
        g_free(ui);
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
    const gchar *days_fmt, *hours_fmt, *minutes_fmt;
    gchar *full_fmt = NULL, *ret = NULL;

    ui = computer_get_uptime();

    days_fmt = ngettext("%d day", "%d days", ui->days);
    hours_fmt = ngettext("%d hour", "%d hours", ui->hours);
    minutes_fmt = ngettext("%d minute", "%d minutes", ui->minutes);

    if (ui->days < 1) {
        if (ui->hours < 1) {
            ret = g_strdup_printf(minutes_fmt, ui->minutes);
        } else {
            full_fmt = g_strdup_printf("%s %s", hours_fmt, minutes_fmt);
            ret = g_strdup_printf(full_fmt, ui->hours, ui->minutes);
        }
    } else {
        full_fmt = g_strdup_printf("%s %s %s", days_fmt, hours_fmt, minutes_fmt);
        ret = g_strdup_printf(full_fmt, ui->days, ui->hours, ui->minutes);
    }
    g_free(full_fmt);
    g_free(ui);
    return ret;
}
