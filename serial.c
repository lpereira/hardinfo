/*
 * Hardware Information, version 0.3.1b
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * May be modified and/or distributed under the terms of GNU GPL version 2.
 *
 * Tested only with 2.4.x kernels on ix86.
 * USB support needs usbdevfs.
 */

#include "hardinfo.h"
#include "serial.h"

SerialDevice *hi_scan_serial(void)
{
	FILE *proc_tty;
	struct stat st;
	const gchar *ser_drv="/proc/tty/driver/serial";
	gint n=0;
	SerialDevice *serial_dev, *serial;
	
	serial = NULL;
	
	if (!stat(ser_drv, &st)) {
		gchar buffer[256];
		
		proc_tty = fopen(ser_drv, "r");
		while(fgets(buffer, 256, proc_tty)){
			gint port, irq;
			gpointer start, end;
			gchar *buf = buffer;
		
			if(*buf == 's') continue;
			if(strstr(buf, "unknown")) continue;
		
			serial_dev = g_new0(SerialDevice, 1);
			
			serial_dev->next = serial;
			serial = serial_dev;

			serial_dev->name = g_strdup_printf
				("Serial Port (tty%d)", buffer[0]-'0');



			walk_until('t');
			buf+=2;
			start = buf;
			walk_until(' ');
			end = buf;
			*buf = 0;
			buf = start;
			
			serial_dev->uart = g_strdup(buf);
			
			buf = end;
			*buf = ' ';
			
			sscanf(buf, " port:%x irq:%d", &port, &irq);
			serial->port = port;
			serial->irq = irq;
			n++;
		}
		fclose(proc_tty);
	}
	
	return serial;
}

void hi_show_serial_info(MainWindow *mainwindow, SerialDevice *device)
{
	GtkWidget *hbox, *vbox, *label;
	gchar *buf;
#ifdef GTK2
	GtkWidget *pixmap;
	
	pixmap = gtk_image_new_from_file(IMG_PREFIX "gen_connector.png");
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

	gtk_frame_set_label(GTK_FRAME(mainwindow->frame), _("Communication Port"));
	
#ifdef GTK2
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
#endif

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

#ifdef GTK2
	buf = g_strdup_printf("<b>%s</b>", device->name);
	label = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	
	g_free(buf);
#else
	label = gtk_label_new(device->name);
#endif
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
	buf = g_strdup_printf(_("I/O port: 0x%x, IRQ: %d"), device->port, device->irq);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);

	buf = g_strdup_printf("UART: %s", device->uart);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);
	
}
