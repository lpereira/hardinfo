#ifndef __DEVICES_H__
#define __DEVICES_H__

#include "hardinfo.h"
#include "processor-platform.h"
#include "dmi_util.h"

typedef struct _Processor Processor;

#define WALK_UNTIL(x)   while((*buf != '\0') && (*buf != x)) buf++

#define GET_STR(field_name,ptr)      					\
  if (!ptr && strstr(tmp[0], field_name)) {				\
    ptr = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));	\
    g_strfreev(tmp);                 					\
    continue;                        					\
  }

#define get_str(field_name,ptr)               \
  if (g_str_has_prefix(tmp[0], field_name)) { \
    ptr = g_strdup(tmp[1]);                   \
    g_strfreev(tmp);                          \
    continue;                                 \
  }
#define get_int(field_name,ptr)               \
  if (g_str_has_prefix(tmp[0], field_name)) { \
    ptr = atoi(tmp[1]);                       \
    g_strfreev(tmp);                          \
    continue;                                 \
  }
#define get_float(field_name,ptr)             \
  if (g_str_has_prefix(tmp[0], field_name)) { \
    ptr = atof(tmp[1]);                       \
    g_strfreev(tmp);                          \
    continue;                                 \
  }


/* Processor */
GSList *processor_scan(void);
void get_processor_strfamily(Processor * processor);
gchar *processor_get_detailed_info(Processor * processor);
gchar *processor_get_info(GSList * processors);
gchar *processor_name(GSList * processors);
gchar *processor_name_default(GSList * processors);
gchar *processor_describe(GSList * processors);
gchar *processor_describe_default(GSList * processors);
gchar *processor_describe_by_counting_names(GSList * processors);
gchar *processor_frequency_desc(GSList *processors);

/* Printers */
void init_cups(void);

/* Battery */
void scan_battery_do(void);

/* PCI */
void scan_pci_do(void);

/* Printers */
void scan_printers_do(void);

/* Sensors */
#define SENSORS_GROUP_BY_TYPE 1

void scan_sensors_do(void);
void sensor_init(void);
void sensor_shutdown(void);
void __scan_dtree(void);
void scan_gpu_do(void);
gboolean __scan_udisks2_devices(void);
void __scan_ide_devices(void);
void __scan_scsi_devices(void);
void __scan_input_devices(void);
void __scan_usb(void);
void __scan_dmi(void);

extern gchar *powerstate;
extern gchar *gpuname;
extern gchar *battery_list;
extern gchar *input_icons;
extern gchar *input_list;
extern gchar *lginterval;
extern gchar *meminfo;
extern gchar *pci_list;
extern gchar *printer_icons;
extern gchar *printer_list;
extern gchar *sensors;
extern gchar *sensor_icons;
extern gchar *storage_icons;
extern gchar *storage_list;
extern gchar *usb_list;
extern gchar *usb_icons;
extern GHashTable *_pci_devices;
extern GHashTable *sensor_compute;
extern GHashTable *sensor_labels;
extern GModule *cups;

extern gchar *dmi_info;
extern gchar *dtree_info;
extern gchar *gpu_list;
extern gchar *gpu_summary;

#endif /* __DEVICES_H__ */
