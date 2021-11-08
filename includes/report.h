/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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

#ifndef __REPORT_H__
#define __REPORT_H__
#include <gtk/gtk.h>
#include <shell.h>

typedef enum {
    REPORT_FORMAT_HTML,
    REPORT_FORMAT_TEXT,
    REPORT_FORMAT_SHELL,
    N_REPORT_FORMAT
} ReportFormat;

typedef enum {
    REPORT_COL_PROGRESS = 1<<0,
    REPORT_COL_VALUE    = 1<<1,
    REPORT_COL_EXTRA1   = 1<<2,
    REPORT_COL_EXTRA2   = 1<<3,
    REPORT_COL_TEXTVALUE= 1<<4
} ReportColumn;

typedef struct _ReportDialog	ReportDialog;
typedef struct _ReportContext	ReportContext;

struct _ReportContext {
  ShellModuleEntry	*entry;
  gchar			*output;

  void (*header)      	(ReportContext *ctx);
  void (*footer)      	(ReportContext *ctx);
  void (*title)      	(ReportContext *ctx, gchar *text);
  void (*subtitle)    	(ReportContext *ctx, gchar *text);
  void (*subsubtitle)	(ReportContext *ctx, gchar *text);
  void (*keyvalue)   	(ReportContext *ctx, gchar *key, gchar *value, gsize longest_key);

  void (*details_start)     (ReportContext *ctx, gchar *key, gchar *value, gsize longest_key);
  void (*details_section)   (ReportContext *ctx, gchar *name);
  void (*details_keyvalue)  (ReportContext *ctx, gchar *key, gchar *value, gsize longest_key);
  void (*details_end)       (ReportContext *ctx);

  ReportFormat		format;

  gboolean		first_table;
  gboolean		in_details;

  gboolean		show_column_headers;
  guint			columns, parent_columns;
  GHashTable		*column_titles;
  GHashTable *icon_refs;
  GHashTable *icon_data;
};

struct _ReportDialog {
  GtkWidget *dialog;
  GtkWidget *filechooser;
  GtkWidget *btn_cancel;
  GtkWidget *btn_generate;
  GtkWidget *btn_sel_all;
  GtkWidget *btn_sel_none;
  GtkWidget *treeview;

  GtkTreeModel *model;
};

void		 report_dialog_show();

ReportContext	*report_context_html_new();
ReportContext	*report_context_text_new();
ReportContext	*report_context_shell_new();

void		 report_header		(ReportContext *ctx);
void		 report_footer		(ReportContext *ctx);
void		 report_title		(ReportContext *ctx, gchar *text);
void 		 report_subtitle	(ReportContext *ctx, gchar *text);
void 		 report_subsubtitle	(ReportContext *ctx, gchar *text);
void		 report_key_value	(ReportContext *ctx, gchar *key, gchar *value, gsize longest_key);
void		 report_table		(ReportContext *ctx, gchar *text);
void		 report_details		(ReportContext *ctx, gchar *key, gchar *value, gchar *details, gsize longest_key);

void             report_create_from_module_list(ReportContext *ctx, GSList *modules);
gchar           *report_create_from_module_list_format(GSList *modules, ReportFormat format);

void		 report_context_free(ReportContext *ctx);
void             report_module_list_free(GSList *modules);

#endif	/* __REPORT_H__ */
