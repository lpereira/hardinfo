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

#ifndef __HARDINFO_H__
#define __HARDINFO_H__

#include <gtk/gtk.h>
#include "config.h"
#include "shell.h"
#include "vendor.h"
#include "gettext.h"

#define HARDINFO_COPYRIGHT_LATEST_YEAR 2017

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

typedef enum {
  MODULE_FLAG_NONE = 0,
  MODULE_FLAG_NO_REMOTE = 1<<0,
  MODULE_FLAG_HAS_HELP = 1<<1,
} ModuleEntryFlags;

typedef struct _ModuleEntry		ModuleEntry;
typedef struct _ModuleAbout		ModuleAbout;
typedef struct _FileTypes		FileTypes;
typedef struct _ProgramParameters	ProgramParameters;

struct _ProgramParameters {
  gboolean create_report;
  gboolean show_version;
  gboolean gui_running;
  gboolean list_modules;
  gboolean autoload_deps;
  gboolean run_xmlrpc_server;

  gint     report_format;

  gchar  **use_modules;
  gchar   *run_benchmark;
  gchar   *path_lib;
  gchar   *path_data;
  gchar   *argv0;
};

struct _FileTypes {
  gchar	*name;
  gchar *mime_type;
  gchar *extension;
  gpointer data;
};

struct _ModuleEntry {
    gchar	*name;
    gchar	*icon;
    gpointer	 callback;
    gpointer	 scan_callback;
    guint32	 flags;
};

struct _ModuleAbout {
    const gchar *description;
    const gchar	*author;
    const gchar	*version;
    const gchar	*license;
};

/* String utility functions */
inline void  remove_quotes(gchar *str);
inline char *strend(gchar *str, gchar chr);
inline void  remove_linefeed(gchar *str);
gchar       *strreplacechr(gchar *string, gchar *replace, gchar new_char);
gchar       *strreplace(gchar *string, gchar *replace, gchar *replacement);

/* Widget utility functions */
void widget_set_cursor(GtkWidget *widget, GdkCursorType cursor_type);
gint tree_view_get_visible_height(GtkTreeView *tv);

/* File Chooser utility functions */
void      file_chooser_open_expander(GtkWidget *chooser);
void      file_chooser_add_filters(GtkWidget *chooser, FileTypes *filters);
gchar    *file_chooser_get_extension(GtkWidget *chooser, FileTypes *filters);
gchar    *file_chooser_build_filename(GtkWidget *chooser, gchar *extension);
gpointer  file_types_get_data_by_name(FileTypes *file_types, gchar *name);

/* Misc utility functions */
#if RELEASE == 1
gpointer idle_free(gpointer ptr);
#else
gpointer __idle_free(gpointer ptr, gchar *f, gint l);
#define  idle_free(p) __idle_free(p, __FILE__, __LINE__)
#endif	/* RELEASE == 1 */

gchar	     *find_program(gchar *program_name);
inline gchar *size_human_readable(gfloat size);
void          nonblock_sleep(guint msec);
void          open_url(gchar *url);
GSList	     *modules_get_list(void);
GSList	     *modules_load_selected(void);
GSList       *modules_load_all(void);
void	      module_unload_all(void);
ModuleAbout  *module_get_about(ShellModule *module);
gchar        *seconds_to_string(unsigned int seconds);

gchar        *h_strdup_cprintf(const gchar *format, gchar *source, ...);
gchar	     *h_strconcat(gchar *string1, ...);
void          h_hash_table_remove_all (GHashTable *hash_table);

void	      module_entry_scan_all_except(ModuleEntry *entries, gint except_entry);
void	      module_entry_scan_all(ModuleEntry *entries);
void	      module_entry_reload(ShellModuleEntry *module_entry);
void	      module_entry_scan(ShellModuleEntry *module_entry);
gchar	     *module_entry_function(ShellModuleEntry *module_entry);
const gchar  *module_entry_get_note(ShellModuleEntry *module_entry);
gchar        *module_entry_get_field(ShellModuleEntry * module_entry, gchar * field);
gchar        *module_entry_get_moreinfo(ShellModuleEntry * module_entry, gchar * field);

/* BinReloc stuff */
gboolean binreloc_init(gboolean try_hardcoded);

/* GTK UI stuff */
gboolean ui_init(int *argc, char ***argv);
void     parameters_init(int *argc, char ***argv, ProgramParameters *params);
extern   ProgramParameters params;

/* Module stuff */
gchar		*module_call_method(gchar *method);
gchar           *module_call_method_param(gchar * method, gchar * parameter);

/* Sysfs stuff */
gfloat		h_sysfs_read_float(gchar *endpoint, gchar *entry);
gint		h_sysfs_read_int(gchar *endpoint, gchar *entry);
gchar	       *h_sysfs_read_string(gchar *endpoint, gchar *entry);

#define SCAN_START()  static gboolean scanned = FALSE; if (reload) scanned = FALSE; if (scanned) return;
#define SCAN_END()    scanned = TRUE;

#define _CONCAT(a,b) a ## b
#define CONCAT(a,b) _CONCAT(a,b)

void moreinfo_init(void);
void moreinfo_shutdown(void);
void moreinfo_add_with_prefix(gchar *prefix, gchar *key, gchar *value);
void moreinfo_add(gchar *key, gchar *value);
void moreinfo_del_with_prefix(gchar *prefix);
void moreinfo_clear(void);
gchar *moreinfo_lookup_with_prefix(gchar *prefix, gchar *key);
gchar *moreinfo_lookup(gchar *key);

#if !GLIB_CHECK_VERSION(2,44,0)
    /* g_strv_contains() requires glib>2.44
     * fallback for older versions in hardinfo/util.c */
gboolean g_strv_contains(const gchar * const * strv, const gchar *str);
#endif

/* Hardinfo labels that have # are truncated and/or hidden.
 * Labels can't have $ because that is the delimiter in
 * moreinfo.
 * replacing = true will free v */
gchar *hardinfo_clean_label(const gchar *v, int replacing);
/* hardinfo uses the values as {ht,x}ml, apparently */
gchar *hardinfo_clean_value(const gchar *v, int replacing);

#endif				/* __HARDINFO_H__ */
