#ifndef __HARDINFO_H__
#define __HARDINFO_H__

#define walk_until(x)   while(*buf != x) buf++
#define walk_until_inclusive(x) walk_until(x); buf++

#define IMG_PREFIX "/usr/share/hardinfo/pixmaps/"

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

typedef struct _GenericDevice	GenericDevice;
typedef enum   _DeviceType	DeviceType;

typedef struct _MainWindow	MainWindow;

enum _DeviceType {
	NONE, PCI, ISAPnP, USB,
	IDE, SCSI, SERIAL, PARPORT
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

GtkCTreeNode *tree_group_new(MainWindow *mainwindow, gchar *name,
        DeviceType type);
void tree_insert_item(MainWindow *mainwindow, GtkCTreeNode *group, gchar *name,
                gpointer data);
void hi_insert_generic(gpointer device, DeviceType type);

#endif
