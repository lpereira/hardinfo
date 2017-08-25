/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    This file
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
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
#include "dmi_util.h"

static const char *dmi_sysfs_root(void) {
    char *candidates[] = {
        "/sys/devices/virtual/dmi",
        "/sys/class/dmi",
        NULL
    };
    int i = 0;
    while (candidates[i] != NULL) {
        if(access(candidates[i], F_OK) != -1)
            return candidates[i];
        i++;
    }
    return NULL;
}

char *dmi_get_str(const char *id_str) {
    static struct {
        char *id;
        char *path;
    } tab_dmi_sysfs[] = {
        /* dmidecode -> sysfs */
        { "bios-release-date", "id/bios_date" },
        { "bios-vendor", "id/bios_vendor" },
        { "bios-version", "id/bios_version" },
        { "baseboard-product-name", "id/board_name" },
        { "baseboard-manufacturer", "id/board_vendor" },
        { "baseboard-version", "id/board_version" },
        { "baseboard-serial-number", "id/board_serial" },
        { "baseboard-asset-tag", "id/board_asset_tag" },
        { "system-product-name", "id/product_name" },
        { "system-manufacturer", "id/sys_vendor" },
        { "system-serial-number", "id/product_serial" },
        { "system-product-family", "id/product_family" },
        { "system-version", "id/product_version" },
        { "system-uuid", "product_uuid" },
        { "chassis-type", "id/chassis_type" },
        { "chassis-serial-number", "id/chassis_serial" },
        { "chassis-manufacturer", "id/chassis_vendor" },
        { "chassis-version", "id/chassis_version" },
        { "chassis-asset-tag", "id/chassis_asset_tag" },
        { NULL, NULL }
    };
    const gchar *dmi_root = dmi_sysfs_root();
    gchar *ret = NULL;
    gchar full_path[PATH_MAX];
    gboolean spawned;
    gchar *out, *err;

    int i = 0;

    /* try sysfs first */
    if (dmi_root) {
        while (tab_dmi_sysfs[i].id != NULL) {
            if (strcmp(id_str, tab_dmi_sysfs[i].id) == 0) {
                snprintf(full_path, PATH_MAX, "%s/%s", dmi_root, tab_dmi_sysfs[i].path);
                if (g_file_get_contents(full_path, &ret, NULL, NULL) )
                    goto dmi_str_done;
            }
            i++;
        }
    }

    /* try dmidecode, but may require root */
    snprintf(full_path, PATH_MAX, "dmidecode -s %s", id_str);
    spawned = g_spawn_command_line_sync(full_path,
            &out, &err, &i, NULL);
    if (spawned) {
        if (i == 0)
            ret = out;
        else
            g_free(out);

        g_free(err);
    }

dmi_str_done:
    if (ret != NULL) {
        ret = strend(ret, '\n');
        ret = g_strstrip(ret);
        /* return NULL on empty */
        if (*ret == 0) {
            g_free(ret);
            ret = NULL;
        }
    }
    return ret;
}

char *dmi_chassis_type_str(int chassis_type, gboolean with_val) {
    static const char *types[] = {
        N_("Invalid chassis type (0)"),
        N_("Unknown chassis type"), /* 1 is "Other", but not helpful in HardInfo */
        N_("Unknown chassis type"),
        N_("Desktop"),
        N_("Low-profile Desktop"),
        N_("Pizza Box"),
        N_("Mini Tower"),
        N_("Tower"),
        N_("Portable"),
        N_("Laptop"),
        N_("Notebook"),
        N_("Handheld"),
        N_("Docking Station"),
        N_("All-in-one"),
        N_("Subnotebook"),
        N_("Space-saving"),
        N_("Lunch Box"),
        N_("Main Server Chassis"),
        N_("Expansion Chassis"),
        N_("Sub Chassis"),
        N_("Bus Expansion Chassis"),
        N_("Peripheral Chassis"),
        N_("RAID Chassis"),
        N_("Rack Mount Chassis"),
        N_("Sealed-case PC"),
    };

    if (chassis_type <= 0) {
        gchar *chassis = dmi_get_str("chassis-type");
        chassis_type = atoi(chassis);
        g_free(chassis);
    }

    if (chassis_type >= 0 && chassis_type < G_N_ELEMENTS(types)) {
        if (with_val)
            return g_strdup_printf("[%d] %s", chassis_type, _(types[chassis_type]));

        return g_strdup(_(types[chassis_type]));
    }
    return NULL;
}
