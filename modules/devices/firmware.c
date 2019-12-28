/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2019 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    Copyright (C) 2019 Burt P. <pburt0@gmail.com>
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

/* TODO Consider: https://fwupd.org/lvfs/docs/users -> "Using the D-Bus API" */

/* Example `fwupdmgr get-devices`:
Unifying Receiver
  DeviceId:             ecd8ed6144290e36b8d2587116f9d3543d3c69b7
  Guid:                 9d131a0c-a606-580f-8eda-80587250b8d6
  Guid:                 279ed287-3607-549e-bacc-f873bb9838c4
  Guid:                 21e75d9a-5ce6-5da2-b7ab-910c7f3f6836
  Summary:              A miniaturised USB wireless receiver
  Plugin:               unifying
  Flags:                updatable|supported|registered
  Vendor:               Logitech
  VendorId:             USB:0x046D
  Version:              RQR12.09_B0030
  VersionBootloader:    BOT01.04_B0016
  Icon:                 preferences-desktop-keyboard
  InstallDuration:      7
  Created:              2019-07-22
*/
/* sudo example:
Unifying Receiver
  DeviceId:             ecd8ed6144290e36b8d2587116f9d3543d3c69b7
  Guid:                 9d131a0c-a606-580f-8eda-80587250b8d6
  Guid:                 279ed287-3607-549e-bacc-f873bb9838c4 <- HIDRAW\VEN_046D&DEV_C52B
  Guid:                 21e75d9a-5ce6-5da2-b7ab-910c7f3f6836 <- HIDRAW\VEN_046D
  Summary:              A miniaturised USB wireless receiver
  Plugin:               unifying
  Flags:                updatable|supported|registered
  Vendor:               Logitech
  VendorId:             USB:0x046D
  Version:              RQR12.09_B0030
  VersionBootloader:    BOT01.04_B0016
  Icon:                 preferences-desktop-keyboard
  InstallDuration:      7
  Created:              2019-07-26
*/

#include "hardinfo.h"
#include "devices.h"
#include <inttypes.h>
#include "util_sysobj.h" /* for SEQ() and appf() */

#define fw_msg(msg, ...) fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

gboolean fail_no_fwupdmgr = TRUE;

const char *find_flag_definition(const char *flag) {
    /* https://github.com/hughsie/fwupd/blob/master/libfwupd/fwupd-enums.{h,c} */
    static const struct { char *flag, *def; } flag_defs[] = {
        { "internal", N_("Device cannot be removed easily") },
        { "updatable", N_("Device is updatable in this or any other mode") },
        { "only-offline", N_("Update can only be done from offline mode") },
        { "require-ac", N_("Requires AC power") },
        { "locked", N_("Is locked and can be unlocked") },
        { "supported", N_("Is found in current metadata") },
        { "needs-bootloader", N_("Requires a bootloader mode to be manually enabled by the user") },
        { "registered", N_("Has been registered with other plugins") },
        { "needs-reboot", N_("Requires a reboot to apply firmware or to reload hardware") },
        { "needs-shutdown", N_("Requires system shutdown to apply firmware") },
        { "reported", N_("Has been reported to a metadata server") },
        { "notified", N_("User has been notified") },
        { "use-runtime-version", N_("Always use the runtime version rather than the bootloader") },
        { "install-parent-first", N_("Install composite firmware on the parent before the child") },
        { "is-bootloader", N_("Is currently in bootloader mode") },
        { "wait-for-replug", N_("The hardware is waiting to be replugged") },
        { "ignore-validation", N_("Ignore validation safety checks when flashing this device") },
        { "another-write-required", N_("Requires the update to be retried with a new plugin") },
        { "no-auto-instance-ids", N_("Do not add instance IDs from the device baseclass") },
        { "needs-activation", N_("Device update needs to be separately activated") },
        { "ensure-semver", N_("Ensure the version is a valid semantic version, e.g. numbers separated with dots") },
        { "trusted", N_("Extra metadata can be exposed about this device") },
        { NULL }
    };
    int i;
    for (i = 0; flag_defs[i].flag; i++) {
        if (SEQ(flag, flag_defs[i].flag))
            return flag_defs[i].def;
    }
    return NULL;
}

const char *find_translation(const char *str) {
    /* TODO: https://github.com/hughsie/fwupd/blob/master/src/README.md */
    static const char *translatable[] = {
        N_("DeviceId"), N_("Guid"), N_("Summary"), N_("Plugin"), N_("Flags"),
        N_("Vendor"), N_("VendorId"), N_("Version"), N_("VersionBootloader"),
        N_("Icon"), N_("InstallDuration"), N_("Created"),
        NULL
    };
    int i;
    if (!str) return NULL;
    for (i = 0; translatable[i]; i++) {
        if (SEQ(str, translatable[i]))
            return _(translatable[i]);
    }
    return str;
};

/* map lvfs icon names to hardinfo icon names */
const char *find_icon(const char *lvfs_name) {
    /* icon names found by looking for fu_device_add_icon ()
     * in the fwupd source. */
    static const
    struct { char *lvfs, *hi; } imap[] = {
        { "applications-internet", "dns.png" },
        { "audio-card", "audio.png" },
        { "computer", "computer.png" },
        { "drive-harddisk", "hdd.png" },
        { "input-gaming", "joystick.png" },
        { "input-tablet", NULL },
        { "network-modem", "wireless.png" },
        { "preferences-desktop-keyboard", "keyboard.png" },
        { "thunderbolt", NULL },
        { "touchpad-disabled", NULL },
        /* default */
        { NULL, "memory.png" } /* a device with firmware maybe */
    };
    unsigned int i = 0;
    for(; imap[i].lvfs; i++) {
        if (SEQ(imap[i].lvfs, lvfs_name) && imap[i].hi)
            return imap[i].hi;
    }
    return imap[i].hi;
}

gchar *fwupdmgr_get_devices_info() {
    gboolean spawned;
    gchar *out, *err, *p, *next_nl, *col;
    gint exit_status;

    struct Info *info = info_new();

    struct InfoGroup *this_group = NULL;
    gboolean has_vendor_field = FALSE;
    const Vendor *gv = NULL;
    int gc = 0;

    fail_no_fwupdmgr = FALSE;
    spawned = g_spawn_command_line_sync("fwupdmgr get-devices",
            &out, &err, &exit_status, NULL);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (!isspace(*p) && *p != 0) {
                if (gv && !has_vendor_field) {
                    info_group_add_field(this_group,
                        info_field(_("Vendor"), gv->name,
                            .value_has_vendor = TRUE,
                            .free_value_on_flatten = FALSE) );
                }
                g_strstrip(p);
                this_group = info_add_group(info, g_strdup(p), info_field_last());
                this_group->sort = INFO_GROUP_SORT_NAME_DESCENDING;
                gv = vendor_match(p, NULL);
                has_vendor_field = FALSE;
            } else {
                if (col = strchr(p, ':')) {
                    *col = 0;
                    g_strstrip(p); // name
                    g_strstrip(col+1); // value
                    if (SEQ(p, "Icon")) {
                        info_group_add_field(this_group,
                            info_field(find_translation(p), g_strdup(col+1),
                            .free_value_on_flatten = TRUE, .icon = g_strdup(find_icon(col+1)) ) );
                    } else if (SEQ(p, "Vendor")) {
                        has_vendor_field = TRUE;
                        const Vendor* v = vendor_match(col+1, NULL);
                        if (v) {
                            info_group_add_field(this_group,
                                info_field(_("Vendor"), v->name,
                                .value_has_vendor = TRUE,
                                .free_value_on_flatten = FALSE) );
                        } else {
                            info_group_add_field(this_group,
                                info_field(find_translation(p), g_strdup(col+1),
                                .free_value_on_flatten = TRUE) );
                        }
                    } else if (SEQ(p, "Guid")) {
                        gchar *ar = NULL;
                        if (ar = strstr(col+1, " <-"))
                            *ar = 0;
                        info_group_add_field(this_group,
                            info_field(find_translation(p), g_strdup(col+1),
                            .tag = g_strdup_printf("guid%d", gc++),
                            .free_value_on_flatten = TRUE) );
                    } else if (SEQ(p, "Flags")) {
                        gchar **flags = g_strsplit(col+1, "|", 0);
                        gchar *flag_list = g_strdup("");
                        int f = 0;
                        for(; flags[f]; f++) {
                            const char *lf = find_flag_definition(flags[f]);
                            flag_list = appfnl(flag_list, "[%s] %s", flags[f], lf ? lf : "");
                        }
                        info_group_add_field(this_group,
                            info_field(find_translation(p), flag_list,
                            .free_value_on_flatten = TRUE) );
                    } else
                        info_group_add_field(this_group,
                            info_field(find_translation(p), g_strdup(col+1),
                            .free_value_on_flatten = TRUE) );
                }
            }
            p = next_nl + 1;
        }

        if (gv && !has_vendor_field) {
            info_group_add_field(this_group,
                info_field(_("Vendor"), gv->name,
                    .value_has_vendor = TRUE,
                    .free_value_on_flatten = FALSE) );
        }

        g_free(out);
        g_free(err);
    } else {
        fail_no_fwupdmgr = TRUE;
    }

    gchar *ret = NULL;
    if (info->groups->len) {
        info_set_view_type(info, SHELL_VIEW_DETAIL);
        //fw_msg("flatten...");
        ret = info_flatten(info);
        //fw_msg("ret: %s", ret);
    } else {
        ret = g_strdup_printf("[%s]\n%s=%s\n" "[$ShellParam$]\nViewType=0\n",
                _("Firmware List"),
                _("Result"), _("(Not available)") );
    }
    return ret;
}

gchar *firmware_get_info() {
    return fwupdmgr_get_devices_info();
}

gboolean firmware_hinote(const char **msg) {
    if (fail_no_fwupdmgr) {
        *msg = g_strdup(
            _("Requires the <i><b>fwupdmgr</b></i> utility."));
        return TRUE;
    }
    return FALSE;
}
