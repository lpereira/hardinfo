/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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
/*
 * Device Tree support
 */
#include <unistd.h>
#include <sys/types.h>
#include "devices.h"

static char *get_dt_string(char *p) {
    char fn[256];
    char *ret = NULL, *rep = NULL;
    snprintf(fn, 256, "/proc/device-tree/%s", p);
    g_file_get_contents(fn, &ret, NULL, NULL);
    if (ret) {
        while((rep = strchr(ret, '\n'))) *rep = ' ';
    }
    return ret;
}

#include "devicetree/rpi_data.c"

gchar *dtree_info = NULL;

static void add_to_moreinfo(const char *group, const char *key, char *value)
{
    char *new_key = g_strconcat("DTREE:", group, ":", key, NULL);
    moreinfo_add_with_prefix("DEV", new_key, g_strdup(g_strstrip(value)));
}

void __scan_dtree()
{
    char *model = NULL, *serial = NULL, *special = NULL;
    model = get_dt_string("model");
    serial = get_dt_string("serial-number");

    if ( strstr(model, "Raspberry Pi") != NULL )
        special = rpi_board_details();
    else
        special = "";

    dtree_info = g_strdup_printf(
            "[%s]\n"
            "%s=%s\n"
            "%s=%s\n"
            "%s",
            _("Device Tree"),
            _("Model"), model,
            _("Serial Number"), serial,
            special
            );
}
