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
#include "isapnp.h"

ISADevice *hi_scan_isapnp(void)
{
	FILE *proc_isapnp;
	gchar buffer[256], *buf;
	gint n=0;
	ISADevice *isa_dev, *isa;
	struct stat st;
	
	isa = NULL;
	
	if(stat("/proc/isapnp", &st)) return NULL;

	proc_isapnp = fopen("/proc/isapnp", "r");
	while(fgets(buffer, 256, proc_isapnp)){
		buf = g_strstrip(buffer);
		if(!strncmp(buf, "Card", 4)){
			gboolean lock = FALSE;
			gfloat pnpversion, prodversion;
			gint card_id;
			gpointer start, end;
			
			sscanf(buf, "Card %d", &card_id);
			
			for (; buf != NULL; buf++) {
				if (lock && *buf == '\'') {
					end = buf;
					break;
				} else if (!lock && *buf == ':') {
					start = buf+1;
					lock = TRUE;
				}
			}
			buf+=2;
			
			sscanf(buf, "PnP version %f Product version %f", &pnpversion, &prodversion);
			
			buf = end;
			*buf=0;
			buf = start;
			
			isa_dev = g_new0(ISADevice, 1);
			
			isa_dev->next = isa;
			isa = isa_dev;
			
			isa_dev->pnpversion = pnpversion;
			isa_dev->prodversion = prodversion;
			isa_dev->card_id = card_id;
			
			isa_dev->card = g_strdup(buf);
			
			n++;			
		}
	}
	fclose(proc_isapnp);
	
	return isa;
}

void hi_show_isa_info(MainWindow *mainwindow, ISADevice *device)
{
	GtkWidget *hbox, *vbox, *label;
	gchar *buf;
#ifdef GTK2
	GtkWidget *pixmap;
	
	pixmap = gtk_image_new_from_file(IMG_PREFIX "pci.png");
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

	gtk_frame_set_label(GTK_FRAME(mainwindow->frame), "ISA Plug and Play Device");
	
#ifdef GTK2
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
#endif

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

#ifdef GTK2
	buf = g_strdup_printf("<b>%s</b>", device->card);
	label = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	
	g_free(buf);
#else
	label = gtk_label_new(device->card);
#endif
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
	buf = g_strdup_printf("Card ID: %d", device->card_id);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);

	buf = g_strdup_printf("PnP version: %.2f, Product version: %.2f",
			device->pnpversion, device->prodversion);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);
}

