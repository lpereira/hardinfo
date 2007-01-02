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
#include <shell.h>

typedef struct _ModuleEntry		ModuleEntry;
typedef struct _FileTypes		FileTypes;
typedef struct _ProgramParameters	ProgramParameters;

struct _ProgramParameters {
  gboolean create_report;
  gboolean show_version;
  gboolean gui_running;
  gboolean list_modules;
  gboolean autoload_deps;
  
  gint     report_format;
  
  gchar  **use_modules;
  gchar   *path_lib;
  gchar   *path_data;
};

struct _FileTypes {
  gchar	*name;
  gchar *mime_type;
  gchar *extension;
  gpointer data;
};

struct _ModuleEntry {
    gchar *name;
    gchar *icon;
};

/* String utility functions */
inline void  remove_quotes(gchar *str);
inline void  strend(gchar *str, gchar chr);
inline void  remove_linefeed(gchar *str);
gchar       *strreplace(gchar *string, gchar *replace, gchar new_char);

/* Widget utility functions */
void widget_set_cursor(GtkWidget *widget, GdkCursorType cursor_type);
gint tree_view_get_visible_height(GtkTreeView *tv);
void tree_view_save_image(gchar *filename);

/* File Chooser utility functions */
void      file_chooser_open_expander(GtkWidget *chooser);
void      file_chooser_add_filters(GtkWidget *chooser, FileTypes *filters);
gchar    *file_chooser_get_extension(GtkWidget *chooser, FileTypes *filters);
gchar    *file_chooser_build_filename(GtkWidget *chooser, gchar *extension);
gpointer  file_types_get_data_by_name(FileTypes *file_types, gchar *name);

/* Misc utility functions */
void	      schedule_free(gpointer ptr);
inline gchar *size_human_readable(gfloat size);
void          nonblock_sleep(guint msec);
void          open_url(gchar *url);
GSList	     *modules_load_selected(void);
GSList       *modules_load_all(void);

/* BinReloc stuff */
gboolean binreloc_init(gboolean try_hardcoded);

/* GTK UI stuff */
gboolean ui_init(int *argc, char ***argv);
void     parameters_init(int *argc, char ***argv, ProgramParameters *params);
extern   ProgramParameters params;

/* Module stuff */
void		 module_register(ShellModule *module);
gchar		*module_call_method(gchar *method);

#endif				/* __HARDINFO_H__ */
