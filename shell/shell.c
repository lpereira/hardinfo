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
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <stdbool.h>

#include "config.h"

#include "hardinfo.h"

#include "shell.h"
#include "syncmanager.h"
#include "iconcache.h"
#include "menu.h"
#include "stock.h"
#include "uri_handler.h"

#include "callbacks.h"

struct UpdateTableItem {
    union {
        GtkWidget *widget;
        GtkTreeIter *iter;
    };
    gboolean is_iter;
};

/*
 * Internal Prototypes ********************************************************
 */

static void create_window(void);
static ShellTree *tree_new(void);
static ShellInfoTree *info_tree_new(void);

static void module_selected(gpointer data);
static void module_selected_show_info(ShellModuleEntry * entry,
				      gboolean reload);
static void info_selected(GtkTreeSelection * ts, gpointer data);
static void info_selected_show_extra(const gchar *tag);
static gboolean reload_section(gpointer data);
static gboolean rescan_section(gpointer data);
static gboolean update_field(gpointer data);
static GSettings *settings=NULL;
/*
 * Globals ********************************************************************
 */

static Shell *shell = NULL;
static GHashTable *update_tbl = NULL;
static GSList *update_sfusrc = NULL;

gchar *lginterval = NULL;

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

void shell_clear_tree_models(Shell *shell)
{
    gtk_tree_store_clear(GTK_TREE_STORE(shell->tree->model));
    gtk_tree_store_clear(GTK_TREE_STORE(shell->info_tree->model));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(shell->info_tree->view), FALSE);
}

void shell_clear_timeouts(Shell *shell)
{
    h_hash_table_remove_all(update_tbl);
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

void shell_action_set_label(const gchar * action_name, gchar * label)
{
#if GTK_CHECK_VERSION(2,16,0)
  if (params.gui_running && shell->action_group) {
	GtkAction *action;

	action = gtk_action_group_get_action(shell->action_group, action_name);
	if (action) gtk_action_set_label(action, label);
    }
#endif
}

void shell_action_set_enabled(const gchar * action_name, gboolean setting)
{
  if (params.gui_running && shell->action_group) {
	GtkAction *action;

	action = gtk_action_group_get_action(shell->action_group, action_name);
	if (action) gtk_action_set_sensitive(action, setting);
    }
}

gboolean shell_action_get_enabled(const gchar * action_name)
{
    GtkAction *action;

    if (!params.gui_running)
	return FALSE;

    action = gtk_action_group_get_action(shell->action_group, action_name);
    if (action) return gtk_action_get_sensitive(action);

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

    if (!params.gui_running) return FALSE;

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

    if (!params.gui_running) return;

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
    } else if (!params.quiet) {
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
    } else if (!params.quiet) {
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
    DEBUG("SHELL_VIEW=%s\n",setting?"Normal":"Busy");
    if (!params.gui_running)
	return;

    if (setting) {
	shell->_pulses = 0;
	widget_set_cursor(shell->window, GDK_LEFT_PTR);
    } else {
        widget_set_cursor(shell->window, GDK_WATCH);
    }

    gtk_widget_set_sensitive(shell->hbox, setting);
    shell_action_set_enabled("ViewMenuAction", setting);
    shell_action_set_enabled("RefreshAction", setting);
    //shell_action_set_enabled("CopyAction", setting);
    shell_action_set_enabled("ReportAction", setting);
    shell_action_set_enabled("SyncManagerAction", setting && sync_manager_count_entries() > 0);
}

void shell_status_set_enabled(gboolean setting)
{
    DEBUG("SHELL_STATUS=%d\n",setting?1:0);
    if (!params.gui_running)
	return;

    if (setting)
        gtk_widget_show(shell->progress);
    else {
        gtk_widget_hide(shell->progress);
	shell_view_set_enabled(TRUE);

	shell_status_update(_("Done."));
    }
}

void shell_do_reload(gboolean reload)
{
    if (!params.gui_running || !shell->selected)
	return;

    shell_action_set_enabled("RefreshAction", FALSE);
    //shell_action_set_enabled("CopyAction", FALSE);
    shell_action_set_enabled("ReportAction", FALSE);

    shell_status_set_enabled(TRUE);

    params.aborting_benchmarks=0;
    if(reload) module_entry_reload(shell->selected);
    if(!params.aborting_benchmarks) {
        module_selected(NULL);
    } else {
        shell_status_update("Ready.");
        shell_status_set_enabled(FALSE);
    }

    params.aborting_benchmarks=0;

    shell_action_set_enabled("RefreshAction", TRUE);
    //shell_action_set_enabled("CopyAction", TRUE);
    shell_action_set_enabled("ReportAction", TRUE);
}

void shell_status_update(const gchar * message)
{
    DEBUG("Shell_status_update %s",message);
    if (params.gui_running) {
	gtk_label_set_markup(GTK_LABEL(shell->status), message);
	gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(shell->progress),1);
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(shell->progress));
        //gtk_widget_set_sensitive(shell->window, TRUE);
	//gtk_window_set_focus(shell->window,);
	//gtk_widget_grab_focus(shell->window);
	//gtk_widget_activate(shell->window);
	//gtk_window_get_focus_visible(shell->window);
	while (gtk_events_pending()) gtk_main_iteration();
    } else if (!params.quiet) {
	fprintf(stderr, "\033[2K\033[40;37;1m %s\033[0m\r", message);
    }
}

static void destroy_me(void)
{
    cb_quit();
}

#if GTK_CHECK_VERSION(3, 0, 0)
int update=0;
int schemeDark=0;
int changed2dark=0;
int newgnome=0;
static void stylechange2_me(void)
{
  if(update==1){
    GtkStyleContext *sctx=gtk_widget_get_style_context(GTK_WIDGET(shell->window));
    GdkRGBA color;
    gtk_style_context_lookup_color(sctx, "theme_bg_color", &color);
    gint darkmode=0;
    if((color.red+color.green+color.blue)<=1.5) darkmode=1;
    //
    if(schemeDark && (!darkmode) ) {
        if(newgnome){
            darkmode=1;
            //g_print("We need to change GTK_THEME to dark\n");
            GtkSettings *set;
	    set=gtk_settings_get_default();
	    g_object_set(set,"gtk-theme-name","HighContrastInverse", NULL);
	    changed2dark=1;
	}
    }
    if( (!schemeDark) && darkmode && changed2dark ) {
        if(newgnome){
	    darkmode=0;
            //g_print("We need to change GTK_THEME to light\n");
            GtkSettings *set;
	    set=gtk_settings_get_default();
	    g_object_set(set,"gtk-theme-name","Adwaita", NULL);
	    changed2dark=0;
	}
    }
    //
    if(darkmode!=params.darkmode){
        params.darkmode=darkmode;
        //g_print("COLOR %f %f %f, schemeDark=%i -> DARKMODE=%d\n",color.red,color.green,color.blue,schemeDark, params.darkmode);
        //update theme
        cb_disable_theme();
    }
    //
  }
  update=0;
}

//gsettings
static void stylechange3_me(void)
{
    gchar *theme=NULL;
    int newDark=0,i=0;
    gchar **keys=NULL;
    if(settings && !newgnome) keys=g_settings_list_keys(settings);
    while(!newgnome && keys && (keys[i]!=NULL)){
        if(strcmp(keys[i],"color-scheme")==0) newgnome=1;
	g_free(keys[i]);
        i++;
    }
    //check Adwaita as new gnome uses it - but new mate/budgie does not
    //theme = g_settings_get_string(settings, "gtk-theme");
    //if(!strstr(theme,"Adwaita")) newgnome=0;

    //new gnome using only normal/dark mode
    if(settings){
        if(newgnome){
            theme = g_settings_get_string(settings, "color-scheme");
            if(strstr(theme,"Dark")||strstr(theme,"dark")) newDark=1;
        } else {//older gnome using themes with dark in theme-name
            theme = g_settings_get_string(settings, "gtk-theme");//normal
            if(strstr(theme,"Dark")||strstr(theme,"dark")) newDark=1;
            g_free(theme);
            theme = g_settings_get_string(settings, "icon-theme");//alternative
            if(strstr(theme,"Dark")||strstr(theme,"dark")) newDark=1;
        }
        g_free(theme);
    }
    //
    if(newDark){
      if(schemeDark!=1) if(!update) update=1;
        schemeDark=1;
    }else{
        if(schemeDark!=0) if(!update) update=1;
        schemeDark=0;
    }
    //g_print("schemeDark=%i -> Update=%d\n",schemeDark,update);
    shell_do_reload(false);
    stylechange2_me();
}

//GTK-signal
static void stylechange_me(void)
{
    update=1;
    //g_print("GTK style signal -> Update=%d\n",update);
}
#endif

static void close_note(GtkWidget * widget, gpointer user_data)
{
    gtk_widget_hide(shell->note->event_box);
}

static ShellNote *note_new(void)
{
    ShellNote *note;
    GtkWidget *hbox, *icon, *button;
    GtkWidget *border_box;
    GdkColor info_default_border_color     = { 0, 0x0000, 0xad00, 0x9d00 };
    GdkColor info_default_fill_color       = { 0, 0x4000, 0x6000, 0xff00 };

    note = g_new0(ShellNote, 1);
    note->label = gtk_label_new("");
    note->event_box = gtk_event_box_new();
    button = gtk_button_new();

    border_box = gtk_event_box_new();
    gtk_container_set_border_width(GTK_CONTAINER(border_box), 1);
    gtk_container_add(GTK_CONTAINER(note->event_box), border_box);
    gtk_widget_show(border_box);

#if GTK_CHECK_VERSION(3, 0, 0)
    GdkRGBA info_default_text_color       = { .red = 0.7, .green = 0.7, .blue = 0.7, .alpha = 1.0 };
    gtk_widget_override_color(note->label, GTK_STATE_FLAG_NORMAL, &info_default_text_color);
#else
    GdkColor info_default_text_color       = { 0, 0xafff, 0xafff, 0xafff };
    gtk_widget_modify_fg(note->label, GTK_STATE_NORMAL, &info_default_text_color);
#endif
    gtk_widget_modify_bg(border_box, GTK_STATE_NORMAL, &info_default_fill_color);
    gtk_widget_modify_bg(note->event_box, GTK_STATE_NORMAL, &info_default_border_color);

    icon = icon_cache_get_image("close.png");
    gtk_widget_show(icon);
    gtk_container_add(GTK_CONTAINER(button), icon);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    g_signal_connect(G_OBJECT(button), "clicked", (GCallback) close_note,
		     NULL);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
#else
    hbox = gtk_hbox_new(FALSE, 3);
#endif
    icon = icon_cache_get_image("dialog-information.png");

    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), note->label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
    gtk_container_add(GTK_CONTAINER(border_box), hbox);
    gtk_widget_show_all(hbox);

    return note;
}

void shell_set_title(Shell *shell, gchar *subtitle)
{
    if (subtitle) {
        gchar *tmp;

        tmp = g_strdup_printf(_("%s - System Information and Benchmark"), subtitle);
        gtk_window_set_title(GTK_WINDOW(shell->window), tmp);

        g_free(tmp);
    } else {
        gtk_window_set_title(GTK_WINDOW(shell->window), _("System Information and Benchmark"));
    }
}

static void create_window(void)
{
    GtkWidget *vbox, *hbox;

    shell = g_new0(Shell, 1);

    shell->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon(GTK_WINDOW(shell->window), icon_cache_get_pixbuf("hardinfo2.svg"));
    shell_set_title(shell, NULL);
    gtk_window_set_default_size(GTK_WINDOW(shell->window), 1280, 800);
    g_signal_connect(G_OBJECT(shell->window), "destroy", destroy_me, NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect(G_OBJECT(shell->window), "style-updated", stylechange_me, NULL);
    g_signal_connect_after(G_OBJECT(shell->window), "draw", stylechange2_me, NULL);
    update=-1;
    if(g_settings_schema_source_lookup(g_settings_schema_source_get_default(),"org.gnome.desktop.interface",FALSE))
        settings=g_settings_new("org.gnome.desktop.interface");
    if(settings) g_signal_connect_after(settings,"changed",stylechange3_me,NULL);
    stylechange3_me();
    update=0;
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(shell->window), vbox);
    shell->vbox = vbox;

    menu_init(shell);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
    hbox = gtk_hbox_new(FALSE, 5);
#endif
    gtk_widget_show(hbox);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

    shell->progress = gtk_progress_bar_new();
    //gtk_widget_set_size_request(shell->progress, 80, 10);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_set_valign(GTK_WIDGET(shell->progress), GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(shell->progress), 0.0, 0.5);
#endif
    gtk_widget_hide(shell->progress);
    gtk_box_pack_end(GTK_BOX(hbox), shell->progress, FALSE, FALSE, 5);

    shell->status = gtk_label_new("Starting...");
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_set_valign(GTK_WIDGET(shell->status), GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(shell->status), 0.0, 0.5);
#endif
    gtk_widget_show(shell->status);
    gtk_box_pack_start(GTK_BOX(hbox), shell->status, FALSE, FALSE, 5);

#if GTK_CHECK_VERSION(3, 0, 0)
    shell->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
    shell->hbox = gtk_hbox_new(FALSE, 5);
#endif
    gtk_widget_show(shell->hbox);
    gtk_box_pack_end(GTK_BOX(vbox), shell->hbox, TRUE, TRUE, 0);

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
    vbox = gtk_vbox_new(FALSE, 5);
#endif
    gtk_widget_show(vbox);
    gtk_box_pack_end(GTK_BOX(shell->hbox), vbox, TRUE, TRUE, 0);

    shell->note = note_new();
    gtk_box_pack_end(GTK_BOX(vbox), shell->note->event_box, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3, 0, 0)
    shell->vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
    shell->vpaned = gtk_vpaned_new();
#endif
    gtk_box_pack_start(GTK_BOX(vbox), shell->vpaned, TRUE, TRUE, 0);
    gtk_widget_show(shell->vpaned);

    shell->notebook = gtk_notebook_new();
    gtk_paned_add2(GTK_PANED(shell->vpaned), shell->notebook);

    GKeyFile *key_file = g_key_file_new();
    gchar *conf_path = g_build_filename(g_get_user_config_dir(), "hardinfo2","settings.ini", NULL);
    g_key_file_load_from_file(key_file, conf_path, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    params.theme = g_key_file_get_integer(key_file, "Theme", "ThemeNumber", NULL);
    if(params.theme==0) params.theme=1;
    if(params.theme<-1) params.theme=-1;
    if(params.theme>6) params.theme=-1;

    g_free(conf_path);
    g_key_file_free(key_file);
#if GTK_CHECK_VERSION(3, 0, 0)
    if(params.theme==-1) shell_action_set_active("DisableThemeAction", TRUE);
    if(params.theme==1) shell_action_set_active("Theme1Action", TRUE);
    if(params.theme==2) shell_action_set_active("Theme2Action", TRUE);
    if(params.theme==3) shell_action_set_active("Theme3Action", TRUE);
    if(params.theme==4) shell_action_set_active("Theme4Action", TRUE);
    if(params.theme==5) shell_action_set_active("Theme5Action", TRUE);
    if(params.theme==6) shell_action_set_active("Theme6Action", TRUE);
#endif

    gtk_widget_show(shell->window);
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

static void menu_item_set_icon_always_visible(Shell *shell,
                                              gchar *parent_path,
                                              gchar *item_id)
{
    //GtkWidget *menuitem;
    //gchar *path;

    //path = g_strdup_printf("%s/%s", parent_path, item_id);
    //menuitem = gtk_ui_manager_get_widget(shell->ui_manager, path);

    //gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(menuitem), TRUE);
    //g_free(path);
}


static void
add_module_entry_to_view_menu(gchar * module, gchar * name,
			      GdkPixbuf * pixbuf, GtkTreeIter * iter)
{
    GtkAction *action;
    //GtkWidget *menuitem;
    gint merge_id;
    gchar *path;
    GtkActionEntry entry = {
       name,			/* name */
       name,			/* stockid */
       name,			/* label */
       NULL,			/* accelerator */
       NULL,			/* tooltip */
       (GCallback) view_menu_select_entry,	/* callback */
    };

    stock_icon_register_pixbuf(pixbuf, name);

    if ((action = gtk_action_group_get_action(shell->action_group, name))) {
        gtk_action_group_remove_action(shell->action_group, action);
    }

    gtk_action_group_add_actions(shell->action_group, &entry, 1, iter);

    merge_id = gtk_ui_manager_new_merge_id(shell->ui_manager);
    path = g_strdup_printf("/menubar/ViewMenu/%s", module);
    gtk_ui_manager_add_ui(shell->ui_manager,
                          merge_id,
                          path,
			  name, name, GTK_UI_MANAGER_AUTO, FALSE);
    shell->merge_ids = g_slist_prepend(shell->merge_ids, GINT_TO_POINTER(merge_id));

    menu_item_set_icon_always_visible(shell, path, name);

    g_free(path);
}

void shell_add_modules_to_gui(gpointer _shell_module, gpointer _shell_tree)
{
    ShellModule *module = (ShellModule *) _shell_module;
    ShellTree *shelltree = (ShellTree *) _shell_tree;
    GtkTreeStore *store = GTK_TREE_STORE(shelltree->model);
    GtkTreeIter parent;

    if (!module) {
        return;
    }

    gtk_tree_store_append(store, &parent, NULL);
    gtk_tree_store_set(store, &parent,
                       TREE_COL_NAME, module->name,
		       TREE_COL_MODULE, module,
		       TREE_COL_MODULE_ENTRY, NULL,
		       TREE_COL_SEL, FALSE,
		       -1);

    if (module->icon) {
	gtk_tree_store_set(store, &parent, TREE_COL_PBUF, module->icon,
			   -1);
    }

    if (module->entries) {
	ShellModuleEntry *entry;
	GSList *p;

	for (p = module->entries; p; p = g_slist_next(p)) {
	    GtkTreeIter child;
	    entry = (ShellModuleEntry *) p->data;
        if (entry->flags & MODULE_FLAG_HIDE) continue;

	    gtk_tree_store_append(store, &child, &parent);
	    gtk_tree_store_set(store, &child, TREE_COL_NAME, entry->name,
			       TREE_COL_MODULE_ENTRY, entry,
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

static void destroy_update_tbl_value(gpointer data)
{
    struct UpdateTableItem *item = data;

    if (item->is_iter) {
        gtk_tree_iter_free(item->iter);
    } else {
        g_object_unref(item->widget);
    }

    g_free(item);
}

DetailView *detail_view_new(void)
{
    DetailView *detail_view;

    detail_view = g_new0(DetailView, 1);
    detail_view->scroll = gtk_scrolled_window_new(NULL, NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
    detail_view->view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    detail_view->view = gtk_vbox_new(FALSE, 0);
#endif

    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(detail_view->scroll),
                                        GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(detail_view->scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_container_add(GTK_CONTAINER(detail_view->scroll), detail_view->view);
#else
    gtk_scrolled_window_add_with_viewport(
        GTK_SCROLLED_WINDOW(detail_view->scroll), detail_view->view);
#endif

    gtk_widget_show_all(detail_view->scroll);

    return detail_view;
}

static gboolean
select_first_tree_item(gpointer data)
{
    GtkTreeIter first;

    if (gtk_tree_model_get_iter_first(shell->tree->model, &first))
        gtk_tree_selection_select_iter(shell->tree->selection, &first);

    return FALSE;
}

static void
check_for_updates(void)
{
    GKeyFile *key_file = g_key_file_new();

    gchar *conf_path = g_build_filename(g_get_user_config_dir(), "hardinfo2",
                                        "settings.ini", NULL);

    g_key_file_load_from_file(
        key_file, conf_path,
        G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    gboolean setting = g_key_file_get_boolean(key_file, "Sync", "OnStartup", NULL);
    shell_action_set_active("SyncOnStartupAction", setting);

    g_free(conf_path);
    g_key_file_free(key_file);

    //fetching data -u is used
    if (setting || params.bench_user_note) {
        sync_manager_update_on_startup(0);
    }
}

gboolean hardinfo_link(const gchar *uri) {
    /* Clicked link events pass through here on their
     * way to the default handler (xdg-open).
     *
     * TODO: In the future, links could be used to
     * jump to different pages in hardinfo.
     *
     * if (g_str_has_prefix(uri, "hardinfo2:")) {
     *       hardinfo_navigate(g_utf8_strchr(uri, strlen("hardinfo2"), ':') + 1);
     *       return TRUE;
     * }
     */

    return FALSE; /* didn't handle it */
}

void shell_set_transient_dialog(GtkWindow *dialog)
{
    //DEBUG("TRANSIENT_DIALOG CHANGED!!!!!");
    shell->transient_dialog = dialog ? dialog : GTK_WINDOW(shell->window);
}

void shell_init(GSList * modules)
{
    if (shell) {
	g_error("Shell already created");
	return;
    }

    DEBUG("initializing shell");

    uri_set_function(hardinfo_link);
    params.fmt_opts = FMT_OPT_PANGO;

    create_window();

    //shell_action_set_property("CopyAction", "is-important", TRUE);
    shell_action_set_property("RefreshAction", "is-important", TRUE);
    shell_action_set_property("ReportAction", "is-important", TRUE);
    shell_action_set_property("SyncManagerAction", "is-important", TRUE);

#if GTK_CHECK_VERSION(3, 0, 0)
    shell_action_set_property("DisableThemeAction", "draw-as-radio", TRUE);
    shell_action_set_property("Theme1Action", "draw-as-radio", TRUE);
    shell_action_set_property("Theme2Action", "draw-as-radio", TRUE);
    shell_action_set_property("Theme3Action", "draw-as-radio", TRUE);
    shell_action_set_property("Theme4Action", "draw-as-radio", TRUE);
    shell_action_set_property("Theme5Action", "draw-as-radio", TRUE);
    shell_action_set_property("Theme6Action", "draw-as-radio", TRUE);
#endif

    shell->tree = tree_new();
    shell->info_tree = info_tree_new();
    shell->loadgraph = load_graph_new(75);
    shell->detail_view = detail_view_new();
    shell_set_transient_dialog(NULL);

    update_tbl = g_hash_table_new_full(g_str_hash, g_str_equal,
                                       g_free, destroy_update_tbl_value);

    gtk_box_pack_start(GTK_BOX(shell->hbox), shell->tree->scroll,
                       FALSE, FALSE, 0);
    gtk_paned_pack1(GTK_PANED(shell->vpaned), shell->info_tree->scroll,
		    SHELL_PACK_RESIZE, SHELL_PACK_SHRINK);

    gtk_notebook_append_page(GTK_NOTEBOOK(shell->notebook),
			     load_graph_get_framed(shell->loadgraph),
			     NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(shell->notebook),
                             shell->detail_view->scroll, NULL);

    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(shell->notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(shell->notebook), FALSE);

    shell_status_set_enabled(TRUE);
    shell_status_update(_("Loading modules..."));

    shell->tree->modules = modules ? modules : modules_load_all();

    check_for_updates();

    g_slist_foreach(shell->tree->modules, shell_add_modules_to_gui, shell->tree);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->tree->view));

    gtk_widget_show_all(shell->hbox);

    load_graph_configure_expose(shell->loadgraph);
    gtk_widget_hide(shell->notebook);
    gtk_widget_hide(shell->note->event_box);

    shell_status_update(_("Done."));
    shell_status_set_enabled(FALSE);

    shell_action_set_enabled("RefreshAction", FALSE);
    //shell_action_set_enabled("CopyAction", FALSE);
    shell_action_set_active("SidePaneAction", TRUE);
    shell_action_set_active("ToolbarAction", TRUE);

    shell_action_set_enabled("SyncManagerAction", sync_manager_count_entries() > 0);

    /* Should select Computer Summary (note: not Computer/Summary) */
    g_idle_add(select_first_tree_item, NULL);
}

static gboolean update_field(gpointer data)
{
    ShellFieldUpdate *fu;
    struct UpdateTableItem *item;

    fu = (ShellFieldUpdate *)data;
    g_return_val_if_fail(fu != NULL, FALSE);

    DEBUG("update_field [%s]", fu->field_name);

    item = g_hash_table_lookup(update_tbl, fu->field_name);
    if (!item) {
        return FALSE;
    }

    /* if the entry is still selected, update it */
    if (fu->entry->selected && fu->entry->fieldfunc) {
        gchar *value = fu->entry->fieldfunc(fu->field_name);
	gdouble v;

        if (item->is_iter) {
            /*
             * this function is also used to feed the load graph when ViewType
             * is SHELL_VIEW_LOAD_GRAPH
             */
            if (shell->view_type == SHELL_VIEW_LOAD_GRAPH &&
                gtk_tree_selection_iter_is_selected(shell->info_tree->selection,
                                                    item->iter)) {

                load_graph_set_title(shell->loadgraph, fu->field_name);
                v=atof(value);
		//fix KiB->Bytes for UberGraph (GTK3)
#if GTK_CHECK_VERSION(3, 0, 0)
                if(strstr(value,"KiB")) v*=1024;
#endif
                load_graph_update(shell->loadgraph, v);
            }

            GtkTreeStore *store = GTK_TREE_STORE(shell->info_tree->model);
            gtk_tree_store_set(store, item->iter, INFO_TREE_COL_VALUE, value, -1);
        } else {
            GList *children = gtk_container_get_children(GTK_CONTAINER(item->widget));
            gtk_label_set_markup(GTK_LABEL(children->next->data), value);
            g_list_free(children);
        }

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

    /* otherwise, cleanup and destroy the timeout */
    g_free(fu->field_name);
    g_free(fu);

    return FALSE;
}

#if GTK_CHECK_VERSION(3, 0, 0)
    #define RANGE_SET_VALUE(tree,scrollbar,value) {while (gtk_events_pending()) gtk_main_iteration();gtk_range_set_value(GTK_RANGE(gtk_scrolled_window_get_##scrollbar(GTK_SCROLLED_WINDOW(shell->tree->scroll))), value);}
    #define RANGE_GET_VALUE(tree,scrollbar) gtk_range_get_value(GTK_RANGE(gtk_scrolled_window_get_##scrollbar(GTK_SCROLLED_WINDOW(shell->tree->scroll))))
#else
  #define RANGE_SET_VALUE(tree, scrollbar, value)			       \
    do {                                                                       \
        GtkRange CONCAT(*range, __LINE__) =                                    \
            GTK_RANGE(GTK_SCROLLED_WINDOW(shell->tree->scroll)->scrollbar);    \
        gtk_range_set_value(CONCAT(range, __LINE__), value);                   \
        gtk_adjustment_value_changed(GTK_ADJUSTMENT(                           \
            gtk_range_get_adjustment(CONCAT(range, __LINE__))));               \
    } while (0)
  #define RANGE_GET_VALUE(tree, scrollbar)                                     \
    gtk_range_get_value(                                                       \
        GTK_RANGE(GTK_SCROLLED_WINDOW(shell->tree->scroll)->scrollbar))
#endif

static void
destroy_widget(GtkWidget *widget, gpointer user_data)
{
    gtk_widget_destroy(widget);
}

static void detail_view_clear(DetailView *detail_view)
{
    gtk_container_forall(GTK_CONTAINER(shell->detail_view->view),
                         destroy_widget, NULL);
    RANGE_SET_VALUE(detail_view, vscrollbar, 0.0);
    RANGE_SET_VALUE(detail_view, hscrollbar, 0.0);
}

static gboolean reload_section(gpointer data)
{
    ShellModuleEntry *entry = (ShellModuleEntry *)data;
#if GTK_CHECK_VERSION(2, 14, 0)
    //GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(shell->window));
#endif

    /* if the entry is still selected, update it */
    if (entry->selected) {
        GtkTreePath *path = NULL;
        GtkTreeIter iter;
        double pos_info_scroll;
        double pos_detail_scroll;

        /* save current position */
        pos_info_scroll = RANGE_GET_VALUE(info_tree, vscrollbar);
        pos_detail_scroll = RANGE_GET_VALUE(detail_view, vscrollbar);

        /* gets the current selected path */
        if (gtk_tree_selection_get_selected(shell->info_tree->selection,
                                            &shell->info_tree->model, &iter)) {
            path = gtk_tree_model_get_path(shell->info_tree->model, &iter);
        }

        /* update the information, clear the treeview and populate it again */
        module_entry_reload(entry);
        detail_view_clear(shell->detail_view);
        module_selected_show_info(entry, TRUE);

        /* if there was a selection, reselect it */
        if (path) {
            gtk_tree_selection_select_path(shell->info_tree->selection, path);
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(shell->info_tree->view),
                                     path, NULL, FALSE);
            gtk_tree_path_free(path);
        } else {
            /* restore position */
            RANGE_SET_VALUE(info_tree, vscrollbar, pos_info_scroll);
        }

        RANGE_SET_VALUE(detail_view, vscrollbar, pos_detail_scroll);

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

static gint
compare_float(float a, float b)
{
    return (a > b) - (a < b);
}

static gint
info_tree_compare_val_func(GtkTreeModel * model,
			   GtkTreeIter * a,
			   GtkTreeIter * b, gpointer userdata)
{
    gint ret = 0;
    gchar *col1, *col2;

    gtk_tree_model_get(model, a, INFO_TREE_COL_VALUE, &col1, -1);
    gtk_tree_model_get(model, b, INFO_TREE_COL_VALUE, &col2, -1);

    if (!col1 && !col2)
        ret = 0;
    else if (!col1)
        ret = -1;
    else if (!col2)
        ret = 1;
    else if (shell->_order_type == SHELL_ORDER_ASCENDING)
        ret = compare_float(atof(col2), atof(col1));
    else
        ret = compare_float(atof(col1), atof(col2));

    g_free(col1);
    g_free(col2);

    return ret;
}

static void set_view_type(ShellViewType viewtype, gboolean reload)
{
#if GTK_CHECK_VERSION(2, 18, 0)
    GtkAllocation* alloc;
#endif
    gboolean type_changed = FALSE;
    if (viewtype != shell->view_type)
        type_changed = TRUE;

    if (viewtype >= SHELL_VIEW_N_VIEWS)
        viewtype = SHELL_VIEW_NORMAL;

    shell->normalize_percentage = TRUE;
    shell->view_type = viewtype;
    shell->_order_type = SHELL_ORDER_DESCENDING;

    /* use an unsorted tree model */
    GtkTreeSortable *sortable = GTK_TREE_SORTABLE(shell->info_tree->model);

    gtk_tree_sortable_set_sort_column_id(sortable,
        GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
        GTK_SORT_ASCENDING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info_tree->view),
        GTK_TREE_MODEL(sortable));

    /* reset to the default view columns */
    if (!reload) {
      gtk_tree_view_column_set_visible(shell->info_tree->col_extra1, FALSE);
      gtk_tree_view_column_set_visible(shell->info_tree->col_extra2, FALSE);
      gtk_tree_view_column_set_visible(shell->info_tree->col_progress, FALSE);
      gtk_tree_view_column_set_visible(shell->info_tree->col_value, TRUE);
    }

    /* turn off the rules hint */
#if GTK_CHECK_VERSION(3, 0, 0)
#else
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(shell->info_tree->view), FALSE);
#endif

    close_note(NULL, NULL);
    detail_view_clear(shell->detail_view);

    switch (viewtype) {
    default:
    case SHELL_VIEW_NORMAL:
        gtk_widget_show(shell->info_tree->scroll);
        gtk_widget_hide(shell->notebook);

        if (!reload) {
            gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(shell->info_tree->view), FALSE);
        }
        break;
    case SHELL_VIEW_DUAL:
        gtk_widget_show(shell->info_tree->scroll);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(shell->notebook), 1);
        gtk_widget_show(shell->notebook);

        if (type_changed) {
#if GTK_CHECK_VERSION(2, 18, 0)
            alloc = g_new(GtkAllocation, 1);
            gtk_widget_get_allocation(shell->hbox, alloc);
            gtk_paned_set_position(GTK_PANED(shell->vpaned), alloc->height / 2);
            g_free(alloc);
#else
            gtk_paned_set_position(GTK_PANED(shell->vpaned),
                            shell->hbox->allocation.height / 2);
#endif
        }
        break;
    case SHELL_VIEW_LOAD_GRAPH:
        gtk_widget_show(shell->info_tree->scroll);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(shell->notebook), 0);
        gtk_widget_show(shell->notebook);
        load_graph_clear(shell->loadgraph);

        if (type_changed) {
#if GTK_CHECK_VERSION(2, 18, 0)
            alloc = g_new(GtkAllocation, 1);
            gtk_widget_get_allocation(shell->hbox, alloc);
            gtk_paned_set_position(GTK_PANED(shell->vpaned),
                    alloc->height - load_graph_get_height(shell->loadgraph) - 16);
            g_free(alloc);
#else
            gtk_paned_set_position(GTK_PANED(shell->vpaned),
                           shell->hbox->allocation.height -
                           load_graph_get_height(shell->loadgraph) - 16);
#endif
        }
        break;
    case SHELL_VIEW_PROGRESS_DUAL:
	gtk_widget_show(shell->notebook);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(shell->notebook), 1);
	/* fallthrough */
    case SHELL_VIEW_PROGRESS:
        gtk_widget_show(shell->info_tree->scroll);

	if (!reload) {
         gtk_tree_view_column_set_visible(shell->info_tree->col_progress, TRUE);
         gtk_tree_view_column_set_visible(shell->info_tree->col_value, FALSE);
        }

	if (viewtype == SHELL_VIEW_PROGRESS)
		gtk_widget_hide(shell->notebook);
	break;
    case SHELL_VIEW_DETAIL:
        gtk_notebook_set_current_page(GTK_NOTEBOOK(shell->notebook), 1);

        gtk_widget_show(shell->notebook);
        gtk_widget_hide(shell->info_tree->scroll);
    }
}

static void group_handle_special(GKeyFile *key_file,
                                 ShellModuleEntry *entry,
                                 const gchar *group,
                                 gchar **keys)
{
    if (!g_str_equal(group, "$ShellParam$")) {
        g_warning("Unknown parameter group: ``%s''", group);
        return;
    }

    gboolean headers_visible = FALSE;
    gint i;

    for (i = 0; keys[i]; i++) {
        gchar *key = keys[i];

        if (g_str_has_prefix(key, "UpdateInterval$")) {
            ShellFieldUpdate *fu = g_new0(ShellFieldUpdate, 1);
            ShellFieldUpdateSource *sfutbl;
            gint ms;

            ms = g_key_file_get_integer(key_file, group, key, NULL);

            /* Old style used just the label which has to be checked by translating it,
             * and potentially could by ambiguous.
             * New style can use tag or label. If new style including a tag,
             * send both tag and label and let the hi_get_field() function use
             * key_get_components() to split it. */
            const gchar *chk = g_utf8_strchr(key, -1, '$');
            fu->field_name = g_strdup(key_is_flagged(chk) ? chk : chk + 1);
            fu->entry = entry;

            sfutbl = g_new0(ShellFieldUpdateSource, 1);
            sfutbl->source_id = g_timeout_add(ms, update_field, fu);
            sfutbl->sfu = fu;

            update_sfusrc = g_slist_prepend(update_sfusrc, sfutbl);
        } else if (g_str_equal(key, "NormalizePercentage")) {
            shell->normalize_percentage =
                g_key_file_get_boolean(key_file, group, key, NULL);
        } else if (g_str_equal(key, "LoadGraphSuffix")) {
            gchar *suffix = g_key_file_get_value(key_file, group, key, NULL);
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
            headers_visible =
                g_key_file_get_boolean(key_file, group, key, NULL);
        } else if (g_str_has_prefix(key, "ColumnTitle")) {
            GtkTreeViewColumn *column = NULL;
            gchar *value, *title = g_utf8_strchr(key, -1, '$') + 1;

            value = g_key_file_get_value(key_file, group, key, NULL);

            if (g_str_equal(title, "Extra1")) {
                column = shell->info_tree->col_extra1;
            } else if (g_str_equal(title, "Extra2")) {
                column = shell->info_tree->col_extra2;
            } else if (g_str_equal(title, "Value")) {
                column = shell->info_tree->col_value;
            } else if (g_str_equal(title, "TextValue")) {
                column = shell->info_tree->col_textvalue;
            } else if (g_str_equal(title, "Progress")) {
                column = shell->info_tree->col_progress;
            }

            if (column) {
                gtk_tree_view_column_set_title(column, value);
                gtk_tree_view_column_set_visible(column, TRUE);
            }

            g_free(value);
        } else if (g_str_equal(key, "OrderType")) {
            shell->_order_type =
                g_key_file_get_integer(key_file, group, key, NULL);
        } else if (g_str_has_prefix(key, "Icon$")) {
            struct UpdateTableItem *item;

            const gchar *ikey = g_utf8_strchr(key, -1, '$');
            gchar *tag, *name;
            key_get_components(ikey, NULL, &tag, &name, NULL, NULL, TRUE);

            if (tag)
                item = g_hash_table_lookup(update_tbl, tag);
             else
                item = g_hash_table_lookup(update_tbl, name);
            g_free(name);
            g_free(tag);

            if (item) {
                gchar *file = g_key_file_get_value(key_file, group, key, NULL);
		GdkPixbuf *pixbuf;
		if(strstr(file,"LARGE")==file){
		    file=strreplace(file,"LARGE","");
                     pixbuf = icon_cache_get_pixbuf_at_size(file, -1, 33);
		} else {
                     pixbuf = icon_cache_get_pixbuf_at_size(file, 22, 22);
		}

                g_free(file);

                if (item->is_iter) {
                    gtk_tree_store_set(
                        GTK_TREE_STORE(shell->info_tree->model), item->iter,
                        INFO_TREE_COL_PBUF, pixbuf, -1);
                } else {
                    GList *children = gtk_container_get_children(GTK_CONTAINER(item->widget));
                    gtk_image_set_from_pixbuf(GTK_IMAGE(children->data), pixbuf);
                    gtk_widget_show(GTK_WIDGET(children->data));
                    g_list_free(children);
                }
            }
        } else if (g_str_equal(key, "Zebra")) {
#if GTK_CHECK_VERSION(3, 0, 0)
#else
            gtk_tree_view_set_rules_hint(
                GTK_TREE_VIEW(shell->info_tree->view),
                g_key_file_get_boolean(key_file, group, key, NULL));
#endif
        }
    }

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(shell->info_tree->view),
                                      headers_visible);
}

static void group_handle_normal(GKeyFile *key_file,
                                ShellModuleEntry *entry,
                                const gchar *group,
                                gchar **keys,
                                gsize ngroups)
{
    GtkTreeIter parent;
    GtkTreeStore *store = GTK_TREE_STORE(shell->info_tree->model);
    gint i;

    if (ngroups > 1) {
        gtk_tree_store_append(store, &parent, NULL);

        gchar *tmp = g_strdup(group);
        strend(tmp, '#');
        gtk_tree_store_set(store, &parent, INFO_TREE_COL_NAME, tmp, -1);
        g_free(tmp);
    }

    g_key_file_set_list_separator(key_file, '|');

    for (i = 0; keys[i]; i++) {
        gchar *key = keys[i];
        gchar **values;
        gsize vcount = 0;
        GtkTreeIter child;

        values = g_key_file_get_string_list(key_file, group, key, &vcount, NULL);
        if (!vcount) {
            /* Check for empty value */
            values = g_new0(gchar*, 2);
            values[0] = g_key_file_get_string(key_file, group, key, NULL);
            if (values[0]) {
                vcount = 1;
            } else {
                g_strfreev(values);
                continue;
            }
        }

        if (entry->fieldfunc && values[0] && g_str_equal(values[0], "...")) {
            g_free(values[0]);
            values[0] = entry->fieldfunc(key);
        }

        if (ngroups == 1) {
            gtk_tree_store_append(store, &child, NULL);
        } else {
            gtk_tree_store_append(store, &child, &parent);
        }

        if (vcount > 0)
            gtk_tree_store_set(store, &child, INFO_TREE_COL_VALUE,
                               values[0], -1);
        if (vcount > 1)
            gtk_tree_store_set(store, &child, INFO_TREE_COL_EXTRA1,
                               values[1], -1);
        if (vcount > 2)
            gtk_tree_store_set(store, &child, INFO_TREE_COL_EXTRA2,
                               values[2], -1);

        struct UpdateTableItem *item = g_new0(struct UpdateTableItem, 1);
        item->is_iter = TRUE;
        item->iter = gtk_tree_iter_copy(&child);
        gchar *flags, *tag, *name, *label;
        key_get_components(key, &flags, &tag, &name, &label, NULL, TRUE);

        if (flags) {
            //TODO: name was formerly used where label is here. Check all uses
            //for problems.
            gtk_tree_store_set(store, &child, INFO_TREE_COL_NAME, label,
                               INFO_TREE_COL_DATA, flags, -1);
            g_hash_table_insert(update_tbl, tag, item);
            g_free(label);
        } else {
            gtk_tree_store_set(store, &child, INFO_TREE_COL_NAME, key,
                               INFO_TREE_COL_DATA, NULL, -1);
            g_hash_table_insert(update_tbl, name, item);
            g_free(tag);
        }
        g_free(flags);

        g_strfreev(values);
    }
}

static void update_progress()
{
    GtkTreeModel *model = shell->info_tree->model;
    GtkTreeStore *store = GTK_TREE_STORE(model);
    GtkTreeIter iter, fiter;
    gchar *tmp;
    gdouble maxv = INT_MIN, minv = INT_MAX, coeff, cur;

    if (!gtk_tree_model_get_iter_first(model, &fiter))
	return;

    /* finds the maximum value */
    if (shell->normalize_percentage) {
	iter = fiter;
	do {
	    gtk_tree_model_get(model, &iter, INFO_TREE_COL_VALUE, &tmp, -1);

	    cur = atof(tmp);
	    if (cur > maxv)
		maxv = cur;
	    if (cur < minv)
		minv = cur;

	    g_free(tmp);
	} while (gtk_tree_model_iter_next(model, &iter));

	if (minv - maxv < 0.001)
	    maxv += 1.0f;
    } else {
	minv = 1.0f;
	maxv = 100.0f;
    }

    coeff = (100.0f - 1.0f) / (maxv - minv);

    /* fix the maximum relative percentage */
    iter = fiter;
    do {
	char *space;
	char formatted[128];
	gdouble pct;

	gtk_tree_model_get(model, &iter, INFO_TREE_COL_VALUE, &tmp, -1);
	cur = atof(tmp);
	space = g_utf8_strchr(tmp, -1, ' ');

	pct = coeff * (cur - minv) + 1.0f;
	if (shell->_order_type == SHELL_ORDER_ASCENDING)
	    pct = 100.0 - pct;
	pct = ceil(pct);

	if (space) {
	    snprintf(formatted, sizeof(formatted), "%.2f%s", cur, space);
	} else {
	    snprintf(formatted, sizeof(formatted), "%.2f", cur);
	}

	gtk_tree_store_set(store, &iter, INFO_TREE_COL_PROGRESS, pct,
			   INFO_TREE_COL_VALUE, strreplacechr(formatted, ",",
							      '.'), -1);

	g_free(tmp);
    } while (gtk_tree_model_iter_next(model, &iter));

    /* now sort everything up. that wasn't as hard as i thought :) */
    GtkTreeSortable *sortable = GTK_TREE_SORTABLE(shell->info_tree->model);

    gtk_tree_sortable_set_sort_func(sortable, INFO_TREE_COL_VALUE,
				    info_tree_compare_val_func, 0, NULL);
    gtk_tree_sortable_set_sort_column_id(sortable, INFO_TREE_COL_VALUE,
					 GTK_SORT_DESCENDING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info_tree->view),
			    GTK_TREE_MODEL(sortable));
}

void shell_set_note_from_entry(ShellModuleEntry * entry)
{
    if (entry->notefunc) {
	const gchar *note = module_entry_get_note(entry);

	if (note) {
	    gtk_label_set_markup(GTK_LABEL(shell->note->label), note);
	    gtk_widget_show(shell->note->event_box);
	} else {
	    gtk_widget_hide(shell->note->event_box);
	}
    } else {
	gtk_widget_hide(shell->note->event_box);
    }
}

void shell_clear_field_updates(void)
{
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
}

/*static gboolean
select_first_item(gpointer data)
{
    GtkTreeIter first;

    if (gtk_tree_model_get_iter_first(shell->info_tree->model, &first))
        gtk_tree_selection_select_iter(shell->info_tree->selection, &first);

    return FALSE;
}*/

static gboolean select_marked_or_first_item(gpointer data)
{
    GtkTreeIter first, it;
    gboolean found_selection = FALSE;
    gchar *datacol;

    if (gtk_tree_model_get_iter_first(shell->info_tree->model, &first)) {
        it = first;
        while (gtk_tree_model_iter_next(shell->info_tree->model, &it)) {
            gtk_tree_model_get(shell->info_tree->model, &it, INFO_TREE_COL_DATA, &datacol, -1);

            if (key_is_highlighted(datacol)) {
                gtk_tree_selection_select_iter(shell->info_tree->selection, &it);

		//scoll to selected this machine benchmark
		GtkTreePath *path=gtk_tree_model_get_path(shell->info_tree->model, &it);
	        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(shell->info_tree->view), path, NULL, TRUE, 0.5, 0.0);
		gtk_tree_path_free(path);

                found_selection = TRUE;
            }
            g_free(datacol);
        }

        if (!found_selection)
            gtk_tree_selection_select_iter(shell->info_tree->selection, &first);
    }
    return FALSE;
}

static void module_selected_show_info_list(GKeyFile *key_file,
                                           ShellModuleEntry *entry,
                                           gchar **groups,
                                           gsize ngroups)
{
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkCssProvider *provider;
    provider = gtk_css_provider_new();
#endif
    GtkTreeStore *store = GTK_TREE_STORE(shell->info_tree->model);
    gint i;

    gtk_tree_store_clear(store);

    g_object_ref(shell->info_tree->model);
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info_tree->view), NULL);

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(shell->info_tree->view), FALSE);

#if GTK_CHECK_VERSION(3, 0, 0)
    if(params.theme>0){
      if(params.darkmode){
        gtk_css_provider_load_from_data(provider, "treeview { background-color: rgba(0xa0, 0xa0, 0xa0, 0.1); } treeview:selected { background-color: rgba(0x60, 0x80, 0xff, 1); } ", -1, NULL);
        gtk_style_context_add_provider(gtk_widget_get_style_context(shell->info_tree->view), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      }else{
        gtk_css_provider_load_from_data(provider, "treeview { background-color: rgba(0x60, 0x60, 0x60, 0.1); } treeview:selected { background-color: rgba(0x40, 0x60, 0xff, 1); } ", -1, NULL);
        gtk_style_context_add_provider(gtk_widget_get_style_context(shell->info_tree->view), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      }
    }
#endif

    for (i = 0; groups[i]; i++) {
        gchar **keys = g_key_file_get_keys(key_file, groups[i], NULL, NULL);

        if (groups[i][0] == '$') {
            group_handle_special(key_file, entry, groups[i], keys);
        } else {
            group_handle_normal(key_file, entry, groups[i], keys, ngroups);
        }

        g_strfreev(keys);
    }

    g_object_unref(shell->info_tree->model);
    gtk_tree_view_set_model(GTK_TREE_VIEW(shell->info_tree->view), shell->info_tree->model);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->info_tree->view));
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(shell->info_tree->view), ngroups > 1);
}

static gboolean detail_activate_link (GtkLabel *label, gchar *uri, gpointer user_data) {
    return uri_open(uri);
}

static gchar *vendor_info_markup(const Vendor *v) {
    if (!v) return NULL;
    gchar *ven_mt = NULL;
    gchar *full_link = NULL, *p = NULL;
    gchar *ven_tag = v->name_short ? g_strdup(v->name_short) : g_strdup(v->name);
    tag_vendor(&ven_tag, 0, ven_tag, v->ansi_color, FMT_OPT_PANGO);
    //if (v->name_short)
    //    ven_mt = appf(ven_mt, "\n", "%s", v->name);
    ven_mt = appf(ven_mt, "\n", "%s", ven_tag);
    if (v->url) {
        if (!g_str_has_prefix(v->url, "http") )
            full_link = g_strdup_printf("http://%s", v->url);
        ven_mt = appf(ven_mt, "\n", "<b>%s:</b> <a href=\"%s\">%s</a>", _("URL"), full_link ? full_link : v->url, v->url);
        g_free(full_link);
        full_link = NULL;
    }
    if (v->url_support) {
        if (!g_str_has_prefix(v->url_support, "http") )
            full_link = g_strdup_printf("http://%s", v->url_support);
        ven_mt = appf(ven_mt, "\n", "<b>%s:</b> <a href=\"%s\">%s</a>", _("Support URL"), full_link ? full_link : v->url_support, v->url_support);
        g_free(full_link);
        full_link = NULL;
    }
    if (v->wikipedia) {
        /* sending the title to wikipedia.com/wiki will autmatically handle the language and section parts,
         * but perhaps that shouldn't be relied on so much? */
        full_link = g_strdup_printf("http://wikipedia.com/wiki/%s", v->wikipedia);
        for(p = full_link; *p; p++) {
            if (*p == ' ') *p = '_';
        }
        ven_mt = appf(ven_mt, "\n", "<b>%s:</b> <a href=\"%s\">%s</a>", _("Wikipedia"), full_link ? full_link : v->wikipedia, v->wikipedia);
        g_free(full_link);
        full_link = NULL;
    }
    g_free(ven_tag);
    return ven_mt;
}

static void module_selected_show_info_detail(GKeyFile *key_file,
                                             ShellModuleEntry *entry,
                                             gchar **groups)
{
    gint i;

    detail_view_clear(shell->detail_view);

    for (i = 0; groups[i]; i++) {
        gsize nkeys;
        gchar **keys = g_key_file_get_keys(key_file, groups[i], &nkeys, NULL);
        gchar *group_label = g_strdup(groups[i]);
        strend(group_label, '#');

        if (entry && groups[i][0] == '$') {
            group_handle_special(key_file, entry, groups[i], keys);
        } else {
            gchar *tmp = g_strdup_printf("<b>%s</b>", group_label);
            GtkWidget *label = gtk_label_new(tmp);
            gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_frame_set_label_widget(GTK_FRAME(frame), label);
            gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
            g_free(tmp);

            gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
            gtk_box_pack_start(GTK_BOX(shell->detail_view->view), frame, FALSE,
                               FALSE, 0);

            GtkWidget *table = gtk_table_new(nkeys, 2, FALSE);
            gtk_container_set_border_width(GTK_CONTAINER(table), 4);
            gtk_container_add(GTK_CONTAINER(frame), table);

            gint j, a = 0;
            for (j = 0; keys[j]; j++) {
                gchar *key_markup;
                gchar *value;
                gchar *name, *label, *tag, *flags;
                key_get_components(keys[j], &flags, &tag, &name, &label, NULL, TRUE);

                value = g_key_file_get_string(key_file, groups[i], keys[j], NULL);

                if (entry && entry->fieldfunc && value && g_str_equal(value, "...")) {
                    g_free(value);
                    value = entry->fieldfunc(keys[j]);
                }

                gboolean has_ven = key_value_has_vendor_string(flags);
                const Vendor *v = has_ven ? vendor_match(value, NULL) : NULL;

                if(params.darkmode){
                    key_markup = g_strdup_printf("<span color=\"#8af\">%s</span>", label);
		} else {
                    key_markup = g_strdup_printf("<span color=\"#24f\">%s</span>", label);
		}

                GtkWidget *key_label = gtk_label_new(key_markup);
                gtk_label_set_use_markup(GTK_LABEL(key_label), TRUE);
                gtk_misc_set_alignment(GTK_MISC(key_label), 1.0f, 0.5f);

                GtkWidget *value_label = gtk_label_new(value);
                gtk_label_set_use_markup(GTK_LABEL(value_label), TRUE);
                gtk_label_set_selectable(GTK_LABEL(value_label), TRUE);
#if !GTK_CHECK_VERSION(3, 0, 0)
                gtk_label_set_line_wrap(GTK_LABEL(value_label), TRUE);
#endif
                gtk_misc_set_alignment(GTK_MISC(value_label), 0.0f, 0.5f);

                GtkWidget *value_icon = gtk_image_new();

                GtkWidget *value_box = gtk_hbox_new(FALSE, 4);
                gtk_box_pack_start(GTK_BOX(value_box), value_icon, FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(value_box), value_label, TRUE, TRUE, 0);

                g_signal_connect(key_label, "activate-link", G_CALLBACK(detail_activate_link), NULL);
                g_signal_connect(value_label, "activate-link", G_CALLBACK(detail_activate_link), NULL);

                gtk_widget_show(key_label);
                gtk_widget_show(value_box);
                gtk_widget_show(value_label);

                gtk_table_attach(GTK_TABLE(table), key_label, 0, 1, j + a, j + a + 1,
                                 GTK_FILL, GTK_FILL, 6, 4);
                gtk_table_attach(GTK_TABLE(table), value_box, 1, 2, j + a, j + a + 1,
                                 GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 4);

                if (v) {
                    a++; /* insert a row */
                    gchar *vendor_markup = vendor_info_markup(v);
                    GtkWidget *vendor_label = gtk_label_new(vendor_markup);
                    gtk_label_set_use_markup(GTK_LABEL(vendor_label), TRUE);
                    gtk_label_set_selectable(GTK_LABEL(vendor_label), TRUE);
                    gtk_misc_set_alignment(GTK_MISC(vendor_label), 0.0f, 0.5f);
                    g_signal_connect(vendor_label, "activate-link", G_CALLBACK(detail_activate_link), NULL);
                    GtkWidget *vendor_box = gtk_hbox_new(FALSE, 4);
                    gtk_box_pack_start(GTK_BOX(vendor_box), vendor_label, TRUE, TRUE, 0);
                    gtk_table_attach(GTK_TABLE(table), vendor_box, 1, 2, j + a, j + a + 1,
                                     GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 4);
                    gtk_widget_show(vendor_box);
                    gtk_widget_show(vendor_label);
                    g_free(vendor_markup);
                }

                struct UpdateTableItem *item = g_new0(struct UpdateTableItem, 1);
                item->is_iter = FALSE;
                item->widget = g_object_ref(value_box);

                if (tag) {
                    g_hash_table_insert(update_tbl, tag, item);
                    g_free(name);
                } else {
                    g_hash_table_insert(update_tbl, name, item);
                    g_free(tag);
                }

                g_free(flags);
                g_free(value);
                g_free(key_markup);
                g_free(label);
            }

            gtk_widget_show(table);
            gtk_widget_show(label);
            gtk_widget_show(frame);
        }

        g_strfreev(keys);
        g_free(group_label);
    }
}

static void
module_selected_show_info(ShellModuleEntry *entry, gboolean reload)
{
    //GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(shell->info_tree->view));
    gsize ngroups;
    gint i;

    module_entry_scan(entry);
    if (!reload) {
        /* recreate the iter hash table */
        h_hash_table_remove_all(update_tbl);
    }
    shell_clear_field_updates();

    GKeyFile *key_file = g_key_file_new();
    gchar *key_data = module_entry_function(entry);

    g_key_file_load_from_data(key_file, key_data, strlen(key_data), 0, NULL);
    set_view_type(g_key_file_get_integer(key_file, "$ShellParam$",
                                         "ViewType", NULL), reload);

    gchar **groups = g_key_file_get_groups(key_file, &ngroups);

    for (i = 0; groups[i]; i++) {
	if (groups[i][0] == '$')
	    ngroups--;
    }

    if (shell->view_type == SHELL_VIEW_DETAIL) {
        module_selected_show_info_detail(key_file, entry, groups);
    } else {
        module_selected_show_info_list(key_file, entry, groups, ngroups);
    }

    g_strfreev(groups);
    g_key_file_free(key_file);
    g_free(key_data);

    switch (shell->view_type) {
    case SHELL_VIEW_PROGRESS_DUAL:
    case SHELL_VIEW_PROGRESS:
        update_progress();
        break;
    default:
        break;
    }

    if (!reload) {
        switch (shell->view_type) {
        case SHELL_VIEW_DUAL:
        case SHELL_VIEW_LOAD_GRAPH:
        case SHELL_VIEW_PROGRESS_DUAL:
            g_idle_add(select_marked_or_first_item, NULL);
        default:
            break;
        }
    }
    shell_set_note_from_entry(entry);
}

static void info_selected_show_extra(const gchar *tag)
{
    if (!tag || !shell->selected->morefunc)
        return;

    GKeyFile *key_file = g_key_file_new();
    gchar *key_data = shell->selected->morefunc((gchar *)tag);
    gchar **groups;

    g_key_file_load_from_data(key_file, key_data, strlen(key_data), 0, NULL);
    groups = g_key_file_get_groups(key_file, NULL);

    module_selected_show_info_detail(key_file, NULL, groups);

    g_strfreev(groups);
    g_key_file_free(key_file);
    g_free(key_data);
}

static gchar *detail_view_clear_value(gchar *value)
{
     GKeyFile *keyfile;
     gchar *return_value;

     keyfile = g_key_file_new();
     if (!value) return_value = g_strdup("");
     else
     if (g_key_file_load_from_data(keyfile, value,
                                   strlen(value), 0, NULL)) {
          gchar **groups;
          gint group;

          return_value = g_strdup("");

          groups = g_key_file_get_groups(keyfile, NULL);
          for (group = 0; groups[group]; group++) {
               gchar **keys;
               gint key;

               keys = g_key_file_get_keys(keyfile, groups[group], NULL, NULL);
               for (key = 0; keys[key]; key++) {
                  gchar *temp = keys[key];

                  if (*temp == '$') {
                     temp++;
                     while (*temp && *temp != '$')
                        temp++;
                     temp++;

                     return_value = h_strdup_cprintf("%s\n", return_value, temp);
                  } else {
                     return_value = g_key_file_get_string(keyfile, groups[group],
                                                          keys[key], NULL);
                  }
               }

               g_strfreev(keys);
          }

          g_strfreev(groups);
     } else {
          return_value = g_strdup(value);
     }

     g_key_file_free(keyfile);

     return g_strstrip(return_value);
}

static void detail_view_add_item(DetailView *detail_view,
                                 gchar *icon,
                                 gchar *name,
                                 gchar *value)
{
#if GTK_CHECK_VERSION(3, 0, 0)
#else
    GtkWidget *alignment;
#endif
    GtkWidget *frame;
    GtkWidget *frame_label_box;
    GtkWidget *frame_image;
    GtkWidget *frame_label;
    GtkWidget *content;
    gchar *temp;

    temp = detail_view_clear_value(value);

    /* creates the frame */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);

#if GTK_CHECK_VERSION(3, 0, 0)
    frame_label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
    frame_label_box = gtk_hbox_new(FALSE, 5);
#endif
    frame_image = icon_cache_get_image(icon);
    frame_label = gtk_label_new(name);
    gtk_label_set_use_markup(GTK_LABEL(frame_label), TRUE);
    gtk_box_pack_start(GTK_BOX(frame_label_box), frame_image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(frame_label_box), frame_label, FALSE, FALSE, 0);

    content = gtk_label_new(temp);
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget *frame_box;
    frame_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(GTK_WIDGET(frame_box), 48);
    gtk_box_pack_start(GTK_BOX(frame_box), content, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), frame_box);
#else
    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 48, 0);
    gtk_widget_show(alignment);
    gtk_container_add(GTK_CONTAINER(frame), alignment);
    gtk_misc_set_alignment(GTK_MISC(content), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(alignment), content);
#endif

    gtk_widget_show_all(frame);
    gtk_widget_show_all(frame_label_box);

    gtk_frame_set_label_widget(GTK_FRAME(frame), frame_label_box);

    /* pack the item on the detail_view screen */
    gtk_box_pack_start(GTK_BOX(shell->detail_view->view), frame, FALSE, FALSE, 4);

    g_free(temp);
}

static void detail_view_create_header(DetailView *detail_view,
                                        gchar *title)
{
    GtkWidget *header, *label;
    gchar *temp;

    temp = g_strdup_printf(_("<b>%s \342\206\222 Summary</b>"), title);

    header = gtk_menu_item_new_with_label(temp);
    gtk_menu_item_select(GTK_MENU_ITEM(header));
    gtk_widget_show(header);

    label = gtk_bin_get_child(GTK_BIN(header));
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

    gtk_box_pack_start(GTK_BOX(shell->detail_view->view), header, FALSE, FALSE, 4);

    g_free(temp);
}

static void shell_show_detail_view(void)
{
    GKeyFile *keyfile;
    gchar *detail;

    set_view_type(SHELL_VIEW_DETAIL, FALSE);
    detail_view_clear(shell->detail_view);
    detail_view_create_header(shell->detail_view, shell->selected_module->name);

    keyfile = g_key_file_new();
    detail = shell->selected_module->summaryfunc();

    if (g_key_file_load_from_data(keyfile, detail,
                                  strlen(detail), 0, NULL)) {
         gchar **groups;
         gint group;

         groups = g_key_file_get_groups(keyfile, NULL);

         for (group = 0; groups[group]; group++) {
             gchar *icon, *method, *method_result;

             shell_status_pulse();

             icon = g_key_file_get_string(keyfile, groups[group], "Icon", NULL);
             method = g_key_file_get_string(keyfile, groups[group], "Method", NULL);
             if (method) {
                  method_result = module_call_method(method);
             } else {
                  method_result = g_strdup("N/A");
             }

             detail_view_add_item(shell->detail_view,
                                    icon, groups[group], method_result);
             shell_status_pulse();

             g_free(icon);
             g_free(method);
             g_free(method_result);
         }

         g_strfreev(groups);
    } else {
         DEBUG("error while parsing detail_view");
         set_view_type(SHELL_VIEW_NORMAL, FALSE);
    }

    g_free(detail);
    g_key_file_free(keyfile);

    shell_view_set_enabled(TRUE);
}

static void module_selected(gpointer data)
{
    ShellTree *shelltree = shell->tree;
    GtkTreeModel *model = GTK_TREE_MODEL(shelltree->model);
    GtkTreeIter iter, parent;
    ShellModuleEntry *entry;
    static ShellModuleEntry *current = NULL;
    static gboolean updating = FALSE;

    /* Gets the currently selected item on the left-side TreeView; if there is
       no selection, silently return */
    if (!gtk_tree_selection_get_selected(shelltree->selection, &model, &iter)) {
        return;
    }

    /* Mark the currently selected module as "unselected"; this is used to kill
       the update timeout. */
    if (current) {
        current->selected = FALSE;
    }

    if (updating) {
        return;
    } else {
        updating = TRUE;
    }

    if (!gtk_tree_model_iter_parent(model, &parent, &iter)) {
        memcpy(&parent, &iter, sizeof(iter));
    }

    gtk_tree_model_get(model, &parent, TREE_COL_MODULE, &shell->selected_module, -1);

    /* Get the current selection and shows its related info */
    gtk_tree_model_get(model, &iter, TREE_COL_MODULE_ENTRY, &entry, -1);
    if (entry && !entry->selected) {
        gchar *title;

        shell_status_set_enabled(TRUE);
        shell_status_update(_("Updating..."));

        entry->selected = TRUE;
        shell->selected = entry;
        module_selected_show_info(entry, FALSE);

        gtk_tree_view_columns_autosize(GTK_TREE_VIEW(shell->info_tree->view));

	if(entry->flags & MODULE_FLAG_BENCHMARK) {
	    //allow to scroll to selected THIS-Machine
	} else {
            RANGE_SET_VALUE(info_tree, vscrollbar, 0.0);
	}
        RANGE_SET_VALUE(info_tree, hscrollbar, 0.0);
        RANGE_SET_VALUE(detail_view, vscrollbar, 0.0);
        RANGE_SET_VALUE(detail_view, hscrollbar, 0.0);

        title = g_strdup_printf("%s - %s", shell->selected_module->name, entry->name);
        shell_set_title(shell, title);
        g_free(title);

        shell_action_set_enabled("RefreshAction", TRUE);
        //shell_action_set_enabled("CopyAction", TRUE);

        shell_status_update(_("Done."));
        shell_status_set_enabled(FALSE);
    } else {
        shell_set_title(shell, NULL);
        shell_action_set_enabled("RefreshAction", FALSE);
        //shell_action_set_enabled("CopyAction", FALSE);

        gtk_tree_store_clear(GTK_TREE_STORE(shell->info_tree->model));
        set_view_type(SHELL_VIEW_NORMAL, FALSE);

        if (shell->selected_module->summaryfunc) {
	    shell_show_detail_view();
        }
    }
    current = entry;
    updating = FALSE;
}

static void info_selected(GtkTreeSelection * ts, gpointer data)
{
    ShellInfoTree *info = (ShellInfoTree *) data;
    GtkTreeModel *model = GTK_TREE_MODEL(info->model);
    GtkTreeIter parent;
    gchar *datacol, *mi_tag;

    if (!gtk_tree_selection_get_selected(ts, &model, &parent))
	return;

    if (shell->view_type == SHELL_VIEW_NORMAL ||
        shell->view_type == SHELL_VIEW_PROGRESS) {
        gtk_tree_selection_unselect_all(ts);
        return;
    }

    gtk_tree_model_get(model, &parent, INFO_TREE_COL_DATA, &datacol, -1);
    mi_tag = key_mi_tag(datacol);
    info_selected_show_extra(mi_tag);
    g_free(mi_tag);
}

static ShellInfoTree *info_tree_new(void)
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
				   GTK_POLICY_ALWAYS);

    store =
	gtk_tree_store_new(INFO_TREE_NCOL, G_TYPE_STRING, G_TYPE_STRING,
			   G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_FLOAT,
                           G_TYPE_STRING, G_TYPE_STRING);
    model = GTK_TREE_MODEL(store);
    treeview = gtk_tree_view_new_with_model(model);

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), TRUE);

    info->col_progress = column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_visible(column, FALSE);
    gtk_tree_view_column_set_min_width(column, 240);
    gtk_tree_view_column_set_clickable(column, TRUE);
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
    gtk_tree_view_column_set_clickable(column, TRUE);

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
    gtk_tree_view_column_set_clickable(column, TRUE);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       INFO_TREE_COL_EXTRA1);

    info->col_extra2 = column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_visible(column, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_set_clickable(column, TRUE);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       INFO_TREE_COL_EXTRA2);

    info->col_value = column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_set_clickable(column, TRUE);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       INFO_TREE_COL_VALUE);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    g_signal_connect(G_OBJECT(sel), "changed", (GCallback)info_selected, info);

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
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkCssProvider *provider;
    provider = gtk_css_provider_new();
#endif

    shelltree = g_new0(ShellTree, 1);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				   GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);


    store = gtk_tree_store_new(TREE_NCOL, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			       G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_BOOLEAN);
    model = GTK_TREE_MODEL(store);
    treeview = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

#if GTK_CHECK_VERSION(2,12,0)
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(treeview), 24);
#endif

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

#if GTK_CHECK_VERSION(3, 0, 0)
    if(params.theme>0){
        gtk_css_provider_load_from_data(provider, "treeview { background-color: rgba(0x60, 0x60, 0x60, 0.1); } treeview:selected { background-color: rgba(0x40, 0x60, 0xff, 1); } ", -1, NULL);
        gtk_style_context_add_provider(gtk_widget_get_style_context(treeview), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
#endif

    shelltree->scroll = scroll;
    shelltree->view = treeview;
    shelltree->model = model;
    shelltree->modules = NULL;
    shelltree->selection = sel;

    gtk_widget_show_all(scroll);

    return shelltree;
}

gboolean key_is_flagged(const gchar *key) {
    return (key && *key == '$' && strchr(key+1, '$')) ? TRUE : FALSE;
}

gboolean key_is_highlighted(const gchar *key) {
    gchar *flags;
    key_get_components(key, &flags, NULL, NULL, NULL, NULL, TRUE);
    if (flags && strchr(flags, '*')) {
        g_free(flags);
        return TRUE;
    }
    return FALSE;
}

gboolean key_wants_details(const gchar *key) {
    gchar *flags;
    key_get_components(key, &flags, NULL, NULL, NULL, NULL, TRUE);
    if (flags && strchr(flags, '!')) {
        g_free(flags);
        return TRUE;
    }
    return FALSE;
}

gboolean key_value_has_vendor_string(const gchar *key) {
    gchar *flags;
    key_get_components(key, &flags, NULL, NULL, NULL, NULL, TRUE);
    if (flags && strchr(flags, '^')) {
        g_free(flags);
        return TRUE;
    }
    return FALSE;
}

gboolean key_label_is_escaped(const gchar *key) {
    gchar *flags;
    key_get_components(key, &flags, NULL, NULL, NULL, NULL, TRUE);
    if (flags && strchr(flags, '@')) {
        g_free(flags);
        return TRUE;
    }
    return FALSE;
}

gchar *key_mi_tag(const gchar *key) {
    static char flag_list[] = "*!^@";
    gchar *p = (gchar*)key, *l, *t;

    if (key_is_flagged(key)) {
        l = strchr(key+1, '$');
        if (*p == '$') p++; /* skip first if exists */
        while(p < l && strchr(flag_list, *p)) {
            p++;
        }
        if (strlen(p)) {
            t = g_strdup(p);
            *(strchr(t, '$')) = 0;
            return t;
        }
    }
    return NULL;
}

const gchar *key_get_name(const gchar *key) {
    if (key_is_flagged(key))
        return strchr(key+1, '$')+1;
    return key;
}

/* key syntax:
 *  [$[<flags>][<tag>]$]<name>[#[<dis>]]
 *
 * example for key = "$*!Foo$Bar#7":
 * flags = "$*!^Foo$"  // key_is/wants_*() still works on flags
 * tag = "Foo"        // the moreinfo/icon tag
 * name = "Bar#7"     // the full unique name
 * label = "Bar"      // the label displayed
 * dis = "7"
 */
void key_get_components(const gchar *key,
    gchar **flags, gchar **tag, gchar **name, gchar **label, gchar **dis,
    gboolean null_empty) {

    if (null_empty) {
#define K_NULL_EMPTY(f) if (f) { *f = NULL; }
        K_NULL_EMPTY(flags);
        K_NULL_EMPTY(tag);
        K_NULL_EMPTY(name);
        K_NULL_EMPTY(label);
        K_NULL_EMPTY(dis);
    }

    if (!key || !*key)
        return;

    const gchar *np = g_utf8_strchr(key+1, -1, '$') + 1;
    if (*key == '$' && np) {
        /* is flagged */
        gchar *f = g_strdup(key);
        gchar *s = g_utf8_strchr(f+1, -1, '$');
        if(s==NULL) {
	    DEBUG("ERROR NOT FOUND");
        }else{
 /*            if((s-f+1)>strlen(key)) {
	    DEBUG("ERROR TOO LATE");
        }else{*/
	  *(g_utf8_strchr(f+1, -1, '$') + 1) = 0;
	  if (flags)
	    *flags = g_strdup(f);
	  if (tag)
	    *tag = key_mi_tag(f);
	  g_free(f);
	  //}
	}
    } else
        np = key;

    if (name)
        *name = g_strdup(np);
    if (label) {
        *label = g_strdup(np);
        gchar *lbp = g_utf8_strchr(*label, -1, '#');
        if (lbp)
            *lbp = 0;
        if (lbp && dis)
            *dis = g_strdup(lbp + 1);

        if (flags && *flags && strchr(*flags, '@')) {
            gchar *ol = *label;
            *label = g_strcompress(ol);
            g_free(ol);
        }
    }
}
