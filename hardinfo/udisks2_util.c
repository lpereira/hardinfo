#include <gio/gio.h>
#include "udisks2_util.h"
#include "hardinfo.h"

#define UDISKS2_INTERFACE           "org.freedesktop.UDisks2"
#define UDISKS2_MANAGER_INTERFACE   "org.freedesktop.UDisks2.Manager"
#define UDISKS2_BLOCK_INTERFACE     "org.freedesktop.UDisks2.Block"
#define UDISKS2_LOOP_INTERFACE      "org.freedesktop.UDisks2.Loop"
#define UDISKS2_PARTITION_INTERFACE "org.freedesktop.UDisks2.Partition"
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

// this function works with udisks2 version 2.7.2 or newer
GSList* get_block_dev_paths_from_udisks2(GDBusConnection* conn){
    GDBusProxy *proxy;
    GVariant *options, *v;
    GVariantIter *iter;
    GError *error = NULL;
    GSList *block_paths = NULL;
    const gchar *block_path = NULL;

    proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                  UDISKS2_INTERFACE, UDISKS2_MANAGER_OBJ_PATH,
                                  UDISKS2_MANAGER_INTERFACE, NULL, &error);
    options = g_variant_new_parsed("@a{sv} { %s: <true> }",
                                   "auth.no_user_interaction");
    if (error != NULL){
        g_error_free (error);
        g_object_unref(proxy);
        return NULL;
    }

    v = g_dbus_proxy_call_sync(proxy, "GetBlockDevices",
                               g_variant_new_tuple(&options, 1),
                               G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    g_object_unref(proxy);

    if (error != NULL){
        g_error_free (error);
        return NULL;
    }

    g_variant_get(v, "(ao)", &iter);
    while (g_variant_iter_loop (iter, "o", &block_path)){
        block_paths = g_slist_append(block_paths, g_strdup(block_path));
    }

    g_variant_iter_free (iter);
    g_variant_unref(v);
    return block_paths;
}

GSList* get_block_dev_paths_from_sysfs(){
    GSList *block_paths = NULL;
    GDir *dir;
    gchar *path;
    const gchar *entry;

    dir = g_dir_open("/sys/class/block", 0, NULL);
    if (!dir)
        return NULL;

    while ((entry = g_dir_read_name(dir))) {
        path = g_strdup_printf("%s/%s", UDISKS2_BLOCK_DEVICES_PATH, entry);
        block_paths = g_slist_append(block_paths, path);
    }

    g_dir_close(dir);
    return block_paths;
}

GSList* udisks2_drives_func_caller(GDBusConnection* conn,
                                   gpointer (*func)(const char*, GDBusProxy*)) {
    GDBusProxy *proxy, *drive_proxy;
    GVariant *block_v, *v;
    GSList *result_list = NULL, *block_dev_list, *node;
    GError *error = NULL;
    gpointer output;

    gchar *block_path = NULL;
    const gchar *block_dev, *drive_path = NULL;

    if (conn == NULL)
        return NULL;

    // get block devices
    block_dev_list = get_block_dev_paths_from_udisks2(conn);
    if (block_dev_list == NULL)
        block_dev_list = get_block_dev_paths_from_sysfs();

    for (node = block_dev_list; node != NULL; node = node->next) {
        block_path = (gchar *)node->data;
        proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE,
                                      NULL, UDISKS2_INTERFACE, block_path,
                                      DBUS_PROPERTIES_INTERFACE, NULL, &error);
        if (error){
            g_error_free(error);
            error = NULL;
            continue;
        }

        // Skip partitions
        v = get_dbus_property(proxy, UDISKS2_PARTITION_INTERFACE, "Size");
        if (v){
            g_variant_unref(v);
            g_object_unref(proxy);
            continue;
        }

        // Skip loop devices
        v = get_dbus_property(proxy, UDISKS2_LOOP_INTERFACE, "BackingFile");
        if (v){
            g_variant_unref(v);
            g_object_unref(proxy);
            continue;
        }

        block_dev = block_path + strlen(UDISKS2_BLOCK_DEVICES_PATH) + 1;
        drive_path = NULL;

        // let's find drive proxy
        v = get_dbus_property(proxy, UDISKS2_BLOCK_INTERFACE, "Drive");
        if (v){
            drive_path = g_variant_get_string(v, NULL);

            if (g_strcmp0(drive_path, "/") != 0){
                drive_proxy = g_dbus_proxy_new_sync(conn,
                                    G_DBUS_PROXY_FLAGS_NONE, NULL,
                                    UDISKS2_INTERFACE, drive_path,
                                    DBUS_PROPERTIES_INTERFACE, NULL, &error);

                if (error == NULL) {
                    // call requested function
                    output = func(block_dev, drive_proxy);

                    if (output != NULL){
                        result_list = g_slist_append(result_list, output);
                    }
                    g_object_unref(drive_proxy);
                }
                else {
                    g_error_free(error);
                    error = NULL;
                }
            }
            g_variant_unref(v);
        }
        g_object_unref(proxy);
    }
    g_slist_free_full(block_dev_list, g_free);

    return result_list;
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

udiskd* udiskd_new() {
    return g_new0(udiskd, 1);
}

void udiskd_free(udiskd *u) {
    if (u) {
        g_free(u->model);
        g_free(u->vendor);
        g_free(u->revision);
        g_free(u->block_dev);
        g_free(u->serial);
        g_free(u->connection_bus);
        g_free(u->media);
        g_free(u->media_compatibility);
        g_free(u);
    }
}

gpointer get_udisks2_drive_info(const char *blockdev, GDBusProxy *drive) {
    GVariant *v;
    const gchar *str;
    udiskd *u = NULL;

    u = udiskd_new();
    u->block_dev = g_strdup(blockdev);

    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Model");
    if (v){
        u->model = g_variant_dup_string(v, NULL);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Vendor");
    if (v){
        u->vendor = g_variant_dup_string(v, NULL);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Revision");
    if (v){
        u->revision = g_variant_dup_string(v, NULL);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "Serial");
    if (v){
        u->serial = g_variant_dup_string(v, NULL);
        g_variant_unref(v);
    }
    v = get_dbus_property(drive, UDISKS2_DRIVE_INTERFACE, "ConnectionBus");
    if (v){
        u->connection_bus = g_variant_dup_string(v, NULL);
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
    return u;
}

GSList* get_udisks2_all_drives_info(void){
    return udisks2_drives_func_caller(udisks2_conn, get_udisks2_drive_info);
}

void udisks2_init(){
    if (udisks2_conn == NULL){
        udisks2_conn = get_udisks2_connection();
    }
}

void udisks2_shutdown(){
    if (udisks2_conn != NULL){
        g_object_unref(udisks2_conn);
        udisks2_conn = NULL;
    }
}
