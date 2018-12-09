#include <gio/gio.h>
#include "udisks2_util.h"
#include "hardinfo.h"

#define UDISKS2_INTERFACE           "org.freedesktop.UDisks2"
#define UDISKS2_MANAGER_INTERFACE   "org.freedesktop.UDisks2.Manager"
#define UDISKS2_BLOCK_INTERFACE     "org.freedesktop.UDisks2.Block"
#define UDISKS2_DRIVE_INTERFACE     "org.freedesktop.UDisks2.Drive"
#define UDISKS2_DRIVE_ATA_INTERFACE "org.freedesktop.UDisks2.Drive.Ata"
#define DBUS_PROPERTIES_INTERFACE   "org.freedesktop.DBus.Properties"
#define UDISKS2_MANAGER_OBJ_PATH    "/org/freedesktop/UDisks2/Manager"
#define UDISKS2_BLOCK_DEVICES_PATH  "/org/freedesktop/UDisks2/block_devices"

GDBusConnection* udisks2_conn = NULL;

GVariant* get_dbus_property(GDBusProxy* proxy, const gchar *interface,
                            const gchar *property) {
    GVariant *result, *v = NULL;
    GError *error = NULL;

    result = g_dbus_proxy_call_sync(proxy, "Get",
                            g_variant_new ("(ss)", interface, property),
                            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error != NULL) {
        g_error_free (error);
        return NULL;
    }

    g_variant_get(result, "(v)", &v);
    g_variant_unref(result);
    return v;
}

GDBusProxy* get_udisks2_drive_proxy(GDBusConnection *conn, const gchar* blockdev_name) {
    GDBusProxy *proxy;
    GVariant *v;
    GError *error = NULL;
    gchar *path;
    const gchar *drive_path;

    // get block device proxy
    path = g_strdup_printf("%s/%s", UDISKS2_BLOCK_DEVICES_PATH, blockdev_name);
    proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    UDISKS2_INTERFACE, path,
                                    DBUS_PROPERTIES_INTERFACE, NULL, &error);
    g_free(path);
    path = NULL;
    if (error != NULL) {
        g_error_free (error);
        g_object_unref(proxy);
        return NULL;
    }

    // get drive object path
    v = get_dbus_property(proxy, UDISKS2_BLOCK_INTERFACE, "Drive");
    if (v) {
        drive_path = g_variant_get_string(v, NULL);
        if (strcmp(drive_path, "/") != 0) {
            path = g_strdup(drive_path);
        }
        g_variant_unref(v);
    }
    g_object_unref(proxy);
    proxy = NULL;

    if (path == NULL)
        return NULL;

    // get drive proxy
    proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    UDISKS2_INTERFACE, path,
                                    DBUS_PROPERTIES_INTERFACE, NULL, &error);
    g_free(path);
    if (error != NULL) {
        g_error_free (error);
        return NULL;
    }

    return proxy;
}

GDBusConnection* get_udisks2_connection(void) {
    GDBusConnection *conn;
    GDBusProxy *proxy;
    GError *error = NULL;
    GVariant *result = NULL;

    // connection to system bus
    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL) {
        g_error_free (error);
        return NULL;
    }

    // let's check if udisks2 is responding
    proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    UDISKS2_INTERFACE, UDISKS2_MANAGER_OBJ_PATH,
                                    DBUS_PROPERTIES_INTERFACE, NULL, &error);
    if (error != NULL) {
        g_error_free (error);
        g_object_unref(conn);
        return NULL;
    }

    result = get_dbus_property(proxy, UDISKS2_MANAGER_INTERFACE, "Version");
    g_object_unref(proxy);
    if (error != NULL) {
        g_error_free (error);
        return NULL;
    }

    // OK, let's return connection to system bus
    g_variant_unref(result);
    return conn;
}

udiskd *udiskd_new() {
    return g_new0(udiskd, 1);
}

void udiskd_free(udiskd *u) {
    if (u) {
        g_free(u->block_dev);
        g_free(u->serial);
        g_free(u->media);
        g_free(u->media_compatibility);
        g_free(u);
    }
}

udiskd *get_udisks2_drive_info(const gchar *blockdev) {
    GDBusProxy *drive;
    GVariant *v;
    const gchar *str;
    udiskd *u;

    drive = get_udisks2_drive_proxy(udisks2_conn, blockdev);
    if (!drive){
        return NULL;
    }

    u = udiskd_new();
    u->block_dev = g_strdup(blockdev);

    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Serial");
    if (v){
        u->serial = g_variant_dup_string(v, NULL);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "RotationRate");
    if (v){
        u->rotation_rate = g_variant_get_int32(v);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Size");
    if (v){
        u->size = g_variant_get_uint64(v);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Media");
    if (v){
        str = g_variant_get_string(v, NULL);
        if (strcmp(str, "") != 0) {
            u->media = g_strdup(str);
        }
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "MediaCompatibility");
    if (v){
        GVariantIter *iter;
        g_variant_get(v, "as", &iter);
        while (g_variant_iter_loop (iter, "s", &str)){
            if (u->media_compatibility == NULL){
                u->media_compatibility = g_strdup(str);
            }
            else{
                u->media_compatibility = h_strdup_cprintf(", %s", u->media_compatibility, str);
            }
        }
        g_variant_iter_free (iter);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Ejectable");
    if (v){
        u->ejectable = g_variant_get_boolean(v);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Removable");
    if (v){
        u->removable = g_variant_get_boolean(v);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_ATA_INTERFACE, "SmartEnabled");
    if (v){
        u->smart_enabled = g_variant_get_boolean(v);
        g_variant_unref(v);
    }
    if (u->smart_enabled){
        v = get_dbus_property(drive, UDISKS2_DRIVE_ATA_INTERFACE, "SmartPowerOnSeconds");
        if (v){
            u->smart_poweron = g_variant_get_uint64(v);
            g_variant_unref(v);
        }
        v = get_dbus_property(drive, UDISKS2_DRIVE_ATA_INTERFACE, "SmartNumBadSectors");
        if (v){
            u->smart_bad_sectors = g_variant_get_int64(v);
            g_variant_unref(v);
        }
        v = get_dbus_property(drive, UDISKS2_DRIVE_ATA_INTERFACE, "SmartTemperature");
        if (v){
            u->smart_temperature = (gint) (g_variant_get_double(v) - 273.15);
            g_variant_unref(v);
        }
        v = get_dbus_property(drive, UDISKS2_DRIVE_ATA_INTERFACE, "SmartFailing");
        if (v){
            u->smart_failing = g_variant_get_boolean(v);
            g_variant_unref(v);
        }
    }

    g_object_unref(drive);
    return u;
}

gboolean udisks2_init(){
    udisks2_conn = get_udisks2_connection();
    return (udisks2_conn != NULL);
}

void udisks2_shutdown(){
    if (udisks2_conn != NULL){
        g_object_unref(udisks2_conn);
        udisks2_conn = NULL;
    }
}
