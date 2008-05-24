/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __ICONCACHE_H__
#define __ICONCACHE_H__

#include <gtk/gtk.h>

void		 icon_cache_init(void);
GdkPixbuf	*icon_cache_get_pixbuf(const gchar *file);
GtkWidget	*icon_cache_get_image(const gchar *file);
GdkPixbuf	*icon_cache_get_pixbuf_at_size(const gchar *file, gint wid, gint hei);
GtkWidget	*icon_cache_get_image_at_size(const gchar *file, gint wid, gint hei);

#endif	/* __ICONCACHE_H__ */
