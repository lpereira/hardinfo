#ifndef __COMPUTER_H__
#define __COMPUTER_H__

#include <glib.h>

#define DB_PREFIX	"/etc/"

typedef struct _ComputerInfo ComputerInfo;
typedef struct _MemoryInfo   MemoryInfo;

struct _MemoryInfo {
	gint total;
	gint used;
	gint cached;

	gdouble ratio;
};

struct _ComputerInfo {
	gchar *distrocode;
	gchar *distroinfo;
	
	gchar *kernel;
	
	gchar *hostname;
	
	gint  procno;
	gchar *processor;
	gint  frequency;
	gint  family, model, stepping;
	gint  cachel2;
	gint  bogomips;
};

ComputerInfo *computer_get_info(void);
MemoryInfo *memory_get_info(void);

GtkWidget *os_get_widget(MainWindow *mainwindow);
GtkWidget *memory_get_widget(MainWindow *mainwindow);
GtkWidget *processor_get_widget(void);

gboolean uptime_update(gpointer data);
gboolean memory_update(gpointer data);

#endif
