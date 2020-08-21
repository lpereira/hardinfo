/*
 * Hardware Information, version 0.3
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * May be modified and/or distributed under the terms of GNU GPL version 2.
 *
 * Tested only with 2.4.x kernels on ix86.
 * USB support needs usbdevfs.
 */

/*
 * SCSI support by Pascal F.Martin <pascalmartin@earthlink.net>
 */

#include "hardinfo.h"
#include "scsi.h"

SCSIDevice *hi_scan_scsi(void)
{
	FILE *proc_scsi;
	gchar buffer[256], *buf;
	gint n=0;
	SCSIDevice *scsi_dev, *scsi;
	struct stat st;
	
	scsi = NULL;
	
	if(stat("/proc/scsi/scsi", &st)) return NULL;

	proc_scsi = fopen("/proc/scsi/scsi", "r");
	while(fgets(buffer, 256, proc_scsi)) {
		buf = g_strstrip(buffer);
		if(!strncmp(buf, "Host: scsi", 10)) {
			gint scsi_controller;
			gint scsi_channel;
			gint scsi_id;
			gint scsi_lun;

			sscanf(buf,
				"Host: scsi%d Channel: %d Id: %d Lun: %d",
				&scsi_controller,
				&scsi_channel,
				&scsi_id,
				&scsi_lun);

			buf = strstr (buffer, "Rev: ");
			if (buf == NULL) {
				buf = "(unknown)";
			} else {
				buf += 5;
			}
			scsi_dev = g_new0(SCSIDevice, 1);
			
			scsi_dev->next = scsi;
			scsi = scsi_dev;
			
			scsi_dev->controller = scsi_controller;
			scsi_dev->channel = scsi_channel;
			scsi_dev->id = scsi_id;
			scsi_dev->lun = scsi_lun;

			n++;			

		} else if (!strncmp(buf, "Vendor: ", 8)) {

			char *p;
			char *model = strstr (buf, "Model: ");
			char *rev = strstr (buf, "Rev: ");

			if (model == NULL) {
				model = buf + strlen(buf);
			}
			p = model;
			while (*(--p) == ' ') ;
			*(++p) = 0;
			scsi_dev->vendor = g_strdup(buf+8);

			if (rev != NULL) {
				scsi_dev->revision = g_strdup(rev+5);
			} else {
				rev = model + strlen(model);
			}
			p = rev;
			while (*(--p) == ' ') ;
			*(++p) = 0;
			scsi_dev->model =
				g_strdup_printf
					("%s %s", scsi_dev->vendor, model+7);

		} else if (!strncmp(buf, "Type:   ", 8)) {
			char *p = strstr (buf, "ANSI SCSI revi");

			if (p != NULL) {
				while (*(--p) == ' ') ;
				*(++p) = 0;
				scsi_dev->type = g_strdup(buf+8);
			}
		}
	}
	fclose(proc_scsi);
	
	return scsi;
}

void hi_show_scsi_info(MainWindow *mainwindow, SCSIDevice *device)
{
        static struct {
                char *type;
                char *label;
                char *icon; 
        } type2icon[] = {   
                {"Direct-Access",       "Disk",         "hdd.png"},
                {"Sequential-Access",   "Tape",         "tape.png"},
                {"Printer",             "Printer",      "lpr.png"}, 
                {"WORM",                "CD-ROM",       "cd.png"},  
                {"CD-ROM",              "CD-ROM",       "cd.png"},  
                {"Scanner",             "Scanner",      "scan.png"},
                {NULL,                  "Generic",      "scsi.png"} 
        };

        int i;
        GtkWidget *hbox, *vbox, *label;
        gchar *buf;
#ifdef GTK2
        GtkWidget *pixmap;
#endif

        if(!device) return;

        for (i = 0; type2icon[i].type != NULL; ++i) {
                if (!strcmp(device->type, type2icon[i].type)) break;
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
        buf = g_strdup_printf("SCSI %s Device", type2icon[i].label);
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

        buf = g_strdup_printf("Revision: %s", device->revision);
        label = gtk_label_new(buf);
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
        g_free(buf);

        buf = g_strdup_printf("Type: %s", device->type);
        label = gtk_label_new(buf);
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
        g_free(buf);

        buf = g_strdup_printf
                        ("Controller: %d, Bus: %d, ID: %d, LUN: %d",
                              device->controller,
                              device->channel,   
                              device->id,
                              device->lun);
        label = gtk_label_new(buf);
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
        g_free(buf);
}
