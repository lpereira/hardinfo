#ifndef __IDE_H__
#define __IDE_H__

typedef struct _IDEDevice	IDEDevice;

struct _IDEDevice {
	gchar *model;
	
	gchar *media;
	gint cache;

	IDEDevice *next;
};

void hi_show_ide_info(MainWindow *mainwindow, IDEDevice *device);
IDEDevice *hi_scan_ide(void);

#endif
