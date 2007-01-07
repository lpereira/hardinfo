/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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
#include <loadgraph.h>

typedef struct _Shell			Shell;
typedef struct _ShellTree		ShellTree;
typedef struct _ShellInfoTree		ShellInfoTree;
typedef struct _ShellNote		ShellNote;

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
    SHELL_VIEW_N_VIEWS
} ShellViewType;

typedef enum {
    TREE_COL_PBUF,
    TREE_COL_NAME,
    TREE_COL_DATA,
    TREE_COL_SEL,
    TREE_NCOL
} ShellTreeColumns;

typedef enum {
    INFO_TREE_COL_NAME,
    INFO_TREE_COL_VALUE,
    INFO_TREE_COL_DATA,
    INFO_TREE_COL_PBUF,
    INFO_TREE_COL_PROGRESS,
    INFO_TREE_NCOL
} ShellInfoTreeColumns;

struct _Shell {
    GtkWidget		*window, *vbox;
    GtkWidget		*status, *progress;
    GtkWidget		*notebook;
    GtkWidget		*hpaned, *vpaned;

    ShellTree		*tree;
    ShellInfoTree	*info, *moreinfo;
    ShellModuleEntry	*selected;
    ShellNote		*note;
    LoadGraph		*loadgraph;

    GtkActionGroup	*action_group;
    GtkUIManager	*ui_manager;

    ShellViewType	 view_type;
    
    gint		_pulses;
    ShellOrderType	_order_type;
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
    
    GtkTreeViewColumn	 *col_progress, *col_value;
};

struct _ShellNote {
    GtkWidget		*frame;
    GtkWidget		*label;
};

struct _ShellModule {
    gchar		*name;
    GdkPixbuf		*icon;
    GModule		*dll;
    
    guchar		 weight;

    GSList		*entries;
};

struct _ShellModuleMethod {
    gchar	*name;
    gchar	*(*function) (void);
};

struct _ShellModuleEntry {
    gchar		*name;
    GdkPixbuf		*icon;
    gboolean		 selected;
    gint		 number;
    
    gchar		*(*func) ();
    void		(*scan_func) ();

    gchar		*(*fieldfunc) (gchar * entry);
    gchar 		*(*morefunc) (gchar * entry);
    gchar		*(*notefunc) (gint entry);
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

#endif				/* __SHELL_H__ */
