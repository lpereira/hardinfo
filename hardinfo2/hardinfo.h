/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

#ifndef __HARDINFO_H__
#define __HARDINFO_H__

#include <gtk/gtk.h>

typedef struct _ModuleEntry ModuleEntry;

struct _ModuleEntry {
    gchar *name;
    gchar *icon;
};

inline  void  remove_quotes(gchar *str);
inline  void  strend(gchar *str, gchar chr);
inline  void  remove_linefeed(gchar *str);
        void  widget_set_cursor(GtkWidget *widget, GdkCursorType cursor_type);
inline gchar *size_human_readable(gfloat size);
        void  nonblock_sleep(guint msec);

extern	gchar*	path_lib;
extern	gchar*	path_data;

#endif				/* __HARDINFO_H__ */
