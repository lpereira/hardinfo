/*
 * Hardware Information, version 0.3
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * May be modified and/or distributed under the terms of GNU GPL version 2.
 *
 * Tested only with 2.4.x kernels on ix86.
 * USB support needs usbdevfs.
 */

#include "hardinfo.h"
#include "ide.h"

#include <stdlib.h>

IDEDevice *hi_scan_ide(void)
{
	FILE *proc_ide;
	gchar *device, iface;
	gint n=0, i=0;
	struct stat st;
	IDEDevice *ide_dev, *ide;
	
	ide = NULL;	
	
	for (i=0; i<=4; i++) {
		iface='a'+i;
		device = g_strdup_printf("/proc/ide/hd%c/model", iface);
		if (!stat(device, &st)) { 
			gchar buf[64];
			
			ide_dev = g_new0(IDEDevice, 1);
			ide_dev->next = ide;
			ide = ide_dev;
			
			proc_ide = fopen(device, "r");
			fgets(buf, 64, proc_ide);
			fclose(proc_ide);
			
			buf[strlen(buf)-1]=0;
			
			ide_dev->model = g_strdup(buf);
			
			g_free(device);
			
			device = g_strdup_printf("/proc/ide/hd%c/media", iface);
			proc_ide = fopen(device, "r");
			fgets(buf, 64, proc_ide);
			fclose(proc_ide);
			buf[strlen(buf)-1]=0;
			
			ide_dev->media = g_strdup(buf);
						
			g_free(device);
			
			device = g_strdup_printf("/proc/ide/hd%c/cache", iface);
			if (!stat(device, &st)) {
				proc_ide = fopen(device, "r");
				fgets(buf, 64, proc_ide);
				fclose(proc_ide);
			
				ide_dev->cache = atoi(buf);
			}					
			n++;
		}
		g_free(device);
	}
	
	return ide;
}

void hi_show_ide_info(MainWindow *mainwindow, IDEDevice *device)
{
	GtkWidget *hbox, *vbox, *label;
        static struct {
                char *type;
                char *label;
                char *icon; 
        } type2icon[] = {   
                {"cdrom",       "CD-ROM",         "cd.png"},
                {"disk",	"Hard Disk",	  "hdd.png"}
        };
        int i;
	gchar *buf;
#ifdef GTK2
	GtkWidget *pixmap;	
#endif

	if(!device) return;
	
	for (i = 0; type2icon[i].type != NULL; ++i) {
		if (!strcmp(device->media, type2icon[i].type)) break;
	}
	
#ifdef GTK2
	buf = g_strdup_printf("%s%s", IMG_PREFIX, type2icon[i].icon);
	pixmap = gtk_image_new_from_file(buf);
	gtk_widget_show(pixmap);
	
	g_free(buf);
#endif
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
	gtk_widget_show(hbox);
	
	if(mainwindow->framec)
		gtk_widget_destroy(mainwindow->framec);

	gtk_container_add(GTK_CONTAINER(mainwindow->frame), hbox);
	mainwindow->framec = hbox;

	buf = g_strdup_printf("ATA/IDE %s Device", type2icon[i].label);
	gtk_frame_set_label(GTK_FRAME(mainwindow->frame), buf);
	g_free(buf);
	
#ifdef GTK2
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
#endif

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

#ifdef GTK2
	buf = g_strdup_printf("<b>%s</b>", device->model);
	label = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	
	g_free(buf);
#else
	label = gtk_label_new(device->model);
#endif
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	if (device->cache) {
		buf = g_strdup_printf("Cache: %d KB", device->cache);
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
		g_free(buf);
	}	
}
