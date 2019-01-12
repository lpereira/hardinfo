typedef struct udiskd {
    gchar *model;
    gchar *vendor;
    gchar *revision;
    gchar *block_dev;
    gchar *serial;
    gchar *connection_bus;
    gboolean ejectable;
    gboolean removable;
    gint32 rotation_rate;
    gint64 size;
    gchar *media;
    gchar *media_compatibility;
    gboolean smart_enabled;
    gboolean smart_failing;
    guint64 smart_poweron;
    gint64 smart_bad_sectors;
    gint32 smart_temperature;
} udiskd;

void udisks2_init();
void udisks2_shutdown();
GSList *get_udisks2_all_drives_info();
