#ifndef __HARDINFO_H__
#define __HARDINFO_H__

#define walk_until(x)   while((*buf != '\0') && (*buf != x)) buf++
#define walk_until_inclusive(x) walk_until(x); buf++

#define PREFIX		"/usr/share/hardinfo/"
#define IMG_PREFIX	PREFIX "pixmaps/"

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

#ifdef ENABLE_NLS
#define	INTL_PREFIX	PREFIX "lang/"
#include "intl.h"
#else
#define _(x)	(x)
#endif

typedef struct _GenericDevice	GenericDevice;
typedef enum   _DeviceType	DeviceType;

typedef struct _MainWindow	MainWindow;

enum _DeviceType {
	NONE, PCI, ISAPnP, USB,
	IDE, SCSI, SERIAL, PARPORT,
	V4L
};

struct _GenericDevice {
	gpointer device;
	DeviceType type;

	GtkCTreeNode *node;
	
	GenericDevice *next;
};

struct _MainWindow {
	GtkWidget *window;
	
	GtkWidget *ctree;
	
	GtkWidget *frame;
	GtkWidget *framec;
	
	GtkWidget *membar;
	GtkWidget *uptime;	
};

extern GenericDevice *generic_devices;

#include "ide.h"
#include "scsi.h"
#include "isapnp.h"
#include "usb.h"
#include "pci.h"
#include "serial.h"
#include "parport.h"
#include "v4l.h"

#include "x11.h"

#include "about.h"

GtkCTreeNode *tree_group_new(MainWindow *mainwindow, const gchar *name,
        DeviceType type);
void tree_insert_item(MainWindow *mainwindow, GtkCTreeNode *group, gchar *name,
                gpointer data);
void hi_insert_generic(gpointer device, DeviceType type);

#endif
