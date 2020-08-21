/*
 * Distribuition detection routines
 * Copyright (c) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * Public domain.
 */

#include "hardinfo.h"
#include "computer.h"

#include <sys/utsname.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

static struct {
	gchar *file, *codename;
} distro_db [] = {
        { DB_PREFIX "debian_version",           "deb"  },
        { DB_PREFIX "slackware-version",        "slk"  },
        { DB_PREFIX "mandrake-release",         "mdk"  },
        { DB_PREFIX "gentoo-release",           "gnt"  },
        { DB_PREFIX "conectiva-release",        "cnc"  },
        { DB_PREFIX "versão-conectiva",         "cnc"  },
        { DB_PREFIX "turbolinux-release",       "tl"   },
        { DB_PREFIX "yellowdog-release",        "yd"   },
        { DB_PREFIX "SuSE-release",             "suse" },

	/*
	 * RedHat must be the *last* one to be checked, since
	 * some distros (like Mandrake) includes a redhat-relase
	 * file too.
	 */ 
	 
        { DB_PREFIX "redhat-release",           "rh"   },
        { NULL, NULL }
};

#define get_int_val(var)  { \
		walk_until_inclusive(':'); buf++; \
		var = atoi(buf); \
		continue; \
	}

#define get_str_val(var)  { \
		walk_until_inclusive(':'); buf++; \
		var = g_strdup(buf); \
		continue; \
	}

MemoryInfo *memory_get_info(void)
{
	MemoryInfo *mi;
	FILE *procmem;
	gchar buffer[64];
	gint memfree, memused;

	mi = g_new0(MemoryInfo, 1);

	procmem = fopen("/proc/meminfo", "r");
	while (fgets(buffer, 128, procmem)) {
		gchar *buf = buffer;
		
		buf = g_strstrip(buf);
		
		if(!strncmp(buf, "MemTotal", 8))
			get_int_val(mi->total)
		else if(!strncmp(buf, "MemFree", 7))
			get_int_val(memfree)
		else if(!strncmp(buf, "Cached", 6))
			get_int_val(mi->cached)

	}
	fclose(procmem);

	mi->used  = mi->total - memfree;

	mi->total/=1000;
	mi->cached/=1000;
	mi->used/=1000;	
	memfree/=1000;

	memused = mi->total - mi->used + mi->cached;

#if 0
	printf("total    = %d\n"
	       "cached   = %d\n"
	       "used     = %d\n"
	       "free     = %d\n"
	       "realused = %d\n"
	       "ratio    = %f\n\n", mi->total, mi->cached, mi->used, memfree,
	       		memused, (gdouble)memused/mi->total);
#endif	
	mi->ratio=1 - (gdouble)memused/mi->total;

	return mi;
}

static void computer_processor_info(ComputerInfo *ci)
{
	FILE *proccpu;
	gchar buffer[128];

	proccpu = fopen("/proc/cpuinfo", "r");
	while (fgets(buffer, 128, proccpu)) {
		gchar *buf = buffer;
		
		buf = g_strstrip(buf);
	
		if(!strncmp(buf, "cpu family", 10))
			get_int_val(ci->family)
		else if(!strncmp(buf, "model name", 10))
			get_str_val(ci->processor)
		else if(!strncmp(buf, "stepping", 8))
			get_int_val(ci->stepping)
		else if(!strncmp(buf, "cpu MHz", 7))
			get_int_val(ci->frequency)
		else if(!strncmp(buf, "cache size", 10))
			get_int_val(ci->cachel2)
		else if(!strncmp(buf, "model", 5))
			get_int_val(ci->model)
	}
	
	fclose(proccpu);

}

ComputerInfo *computer_get_info(void)
{
	gint i;
	struct stat st;
	ComputerInfo *ci;
	struct utsname utsbuf;

	ci = g_new0(ComputerInfo, 1);

	for (i = 0; ; i++) {
		if (distro_db[i].file == NULL) {
			ci->distrocode = g_strdup("unk");
			ci->distroinfo = g_strdup("Unknown distribuition");
			break;
		}
		
		if (!stat(distro_db[i].file, &st)) {
			FILE *distro_ver;
			char buf[128];
			
			distro_ver = fopen(distro_db[i].file, "r");
			fgets(buf, 128, distro_ver);
			fclose(distro_ver);
	
			buf[strlen(buf)-1]=0;

			/*
			 * Some Debian systems doesn't include
			 * the distribuition name in /etc/debian_release,
			 * so add them here. This is a hack, though...
			 */
			if (!strncmp(distro_db[i].codename, "deb", 3) &&
				buf[0] >= '0' && buf[0] <= '9') {
				ci->distroinfo = g_strdup_printf
					("Debian GNU/Linux %s", buf);
			} else {
				ci->distroinfo = g_strdup(buf);
			}
			

			ci->distrocode = g_strdup(distro_db[i].codename);
						
			break;
		}
	}

	uname(&utsbuf);
	
	ci->kernel = g_strdup_printf("%s %s (%s)", utsbuf.sysname,
		utsbuf.release, utsbuf.machine);

	ci->hostname = g_strdup(utsbuf.nodename);

	computer_processor_info(ci);

	return ci;
}

/*
 * Code stolen from GKrellM <http://www.gkrellm.net>
 * Copyright (c) 1999-2002 Bill Wilson <bill@gkrellm.net>
 */
gboolean uptime_update(gpointer data)
{
	MainWindow *mainwindow = (MainWindow *) data;
	gchar *buf;
	gint days, hours;
	FILE *procuptime;
	gulong minutes = 0;
	
	if(!mainwindow) return FALSE;
	
#define plural(a) (a == 1) ? "" : "s"

	if ((procuptime = fopen("/proc/uptime", "r")) != NULL) {
		fscanf(procuptime, "%lu", &minutes);
		minutes /= 60;
		fclose(procuptime);
	} else
		return FALSE;
	
	hours	 = minutes / 60;
	minutes	%= 60;
	days	 = hours / 24;
	hours	%= 24;
	
	if (days < 1) {
		buf = g_strdup_printf("%d hour%s and %ld minute%s", hours,
				plural(hours), minutes, plural(minutes)); 
	} else {
		buf = g_strdup_printf("%d day%s, %d hour%s and %ld minute%s",
				days, plural(days), hours, plural(hours),
				minutes, plural(minutes)); 
	}

	gtk_label_set_text(GTK_LABEL(mainwindow->uptime), buf);
	g_free(buf);
	
	return TRUE;
}

GtkWidget *os_get_widget(MainWindow *mainwindow)
{
	GtkWidget *label, *hbox;
	GtkWidget *table;
#ifdef GTK2
	GtkWidget *pixmap;
	gchar *buf;
#endif
	ComputerInfo *info;
		
	if(!mainwindow) return NULL;
		
	info = computer_get_info();
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);

#ifdef GTK2	
	buf = g_strdup_printf("%s/distro/%s.png", IMG_PREFIX, info->distrocode);
	pixmap = gtk_image_new_from_file(buf);
	gtk_widget_set_usize(GTK_WIDGET(pixmap), 64, 0);
	gtk_widget_show(pixmap);
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
	g_free(buf);
#endif

	table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(table), 10);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);

	/*
	 * Table headers
	 */ 
#ifdef GTK2
	label = gtk_label_new("<b>Computer name:</b>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new("Computer name:");
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

#ifdef GTK2
	label = gtk_label_new("<b>Distribution:</b>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new("Distribution:");
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

#ifdef GTK2
	label = gtk_label_new("<b>Kernel:</b>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new("Kernel:");
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

#ifdef GTK2
	label = gtk_label_new("<b>Uptime:</b>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new("Uptime:");
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);


	/*
	 * Table content 
	 */ 
	label = gtk_label_new(info->hostname);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	label = gtk_label_new(info->distroinfo);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	
	label = gtk_label_new(info->kernel);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 3);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	label = gtk_label_new("Updating...");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 3, 4);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	mainwindow->uptime = label;

	uptime_update(mainwindow);
	gtk_timeout_add(30000, uptime_update, mainwindow);

	g_free(info);
	
	return hbox;
}

gboolean memory_update(gpointer data)
{
	MainWindow *mainwindow = (MainWindow*) data;
	MemoryInfo *mi;
	
	if(!mainwindow) return FALSE;

	mi = memory_get_info();
	
#ifdef GTK2
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mainwindow->membar),
		mi->ratio);
#else
	gtk_progress_set_percentage(GTK_PROGRESS(mainwindow->membar),
		mi->ratio);
#endif
	
	g_free(mi);
	
	return TRUE;
}

GtkWidget *memory_get_widget(MainWindow *mainwindow)
{
	GtkWidget *label, *vbox, *hbox, *hbox2, *progress;
#ifdef GTK2
	GtkWidget *pixmap;
#endif
	MemoryInfo *mi;
	gchar *buf;
		
	mi = memory_get_info();
		
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);

#ifdef GTK2	
	buf = g_strdup_printf("%s/mem.png", IMG_PREFIX);
	pixmap = gtk_image_new_from_file(buf);
	gtk_widget_set_usize(GTK_WIDGET(pixmap), 64, 0);
	gtk_widget_show(pixmap);
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
	g_free(buf);
#endif

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_box_set_spacing(GTK_BOX(vbox), 4);

	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, TRUE, 0);

	label = gtk_label_new("0MB");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	buf = g_strdup_printf("%dMB", mi->total);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_end(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	g_free(buf);

	progress = gtk_progress_bar_new();
	mainwindow->membar = progress;
	gtk_widget_show(progress);
	gtk_box_pack_start(GTK_BOX(vbox), progress, TRUE, TRUE, 0);

	memory_update(mainwindow);

	gtk_timeout_add(2000, memory_update, mainwindow);

	g_free(mi);	
	return hbox;
}

GtkWidget *processor_get_widget(void)
{
	GtkWidget *label, *vbox, *hbox;
#ifdef GTK2
	GtkWidget *pixmap;
#endif
	ComputerInfo *info;
	gchar *buf;
		
	info = computer_get_info();
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);

#ifdef GTK2	
	buf = g_strdup_printf("%s/cpu.png", IMG_PREFIX);
	pixmap = gtk_image_new_from_file(buf);
	gtk_widget_set_usize(GTK_WIDGET(pixmap), 64, 0);
	gtk_widget_show(pixmap);
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
	g_free(buf);
#endif

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_box_set_spacing(GTK_BOX(vbox), 4);

#ifdef GTK2
	buf = g_strdup_printf("<b>%s</b> at %d MHz", info->processor,
		info->frequency);
#else
	buf = g_strdup_printf("%s at %d MHz", info->processor,
		info->frequency);
#endif
	label = gtk_label_new(buf);
	gtk_widget_show(label);
#ifdef GTK2
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	g_free(buf);

	buf = g_strdup_printf("Family %d, model %d, stepping %d",
		info->family, info->model, info->stepping);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	g_free(buf);

	buf = g_strdup_printf("%d KB L2 cache", info->cachel2);
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	g_free(buf);

	g_free(info);
	return hbox;
}

