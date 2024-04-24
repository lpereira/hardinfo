/*
 * HardInfo
 * Copyright(C) 2003-2007 L. A. F. Pereira.
 *
 * menu.c is based on UI Manager tutorial by Ryan McDougall
 * Copyright(C) 2005 Ryan McDougall.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <menu.h>
#include <config.h>

#include <stock.h>

#include <callbacks.h>
#include <hardinfo.h>


#include "uidefs.h"

#ifndef GTK_STOCK_COPY
#define GTK_STOCK_COPY "_Copy"
#endif

#ifndef GTK_STOCK_REFRESH
#define GTK_STOCK_REFRESH "_Refresh"
#endif

static GtkActionEntry entries[] = {
    {"InformationMenuAction", NULL, N_("_Information")},	/* name, stock id, label */
    {"ViewMenuAction", NULL, N_("_View")},
    {"HelpMenuAction", NULL, N_("_Help")},
    {"MainMenuBarAction", NULL, ""},

    {"ReportAction", HI_STOCK_REPORT,	/* name, stock id */
     N_("Generate _Report"), "<control>R",	/* label, accelerator */
     N_("Generates a report with detailed system information"),			/* tooltip */
     G_CALLBACK(cb_generate_report)},

    {"SyncManagerAction", HI_STOCK_SYNC_MENU,
     N_("Synchronize"), NULL,
     N_("Send benchmark results and receive updated data from the network"),
     G_CALLBACK(cb_sync_manager)},

    {"OpenAction", "_Open",
     N_("_Open..."), NULL,
     NULL,
     G_CALLBACK(cb_sync_manager)},

    {"CopyAction", HI_STOCK_CLIPBOARD,
     N_("_Copy to Clipboard"), "<control>C",
     N_("Copy to clipboard"),
     G_CALLBACK(cb_copy_to_clipboard)},

    {"RefreshAction", HI_STOCK_REFRESH,
     N_("_Refresh"), "F5",
     NULL,
     G_CALLBACK(cb_refresh)},

    {"HomePageAction", HI_STOCK_INTERNET,
     N_("_Open HardInfo2 Web Site"), NULL,
     NULL,
     G_CALLBACK(cb_open_web_page)},

    {"ReportBugAction", HI_STOCK_INTERNET,
     N_("_Report bug"), NULL,
     NULL,
     G_CALLBACK(cb_report_bug)},

    {"AboutAction", "_About",
     N_("_About HardInfo2"), NULL,
     N_("Displays program version information"),
     G_CALLBACK(cb_about)},

    {"QuitAction", "_Quit",
     N_("_Quit"), "<control>Q",
     NULL,
     G_CALLBACK(cb_quit)}
};

static GtkToggleActionEntry toggle_entries[] = {
    {"SidePaneAction", NULL,
     N_("_Side Pane"), NULL,
     N_("Toggles side pane visibility"),
     G_CALLBACK(cb_side_pane)},
    {"ToolbarAction", NULL,
     N_("_Toolbar"), NULL,
     NULL,
     G_CALLBACK(cb_toolbar)},
    {"SyncOnStartupAction", NULL,
     N_("Synchronize on startup"), NULL,
     NULL,
     G_CALLBACK(cb_sync_on_startup)},
#if GTK_CHECK_VERSION(3, 0, 0)
    {"DisableThemeAction", NULL,
     N_("Disable Theme"), NULL,
     NULL,
     G_CALLBACK(cb_disable_theme)},
#endif
};

/* Implement a handler for GtkUIManager's "add_widget" signal. The UI manager
 * will emit this signal whenever it needs you to place a new widget it has. */
static void
menu_add_widget(GtkUIManager * ui, GtkWidget * widget,
		GtkContainer * container)
{
    gtk_box_pack_start(GTK_BOX(container), widget, FALSE, FALSE, 0);
    gtk_widget_show(widget);
}

void menu_init(Shell * shell)
{
    GtkWidget *menu_box;	/* Packing box for the menu and toolbars */
    GtkActionGroup *action_group;	/* Packing group for our Actions */
    GtkUIManager *menu_manager;	/* The magic widget! */
    GError *error;		/* For reporting exceptions or errors */
    GtkAccelGroup *accel_group;

    /* Create our objects */
    menu_box = shell->vbox;
    action_group = gtk_action_group_new("HardInfo2");
    menu_manager = gtk_ui_manager_new();

    shell->action_group = action_group;
    shell->ui_manager = menu_manager;

    /* Pack up our objects:
     * menu_box -> window
     * actions -> action_group
     * action_group -> menu_manager */
    gtk_action_group_set_translation_domain( action_group, "hardinfo2" );//gettext
    gtk_action_group_add_actions(action_group, entries,
				 G_N_ELEMENTS(entries), NULL);
    gtk_action_group_add_toggle_actions(action_group, toggle_entries,
					G_N_ELEMENTS(toggle_entries),
					NULL);
    gtk_ui_manager_insert_action_group(menu_manager, action_group, 0);


    /* Read in the UI from our XML file */
    error = NULL;
    gtk_ui_manager_add_ui_from_string(menu_manager, uidefs_str, -1,
				      &error);

    if (error) {
	g_error("Building menus failed: %s", error->message);
	g_error_free(error);
	return;
    }

    /* Enable menu accelerators */
    accel_group = gtk_ui_manager_get_accel_group(menu_manager);
    gtk_window_add_accel_group(GTK_WINDOW(shell->window), accel_group);

    /* Connect up important signals */
    /* This signal is necessary in order to place widgets from the UI manager
     * into the menu_box */
    g_signal_connect(menu_manager, "add_widget",
		     G_CALLBACK(menu_add_widget), menu_box);

    /* Show the window and run the main loop, we're done! */
    gtk_widget_show(menu_box);

    gtk_toolbar_set_style(GTK_TOOLBAR
			  (gtk_ui_manager_get_widget
			   (shell->ui_manager, "/MainMenuBarAction")),
			  GTK_TOOLBAR_BOTH_HORIZ);
}
