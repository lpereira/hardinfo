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
#include "parport.h"

#define srch_def(str,len,var) \
	if (!strncmp(buffer, str, len)) { \
		walk_until_inclusive(':'); \
		parport_dev->var = g_strdup(buf); \
	}

ParportDevice *hi_scan_parport(void)
{
	FILE *autoprobe;
	struct stat st;
	gint n=0, i;
	ParportDevice *parport_dev, *parport;

	parport = NULL;
	
	for (i = 0; i <= 16; i++) {
		gchar *file;
		gchar buffer[128];
		gint port, dma;
		
		file = g_strdup_printf(PARPORT_PROC_BASE "parport%d", i);

		if (stat(file, &st)) {
			g_free(file);
			continue;
		}
		g_free(file);

		file = g_strdup_printf
			(PARPORT_PROC_BASE "parport%d/autoprobe", i);

		parport_dev = g_new0(ParportDevice, 1);		
		parport_dev->next = parport;
		parport = parport_dev;
		
		n++;

		parport_dev->number = i;
		
		autoprobe = fopen(file, "r");
		while (autoprobe && fgets(buffer, 128, autoprobe)) {
			char *buf;

			buf = g_strstrip(buffer);
			*(buf + strlen(buf) - 1) = 0;
			
			srch_def("CLASS:", 6, pclass);
			srch_def("MODEL:", 6, model);
			srch_def("MANUFA", 6, manufacturer);
			srch_def("DESCRI", 6, description);
			srch_def("COMMAN", 6, cmdset);
		}
		if(autoprobe) fclose(autoprobe);
		
		g_free(file);
		
		if (parport_dev->model) {
			parport_dev->name = 
				g_strdup_printf("%s %s (lp%d)",
					parport_dev->manufacturer,
					parport_dev->model, i);
		} else {
			parport_dev->name =
				g_strdup_printf ("Parallel port (lp%d)", i);
		}

		file = g_strdup_printf
			(PARPORT_PROC_BASE "parport%d/base-addr", i);
		autoprobe = fopen(file, "r");
		if (autoprobe) {
			fscanf(autoprobe, "%d", &port);
			fclose(autoprobe);
		
			parport_dev->port = port;
		}		
		g_free(file);
		
		file = g_strdup_printf
			(PARPORT_PROC_BASE "parport%d/dma", i);
		autoprobe = fopen(file, "r");
		if (autoprobe) {
			fscanf(autoprobe, "%d", &dma);
			fclose(autoprobe);

			parport_dev->dma = (dma == -1) ? FALSE : TRUE;
		}
		g_free(file);
		
		file = g_strdup_printf
			(PARPORT_PROC_BASE "parport%d/modes", i);
		autoprobe = fopen(file, "r");
		if (autoprobe) {
			gchar modes[64];
			
			fgets(modes, 64, autoprobe);
			fclose(autoprobe);

			modes[strlen(modes)-1]=0;
			parport_dev->modes = g_strdup(modes);

		}
		if(!parport_dev->modes)
			parport_dev->modes = g_strdup(_("N/A"));
			
		g_free(file);
	}
	
	return parport;

}

void hi_show_parport_info(MainWindow *mainwindow, ParportDevice *device)
{
	GtkWidget *hbox, *vbox, *label;
	gchar *buf;
	static struct {
		gchar *type, *label, *icon;
	} type2icon[] = {
		{"PRINTER",	"Printer",		"lpr.png"},
		{"MEDIA",	"Multimedia",	"media.png"},		
		{NULL,		"Legacy Device",	"gen_connector.png"},
	};
	gint i;
#ifdef GTK2
	GtkWidget *pixmap;		
#endif

	if(!device) return;

	if (device->pclass) {
		for (i = 0; type2icon[i].type != NULL; ++i)
			if(!strcmp(device->pclass, type2icon[i].type))
				break;
	} else
		i = sizeof(type2icon) / sizeof(type2icon[0]) - 1;
		

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

	gtk_frame_set_label(GTK_FRAME(mainwindow->frame), _("Parallel Port"));
	
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

	if (device->description) {
		buf = g_strdup_printf(_("Description: %s"), device->description);
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);
	}
	
	if (device->cmdset) {
		buf = g_strdup_printf(_("Command set: %s"), device->cmdset);
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);
	}

	buf = g_strdup_printf(_("Class: %s"), type2icon[i].label);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);	
	g_free(buf);	

	buf = g_strdup_printf(_("Base I/O address: 0x%x"), device->port);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);	
	g_free(buf);	

	buf = g_strdup_printf(_("Modes: %s"), device->modes);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);	
	g_free(buf);	

	if (device->dma) {
		label = gtk_label_new(_("Uses DMA"));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}

}
