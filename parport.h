#ifndef __PARPORT_H__
#define __PARPORT_H__

#include "hardinfo.h"

#define PARPORT_PROC_BASE "/proc/sys/dev/parport/"

typedef struct _ParportDevice	ParportDevice;

struct _ParportDevice {
	gchar *name;

	gchar *cmdset;
	gchar *model;
	gchar *manufacturer;
	gchar *description;
	gchar *pclass;

	gint number, port;
	gboolean dma;
	
	gchar *modes;

	ParportDevice *next;
};

ParportDevice *hi_scan_parport(void);
void hi_show_parport_info(MainWindow *mainwindow, ParportDevice *device);

#endif
