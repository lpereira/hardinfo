/*
 * Hardware Information, version 0.3
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * May be modified and/or distributed under the terms of GNU GPL version 2.
 *
 * This module contains code from xdpyinfo.c, by Jim Fulton, MIT X Consortium
 * Copyright 1988, 1998 The Open Group
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of The Open Group shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from The Open Group. 
 */

#include "hardinfo.h"
#include "x11.h"

#include <stdlib.h>

X11Info *
x11_get_info(void)
{
	X11Info *info;
	Display *dpy;
	gint default_screen = 0, color_depth = 0;
	gchar *buf;

	dpy = XOpenDisplay(NULL);
	if(!dpy) return NULL;

	info = g_new0(X11Info, 1);

	buf = DisplayString(dpy);
	if (*buf == ':')
		info->display = g_strdup_printf(_("Local display (%s)"), buf);
	else
		info->display = g_strdup_printf(_("Remote display (%s)"), buf);
	
	info->vendor = g_strdup(ServerVendor(dpy));

	if (strstr(ServerVendor(dpy), "XFree86")) {
		int vendrel = VendorRelease(dpy);

		if (vendrel < 336) {
			/*
			 * vendrel was set incorrectly for 3.3.4 and 3.3.5, so handle
			 * those cases here.
			 */
			buf = g_strdup_printf
				("%d.%d.%d", vendrel / 100, (vendrel / 10) % 10,
				 vendrel % 10);
		} else if (vendrel < 3900) {
			/* 3.3.x versions, other than the exceptions handled above */
			buf = g_strdup_printf("%d.%d", vendrel / 1000, (vendrel / 100) % 10);
			if (((vendrel / 10) % 10) || (vendrel % 10)) {
				gchar *buf2;
				
				buf2 = g_strdup_printf(".%d", (vendrel / 10) % 10);
				buf = g_strconcat(buf, buf2, NULL);
				g_free(buf2);
				
				if (vendrel % 10) {
					buf2 = g_strdup_printf(".%d", vendrel % 10);
					buf = g_strconcat(buf, buf2, NULL);
					g_free(buf2);
				}
			}
		} else if (vendrel < 40000000) {
			gchar *buf2;
			/* 4.0.x versions */
			buf = g_strdup_printf("%d.%d", vendrel / 1000, (vendrel / 10) % 10);

			if (vendrel % 10) {
				buf2 = g_strdup_printf(".%d", vendrel % 10);
				buf = g_strconcat(buf, buf2, NULL);
				g_free(buf2);
			}
		} else {
			/* post-4.0.x */
			/* post-4.0.x */
			buf = g_strdup_printf("%d.%d.%d", vendrel / 10000000,
			       (vendrel / 100000) % 100,
			       (vendrel / 1000) % 100);
			if (vendrel % 1000) {
				gchar *buf2;
				
				buf2 = g_strdup_printf(".%d", vendrel % 1000);
				buf = g_strconcat(buf, buf2, NULL);
				g_free(buf2);
			}
		}
	}
	if (buf) {
		info->xf86version = g_strdup(buf);
		g_free(buf);
	}

	default_screen = DefaultScreen(dpy);
	color_depth = XDefaultDepth(dpy, default_screen);
	info->screen_size = g_strdup_printf("%dx%d pixels (%d bpp)",
					    DisplayWidth(dpy, default_screen),
					    DisplayHeight(dpy, default_screen),
					    color_depth);


	if (info->xf86version) {
		info->version = g_strdup_printf(_("XFree86 version %s (protocol version %d.%d)"),
					info->xf86version, ProtocolVersion(dpy),
				 	        ProtocolRevision(dpy));
	} else {
		info->version = g_strdup_printf(_("%d (protocol version %d.%d)"),
					VendorRelease(dpy), ProtocolVersion(dpy),
				 	        ProtocolRevision(dpy));
	}

	XCloseDisplay(dpy);

	return info;
}

GtkWidget *
x11_get_widget(MainWindow * mainwindow)
{
	GtkWidget *label, *hbox;
	GtkWidget *table;
#ifdef GTK2
	GtkWidget *pixmap;
	gchar *buf;
#endif
	X11Info *info;

	if (!mainwindow)
		return NULL;

	info = x11_get_info();
	
	if (!info)
		return NULL;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);

#ifdef GTK2
	buf = g_strdup_printf("%s/x11.png", IMG_PREFIX);
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
	label = gtk_label_new(_("<b>Display:</b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("Display:"));
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

#ifdef GTK2
	label = gtk_label_new(_("<b>Vendor:</b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("Vendor:"));
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

#ifdef GTK2
	label = gtk_label_new(_("<b>Release:</b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("Release:"));
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

#ifdef GTK2
	label = gtk_label_new(_("<b>Resolution:</b>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#else
	label = gtk_label_new(_("Resolution:"));
#endif
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	/*
	 * Table content 
	 */
	label = gtk_label_new(info->display);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	label = gtk_label_new(info->vendor);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	label = gtk_label_new(info->version);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 3);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	label = gtk_label_new(info->screen_size);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 3, 4);
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	g_free(info);

	return hbox;
}
