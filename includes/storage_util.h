#include "vendor.h"
#include "pci_util.h"

// udisks2 drive info extended
typedef struct u2driveext {
    udiskd *d;
    pcid *nvme_controller;
    struct{
        gchar *oui;
        gchar *vendor;
    } wwid_oui;
    vendor_list vendors;
} u2driveext;


GSList *get_udisks2_drives_ext();

u2driveext* u2drive_ext(udiskd * udisks_drive_data);
void u2driveext_free(u2driveext *u);

void udisks2_shutdown(void);
void storage_shutdown(void);
