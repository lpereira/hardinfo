typedef struct udiskp {
    gchar *block;
    gchar *type;
    gchar *version;
    gchar *label;
    guint64 size;
    struct udiskp* next;
} udiskp;

typedef struct udiskd {
    gchar *model;
    gchar *vendor;
    gchar *revision;
    gchar *block_dev;
    gchar *serial;
    gchar *connection_bus;
    gchar *partition_table;
    udiskp *partitions;
    gboolean ejectable;
    gboolean removable;
    gint32 rotation_rate;
    gint64 size;
    gchar *media;
    gchar **media_compatibility;
    gboolean pm_supported;
    gboolean aam_supported;
    gboolean apm_supported;
    gboolean smart_supported;
    gboolean smart_enabled;
    gboolean smart_failing;
    guint64 smart_poweron;
    gint64 smart_bad_sectors;
    gint32 smart_temperature;
} udiskd;

typedef struct udiskt {
    gchar *drive;
    gint32 temperature;
} udiskt;

void udisks2_init();
void udisks2_shutdown();
GSList *get_udisks2_temps();
GSList *get_udisks2_all_drives_info();
