/*
 * Hardware Information, version 0.3.2
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * May be modified and/or distributed under the terms of GNU GPL version 2.
 *
 */

#include "hardinfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "pixmaps/pci.xpm"
#include "pixmaps/usb.xpm"
#include "pixmaps/hdd.xpm"
#include "pixmaps/gen_connector.xpm"
#include "pixmaps/scsi.xpm"

#include "computer.h"

GenericDevice *generic_devices = NULL;

void hi_show_device_info(GtkCTree * tree, GList * node,
			 gint column, gpointer user_data);
void hi_hide_device_info(GtkCTree * tree, GList * node,
			 gint column, gpointer user_data);
void hi_scan_all(MainWindow * mainwindow);

void
main_window_refresh(GtkWidget * widget, gpointer data)
{
	MainWindow *mainwindow = (MainWindow *) data;

	if (!mainwindow)
		return;

	hi_hide_device_info(NULL, NULL, 0, mainwindow);

	memory_update(mainwindow);
	uptime_update(mainwindow);
	hi_scan_all(mainwindow);
}

static void
about_window_create(void)
{
	GtkAbout *about;
	const gchar *authors[] = {
		">Written by:",
			"Leandro Pereira (leandro@linuxmag.com.br)",
		">Disclaimer:",
			"This is free software; you can modify and/or distribute it",
			"under the terms of GNU GPL version 2. See http://www.fsf.org/",
			"for more information.",
		NULL
	};

	about = gtk_about_new("HardInfo", VERSION,
			      _("System information tool for Linux.\n"),
			      authors, IMG_PREFIX "logo.png");

}

MainWindow *
main_window_create(void)
{
	GtkWidget *window, *mbox, *vbox, *frame, *ctree, *scroll;
	GtkWidget *notebook, *label, *hbox, *btn;
	MainWindow *mainwindow;

	mainwindow = g_new0(MainWindow, 1);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 4);
	gtk_window_set_title(GTK_WINDOW(window), _("System Information"));

#ifdef GTK2
	g_signal_connect(G_OBJECT(window), "delete-event", gtk_main_quit, NULL);
#else
	gtk_signal_connect(GTK_OBJECT(window), "delete-event",
			   (GtkSignalFunc) gtk_main_quit, NULL);
#endif

	mbox = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(mbox);
	gtk_container_add(GTK_CONTAINER(window), mbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(mbox), notebook, TRUE, TRUE, 0);

	/*
	 * Computer tab
	 */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
				 gtk_label_new(_("Computer")));

#ifdef GTK2
	label = gtk_label_new(_("<b><big>Operating System</big></b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("Operating System"));
#endif
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), os_get_widget(mainwindow),
			   FALSE, FALSE, 0);

#ifdef GTK2
	label = gtk_label_new(_("<b><big>Processor</big></b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("Processor"));
#endif
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), processor_get_widget(), FALSE,
			   FALSE, 0);

#ifdef GTK2
	label = gtk_label_new(_("<b><big>Memory Usage</big></b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("Memory Usage"));
#endif
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), memory_get_widget(mainwindow),
			   FALSE, FALSE, 0);

	/*
	 * Devices tab
	 */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
				 gtk_label_new(_("Devices")));

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	ctree = gtk_ctree_new(2, 0);
	gtk_container_add(GTK_CONTAINER(scroll), ctree);
	gtk_widget_set_usize(GTK_WIDGET(ctree), 400, 300);
	gtk_ctree_set_expander_style(GTK_CTREE(ctree),
				     GTK_CTREE_EXPANDER_TRIANGLE);
	gtk_ctree_set_line_style(GTK_CTREE(ctree), GTK_CTREE_LINES_NONE);
	gtk_clist_set_column_width(GTK_CLIST(ctree), 0, 32);
	gtk_clist_set_column_width(GTK_CLIST(ctree), 1, 32);
	gtk_clist_set_row_height(GTK_CLIST(ctree), 18);
#ifdef GTK2
	g_signal_connect(G_OBJECT(ctree), "tree-select-row",
			 (GCallback) hi_show_device_info, mainwindow);
	g_signal_connect(G_OBJECT(ctree), "tree-unselect-row",
			 (GCallback) hi_hide_device_info, mainwindow);
#else
	gtk_signal_connect(GTK_OBJECT(ctree), "tree-select-row",
			   (GtkCTreeFunc) hi_show_device_info, mainwindow);
	gtk_signal_connect(GTK_OBJECT(ctree), "tree-unselect-row",
			   (GtkCTreeFunc) hi_hide_device_info, mainwindow);
#endif

	frame = gtk_frame_new(_("Device information"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

	hbox = gtk_hbutton_box_new();
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(mbox), hbox, FALSE, FALSE, 0);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 6);

	btn = gtk_button_new_with_label(_("About"));
#ifdef GTK2
	g_signal_connect(G_OBJECT(btn), "clicked",
			 (GCallback) about_window_create, NULL);
#else
	gtk_signal_connect(GTK_OBJECT(btn), "clicked",
			   (GtkSignalFunc) about_window_create, NULL);
#endif
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

#ifdef GTK2
	btn = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	g_signal_connect(G_OBJECT(btn), "clicked",
			 (GCallback) main_window_refresh, mainwindow);
#else
	btn = gtk_button_new_with_label(_("Refresh"));
	gtk_signal_connect(GTK_OBJECT(btn), "clicked",
			   (GtkSignalFunc) main_window_refresh, mainwindow);
#endif
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

#ifdef GTK2
	btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(btn), "clicked", gtk_main_quit, NULL);
#else
	btn = gtk_button_new_with_label(_("Close"));
	gtk_signal_connect(GTK_OBJECT(btn), "clicked",
			   (GtkSignalFunc) gtk_main_quit, NULL);
#endif
	gtk_widget_show(btn);
	gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

	/*
	 * Environment tab
	 */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
				 gtk_label_new(_("Environment")));

#ifdef GTK2
	label = gtk_label_new(_("<b><big>X-Window System</big></b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("X-Window System"));
#endif
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), x11_get_widget(mainwindow),
			   FALSE, FALSE, 0);

	gtk_widget_show_all(window);
	gtk_widget_hide(frame);

	mainwindow->window = window;
	mainwindow->ctree = ctree;
	mainwindow->frame = frame;

	return mainwindow;
}

static GdkColor
gdk_color_from_rgb(gint r, gint g, gint b)
{
	GdkColormap *cmap = NULL;
	GdkColor color;

	if (!cmap) {
		cmap = gdk_colormap_get_system();
		gdk_colormap_ref(cmap);
	}

	color.red = r * 255;
	color.green = g * 255;
	color.blue = b * 255;

	gdk_color_alloc(cmap, &color);
	gdk_colormap_unref(cmap);

	return color;
}

GtkCTreeNode *
tree_group_new(MainWindow * mainwindow, const gchar * name, DeviceType type)
{
	GtkCTreeNode *node;
	const gchar *text[] = { NULL, name, NULL };
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask = NULL;
	GdkColor color;
	GdkColormap *colormap = NULL;

	if (!name)
		return NULL;
	if (type == NONE)
		return NULL;
	if (!mainwindow)
		return NULL;

	colormap = gtk_widget_get_colormap(mainwindow->window);

	switch (type) {
	case PARPORT:
	case SERIAL:
		pixmap = gdk_pixmap_colormap_create_from_xpm_d
		    (NULL, colormap, &mask, NULL, gen_connector_xpm);

		break;
	case V4L:
	case PCI:
	case ISAPnP:
		pixmap = gdk_pixmap_colormap_create_from_xpm_d
		    (NULL, colormap, &mask, NULL, pci_xpm);
		break;
	case SCSI:
		pixmap = gdk_pixmap_colormap_create_from_xpm_d
		    (NULL, colormap, &mask, NULL, scsi_xpm);
		break;
	case IDE:
		pixmap = gdk_pixmap_colormap_create_from_xpm_d
		    (NULL, colormap, &mask, NULL, hdd_xpm);
		break;
	case USB:
		pixmap = gdk_pixmap_colormap_create_from_xpm_d
		    (NULL, colormap, &mask, NULL, usb_xpm);
		break;
	default:
		mask = pixmap = NULL;
		break;
	}

	node = gtk_ctree_insert_node(GTK_CTREE(mainwindow->ctree),
				     NULL, NULL, text, 0, pixmap, mask, pixmap,
				     mask, FALSE, TRUE);

	color = gdk_color_from_rgb(0xcc, 0xcc, 0xcc);
	gtk_ctree_node_set_background(GTK_CTREE(mainwindow->ctree),
				      node, &color);

	gdk_pixmap_unref(pixmap);
	gdk_bitmap_unref(mask);

	return node;
}

void
tree_insert_item(MainWindow * mainwindow, GtkCTreeNode * group, gchar * name,
		 gpointer data)
{
	GtkCTreeNode *node;
	gchar *text[] = { NULL, name, NULL };

	if (!mainwindow)
		return;
	if (!name)
		return;
	if (!group)
		return;

	node = gtk_ctree_insert_node(GTK_CTREE(mainwindow->ctree),
				     group, NULL,
				     text, 0,
				     NULL, NULL, NULL, NULL, FALSE, TRUE);
	gtk_ctree_node_set_row_data(GTK_CTREE(mainwindow->ctree), node, data);
}

void
hi_insert_generic(gpointer device, DeviceType type)
{
	GenericDevice *generic;

	generic = g_new0(GenericDevice, 1);

	generic->next = generic_devices;
	generic_devices = generic;

	generic_devices->device = device;
	generic_devices->type = type;
}

void
hi_hide_device_info(GtkCTree * tree, GList * node,
		    gint column, gpointer user_data)
{
	MainWindow *mainwindow = (MainWindow *) user_data;

	gtk_widget_hide(mainwindow->frame);
}

void
hi_show_device_info(GtkCTree * tree, GList * node,
		    gint column, gpointer user_data)
{
	GenericDevice *dev;
	MainWindow *mainwindow = (MainWindow *) user_data;

	dev = (GenericDevice *) gtk_ctree_node_get_row_data
	    (GTK_CTREE(tree), GTK_CLIST(tree)->selection->data);

	if (!dev)
		return;

#define dev_info(type,name,func) \
	{ \
		type *name; \
		name = (type *)dev->device; \
		func(mainwindow, name); \
	} \
	break;

	switch (dev->type) {
	case PCI:
		dev_info(PCIDevice, pci, hi_show_pci_info);
	case ISAPnP:
		dev_info(ISADevice, isa, hi_show_isa_info);
	case USB:
		dev_info(USBDevice, usb, hi_show_usb_info);
	case SERIAL:
		dev_info(SerialDevice, serial, hi_show_serial_info);
	case PARPORT:
		dev_info(ParportDevice, parport, hi_show_parport_info);
	case IDE:
		dev_info(IDEDevice, ide, hi_show_ide_info);
	case SCSI:
		dev_info(SCSIDevice, scsi, hi_show_scsi_info);
	case V4L:
		dev_info(V4LDevice, v4l, hi_show_v4l_info);
	default:
		g_print("Showing info of device type %d not implemented yet.\n",
			dev->type);
		return;
		break;
	}

	gtk_widget_show(mainwindow->frame);

}

void
hi_scan_all(MainWindow * mainwindow)
{
	PCIDevice *pci = hi_scan_pci();
	ISADevice *isa = hi_scan_isapnp();
	IDEDevice *ide = hi_scan_ide();
	SCSIDevice *scsi = hi_scan_scsi();
	V4LDevice *v4l = hi_scan_v4l();
	ParportDevice *pp = hi_scan_parport();
	SerialDevice *sd = hi_scan_serial();
	GtkCTreeNode *node;
	GenericDevice *gd = generic_devices;

	gtk_clist_freeze(GTK_CLIST(mainwindow->ctree));

	/*
	 * Free the generic_devices stuff and remove all device-related
	 * nodes.
	 */
	if (gd != NULL) {
		for (; gd != NULL; gd = gd->next) {
			g_free(gd->device);
			gtk_ctree_remove_node(GTK_CTREE(mainwindow->ctree),
					      gd->node);
			g_free(gd);
		}
		generic_devices = NULL;
	}
#define check_insert(a,b,c,d) \
	if (a) { \
		node = tree_group_new(mainwindow, b, c); \
		for (; a != NULL; a = a->next) { \
			if (a->d) { \
				hi_insert_generic(a, c); \
				tree_insert_item(mainwindow, node, a->d, \
					generic_devices); \
			} \
		} \
	}

	check_insert(pci, _("PCI Devices"), PCI, name);
	check_insert(isa, _("ISA PnP Devices"), ISAPnP, card);

	/*
	 * USB is different since it's Plug'n'Pray, so we have a
	 * routine just for it :)
	 */
	usb_update(mainwindow);

	check_insert(ide, _("ATA/IDE Block Devices"), IDE, model);
	check_insert(scsi, _("SCSI Devices"), SCSI, model);
	check_insert(v4l, _("Video for Linux"), V4L, name);
	check_insert(sd, _("Communication Ports"), SERIAL, name);
	check_insert(pp, _("Parallel Ports"), PARPORT, name);

	gtk_clist_thaw(GTK_CLIST(mainwindow->ctree));
}

#if 0
static void
usage(char *argv0)
{
	g_print("%s [--prefix <prefix>]\n", argv0);
	exit(1);
}
#endif

int
main(int argc, char **argv)
{
	MainWindow *mainwindow;
#if 0
	gint i;
#endif

#ifdef ENABLE_NLS
	intl_init();
#endif

	g_print("HardInfo " VERSION "\n");
	g_print
	    ("Copyright (c) 2003 Leandro Pereira <leandro@linuxmag.com.br>\n\n");
	g_print(_
		("This is free software; you can modify and/or distribute it\n"));
	g_print(_
		("under the terms of GNU GPL version 2. See http://www.fsf.org/\n"));
	g_print(_("for more information.\n\n"));

	gtk_init(&argc, &argv);

#ifndef GTK2
	gdk_rgb_init();
	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
	gtk_widget_set_default_visual(gdk_rgb_get_visual());
#endif

#if 0
	for (i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "--help", 6) ||
		    !strncmp(argv[i], "-h", 2)) {
			usage(argv[0]);
		} else
		    if (!strncmp(argv[i], "--prefix", 8) ||
			!strncmp(argv[i], "-p", 2)) {
			i++;
			if (argv[i][0] == '-')
				usage(argv[0]);

			g_print("prefix = %s\n", argv[i]);
		}
	}
#endif

	mainwindow = main_window_create();
	main_window_refresh(NULL, mainwindow);

	gtk_main();

	return 0;
}
