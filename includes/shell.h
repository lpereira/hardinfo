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
#ifndef __SHELL_H__
#define __SHELL_H__

#include <gtk/gtk.h>

#include "loadgraph.h"
#include "help-viewer.h"

typedef struct _Shell			Shell;
typedef struct _ShellTree		ShellTree;
typedef struct _ShellInfoTree		ShellInfoTree;
typedef struct _ShellNote		ShellNote;
typedef struct _DetailView		DetailView;

typedef struct _ShellModule		ShellModule;
typedef struct _ShellModuleMethod	ShellModuleMethod;
typedef struct _ShellModuleEntry	ShellModuleEntry;

typedef struct _ShellFieldUpdate	ShellFieldUpdate;
typedef struct _ShellFieldUpdateSource	ShellFieldUpdateSource;

typedef enum {
    SHELL_ORDER_DESCENDING,
    SHELL_ORDER_ASCENDING,
} ShellOrderType;

typedef enum {
    SHELL_PACK_RESIZE = 1 << 0,
    SHELL_PACK_SHRINK = 1 << 1
} ShellPackOptions;

typedef enum {
    SHELL_VIEW_NORMAL,
    SHELL_VIEW_DUAL,
    SHELL_VIEW_LOAD_GRAPH,
    SHELL_VIEW_PROGRESS,
    SHELL_VIEW_PROGRESS_DUAL,
    SHELL_VIEW_DETAIL,
    SHELL_VIEW_N_VIEWS
} ShellViewType;

typedef enum {
    TREE_COL_PBUF,
    TREE_COL_NAME,
    TREE_COL_MODULE_ENTRY,
    TREE_COL_MODULE,
    TREE_COL_SEL,
    TREE_NCOL
} ShellTreeColumns;

typedef enum {
    INFO_TREE_COL_NAME,
    INFO_TREE_COL_VALUE,
    INFO_TREE_COL_DATA,
    INFO_TREE_COL_PBUF,
    INFO_TREE_COL_PROGRESS,
    INFO_TREE_COL_EXTRA1,
    INFO_TREE_COL_EXTRA2,
    INFO_TREE_NCOL
} ShellInfoTreeColumns;

struct _Shell {
    GtkWidget		*window, *vbox;
    GtkWidget		*status, *progress;
    GtkWidget		*remote_label;
    GtkWidget		*notebook;
    GtkWidget		*hbox, *vpaned;

    ShellTree		*tree;
    ShellInfoTree	*info_tree;
    ShellModule		*selected_module;
    ShellModuleEntry	*selected;
    ShellNote		*note;
    DetailView		*detail_view;
    LoadGraph		*loadgraph;

    GtkActionGroup	*action_group;
    GtkUIManager	*ui_manager;
    GSList		*merge_ids;

    ShellViewType	 view_type;
    gboolean		 normalize_percentage;

    gint		_pulses;
    ShellOrderType	_order_type;

    GKeyFile		*hosts;
    HelpViewer		*help_viewer;
};

struct _DetailView {
    GtkWidget		*scroll;
    GtkWidget		*view;
    GtkWidget		*detail_box;
};

struct _ShellTree {
    GtkWidget		*scroll;
    GtkWidget		*view;
    GtkTreeModel	*model;
    GtkTreeSelection	*selection;

    GSList		*modules;
};

struct _ShellInfoTree {
    GtkWidget		*scroll;
    GtkWidget		*view;
    GtkTreeModel        *model;
    GtkTreeSelection	*selection;

    GtkTreeViewColumn	 *col_progress, *col_value, *col_extra1, *col_extra2, *col_textvalue;
};

struct _ShellNote {
    GtkWidget		*event_box;
    GtkWidget		*label;
};

struct _ShellModule {
    gchar		*name;
    GdkPixbuf		*icon;
    GModule		*dll;

    gpointer		(*aboutfunc) ();
    gchar		*(*summaryfunc) ();
    void		(*deinit) ();

    guchar		 weight;

    GSList		*entries;
};

struct _ShellModuleMethod {
    gchar	*name;
    gpointer	function;
};

struct _ShellModuleEntry {
    gchar		*name;
    GdkPixbuf		*icon;
    gchar		*icon_file;
    gboolean		 selected;
    gint		 number;
    guint32		 flags;

    gchar		*(*func) ();
    void		(*scan_func) ();

    gchar		*(*fieldfunc) (gchar * entry);
    gchar 		*(*morefunc)  (gchar * entry);
    gchar		*(*notefunc)  (gint entry);
};

struct _ShellFieldUpdate {
    ShellModuleEntry	*entry;
    gchar		*field_name;
};

struct _ShellFieldUpdateSource {
    guint		 source_id;
    ShellFieldUpdate	*sfu;
};

void		shell_init(GSList *modules);
void		shell_do_reload(void);

Shell	       *shell_get_main_shell();

void		shell_action_set_enabled(const gchar *action_name,
                                         gboolean setting);
gboolean	shell_action_get_enabled(const gchar *action_name);
gboolean	shell_action_get_active(const gchar *action_name);
void		shell_action_set_active(const gchar *action_name,
                                        gboolean setting);
void		shell_action_set_property(const gchar *action_name,
                                          const gchar *property,
                                          gboolean setting);

void		shell_set_side_pane_visible(gboolean setting);
void		shell_set_note_from_entry(ShellModuleEntry *entry);
void		shell_ui_manager_set_visible(const gchar *path,
                                             gboolean setting);

void		shell_status_update(const gchar *message);
void		shell_status_pulse(void);
void		shell_status_set_percentage(gint percentage);
void		shell_status_set_enabled(gboolean setting);

void		shell_view_set_enabled(gboolean setting);

void		shell_clear_timeouts(Shell *shell);
void		shell_clear_tree_models(Shell *shell);
void		shell_clear_field_updates(void);
void		shell_set_title(Shell *shell, char *subtitle);

void		shell_add_modules_to_gui(gpointer _shell_module, gpointer _shell_tree);

void		shell_save_hosts_file(void);
void		shell_update_remote_menu(void);

void		shell_set_remote_label(Shell *shell, gchar *label);

/* decode special information in keys
 *
 * key syntax:
 *  [$[<flags>][<tag>]$]<name>[#[<dis>]]
 * flags:
 *  [[!][*]]
 *  ! = show details (moreinfo) in reports
 *  * = highlight/select this row
 */
gboolean    key_is_flagged(const gchar *key);       /* has $[<flags>][<tag>]$ at the start of the key */
gboolean    key_is_highlighted(const gchar *key);   /* flag '*' = select/highlight */
gboolean    key_wants_details(const gchar *key);    /* flag '!' = report should include the "moreinfo" */
gchar       *key_mi_tag(const gchar *key);          /* moreinfo lookup tag */
const gchar *key_get_name(const gchar *key);        /* get the key's name, flagged or not */
/*
 * example for key = "$*!Foo$Bar#7":
 * flags = "$*!Foo$"  // key_is/wants_*() still works on flags
 * tag = "Foo"        // the moreinfo/icon tag
 * name = "Bar#7"     // the full unique name
 * label = "Bar"      // the label displayed
 * dis = "7"
 */
void key_get_components(const gchar *key,
    gchar **flags, gchar **tag, gchar **name, gchar **label, gchar **dis,
    gboolean null_empty);

#endif				/* __SHELL_H__ */


