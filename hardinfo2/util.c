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
#include <string.h>
#include <hardinfo.h>
#include <gtk/gtk.h>

#define KiB 1024
#define MiB 1048576
#define GiB 1073741824

inline gchar *
size_human_readable(gfloat size)
{
    if (size < KiB)
	return g_strdup_printf("%.1f B", size);
    if (size < MiB)
	return g_strdup_printf("%.1f KiB", size / KiB);
    if (size < GiB)
	return g_strdup_printf("%.1f MiB", size / MiB);

    return g_strdup_printf("%.1f GiB", size / GiB);
}

inline void
strend(gchar *str, gchar chr)
{
    if (!str)
        return;
        
    char *p;
    if ((p = strchr(str, chr)))
        *p = 0;
}

inline void
remove_quotes(gchar *str)
{
    if (!str)
        return;

    while (*str == '"')
        *(str++) = ' ';
    
    strend(str, '"');
}

inline void
remove_linefeed(gchar * str)
{
    strend(str, '\n');
}

void
widget_set_cursor(GtkWidget *widget, GdkCursorType cursor_type)
{   
        GdkCursor *cursor;
 
        cursor = gdk_cursor_new(cursor_type);
        gdk_window_set_cursor(GDK_WINDOW(widget->window), cursor);
        gdk_cursor_unref(cursor);
        
        while(gtk_events_pending())
                gtk_main_iteration();
}

static gboolean
__nonblock_cb(gpointer data)
{
        gtk_main_quit();
        return FALSE;
}

void
nonblock_sleep(guint msec)
{
        g_timeout_add(msec, (GSourceFunc)__nonblock_cb, NULL);
        gtk_main();
}
