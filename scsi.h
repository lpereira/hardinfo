#ifndef __SCSI_H__
#define __SCSI_H__

typedef struct _SCSIDevice	SCSIDevice;

struct _SCSIDevice {
	gchar *model;
	
	gchar *vendor;
	gchar *type;
	gchar *revision;
	gint controller;
	gint channel;
	gint id;
	gint lun;

	SCSIDevice *next;
};

void hi_show_scsi_info(MainWindow *mainwindow, SCSIDevice *device);
SCSIDevice *hi_scan_scsi(void);

#endif
