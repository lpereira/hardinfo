/*
 *    HelpViewer - Simple Help file browser
 *    Copyright (C) 2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#ifndef __HELP_VIEWER_H__
#define __HELP_VIEWER_H__

typedef struct _HelpViewer	HelpViewer;

struct _HelpViewer {
    GtkWidget *window;
    GtkWidget *status_bar;
    
    GtkWidget *btn_back, *btn_forward;
    GtkWidget *text_view;
    GtkWidget *text_search;
    
    gchar *current_file;
    gchar *help_directory;

    GSList *back_stack, *forward_stack;
};

HelpViewer *help_viewer_new(const gchar *help_dir, const gchar *help_file);
void help_viewer_open_page(HelpViewer *help_viewer, const gchar *page);
void help_viewer_destroy(HelpViewer *help_viewer);

#endif	/* __HELP_VIEWER_H__ */