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
#include "usb.h"

gboolean usb_update(gpointer data)
{
	MainWindow *mainwindow = (MainWindow*)data;
	GtkCTreeNode *node;
	USBDevice *usb;	
	
	if(!mainwindow) return FALSE;

	g_print("Refreshing USB... ");
	
	gtk_clist_freeze(GTK_CLIST(mainwindow->ctree));

	usb = hi_scan_usb();

        if (usb != NULL) {
                node = tree_group_new(mainwindow, "USB Devices", USB);
                for (; usb != NULL; usb=usb->next) {
                        hi_insert_generic(usb, USB);
                        if (!usb->product) 
                                usb->product = g_strdup_printf
                                        ("Unknown device (%s)", usb->class);
                        
                        tree_insert_item(mainwindow, node, usb->product,
                                         generic_devices);
                }
        }

	gtk_clist_thaw(GTK_CLIST(mainwindow->ctree));

	g_print("done.\n");
	
	return TRUE;	
}

USBDevice *hi_scan_usb(void)
{
	FILE *proc_usb;
	gchar buffer[64];
	gint n=0;
	USBDevice *usb_dev, *usb;
	struct stat st;
	
	usb = NULL;

	if (stat("/proc/bus/usb/devices", &st)) return NULL;

	proc_usb = fopen("/proc/bus/usb/devices", "r");
	while(fgets(buffer, 64, proc_usb)){
		if (strstr(buffer, "Manufacturer=")) {
			gchar *buf = buffer;
			gboolean lock = FALSE;
			gpointer start, end;
			
			for (; buf != NULL; buf++) {
				if (lock && *buf == '\n') {
					end = buf;
					break;
				} else if (!lock && *buf == '=') {
					start = buf+1;
					lock = TRUE;
				}
			}
			
			buf = end;
			*buf = 0;
			buf = start;
			
			usb->vendor = g_strdup(buf);
		} else if (strstr(buffer, "Product=")) {
			gchar *buf = buffer;
			gboolean lock = FALSE;
			gpointer start, end;
			
			for (; buf != NULL; buf++) {
				if (lock && *buf == '\n') {
					end = buf;
					break;
				} else if (!lock && *buf == '=') {
					start = buf+1;
					lock = TRUE;
				}
			}
			
			buf = end;
			*buf=0;
			buf = start;

			usb_dev->product = g_strdup(buf);
		} else if (!strncmp(buffer, "D:  Ve", 6)) {
			gchar *buf = buffer;
			gpointer start;
			gfloat version;
			gint class_id;
			
			usb_dev = g_new0(USBDevice, 1);
			usb_dev->next = usb;
			usb = usb_dev;

			buf+=4;
			
			sscanf(buf, "Ver =%f Cls=%d", &version, &class_id);
			
			usb_dev->version = version;
			usb_dev->class_id= class_id;

			walk_until('(');
			start = buf+1;
			buf+=6;
			*buf = 0;
			buf = start;
			
			usb_dev->class = g_strdup(buf);
			n++;		
		} else if (!strncmp(buffer, "P:  Ve", 6)) {
			gchar *buf = buffer;
			gint vendor_id, prod_id;
			gfloat rev;
			
			buf+=4;
			
			sscanf(buf, "Vendor=%x ProdID=%x Rev= %f",
				&vendor_id, &prod_id, &rev);
			
			usb_dev->vendor_id = vendor_id;
			usb_dev->prod_id = prod_id;
			usb_dev->revision = rev;
		}
	}
	fclose(proc_usb);
	
	return usb;
}

void hi_show_usb_info(MainWindow *mainwindow, USBDevice *device)
{
	GtkWidget *hbox, *vbox, *label;
	gchar *buf;
#ifdef GTK2
	GtkWidget *pixmap;
	
	pixmap = gtk_image_new_from_file(IMG_PREFIX "usb.png");
	gtk_widget_show(pixmap);
#endif

	if(!device) return;

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
	gtk_widget_show(hbox);
	
	if(mainwindow->framec)
		gtk_widget_destroy(mainwindow->framec);

	gtk_container_add(GTK_CONTAINER(mainwindow->frame), hbox);
	mainwindow->framec = hbox;

	gtk_frame_set_label(GTK_FRAME(mainwindow->frame), "USB Device");
	
#ifdef GTK2
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
#endif

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

#ifdef GTK2
	buf = g_strdup_printf("<b>%s</b>", device->product);
	label = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	
	g_free(buf);
#else
	label = gtk_label_new(device->product);
#endif
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	if (device->vendor) {
		buf = g_strdup_printf("Manufacturer: %s", device->vendor);
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
		g_free(buf);
	}

	buf = g_strdup_printf("Class: %s (%d)", device->class, device->class_id);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);

	buf = g_strdup_printf("Version: %.2f, Revision: %.2f", device->version, device->revision);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);

	if(!device->prod_id) return;
	
	buf = g_strdup_printf("Vendor ID: 0x%X, Product ID: 0x%X",
			device->vendor_id, device->prod_id);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);
	
}

