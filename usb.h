#ifndef __USB_H__
#define __USB_H__

#include "hardinfo.h"

typedef struct _USBDevice	USBDevice;

struct _USBDevice {
	gchar *product;
	gchar *vendor;
	gchar *class;
	
	gfloat version, revision;
	
	gint bus, port, vendor_id, prod_id, class_id;
	
	gchar *driver;

	USBDevice *next;
};


USBDevice *hi_scan_usb(void);
void hi_show_usb_info(MainWindow *mainwindow, USBDevice *device);

gboolean usb_update(gpointer data);

#endif
