#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "hardinfo.h"

typedef struct _SerialDevice	SerialDevice;

struct _SerialDevice {
	gchar *name;
	gchar *uart;
	gint port, irq, baud;

	SerialDevice *next;
};

SerialDevice *hi_scan_serial(void);
void hi_show_serial_info(MainWindow *mainwindow, SerialDevice *device);

#endif
