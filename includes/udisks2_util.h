typedef struct udiskd {
    gchar *block_dev;
    gchar *serial;
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

gboolean udisks2_init();
void udisks2_shutdown();
udiskd *get_udisks2_drive_info(const gchar *blockdev);
