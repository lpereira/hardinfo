#ifndef __X11_H__
#define __X11_H__

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct _X11Info	X11Info;

struct _X11Info {
	gchar *display;
	gchar *version;
	gchar *vendor;
	gchar *xf86version;
	gchar *screen_size;
};

X11Info	*x11_get_info(void);
GtkWidget *x11_get_widget(MainWindow *mainwindow);

#endif
