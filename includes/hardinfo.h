/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "config.h"
#include "shell.h"
#include "vendor.h"
#include "gettext.h"
#include "info.h"
#include "format_early.h"

#define HARDINFO2_COPYRIGHT_LATEST_YEAR 2024

typedef enum {
  MODULE_FLAG_NONE = 0,
  MODULE_FLAG_NO_REMOTE = 1<<0,
  MODULE_FLAG_HAS_HELP = 1<<1,
  MODULE_FLAG_HIDE = 1<<2,
  MODULE_FLAG_BENCHMARK = 1<<3,
} ModuleEntryFlags;

typedef struct _ModuleEntry		ModuleEntry;
typedef struct _ModuleAbout		ModuleAbout;
typedef struct _FileTypes		FileTypes;
typedef struct _ProgramParameters	ProgramParameters;

struct _ProgramParameters {
  gint create_report;
  gint force_all_details; /* for create_report, include any "moreinfo" that exists for any item */
  gint show_version;
  gint gui_running;
  gint skip_benchmarks;
  gint quiet;
  gint theme;
  gint darkmode;
  gint aborting_benchmarks;
  /*
   * OK to use the common parts of HTML(4.0) and Pango Markup
   * in the value part of a key/value.
   * Including the (b,big,i,s,sub,sup,small,tt,u) tags.
   * https://developer.gnome.org/pango/stable/PangoMarkupFormat.html
   */
  gint markup_ok;
  int fmt_opts;

  gint     report_format;
  gint     max_bench_results;
  gchar   *run_benchmark;
  gchar   *bench_user_note;
  gchar   *result_format;
  gchar   *path_lib;
  gchar   *path_data;
  gchar   *path_locale;
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
void   remove_quotes(gchar *str);
char  *strend(gchar *str, gchar chr);
void   remove_linefeed(gchar *str);
gchar *strreplacechr(gchar *string, gchar *replace, gchar new_char);
gchar *strreplace(gchar *string, gchar *replace, gchar *replacement);

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
//DISABLED DEBUG_AUTO_FREE as debian does not use Release
//#if !(RELEASE == 1)
//#define DEBUG_AUTO_FREE 2
//#endif
#include "auto_free.h"
#define idle_free(ptr) auto_free(ptr)

gchar	     *find_program(gchar *program_name);
gchar      *size_human_readable(gfloat size);
void          nonblock_sleep(guint msec);
GSList	     *modules_get_list(void);
GSList	     *modules_load_selected(void);
GSList       *modules_load_all(void);
void	      module_unload_all(void);
gchar        *seconds_to_string(unsigned int seconds);

gchar        *h_strdup_cprintf(const gchar *format, gchar *source, ...)
                                __attribute__((format(gnu_printf, 1, 3)));
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

/* GTK UI stuff */
gint     ui_init(int *argc, char ***argv);
void     parameters_init(int *argc, char ***argv, ProgramParameters *params);
extern   ProgramParameters params;

/* Module stuff */
gchar		*module_call_method(gchar *method);
gchar           *module_call_method_param(gchar * method, gchar * parameter);

/* Sysfs stuff */
gfloat		h_sysfs_read_float(const gchar *endpoint, const gchar *entry);
gint		h_sysfs_read_int(const gchar *endpoint, const gchar *entry);
gint		h_sysfs_read_hex(const gchar *endpoint, const gchar *entry);
gchar	       *h_sysfs_read_string(const gchar *endpoint, const gchar *entry);

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

/* in gg_key_file_parse_string_as_value.c */
gchar *
gg_key_file_parse_string_as_value (const gchar *string, const gchar list_separator);

gchar *hardinfo_clean_grpname(const gchar *v, int replacing);
/* Hardinfo labels that have # are truncated and/or hidden.
 * Labels can't have $ because that is the delimiter in
 * moreinfo.
 * replacing = true will free v */
gchar *hardinfo_clean_label(const gchar *v, int replacing);
/* hardinfo uses the values as {ht,x}ml, apparently */
gchar *hardinfo_clean_value(const gchar *v, int replacing);

/* Same as hardinfo_spawn_command_line_sync(), but calls shell_status_pulse()
 * before. */
gboolean hardinfo_spawn_command_line_sync(const gchar *command_line,
                                          gchar **standard_output,
                                          gchar **standard_error,
                                          gint *exit_status,
                                          GError **error);

/* a marker in text to point out problems, using markup where possible */
const char *problem_marker();

/* a version of g_strescape() that allows escaping extra characters.
 * use with g_strcompress() as normal. */
gchar *
gg_strescape (const gchar *source,
             const gchar *exceptions,
             const gchar *extra);

/* hinote helpers */
#define note_max_len 512
#define note_printf(note_buff, fmt, ...)  \
    snprintf((note_buff) + strlen(note_buff), note_max_len - strlen(note_buff) - 1, \
        fmt, ##__VA_ARGS__)
#define note_print(note_buff, str) note_printf((note_buff), "%s", str)
gboolean note_cond_bullet(gboolean cond, gchar *note_buff, const gchar *desc_str);
gboolean note_require_tool(const gchar *tool, gchar *note_buff, const gchar *desc_str);
int cpu_procs_cores_threads(int *p, int *c, int *t);

gchar *strwrap(const gchar *st, size_t w, gchar delimiter);

#endif				/* __HARDINFO_H__ */
