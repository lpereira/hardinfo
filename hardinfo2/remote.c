/*
 *    Remote Client
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include <stdio.h>
#include <string.h>
#include <shell.h>
#include <iconcache.h>
#include <hardinfo.h>
#include <config.h>

#include "xmlrpc-client.h"

#define XMLRPC_SERVER_VERSION		1

/*
 * TODO
 *
 * - Completely disable this feature if libsoup support isn't compiled in
 * - Add "Local Computer"
 * - Detect machines on local network that runs SSH
 *   - IP range scan
 *   - mDNS
 * - Allow the user to add/remove/edit a machine
 *   - Use ~/.ssh/known_hosts as a starting point?
 *   - Different icons for different machines?
 *   - Make sure SSH can do port forwarding
 *   - Make sure the remote host has HardInfo installed, with the correct
 *     version.
 * - Allow the user to choose/guess the SSH authentication method
 *   - Plain old username / password
 *   - Passwordless RSA keys
 *   - RSA Keys with passwords
 * - Make sure SSH can do port forwarding
 * - Make sure the remote host has HardInfo installed
 * - Implement XML-RPC server (use libsoup; there's a example with libsoup's sources)
 *   - Create abstractions for common module operations
 *   - Use function pointers to choose between local and remote modes
 *   - Generate a random username/password to be passed to the XML-RPC server; use
 *     that username/password on the client; this is just to make sure nobody on the
 *     machine will be allowed to obtain information that might be sensitive. This
 *     will be passed with base64, so it can be sniffed; this needs root access anyway,
 *     so not really a problem.
 * - Determine if we're gonna use GIOChannels or create a communication thread
 * - Use libsoup XML-RPC support to implement the remote mode
 * - Introduce a flag on the modules, stating their ability to be used locally/remotely
 *   (Benchmarks can't be used remotely; Displays won't work remotely [unless we use
 *    X forwarding, but that'll be local X11 info anyway]).
 */
typedef struct _RemoteDialog RemoteDialog;
struct _RemoteDialog {
    GtkWidget *dialog;
    GtkWidget *btn_connect, *btn_cancel;
};

static RemoteDialog *remote_dialog_new(GtkWidget * parent);

static gboolean remote_version_is_supported(void)
{
    gint remote_ver;

    shell_status_update("Obtaining remote server API version...");
    remote_ver =
	xmlrpc_get_integer("http://localhost:4242/xmlrpc",
			   "server.getAPIVersion", NULL);

    switch (remote_ver) {
    case -1:
	g_warning("Remote Host didn't respond.");
	break;
    case XMLRPC_SERVER_VERSION:
	return TRUE;
    default:
	g_warning
	    ("Remote Host has an unsupported API version (%d). Expected version is %d.",
	     remote_ver, XMLRPC_SERVER_VERSION);
    }

    return FALSE;
}

static gchar *remote_module_entry_func()
{
    Shell *shell = shell_get_main_shell();
    gchar *ret;

    ret =
	xmlrpc_get_string("http://localhost:4242/xmlrpc",
			  "module.entryFunction", "%s%i",
			  shell->selected_module_name,
			  shell->selected->number);

    if (!ret) {
	ret = g_strdup("");
    }

    return ret;
}

static void remote_module_entry_scan_func(gboolean reload)
{
    Shell *shell = shell_get_main_shell();

    if (reload) {
	xmlrpc_get_string("http://localhost:4242/xmlrpc",
			  "module.entryReload", "%s%i",
			  shell->selected_module_name,
			  shell->selected->number);
    } else {
	xmlrpc_get_string("http://localhost:4242/xmlrpc",
			  "module.entryScan", "%s%i",
			  shell->selected_module_name,
			  shell->selected->number);
    }
}

static gchar *remote_module_entry_field_func(gchar * entry)
{
    Shell *shell = shell_get_main_shell();
    gchar *ret;

    ret =
	xmlrpc_get_string("http://localhost:4242/xmlrpc",
			  "module.entryGetField", "%s%i%s",
			  shell->selected_module_name,
			  shell->selected->number, entry);

    return ret;
}

static gchar *remote_module_entry_more_func(gchar * entry)
{
    Shell *shell = shell_get_main_shell();
    gchar *ret;

    ret =
	xmlrpc_get_string("http://localhost:4242/xmlrpc",
			  "module.entryGetMoreInfo", "%s%i%s",
			  shell->selected_module_name,
			  shell->selected->number, entry);

    return ret;
}

static gchar *remote_module_entry_note_func(gint entry)
{
    Shell *shell = shell_get_main_shell();
    gchar *note;

    note =
	xmlrpc_get_string("http://localhost:4242/xmlrpc",
			  "module.entryGetNote", "%s%i",
			  shell->selected_module_name,
			  shell->selected->number);
    if (note && *note == '\0') {
	g_free(note);
	return NULL;
    }

    return note;
}

static ModuleAbout *remote_module_get_about()
{
    return NULL;
}

static gboolean load_module_list()
{
    Shell *shell;
    GValueArray *modules;
    int i = 0;

    shell_status_update("Unloading local modules...");
    module_unload_all();

    shell_status_update("Obtaining remote server module list...");
    modules =
	xmlrpc_get_array("http://localhost:4242/xmlrpc",
			 "module.getModuleList", NULL);
    if (!modules) {
	return FALSE;
    }

    shell = shell_get_main_shell();

    for (; i < modules->n_values; i++) {
	ShellModule *m;
	ShellModuleEntry *e;
	GValueArray *entries, *module;
	int j = 0;
	
	module = g_value_get_boxed(&modules->values[i]);
	DEBUG("%s - %s",
	      g_value_get_string(&module->values[0]),
	      g_value_get_string(&module->values[1]));
	      
	m = g_new0(ShellModule, 1);
	m->name = g_strdup(g_value_get_string(&module->values[0]));
        m->icon = icon_cache_get_pixbuf(g_value_get_string(&module->values[1]));
	m->aboutfunc = (gpointer) remote_module_get_about;

	shell_status_pulse();
	entries =
	    xmlrpc_get_array("http://localhost:4242/xmlrpc",
			     "module.getEntryList", "%s", m->name);
	if (entries) {
	    for (; j < entries->n_values; j++) {
		GValueArray *tuple =
		    g_value_get_boxed(&entries->values[j]);

		e = g_new0(ShellModuleEntry, 1);
		e->name = g_strdup(g_value_get_string(&tuple->values[0]));
		e->icon =
		    icon_cache_get_pixbuf(g_value_get_string
					  (&tuple->values[1]));
		e->icon_file =
		    g_strdup(g_value_get_string(&tuple->values[1]));
		e->number = j;

		e->func = remote_module_entry_func;
		e->scan_func = remote_module_entry_scan_func;
		e->fieldfunc = remote_module_entry_field_func;
		e->morefunc = remote_module_entry_more_func;
		e->notefunc = remote_module_entry_note_func;

		m->entries = g_slist_append(m->entries, e);

		shell_status_pulse();
	    }

	    g_value_array_free(entries);
	}

	shell->tree->modules = g_slist_append(shell->tree->modules, m);
    }

    g_slist_foreach(shell->tree->modules, shell_add_modules_to_gui,
		    shell->tree);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->tree->view));

    g_value_array_free(modules);

    return TRUE;
}

static void remote_connect(RemoteDialog * rd)
{
    xmlrpc_init();

    /* check API version */
    if (remote_version_is_supported()) {
	if (!load_module_list()) {
	    g_warning("Remote module list couldn't be loaded.");
	}
    }

    shell_status_update("Done.");
}

void remote_dialog_show(GtkWidget * parent)
{
    gboolean success;
    RemoteDialog *rd = remote_dialog_new(parent);

    if (gtk_dialog_run(GTK_DIALOG(rd->dialog)) == GTK_RESPONSE_ACCEPT) {
	gtk_widget_hide(rd->dialog);
	shell_view_set_enabled(FALSE);
	shell_status_set_enabled(TRUE);

	remote_connect(rd);

	shell_status_set_enabled(FALSE);
	shell_view_set_enabled(TRUE);
    }

    gtk_widget_destroy(rd->dialog);
    g_free(rd);
}

static void populate_store(GtkListStore * store)
{
    GKeyFile *remote;
    GtkTreeIter iter;
    gchar *path;

    gtk_list_store_clear(store);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
		       0, icon_cache_get_pixbuf("home.png"),
		       1, g_strdup("Local Computer"),
		       2, GINT_TO_POINTER(-1), -1);

    remote = g_key_file_new();
    path =
	g_build_filename(g_get_home_dir(), ".hardinfo", "remote.conf",
			 NULL);
    if (g_key_file_load_from_file(remote, path, 0, NULL)) {
	gint no_hosts, i;

	no_hosts =
	    g_key_file_get_integer(remote, "global", "no_hosts", NULL);
	for (i = 0; i < no_hosts; i++) {
	    gchar *hostname;
	    gchar *hostgroup;
	    gchar *icon;

	    hostgroup = g_strdup_printf("host%d", i);

	    hostname =
		g_key_file_get_string(remote, hostgroup, "name", NULL);
	    icon = g_key_file_get_string(remote, hostgroup, "icon", NULL);

	    gtk_list_store_append(store, &iter);
	    gtk_list_store_set(store, &iter,
			       0,
			       icon_cache_get_pixbuf(icon ? icon :
						     "server.png"), 1,
			       hostname, 2, GINT_TO_POINTER(i), -1);

	    g_free(hostgroup);
	    g_free(icon);
	}
    }

    g_free(path);
    g_key_file_free(remote);
}

static GtkWidget *host_editor_dialog_new(GtkWidget * parent, gchar * title)
{
    GtkWidget *dialog, *dialog1_action_area, *button7, *button8;

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog),
			    GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);

    dialog1_action_area = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog1_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog1_action_area),
			      GTK_BUTTONBOX_END);

    button8 = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_widget_show(button8);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button8,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);

    button7 = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    gtk_widget_show(button7);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button7,
				 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);

    return dialog;
}

static void remote_dialog_add(GtkWidget * button, gpointer data)
{
    RemoteDialog *rd = (RemoteDialog *) data;
    GtkWidget *host_editor =
	host_editor_dialog_new(rd->dialog, "Add a host");

    if (gtk_dialog_run(GTK_DIALOG(host_editor)) == GTK_RESPONSE_ACCEPT) {
	DEBUG("saving");
    }

    gtk_widget_destroy(host_editor);
}

static void remote_dialog_edit(GtkWidget * button, gpointer data)
{
    RemoteDialog *rd = (RemoteDialog *) data;
    GtkWidget *host_editor =
	host_editor_dialog_new(rd->dialog, "Edit a host");

    if (gtk_dialog_run(GTK_DIALOG(host_editor)) == GTK_RESPONSE_ACCEPT) {
	DEBUG("saving");
    }

    gtk_widget_destroy(host_editor);
}

static void remote_dialog_remove(GtkWidget * button, gpointer data)
{
    RemoteDialog *rd = (RemoteDialog *) data;
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(GTK_WINDOW(rd->dialog),
				    GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_NONE,
				    "Remove the host <b>%s</b>?",
				    "selected.host.name");

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			   GTK_STOCK_NO, GTK_RESPONSE_REJECT,
			   GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	DEBUG("removing");
    }

    gtk_widget_destroy(dialog);
}

static RemoteDialog *remote_dialog_new(GtkWidget * parent)
{
    RemoteDialog *rd;
    GtkWidget *dialog;
    GtkWidget *dialog1_vbox;
    GtkWidget *scrolledwindow2;
    GtkWidget *treeview2;
    GtkWidget *vbuttonbox3;
    GtkWidget *button3;
    GtkWidget *button6;
    GtkWidget *dialog1_action_area;
    GtkWidget *button8;
    GtkWidget *button7;
    GtkWidget *label;
    GtkWidget *hbox;

    GtkTreeViewColumn *column;
    GtkCellRenderer *cr_text, *cr_pbuf, *cr_toggle;
    GtkListStore *store;
    GtkTreeModel *model;

    rd = g_new0(RemoteDialog, 1);

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Connect to Remote Computer");
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog),
			    GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);

    dialog1_vbox = GTK_DIALOG(dialog)->vbox;
    gtk_box_set_spacing(GTK_BOX(dialog1_vbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(dialog1_vbox), 4);
    gtk_widget_show(dialog1_vbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("<big><b>Connect To</b></big>\n"
			  "Please choose the remote computer to connect to:");
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

    gtk_box_pack_start(GTK_BOX(hbox),
		       icon_cache_get_image("server-large.png"),
		       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show_all(hbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow2);
    gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow2, TRUE, TRUE, 0);
    gtk_widget_set_size_request(scrolledwindow2, -1, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scrolledwindow2), GTK_SHADOW_IN);

    store =
	gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    model = GTK_TREE_MODEL(store);

    treeview2 = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview2), FALSE);
    gtk_widget_show(treeview2);
    gtk_container_add(GTK_CONTAINER(scrolledwindow2), treeview2);

    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), column);

    cr_pbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, cr_pbuf, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_pbuf, "pixbuf",
				       TREE_COL_PBUF);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, TRUE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       TREE_COL_NAME);

    populate_store(store);

    vbuttonbox3 = gtk_vbutton_box_new();
    gtk_widget_show(vbuttonbox3);
    gtk_box_pack_start(GTK_BOX(hbox), vbuttonbox3, FALSE, TRUE, 0);
    gtk_box_set_spacing(GTK_BOX(vbuttonbox3), 5);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox3),
			      GTK_BUTTONBOX_START);

    button3 = gtk_button_new_with_mnemonic("_Add");
    gtk_widget_show(button3);
    gtk_container_add(GTK_CONTAINER(vbuttonbox3), button3);
    GTK_WIDGET_SET_FLAGS(button3, GTK_CAN_DEFAULT);
    g_signal_connect(button3, "clicked",
		     G_CALLBACK(remote_dialog_add), rd);

    button6 = gtk_button_new_with_mnemonic("_Edit");
    gtk_widget_show(button6);
    gtk_container_add(GTK_CONTAINER(vbuttonbox3), button6);
    GTK_WIDGET_SET_FLAGS(button6, GTK_CAN_DEFAULT);
    g_signal_connect(button6, "clicked", G_CALLBACK(remote_dialog_edit),
		     rd);

    button6 = gtk_button_new_with_mnemonic("_Remove");
    gtk_widget_show(button6);
    gtk_container_add(GTK_CONTAINER(vbuttonbox3), button6);
    GTK_WIDGET_SET_FLAGS(button6, GTK_CAN_DEFAULT);
    g_signal_connect(button6, "clicked", G_CALLBACK(remote_dialog_remove),
		     rd);

    dialog1_action_area = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog1_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog1_action_area),
			      GTK_BUTTONBOX_END);

    button8 = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_widget_show(button8);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button8,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);

    button7 = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
    gtk_widget_show(button7);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button7,
				 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);

    rd->dialog = dialog;
    rd->btn_cancel = button8;
    rd->btn_connect = button7;

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(treeview2));

    return rd;
}

