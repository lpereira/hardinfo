#ifndef __V4L_H__
#define __V4L_H__

typedef struct _V4LDevice V4LDevice;

struct _V4LDevice {
	gchar *name;
	gchar *type;

	V4LDevice *next;
};

void hi_show_v4l_info(MainWindow *mainwindow, V4LDevice *device);
V4LDevice *hi_scan_v4l(void);

#endif
