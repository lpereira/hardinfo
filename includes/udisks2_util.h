#include "vendor.h"
#include "pci_util.h"

typedef struct udiskp {
    gchar *block;
    gchar *type;
    gchar *version;
    gchar *label;
    guint64 size;
    struct udiskp* next;
} udiskp;

enum{
    UDSK_INTPVAL_SKIP          = 0,
    UDSK_INTPVAL_DIMENSIONLESS = 1,
    UDSK_INTPVAL_MILISECONDS   = 2,
    UDSK_INTPVAL_HOURS         = 3,
    UDSK_INTPVAL_SECTORS       = 4,
    UDSK_INTPVAL_CELSIUS       = 5,
};

typedef struct udisksa {
    guint8 id;
    gchar *identifier;
    gint value;
    gint worst;
    gint threshold;
    gint64 interpreted;
    guint8 interpreted_unit; // enum
    struct udisksa* next;
} udisksa;

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
    guint64 size;
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
    udisksa *smart_attributes;
    pcid *nvme_controller;
    vendor_list vendors;
} udiskd;

typedef struct udiskt {
    gchar *drive;
    gint32 temperature;
} udiskt;
void udisks2_init();
void udisks2_shutdown();
GSList *get_udisks2_temps();
GSList *get_udisks2_all_drives_info();
