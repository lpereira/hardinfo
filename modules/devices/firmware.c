/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2019 L. A. F. Pereira <l@tia.mat.br>
 *    Copyright (C) 2019 Burt P. <pburt0@gmail.com>
 *    Copyright (C) 2020 Ondrej Čerman
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
#include "devices.h"
#include <inttypes.h>
#include <gio/gio.h>
#include "util_sysobj.h" /* for SEQ() and appf() */

#define fw_msg(msg, ...) fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

#define FWUPT_INTERFACE  "org.freedesktop.fwupd"

gboolean fail_no_fwupd = TRUE;

char *decode_flags(guint64 flags) {
    /* https://github.com/hughsie/fwupd/blob/master/libfwupd/fwupd-enums.{h,c} */
    static const struct { guint64 b; char *flag, *def; } flag_defs[] = {
        { (1u << 0),  "internal", N_("Device cannot be removed easily") },
        { (1u << 1),  "updatable", N_("Device is updatable in this or any other mode") },
        { (1u << 2),  "only-offline", N_("Update can only be done from offline mode") },
        { (1u << 3),  "require-ac", N_("Requires AC power") },
        { (1u << 4),  "locked", N_("Is locked and can be unlocked") },
        { (1u << 5),  "supported", N_("Is found in current metadata") },
        { (1u << 6),  "needs-bootloader", N_("Requires a bootloader mode to be manually enabled by the user") },
        { (1u << 7),  "registered", N_("Has been registered with other plugins") },
        { (1u << 8),  "needs-reboot", N_("Requires a reboot to apply firmware or to reload hardware") },
        { (1u << 17), "needs-shutdown", N_("Requires system shutdown to apply firmware") },
        { (1u << 9),  "reported", N_("Has been reported to a metadata server") },
        { (1u << 10), "notified", N_("User has been notified") },
        { (1u << 11), "use-runtime-version", N_("Always use the runtime version rather than the bootloader") },
        { (1u << 12), "install-parent-first", N_("Install composite firmware on the parent before the child") },
        { (1u << 13), "is-bootloader", N_("Is currently in bootloader mode") },
        { (1u << 14), "wait-for-replug", N_("The hardware is waiting to be replugged") },
        { (1u << 15), "ignore-validation", N_("Ignore validation safety checks when flashing this device") },
        { (1u << 18), "another-write-required", N_("Requires the update to be retried with a new plugin") },
        { (1u << 19), "no-auto-instance-ids", N_("Do not add instance IDs from the device baseclass") },
        { (1u << 20), "needs-activation", N_("Device update needs to be separately activated") },
        { (1u << 21), "ensure-semver", N_("Ensure the version is a valid semantic version, e.g. numbers separated with dots") },
        { (1u << 16), "trusted", N_("Extra metadata can be exposed about this device") },
        {  0, NULL }
    };

    gchar *flag_list = g_strdup("");

    int i;
    for (i = 0; flag_defs[i].flag; i++) {
        if (flags & flag_defs[i].b)
            flag_list = appfnl(flag_list, "[%s] %s", flag_defs[i].flag, flag_defs[i].def);
    }
    return flag_list;
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
    struct Info *info = info_new();
    struct InfoGroup *this_group = NULL;
    gboolean has_vendor_field = FALSE;
    gboolean updatable = FALSE;
    const Vendor *gv = NULL;
    int gc = 0;

    GDBusConnection *conn;
    GDBusProxy *proxy;
    GVariant *devices, *value;
    GVariantIter *deviter, *dictiter, *iter;
    const gchar *key, *tmpstr;

    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    if (!conn)
        return g_strdup("");

    proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                  FWUPT_INTERFACE, "/", FWUPT_INTERFACE,
                                  NULL, NULL);
    if (!proxy) {
        g_object_unref(conn);
        return g_strdup("");
    }

    fail_no_fwupd = FALSE;
    devices = g_dbus_proxy_call_sync(proxy, "GetDevices", NULL,
                                     G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);

    if (devices) {
        g_variant_get(devices, "(aa{sv})", &deviter);
        while(g_variant_iter_loop(deviter, "a{sv}", &dictiter)){

            this_group = info_add_group(info, _("Unknown"), info_field_last());
            this_group->sort = INFO_GROUP_SORT_NAME_DESCENDING;
            has_vendor_field = FALSE;
            updatable = FALSE;
            gv = NULL;

            while (g_variant_iter_loop(dictiter, "{&sv}", &key, &value)) {
                if (SEQ(key, "Name")) {
                    tmpstr = g_variant_get_string(value, NULL);
                    this_group->name = hardinfo_clean_grpname(tmpstr, 0);
                    gv = vendor_match(tmpstr, NULL);
                } else if (SEQ(key, "Vendor")) {
                    has_vendor_field = TRUE;
                    tmpstr = g_variant_get_string(value, NULL);

                    const Vendor* v = vendor_match(tmpstr, NULL);
                    if (v) {
                        info_group_add_field(this_group,
                            info_field(_("Vendor"), v->name,
                            .value_has_vendor = TRUE,
                            .free_value_on_flatten = FALSE) );
                    } else {
                        info_group_add_field(this_group,
                            info_field(_("Vendor"), g_strdup(tmpstr),
                            .free_value_on_flatten = TRUE) );
                    }
                } else if (SEQ(key, "Icon")) {
                    g_variant_get(value, "as", &iter);
                    while (g_variant_iter_loop(iter, "s", &tmpstr)) {
                        info_group_add_field(this_group,
                            info_field(_("Icon"), g_strdup(tmpstr),
                            .free_value_on_flatten = TRUE,
                            .icon = g_strdup(find_icon(tmpstr)) ) );
                    }
                } else if (SEQ(key, "Guid")) {
                    g_variant_get(value, "as", &iter);
                    while (g_variant_iter_loop(iter, "s", &tmpstr)) {
                        info_group_add_field(this_group,
                            info_field(_("Guid"), g_strdup(tmpstr),
                            .tag = g_strdup_printf("guid%d", gc++),
                            .free_value_on_flatten = TRUE) );
                    }
                    g_variant_iter_free(iter);
                } else if (SEQ(key, "Created")) {
                    guint64 created = g_variant_get_uint64(value);
                    GDateTime *dt = g_date_time_new_from_unix_local(created);
                    if (dt) {
                        info_group_add_field(this_group,
                            info_field(_("Created"), g_date_time_format(dt, "%x"),
                            .free_value_on_flatten = TRUE) );
                        g_date_time_unref(dt);
                    }
                } else if (SEQ(key, "Flags")) {
                    guint64 flags = g_variant_get_uint64(value);
                    updatable = (gboolean)(flags & (1u << 1));
                    info_group_add_field(this_group,
                            info_field(_("Flags"), decode_flags(flags),
                            .free_value_on_flatten = TRUE) );
                } else {
                    if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING)) {
                        info_group_add_field(this_group,
                            info_field(find_translation(key),
                            g_variant_dup_string(value, NULL),
                            .free_value_on_flatten = TRUE) );
                    }
                }
            }

            if (gv && !has_vendor_field) {
                info_group_add_field(this_group,
                    info_field(_("Vendor"), gv->name,
                        .value_has_vendor = TRUE,
                        .free_value_on_flatten = FALSE) );
            }

            // hide devices that are not updatable
            if (!updatable) {
                info_remove_group(info, info->groups->len - 1);
            }
        }
        g_variant_iter_free(deviter);
        g_variant_unref(devices);
    }

    g_object_unref(proxy);
    g_object_unref(conn);

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
    if (fail_no_fwupd) {
        *msg = g_strdup(
            _("Requires the <i><b>fwupd</b></i> daemon."));
        return TRUE;
    }
    return FALSE;
}
