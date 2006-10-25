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

static void
__expand_cb(GtkWidget *widget, gpointer data)
{
    if (GTK_IS_EXPANDER(widget)) {
        gtk_expander_set_expanded(GTK_EXPANDER(widget), TRUE);
        gtk_widget_hide(widget);
    } else if (GTK_IS_CONTAINER(widget)) {
        gtk_container_forall(GTK_CONTAINER(widget), (GtkCallback)__expand_cb, NULL);
    }
}

void
file_chooser_open_expander(GtkWidget *chooser)
{
    gtk_container_forall(GTK_CONTAINER(chooser), (GtkCallback)__expand_cb, NULL);
}

void
file_chooser_add_filters(GtkWidget *chooser, FileTypes *filters)
{
    GtkFileFilter *filter;
    gint i;
    
    for (i = 0; filters[i].name; i++) {
        filter = gtk_file_filter_new();
        gtk_file_filter_add_mime_type(filter, filters[i].mime_type);
        gtk_file_filter_set_name(filter, filters[i].name);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
    }
}

gchar 
*file_chooser_get_extension(GtkWidget *chooser, FileTypes *filters)
{
    GtkFileFilter *filter;
    const gchar *filter_name;
    gint i;
    
    filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(chooser));
    filter_name = gtk_file_filter_get_name(filter);
    for (i = 0; filters[i].name; i++) {
        if (g_str_equal(filter_name, filters[i].name)) {
            return filters[i].extension;
        }
    }
    
    return NULL;
}

gpointer
file_types_get_data_by_name(FileTypes *filters, gchar *filename)
{
    gint i;
    
    for (i = 0; filters[i].name; i++) {
        if (g_str_has_suffix(filename, filters[i].extension)) {
            return filters[i].data;
        }
    }
    
    return NULL;
}

gchar
*file_chooser_build_filename(GtkWidget *chooser, gchar *extension)
{
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    gchar *retval;
    
    if (g_str_has_suffix(filename, extension)) {
        return filename;
    }
    
    retval = g_strconcat(filename, extension, NULL);
    g_free(filename);
    
    return retval;
}
