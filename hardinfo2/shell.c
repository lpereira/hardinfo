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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <config.h>

#include <hardinfo.h>

#include <shell.h>
#include <syncmanager.h>
#include <iconcache.h>
#include <menu.h>
#include <stock.h>

#include <callbacks.h>

/*
 * Internal Prototypes ********************************************************
 */

static void create_window();
static ShellTree *tree_new(void);
static ShellInfoTree *info_tree_new(gboolean extra);

static void module_selected(gpointer data);
static void module_selected_show_info(ShellModuleEntry * entry,
				      gboolean reload);
static void info_selected(GtkTreeSelection * ts, gpointer data);
static void info_selected_show_extra(gchar * data);
static gboolean reload_section(gpointer data);
static gboolean rescan_section(gpointer data);
static gboolean update_field(gpointer data);

/*
 * Globals ********************************************************************
 */

static Shell *shell = NULL;
static GHashTable *update_tbl = NULL;
static GSList *update_sfusrc = NULL;

/*
 * Code :) ********************************************************************
 */

Shell *shell_get_main_shell(void)
{
    return shell;
}

void shell_ui_manager_set_visible(const gchar * path, gboolean setting)
{
    GtkWidget *widget;

    if (!params.gui_running)
	return;

    widget = gtk_ui_manager_get_widget(shell->ui_manager, path);
    if (!widget)
	return;

    if (setting)
	gtk_widget_show(widget);
    else
	gtk_widget_hide(widget);
}

void shell_action_set_property(const gchar * action_name,
			       const gchar * property, gboolean setting)
{
    GtkAction *action;

    if (!params.gui_running)
	return;

    action = gtk_action_group_get_action(shell->action_group, action_name);
    if (action) {
	GValue value = { 0 };

	g_value_init(&value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&value, setting);

	g_object_set_property(G_OBJECT(action), property, &value);

	g_value_unset(&value);
    }
}

void shell_action_set_enabled(const gchar * action_name, gboolean setting)
{
    if (params.gui_running && shell->action_group) {
	GtkAction *action;

	action =
	    gtk_action_group_get_action(shell->action_group, action_name);
	if (action) {
	    gtk_action_set_sensitive(action, setting);
	}
    }
}

gboolean shell_action_get_enabled(const gchar * action_name)
{
    GtkAction *action;

    if (!params.gui_running)
	return FALSE;

    action = gtk_action_group_get_action(shell->action_group, action_name);
    if (action) {
	return gtk_action_get_sensitive(action);
    }

    return FALSE;
}

void shell_set_side_pane_visible(gboolean setting)
{
    if (!params.gui_running)
	return;

    if (setting)
	gtk_widget_show(shell->tree->scroll);
    else
	gtk_widget_hide(shell->tree->scroll);
}

gboolean shell_action_get_active(const gchar * action_name)
{
    GtkAction *action;
    GSList *proxies;

    /* FIXME: Ugh. Are you sure there isn't any simpler way? O_o */
    if (!params.gui_running)
	return FALSE;

    action = gtk_action_group_get_action(shell->action_group, action_name);
    if (action) {
	proxies = gtk_action_get_proxies(action);

	for (; proxies; proxies = proxies->next) {
	    GtkWidget *widget = (GtkWidget *) proxies->data;

	    if (GTK_IS_CHECK_MENU_ITEM(widget)) {
		return
		    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM
						   (widget));
	    }
	}
    }

    return FALSE;
}

void shell_action_set_active(const gchar * action_name, gboolean setting)
{
    GtkAction *action;
    GSList *proxies;

    /* FIXME: Ugh. Are you sure there isn't any simpler way? O_o */
    if (!params.gui_running)
	return;

    action = gtk_action_group_get_action(shell->action_group, action_name);
    if (action) {
	proxies = gtk_action_get_proxies(action);

	for (; proxies; proxies = proxies->next) {
	    GtkWidget *widget = (GtkWidget *) proxies->data;

	    if (GTK_IS_CHECK_MENU_ITEM(widget)) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget),
					       setting);
		return;
	    }
	}
    }
}

void shell_status_pulse(void)
{
    if (params.gui_running) {
	if (shell->_pulses++ == 5) {
	    /* we're pulsing for some time, disable the interface and change the cursor
	       to a hourglass */
	    shell_view_set_enabled(FALSE);
	}

	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(shell->progress));
	while (gtk_events_pending())
	    gtk_main_iteration();
    } else {
	static gint counter = 0;

	fprintf(stderr, "\033[2K\033[40;37;1m %c\033[0m\r",
		"|/-\\"[counter++ % 4]);
    }
}

void shell_status_set_percentage(gint percentage)
{
    if (params.gui_running) {
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(shell->progress),
				      (float) percentage / 100.0);
	while (gtk_events_pending())
	    gtk_main_iteration();
    } else {
	if (percentage < 1 || percentage >= 100) {
	    fprintf(stderr, "\033[2K");
	} else {
	    gchar pbar[] = "----------";

	    memset(pbar, '#', percentage / 10);

	    fprintf(stderr, "\r\033[40;37;1m%3d%% \033[40;34;1m"
		    "%s\033[0m\r", percentage, pbar);
	}
    }
}

void shell_view_set_enabled(gboolean setting)
{
    if (!params.gui_running)
	return;

    if (setting) {
	shell->_pulses = 0;
	widget_set_cursor(shell->window, GDK_LEFT_PTR);
    } else {
	widget_set_cursor(shell->window, GDK_WATCH);
    }

    gtk_widget_set_sensitive(shell->hpaned, setting);
    shell_action_set_enabled("ViewMenuAction", setting);
    shell_action_set_enabled("RefreshAction", setting);
    shell_action_set_enabled("CopyAction", setting);
    shell_action_set_enabled("ReportAction", setting);
    shell_action_set_enabled("SyncManagerAction", setting && sync_manager_count_entries() > 0);
    shell_action_set_enabled("SaveGraphAction",
			     setting ? shell->view_type ==
			     SHELL_VIEW_PROGRESS : FALSE);
}

void shell_status_set_enabled(gboolean setting)
{
    if (!params.gui_running)
	return;

    if (setting)
	gtk_widget_show(shell->progress);
    else {
	gtk_widget_hide(shell->progress);
	shell_view_set_enabled(TRUE);

	shell_status_update("Done.");
    }
}

void shell_do_reload(void)
{
    if (!params.gui_running || !shell->selected)
	return;

    shell_action_set_enabled("RefreshAction", FALSE);
    shell_action_set_enabled("CopyAction", FALSE);
    shell_action_set_enabled("ReportAction", FALSE);

    shell_status_set_enabled(TRUE);

    module_entry_reload(shell->selected);
    module_selected(NULL);

    shell_action_set_enabled("RefreshAction", TRUE);
    shell_action_set_enabled("CopyAction", TRUE);
    shell_action_set_enabled("ReportAction", TRUE);
}

void shell_status_update(const gchar * message)
{
    if (params.gui_running) {
	gtk_label_set_markup(GTK_LABEL(shell->status), message);
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(shell->progress));
	while (gtk_events_pending())
	    gtk_main_iteration();
    } else {
	fprintf(stderr, "\033[2K\033[40;37;1m %s\033[0m\r", message);
    }
}

static void destroy_me(void)
{
    cb_quit();
}

static void close_note(GtkWidget * widget, gpointer user_data)
{
    gtk_widget_hide(shell->note->frame);
}

static ShellNote *note_new(void)
{
    ShellNote *note;
    GtkWidget *hbox, *icon, *button;

    note = g_new0(ShellNote, 1);
    note->label = gtk_label_new("");
    note->frame = gtk_frame_new(NULL);
    button = gtk_button_new();

    icon = icon_cache_get_image("close.png");
    gtk_widget_show(icon);
    gtk_container_add(GTK_CONTAINER(button), icon);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    g_signal_connect(G_OBJECT(button), "clicked", (GCallback) close_note,
		     NULL);

    hbox = gtk_hbox_new(FALSE, 3);
    icon = icon_cache_get_image("dialog-information.png");

    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), note->label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_container_add(GTK_CONTAINER(note->frame), hbox);
    gtk_widget_show_all(hbox);

    return note;
}

static void create_window(void)
{
    GtkWidget *vbox, *hbox;

    shell = g_new0(Shell, 1);

    shell->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(shell->window),
			icon_cache_get_pixbuf("logo.png"));
    gtk_window_set_title(GTK_WINDOW(shell->window), "System Information");
    gtk_widget_set_size_request(shell->window, 700, 480);
    g_signal_connect(G_OBJECT(shell->window), "destroy", destroy_me, NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(shell->window), vbox);
    shell->vbox = vbox;

    menu_init(shell);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(hbox);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

    shell->progress = gtk_progress_bar_new();
    gtk_widget_set_size_request(shell->progress, 80, 10);
    gtk_widget_hide(shell->progress);
    gtk_box_pack_end(GTK_BOX(hbox), shell->progress, FALSE, FALSE, 5);

    shell->status = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(shell->status), 0.0, 0.5);
    gtk_widget_show(shell->status);
    gtk_box_pack_start(GTK_BOX(hbox), shell->status, FALSE, FALSE, 5);

    shell->hpaned = gtk_hpaned_new();
    gtk_widget_show(shell->hpaned);
    gtk_box_pack_end(GTK_BOX(vbox), shell->hpaned, TRUE, TRUE, 0);
    gtk_paned_set_position(GTK_PANED(shell->hpaned), 210);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_widget_show(vbox);
    gtk_paned_add2(GTK_PANED(shell->hpaned), vbox);

    shell->note = note_new();
    gtk_box_pack_end(GTK_BOX(vbox), shell->note->frame, FALSE, FALSE, 0);

    shell->vpaned = gtk_vpaned_new();
    gtk_box_pack_start(GTK_BOX(vbox), shell->vpaned, TRUE, TRUE, 0);
    gtk_widget_show(shell->vpaned);

    shell->notebook = gtk_notebook_new();
    gtk_paned_add2(GTK_PANED(shell->vpaned), shell->notebook);

    gtk_widget_show(shell->window);
    while (gtk_events_pending())
	gtk_main_iteration();
}

static void view_menu_select_entry(gpointer data, gpointer data2)
{
    GtkTreePath *path;
    GtkTreeIter *iter = (GtkTreeIter *) data2;

    path = gtk_tree_model_get_path(shell->tree->model, iter);

    gtk_tree_selection_select_path(shell->tree->selection, path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(shell->tree->view), path, NULL,
			     FALSE);
    gtk_tree_path_free(path);
}

static void add_module_to_menu(gchar * name, GdkPixbuf * pixbuf)
{
    gchar *about_module = g_strdup_printf("AboutModule%s", name);

    GtkActionEntry entries[] = {
	{
	 name,			/* name */
	 name,			/* stockid */
	 name,			/* label */
	 NULL,			/* accelerator */
	 NULL,			/* tooltip */
	 NULL,			/* callback */
	 },
	{
	 about_module,
	 name,
	 name,
	 NULL,
	 name,
	 (GCallback) cb_about_module,
	 },
    };

    stock_icon_register_pixbuf(pixbuf, name);

    gtk_action_group_add_actions(shell->action_group, entries, 2, NULL);

    gtk_ui_manager_add_ui(shell->ui_manager,
			  gtk_ui_manager_new_merge_id(shell->ui_manager),
			  "/menubar/ViewMenu/LastSep",
			  name, name, GTK_UI_MANAGER_MENU, TRUE);

    gtk_ui_manager_add_ui(shell->ui_manager,
			  gtk_ui_manager_new_merge_id(shell->ui_manager),
			  "/menubar/HelpMenu/HelpMenuModules/LastSep",
			  about_module, about_module, GTK_UI_MANAGER_AUTO,
			  TRUE);
}

static void
add_module_entry_to_view_menu(gchar * module, gchar * name,
			      GdkPixbuf * pixbuf, GtkTreeIter * iter)
{
    GtkActionEntry entries[] = {
	{
	 name,			/* name */
	 name,			/* stockid */
	 name,			/* label */
	 NULL,			/* accelerator */
	 NULL,			/* tooltip */
	 (GCallback) view_menu_select_entry,	/* callback */
	 },
    };

    stock_icon_register_pixbuf(pixbuf, name);
    gtk_action_group_add_actions(shell->action_group, entries, 1, iter);

    gtk_ui_manager_add_ui(shell->ui_manager,
			  gtk_ui_manager_new_merge_id(shell->ui_manager),
			  g_strdup_printf("/menubar/ViewMenu/%s", module),
			  name, name, GTK_UI_MANAGER_AUTO, FALSE);
}

static void add_modules_to_gui(gpointer data, gpointer user_data)
{
    ShellTree *shelltree = (ShellTree *) user_data;
    ShellModule *module = (ShellModule *) data;
    GtkTreeStore *store = GTK_TREE_STORE(shelltree->model);
    GtkTreeIter parent;
    
    if (!module) {
        return;
    }

    gtk_tree_store_append(store, &parent, NULL);
    gtk_tree_store_set(store, &parent, TREE_COL_NAME, module->name,
		       TREE_COL_DATA, NULL, TREE_COL_SEL, FALSE, -1);

    if (module->icon) {
	gtk_tree_store_set(store, &parent, TREE_COL_PBUF, module->icon,
			   -1);
    }

    add_module_to_menu(module->name, module->icon);

    if (module->entries) {
	ShellModuleEntry *entry;
	GSList *p;

	for (p = module->entries; p; p = g_slist_next(p)) {
	    GtkTreeIter child;
	    entry = (ShellModuleEntry *) p->data;

	    gtk_tree_store_append(store, &child, &parent);
	    gtk_tree_store_set(store, &child, TREE_COL_NAME, entry->name,
			       TREE_COL_DATA, entry,
			       TREE_COL_SEL, FALSE, -1);

	    if (entry->icon) {
		gtk_tree_store_set(store, &child, TREE_COL_PBUF,
				   entry->icon, -1);
	    }

	    add_module_entry_to_view_menu(module->name, entry->name,
					  entry->icon,
					  gtk_tree_iter_copy(&child));

	    shell_status_pulse();
	}

    }
}

static void __tree_iter_destroy(gpointer data)
{
    gtk_tree_iter_free((GtkTreeIter *) data);
}

void shell_init(GSList * modules)
{
    if (shell) {
	g_error("Shell already created");
	return;
    }

    DEBUG("initializing shell");

    create_window();

    shell_action_set_property("CopyAction", "is-important", TRUE);
    shell_action_set_property("RefreshAction", "is-important", TRUE);
    shell_action_set_property("ReportAction", "is-important", TRUE);

    shell->tree = tree_new();
    shell->info = info_tree_new(FALSE);
    shell->moreinfo = info_tree_new(TRUE);
    shell->loadgraph = load_graph_new(75);
    update_tbl = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       g_free, __tree_iter_destroy);

    gtk_paned_pack1(GTK_PANED(shell->hpaned), shell->tree->scroll,
		    SHELL_PACK_RESIZE, SHELL_PACK_SHRINK);
    gtk_paned_pack1(GTK_PANED(shell->vpaned), shell->info->scroll,
		    SHELL_PACK_RESIZE, SHELL_PACK_SHRINK);

    gtk_notebook_append_page(GTK_NOTEBOOK(shell->notebook),
			     shell->moreinfo->scroll, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(shell->notebook),
			     load_graph_get_framed(shell->loadgraph),
			     NULL);

    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(shell->notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(shell->notebook), FALSE);

    shell_status_set_enabled(TRUE);
    shell_status_update("Loading modules...");

    shell->tree->modules = modules ? modules : modules_load_all();

    g_slist_foreach(shell->tree->modules, add_modules_to_gui, shell->tree);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->tree->view));

    gtk_widget_show_all(shell->hpaned);

    load_graph_configure_expose(shell->loadgraph);
    gtk_widget_hide(shell->notebook);
    gtk_widget_hide(shell->note->frame);

    shell_status_update("Done.");
    shell_status_set_enabled(FALSE);

    shell_action_set_enabled("RefreshAction", FALSE);
    shell_action_set_enabled("CopyAction", FALSE);
    shell_action_set_enabled("SaveGraphAction", FALSE);
    shell_action_set_active("SidePaneAction", TRUE);
    shell_action_set_active("ToolbarAction", TRUE);
    
#ifndef HAS_LIBSOUP
    shell_action_set_enabled("SyncManagerAction", FALSE);
#else
    shell_action_set_enabled("SyncManagerAction", sync_manager_count_entries() > 0);
#endif
}

static gboolean update_field(gpointer data)
{
    ShellFieldUpdate *fu;
    GtkTreeIter *iter;
    
    fu = (ShellFieldUpdate *) data;
    g_return_val_if_fail(fu != NULL, FALSE);
    
    iter = g_hash_table_lookup(update_tbl, fu->field_name);
    g_return_val_if_fail(iter != NULL, FALSE);

    /* if the entry is still selected, update it */
    if (iter && fu->entry->selected && fu->entry->fieldfunc) {
	GtkTreeStore *store = GTK_TREE_STORE(shell->info->model);
	gchar *value = fu->entry->fieldfunc(fu->field_name);

	/*
	 * this function is also used to feed the load graph when ViewType
	 * is SHELL_VIEW_LOAD_GRAPH
	 */
	if (shell->view_type == SHELL_VIEW_LOAD_GRAPH &&
	    gtk_tree_selection_iter_is_selected(shell->info->selection,
						iter)) {
	    load_graph_update(shell->loadgraph, atoi(value));
	}

	gtk_tree_store_set(store, iter, INFO_TREE_COL_VALUE, value, -1);

	g_free(value);
	return TRUE;
    }

    if (update_sfusrc) {
	GSList *sfu;

	for (sfu = update_sfusrc; sfu; sfu = sfu->next) {
	    g_free(sfu->data);
	}

	g_slist_free(update_sfusrc);
	update_sfusrc = NULL;
    }

    DEBUG("destroying ShellFieldUpdate for field %s", fu->field_name);

    /* otherwise, cleanup and destroy the timeout */
    g_free(fu->field_name);
    g_free(fu);

    return FALSE;
}

static gboolean reload_section(gpointer data)
{
    ShellModuleEntry *entry = (ShellModuleEntry *) data;

    /* if the entry is still selected, update it */
    if (entry->selected) {
	GtkTreePath *path = NULL;
	GtkTreeIter iter;

	/* gets the current selected path */
	if (gtk_tree_selection_get_selected
	    (shell->info->selection, &shell->info->model, &iter))
	    path = gtk_tree_model_get_path(shell->info->model, &iter);

	/* update the information, clear the treeview and populate it again */
	module_entry_reload(entry);
	info_selected_show_extra(NULL);	/* clears the more info store */
	module_selected_show_info(entry, TRUE);

	/* if there was a selection, reselect it */
	if (path) {
	    gtk_tree_selection_select_path(shell->info->selection, path);
	    gtk_tree_path_free(path);
	}
    }

    /* destroy the timeout: it'll be set up again */
    return FALSE;
}

static gboolean rescan_section(gpointer data)
{
    ShellModuleEntry *entry = (ShellModuleEntry *) data;

    module_entry_reload(entry);

    return entry->selected;
}

gint
info_tree_compare_val_func(GtkTreeModel * model,
			   GtkTreeIter * a,
			   GtkTreeIter * b, gpointer userdata)
{
    gint ret = 0;
    gchar *col1, *col2;

    gtk_tree_model_get(model, a, INFO_TREE_COL_VALUE, &col1, -1);
    gtk_tree_model_get(model, b, INFO_TREE_COL_VALUE, &col2, -1);

    if (col1 == NULL || col2 == NULL) {
	if (col1 == NULL && col2 == NULL)
	    return 0;

	ret = (col1 == NULL) ? -1 : 1;
    } else {
	ret = shell->_order_type ? (atof(col1) < atof(col2)) :
	    (atof(col1) > atof(col2));
    }

    g_free(col1);
    g_free(col2);

    return ret;
}

static void set_view_type(ShellViewType viewtype)
{
    /* reset to the default model */
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info->view),
			    shell->info->model);

    /* reset to the default view columns */
    gtk_tree_view_column_set_visible(shell->info->col_extra1, FALSE);
    gtk_tree_view_column_set_visible(shell->info->col_extra2, FALSE);
    gtk_tree_view_column_set_visible(shell->info->col_progress, FALSE);
    gtk_tree_view_column_set_visible(shell->info->col_value, TRUE);
    
    /* turn off the rules hint */
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(shell->info->view), FALSE);

    /* turn off the save graphic action */
    shell_action_set_enabled("SaveGraphAction", FALSE);

    if (viewtype == shell->view_type)
	return;

    if (viewtype < SHELL_VIEW_NORMAL || viewtype >= SHELL_VIEW_N_VIEWS)
	viewtype = SHELL_VIEW_NORMAL;

    shell->normalize_percentage = TRUE;
    shell->view_type = viewtype;

    switch (viewtype) {
    default:
    case SHELL_VIEW_NORMAL:
	gtk_widget_hide(shell->notebook);
	break;
    case SHELL_VIEW_DUAL:
	gtk_notebook_set_page(GTK_NOTEBOOK(shell->notebook), 0);
	gtk_widget_show(shell->notebook);
	break;
    case SHELL_VIEW_LOAD_GRAPH:
	gtk_notebook_set_page(GTK_NOTEBOOK(shell->notebook), 1);
	gtk_widget_show(shell->notebook);
	load_graph_clear(shell->loadgraph);

	gtk_paned_set_position(GTK_PANED(shell->vpaned),
			       shell->hpaned->allocation.height -
			       shell->loadgraph->height - 16);
	break;
    case SHELL_VIEW_PROGRESS_DUAL:
	gtk_notebook_set_page(GTK_NOTEBOOK(shell->notebook), 0);
	gtk_widget_show(shell->notebook);
	/* fallthrough */
    case SHELL_VIEW_PROGRESS:
	shell_action_set_enabled("SaveGraphAction", TRUE);
	gtk_tree_view_column_set_visible(shell->info->col_progress, TRUE);
	gtk_tree_view_column_set_visible(shell->info->col_value, FALSE);

	if (viewtype == SHELL_VIEW_PROGRESS)
		gtk_widget_hide(shell->notebook);
	break;
    }
}

static void
group_handle_special(GKeyFile * key_file, ShellModuleEntry * entry,
		     gchar * group, gchar ** keys)
{
    if (g_str_equal(group, "$ShellParam$")) {
        gboolean headers_visible = FALSE;
	gint i;

	for (i = 0; keys[i]; i++) {
	    gchar *key = keys[i];

	    if (g_str_has_prefix(key, "UpdateInterval")) {
		ShellFieldUpdate *fu = g_new0(ShellFieldUpdate, 1);
		ShellFieldUpdateSource *sfutbl;
		gint ms;

		ms = g_key_file_get_integer(key_file, group, key, NULL);

		fu->field_name = g_strdup(strchr(key, '$') + 1);
		fu->entry = entry;

		sfutbl = g_new0(ShellFieldUpdateSource, 1);
		sfutbl->source_id = g_timeout_add(ms, update_field, fu);
		sfutbl->sfu = fu;

		update_sfusrc = g_slist_prepend(update_sfusrc, sfutbl);
	    } else if (g_str_equal(key, "NormalizePercentage")) {
		shell->normalize_percentage = g_key_file_get_boolean(key_file, group, key, NULL);
	    } else if (g_str_equal(key, "LoadGraphSuffix")) {
		gchar *suffix =
		    g_key_file_get_value(key_file, group, key, NULL);
		load_graph_set_data_suffix(shell->loadgraph, suffix);
		g_free(suffix);
	    } else if (g_str_equal(key, "ReloadInterval")) {
		gint ms;

		ms = g_key_file_get_integer(key_file, group, key, NULL);

		g_timeout_add(ms, reload_section, entry);
	    } else if (g_str_equal(key, "RescanInterval")) {
		gint ms;

		ms = g_key_file_get_integer(key_file, group, key, NULL);

		g_timeout_add(ms, rescan_section, entry);
	    } else if (g_str_equal(key, "ShowColumnHeaders")) {
		headers_visible = g_key_file_get_boolean(key_file, group, key, NULL);
	    } else if (g_str_has_prefix(key, "ColumnTitle")) {
                GtkTreeViewColumn *column;
		gchar *value, *title = strchr(key, '$') + 1;

                value = g_key_file_get_value(key_file, group, key, NULL);

                if (g_str_equal(title, "Extra1")) {
			column = shell->info->col_extra1;
                } else if (g_str_equal(title, "Extra2")) {
			column = shell->info->col_extra2;
                } else if (g_str_equal(title, "Value")) {
			column = shell->info->col_value;
                } else if (g_str_equal(title, "TextValue")) {
			column = shell->info->col_textvalue;
                } else if (g_str_equal(title, "Progress")) {
			column = shell->info->col_progress;
                }

                gtk_tree_view_column_set_title(column, value);
                gtk_tree_view_column_set_visible(column, TRUE);

                g_free(value);
	    } else if (g_str_equal(key, "OrderType")) {
		shell->_order_type = g_key_file_get_integer(key_file,
							    group,
							    key, NULL);
	    } else if (g_str_equal(key, "ViewType")) {
		set_view_type(g_key_file_get_integer(key_file, group,
						     key, NULL));
	    } else if (g_str_has_prefix(key, "Icon")) {
		GtkTreeIter *iter = g_hash_table_lookup(update_tbl,
							strchr(key,
							       '$') + 1);

		if (iter) {
		    gchar *file =
			g_key_file_get_value(key_file, group, key, NULL);
		    gtk_tree_store_set(GTK_TREE_STORE(shell->info->model),
				       iter, INFO_TREE_COL_PBUF,
				       icon_cache_get_pixbuf_at_size(file,
								     22,
								     22),
				       -1);
		    g_free(file);
		}
	    } else if (g_str_equal(key, "Zebra")) {
		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW
					     (shell->info->view),
					     g_key_file_get_boolean
					     (key_file, group, key, NULL));
	    }
	}

        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(shell->info->view), headers_visible);
    } else {
	g_warning("Unknown parameter group: ``%s''", group);
    }
}

static void
group_handle_normal(GKeyFile * key_file, ShellModuleEntry * entry,
		    gchar * group, gchar ** keys, gsize ngroups)
{
    GtkTreeIter parent;
    GtkTreeStore *store = GTK_TREE_STORE(shell->info->model);
    gchar *tmp = g_strdup(group);
    gint i;

    if (ngroups > 1) {
	gtk_tree_store_append(store, &parent, NULL);

	strend(tmp, '#');
	gtk_tree_store_set(store, &parent, INFO_TREE_COL_NAME, tmp, -1);
	g_free(tmp);
    }

    for (i = 0; keys[i]; i++) {
	gchar *key = keys[i];
	gchar *value;
	GtkTreeIter child;

	value = g_key_file_get_value(key_file, group, key, NULL);
	if (entry->fieldfunc && value && g_str_equal(value, "...")) {
	    g_free(value);
	    value = entry->fieldfunc(key);
	}

	if ((key && value) && g_utf8_validate(key, -1, NULL) && g_utf8_validate(value, -1, NULL)) {
	    if (ngroups == 1) {
		gtk_tree_store_append(store, &child, NULL);
	    } else {
		gtk_tree_store_append(store, &child, &parent);
	    }

	    if (strchr(value, '|')) {
		gchar **columns = g_strsplit(value, "|", 0);

		gtk_tree_store_set(store, &child, INFO_TREE_COL_VALUE, columns[0], -1);
		if (columns[1]) {
			gtk_tree_store_set(store, &child, INFO_TREE_COL_EXTRA1, columns[1], -1);
			if (columns[2]) {
				gtk_tree_store_set(store, &child, INFO_TREE_COL_EXTRA2, columns[2], -1);
			}
		}

		g_strfreev(columns);
	    } else {
	    	gtk_tree_store_set(store, &child, INFO_TREE_COL_VALUE, value, -1);
	    }

	    strend(key, '#');

	    if (*key == '$') {
		gchar **tmp;

		tmp = g_strsplit(++key, "$", 0);

		gtk_tree_store_set(store, &child, INFO_TREE_COL_NAME,
				   tmp[1], INFO_TREE_COL_DATA, tmp[0], -1);

		g_strfreev(tmp);
	    } else {
		gtk_tree_store_set(store, &child, INFO_TREE_COL_NAME, key,
				   INFO_TREE_COL_DATA, NULL, -1);
	    }

	    g_hash_table_insert(update_tbl, g_strdup(key),
				gtk_tree_iter_copy(&child));

	}

	g_free(value);
    }
}

static void
moreinfo_handle_normal(GKeyFile * key_file, gchar * group, gchar ** keys)
{
    GtkTreeIter parent;
    GtkTreeStore *store = GTK_TREE_STORE(shell->moreinfo->model);
    gint i;

    gtk_tree_store_append(store, &parent, NULL);
    gtk_tree_store_set(store, &parent, INFO_TREE_COL_NAME, group, -1);

    for (i = 0; keys[i]; i++) {
	gchar *key = keys[i];
	GtkTreeIter child;
	gchar *value;

	value = g_key_file_get_value(key_file, group, key, NULL);

	if (g_utf8_validate(key, -1, NULL)
	    && g_utf8_validate(value, -1, NULL)) {
	    strend(key, '#');

	    gtk_tree_store_append(store, &child, &parent);
	    gtk_tree_store_set(store, &child, INFO_TREE_COL_VALUE,
			       value, INFO_TREE_COL_NAME, key, -1);
	}

	g_free(value);
    }
}

static void update_progress()
{
    GtkTreeModel *model = shell->info->model;
    GtkTreeStore *store = GTK_TREE_STORE(model);
    GtkTreeIter iter, fiter;
    gchar *tmp;
    gdouble maxv = 0, maxp = 0, cur, floatval;

    gtk_tree_model_get_iter_first(model, &fiter);

    /* finds the maximum value */
    if (shell->normalize_percentage) {
	iter = fiter;
	do {
		gtk_tree_model_get(model, &iter, INFO_TREE_COL_VALUE, &tmp, -1);
	
		cur = atof(tmp);
		maxv = MAX(maxv, cur);
	
		g_free(tmp);
	} while (gtk_tree_model_iter_next(model, &iter));
    } else {
	maxv = 100.0f;
    }

    /* calculates the relative percentage and finds the maximum percentage */
    if (shell->_order_type == SHELL_ORDER_ASCENDING) {
	iter = fiter;
	do {
	    gtk_tree_model_get(model, &iter, INFO_TREE_COL_VALUE, &tmp, -1);

	    cur = 100 - 100 * atof(tmp) / maxv;
	    maxp = MAX(cur, maxp);

	    g_free(tmp);
	} while (gtk_tree_model_iter_next(model, &iter));

	maxp = 100 - maxp;
    }

    /* fix the maximum relative percentage */
    iter = fiter;
    do {
	gtk_tree_model_get(model, &iter, INFO_TREE_COL_VALUE, &tmp, -1);
	floatval = atof(tmp);
	g_free(tmp);
	
	cur = 100 * floatval / maxv;

	if (shell->_order_type == SHELL_ORDER_ASCENDING)
	    cur = 100 - cur + maxp;
	    
        tmp = g_strdup_printf("%.2f", floatval);
        tmp = strreplace(tmp, ",", '.');
	gtk_tree_store_set(store, &iter, INFO_TREE_COL_PROGRESS, cur,
                                         INFO_TREE_COL_VALUE, tmp, -1);
        g_free(tmp);
    } while (gtk_tree_model_iter_next(model, &iter));

    /* now sort everything up. that wasn't as hard as i thought :) */
    GtkTreeSortable *sortable = GTK_TREE_SORTABLE(shell->info->model);

    gtk_tree_sortable_set_sort_func(sortable, INFO_TREE_COL_VALUE,
				    info_tree_compare_val_func, 0, NULL);
    gtk_tree_sortable_set_sort_column_id(sortable,
					 INFO_TREE_COL_VALUE,
					 GTK_SORT_DESCENDING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info->view),
			    GTK_TREE_MODEL(sortable));
}

void shell_set_note_from_entry(ShellModuleEntry * entry)
{
    if (entry->notefunc) {
	const gchar *note = module_entry_get_note(entry);

	if (note) {
	    gtk_label_set_markup(GTK_LABEL(shell->note->label), note);
	    gtk_widget_show(shell->note->frame);
	} else {
	    gtk_widget_hide(shell->note->frame);
	}
    } else {
	gtk_widget_hide(shell->note->frame);
    }
}

static void
module_selected_show_info(ShellModuleEntry * entry, gboolean reload)
{
    GKeyFile *key_file = g_key_file_new();
    GtkTreeStore *store;
    gchar *key_data, **groups;
    gboolean has_shell_param = FALSE;
    gint i;
    gsize ngroups;

    module_entry_scan(entry);
    key_data = module_entry_function(entry);

    /* */
    gdk_window_freeze_updates(shell->info->view->window);

    g_object_ref(shell->info->model);
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info->view), NULL);

    /* reset the view type to normal */
    set_view_type(SHELL_VIEW_NORMAL);

    /* recreate the iter hash table */
    if (!reload) {
        h_hash_table_remove_all(update_tbl);
    }
    
    if (update_sfusrc) {
	GSList *sfusrc;

	for (sfusrc = update_sfusrc; sfusrc; sfusrc = sfusrc->next) {
	    ShellFieldUpdateSource *src =
		(ShellFieldUpdateSource *) sfusrc->data;

	    g_source_remove(src->source_id);
	    g_free(src->sfu->field_name);
	    g_free(src->sfu);
	    g_free(src);
	}

	g_slist_free(update_sfusrc);
	update_sfusrc = NULL;
    }

    store = GTK_TREE_STORE(shell->info->model);

    gtk_tree_store_clear(store);

    g_key_file_load_from_data(key_file, key_data, strlen(key_data), 0,
			      NULL);
    groups = g_key_file_get_groups(key_file, &ngroups);

    for (i = 0; groups[i]; i++)
	if (groups[i][0] == '$')
	    ngroups--;

    for (i = 0; groups[i]; i++) {
	gchar *group = groups[i];
	gchar **keys = g_key_file_get_keys(key_file, group, NULL, NULL);

	if (*group == '$') {
	    group_handle_special(key_file, entry, group, keys);
	    has_shell_param = TRUE;
	} else {
	    group_handle_normal(key_file, entry, group, keys, ngroups);
	}

	g_strfreev(keys);
    }
    
    /* */
    if (!has_shell_param) {
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(shell->info->view), FALSE);
    }
    
    /* */
    g_object_unref(shell->info->model);
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info->view), shell->info->model);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->info->view));

    gdk_window_thaw_updates(shell->info->view->window);
    shell_set_note_from_entry(entry);

    if (shell->view_type == SHELL_VIEW_PROGRESS || shell->view_type == SHELL_VIEW_PROGRESS_DUAL) {
	update_progress();
    }

    g_strfreev(groups);
    g_key_file_free(key_file);
    g_free(key_data);
}

static void info_selected_show_extra(gchar * data)
{
    GtkTreeStore *store;

    store = GTK_TREE_STORE(shell->moreinfo->model);
    gtk_tree_store_clear(store);

    if (!shell->selected->morefunc)
	return;

    if (data) {
	GKeyFile *key_file = g_key_file_new();
	gchar *key_data = shell->selected->morefunc(data);
	gchar **groups;
	gint i;

	g_key_file_load_from_data(key_file, key_data, strlen(key_data), 0,
				  NULL);
	groups = g_key_file_get_groups(key_file, NULL);

	for (i = 0; groups[i]; i++) {
	    gchar *group = groups[i];
	    gchar **keys =
		g_key_file_get_keys(key_file, group, NULL, NULL);

	    moreinfo_handle_normal(key_file, group, keys);
	}

	gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->moreinfo->view));

	g_strfreev(groups);
	g_key_file_free(key_file);
	g_free(key_data);
    }
}

static void module_selected(gpointer data)
{
    ShellTree *shelltree = shell->tree;
    GtkTreeModel *model = GTK_TREE_MODEL(shelltree->model);
    GtkTreeIter parent;
    ShellModuleEntry *entry;
    static ShellModuleEntry *current = NULL;
    static gboolean updating = FALSE;
    

    /* Gets the currently selected item on the left-side TreeView; if there is no
       selection, silently return */
    if (!gtk_tree_selection_get_selected
	(shelltree->selection, &model, &parent)) {
	return;
    }

    /* Mark the currently selected module as "unselected"; this is used to kill the 
       update timeout. */
    if (current) {
	current->selected = FALSE;
    }

    if (updating) {
      return;
    } else {
      updating = TRUE;
    }

    /* Get the current selection and shows its related info */
    gtk_tree_model_get(model, &parent, TREE_COL_DATA, &entry, -1);
    if (entry && !entry->selected) {
	gchar *title;

	shell_status_set_enabled(TRUE);
	shell_status_update("Updating...");

	entry->selected = TRUE;
	shell->selected = entry;
	module_selected_show_info(entry, FALSE);

	info_selected_show_extra(NULL);	/* clears the more info store */
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(shell->info->view));

	/* urgh. why don't GTK do this when the model is cleared? */
#define RANGE_SET_VALUE(tree,scrollbar,value) \
  	  gtk_range_set_value(GTK_RANGE \
	  		    (GTK_SCROLLED_WINDOW(shell->tree->scroll)-> \
			     scrollbar), value);

        RANGE_SET_VALUE(info, vscrollbar, 0.0);
        RANGE_SET_VALUE(info, hscrollbar, 0.0);
        RANGE_SET_VALUE(moreinfo, vscrollbar, 0.0);
        RANGE_SET_VALUE(moreinfo, hscrollbar, 0.0);

#undef RANGE_SET_VALUE

	shell_status_update("Done.");
	shell_status_set_enabled(FALSE);

	title = g_strdup_printf("%s - System Information", entry->name);
	gtk_window_set_title(GTK_WINDOW(shell->window), title);
	g_free(title);

	shell_action_set_enabled("RefreshAction", TRUE);
	shell_action_set_enabled("CopyAction", TRUE);
    } else {
	gtk_window_set_title(GTK_WINDOW(shell->window),
			     "System Information");
	shell_action_set_enabled("RefreshAction", FALSE);
	shell_action_set_enabled("CopyAction", FALSE);

	gtk_tree_store_clear(GTK_TREE_STORE(shell->info->model));
	set_view_type(SHELL_VIEW_NORMAL);
    }

    current = entry;
    updating = FALSE;
}

static void info_selected(GtkTreeSelection * ts, gpointer data)
{
    ShellInfoTree *info = (ShellInfoTree *) data;
    GtkTreeModel *model = GTK_TREE_MODEL(info->model);
    GtkTreeIter parent;
    gchar *datacol;

    if (!gtk_tree_selection_get_selected(ts, &model, &parent))
	return;

    gtk_tree_model_get(model, &parent, INFO_TREE_COL_DATA, &datacol, -1);
    info_selected_show_extra(datacol);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(shell->moreinfo->view));
}

static ShellInfoTree *info_tree_new(gboolean extra)
{
    ShellInfoTree *info;
    GtkWidget *treeview, *scroll;
    GtkTreeModel *model;
    GtkTreeStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cr_text, *cr_pbuf, *cr_progress;
    GtkTreeSelection *sel;

    info = g_new0(ShellInfoTree, 1);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

    store =
	gtk_tree_store_new(INFO_TREE_NCOL, G_TYPE_STRING, G_TYPE_STRING,
			   G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_FLOAT,
                           G_TYPE_STRING, G_TYPE_STRING);
    model = GTK_TREE_MODEL(store);
    treeview = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

    info->col_progress = column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_visible(column, FALSE);
    gtk_tree_view_column_set_min_width(column, 240);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    cr_progress = gtk_cell_renderer_progress_new();
    gtk_tree_view_column_pack_start(column, cr_progress, TRUE);
    gtk_tree_view_column_add_attribute(column, cr_progress, "value",
				       INFO_TREE_COL_PROGRESS);
    gtk_tree_view_column_add_attribute(column, cr_progress, "text",
				       INFO_TREE_COL_VALUE);
    gtk_tree_view_column_set_visible(column, FALSE);

    info->col_textvalue = column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    cr_pbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, cr_pbuf, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_pbuf, "pixbuf",
				       INFO_TREE_COL_PBUF);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, TRUE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       INFO_TREE_COL_NAME);

    info->col_extra1 = column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_visible(column, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       INFO_TREE_COL_EXTRA1);

    info->col_extra2 = column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_visible(column, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       INFO_TREE_COL_EXTRA2);

    info->col_value = column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       INFO_TREE_COL_VALUE);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (!extra)
	g_signal_connect(G_OBJECT(sel), "changed",
			 (GCallback) info_selected, info);

    gtk_container_add(GTK_CONTAINER(scroll), treeview);

    info->scroll = scroll;
    info->view = treeview;
    info->model = model;
    info->selection = sel;

    gtk_widget_show_all(scroll);

    return info;
}

static ShellTree *tree_new()
{
    ShellTree *shelltree;
    GtkWidget *treeview, *scroll;
    GtkTreeModel *model;
    GtkTreeStore *store;
    GtkCellRenderer *cr_text, *cr_pbuf;
    GtkTreeViewColumn *column;
    GtkTreeSelection *sel;

    shelltree = g_new0(ShellTree, 1);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				   GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);

    store = gtk_tree_store_new(TREE_NCOL, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			       G_TYPE_POINTER, G_TYPE_BOOLEAN);
    model = GTK_TREE_MODEL(store);
    treeview = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    cr_pbuf = gtk_cell_renderer_pixbuf_new();
    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_pbuf, FALSE);
    gtk_tree_view_column_pack_start(column, cr_text, TRUE);

    gtk_tree_view_column_add_attribute(column, cr_pbuf, "pixbuf",
				       TREE_COL_PBUF);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       TREE_COL_NAME);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    g_signal_connect(G_OBJECT(sel), "changed", (GCallback) module_selected,
		     NULL);

    gtk_container_add(GTK_CONTAINER(scroll), treeview);

    shelltree->scroll = scroll;
    shelltree->view = treeview;
    shelltree->model = model;
    shelltree->modules = NULL;
    shelltree->selection = sel;

    gtk_widget_show_all(scroll);

    return shelltree;
}
