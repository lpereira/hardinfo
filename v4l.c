/*
 * Hardware Information, version 0.3
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * May be modified and/or distributed under the terms of GNU GPL version 2.
 *
 */

#include "hardinfo.h"
#include "v4l.h"

#include <stdlib.h>
#include <dirent.h>

#define get_str_val(var) { \
	        walk_until_inclusive(':'); buf++; \
	        var = g_strdup(buf); \
        	continue; \
 	}

V4LDevice *hi_scan_v4l(void)
{
	FILE *proc_video;
	DIR *proc_dir;
	struct dirent *sd;
	V4LDevice *v4l_dev, *v4l;
	struct stat st;
	
	v4l = NULL;

	if (stat("/proc/video/dev", &st))
		return NULL;
	
	proc_dir = opendir("/proc/video/dev");
	if(!proc_dir)
		return NULL;

	while (sd = readdir(proc_dir)) {
		gchar *dev, buffer[128];
		
		dev = g_strdup_printf("/proc/video/dev/%s", sd->d_name);
		proc_video = fopen(dev, "r");
		g_free(dev);
		
		if(!proc_video)
			continue;
			
		v4l_dev = g_new0(V4LDevice, 1);
		v4l_dev->next = v4l;
		v4l = v4l_dev;
		
		while (fgets(buffer, 128, proc_video)) {
			char *buf = g_strstrip(buffer);
			
			if(!strncmp(buf, "name", 4))
				get_str_val(v4l_dev->name)
			else if(!strncmp(buf, "type", 4))
				get_str_val(v4l_dev->type)
		}		

		fclose(proc_video);		
	}

	return v4l;
}

void hi_show_v4l_info(MainWindow *mainwindow, V4LDevice *device)
{
	GtkWidget *hbox, *vbox, *label;
	gchar *buf;
#ifdef GTK2
	GtkWidget *pixmap;	
#endif

	if(!device) return;

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
	gtk_widget_show(hbox);
	
#ifdef GTK2
	buf = g_strdup_printf("%sv4l.png", IMG_PREFIX);
	pixmap = gtk_image_new_from_file(buf);
	gtk_widget_show(pixmap);
	
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
	
	g_free(buf);
#endif

	if(mainwindow->framec)
		gtk_widget_destroy(mainwindow->framec);

	gtk_container_add(GTK_CONTAINER(mainwindow->frame), hbox);
	mainwindow->framec = hbox;
	
	gtk_frame_set_label(GTK_FRAME(mainwindow->frame), _("Device Information"));
	
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

#ifdef GTK2
	buf = g_strdup_printf("<b>%s</b>", device->name);
	
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
	g_free(buf);
#else
	label = gtk_label_new(device->name);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
#endif

	if (device->type) {
		gchar *b = g_strdup(device->type);
		gpointer b_start = b;
	
		do {
			if (*b == '|') *b = '\n';
			b++;
		} while(*b);
		b = b_start;
	
		buf = g_strdup_printf("Type:\n%s", b);
	
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);
		g_free(b);
	}
}
