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
typedef struct _HostEditorDialog HostEditorDialog;

struct _RemoteDialog {
    GKeyFile *key_file;

    GtkWidget *dialog;
    GtkWidget *btn_connect, *btn_cancel;
    GtkWidget *btn_add, *btn_edit, *btn_remove;

    GtkListStore *tree_store;

    gint selected_id;
    gchar *selected_name;
    GtkTreeIter *selected_iter;
};

struct _HostEditorDialog {
    GtkWidget *dialog;
    GtkWidget *notebook;
    
    GtkWidget *txt_hostname, *txt_port;
    GtkWidget *txt_ssh_user, *txt_ssh_password;
    
    GtkWidget *cmb_type;
};

static RemoteDialog *remote_dialog_new(GtkWidget * parent);
static void remote_dialog_destroy(RemoteDialog * rd);

static gboolean remote_version_is_supported(RemoteDialog * rd)
{
    gint remote_ver;
    GtkWidget *dialog;

    shell_status_update("Obtaining remote server API version...");
    remote_ver =
	xmlrpc_get_integer("http://localhost:4242/xmlrpc",
			   "server.getAPIVersion", NULL);

    switch (remote_ver) {
    case -1:
	dialog = gtk_message_dialog_new(GTK_WINDOW(rd->dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Remote host didn't respond.");

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	break;
    case XMLRPC_SERVER_VERSION:
	return TRUE;
    default:
	dialog = gtk_message_dialog_new(GTK_WINDOW(rd->dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Remote Host has an unsupported "
					"API version (%d). Expected "
					"version is %d.",
					remote_ver, XMLRPC_SERVER_VERSION);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
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

static gboolean load_module_list(RemoteDialog * rd)
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

	m = g_new0(ShellModule, 1);
	m->name = g_strdup(g_value_get_string(&module->values[0]));
	m->icon =
	    icon_cache_get_pixbuf(g_value_get_string(&module->values[1]));
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
    if (remote_version_is_supported(rd)) {
	if (!load_module_list(rd)) {
	    GtkWidget *dialog;

	    dialog = gtk_message_dialog_new(GTK_WINDOW(rd->dialog),
					    GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_CLOSE,
					    "Cannot load remote module list.");

	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
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

    remote_dialog_destroy(rd);
}

static void populate_store(RemoteDialog * rd, GtkListStore * store)
{
    GtkTreeIter iter;
    gchar *path;
    gchar **hosts;
    gint i, no_groups;

    gtk_list_store_clear(store);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
		       0, icon_cache_get_pixbuf("home.png"),
		       1, g_strdup("Local Computer"),
		       2, GINT_TO_POINTER(-1), -1);


    hosts = g_key_file_get_groups(rd->key_file, &no_groups);
    DEBUG("%d hosts found", no_groups);
    for (i = 0; i < no_groups; i++) {
	gchar *icon;

	DEBUG("host #%d: %s", i, hosts[i]);

	icon = g_key_file_get_string(rd->key_file, hosts[i], "icon", NULL);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			   0,
			   icon_cache_get_pixbuf(icon ? icon :
						 "server.png"), 1,
			   g_strdup(hosts[i]), 2, GINT_TO_POINTER(i), -1);

	g_free(icon);
    }

    g_strfreev(hosts);
}

static void host_editor_combo_changed_cb(GtkComboBox * widget,
					 gpointer user_data)
{
    HostEditorDialog *host_dlg = (HostEditorDialog *) user_data;
    gint index;

    index = gtk_combo_box_get_active(widget);
    
    gtk_notebook_set_current_page(GTK_NOTEBOOK(host_dlg->notebook), index);
}

static HostEditorDialog *host_editor_dialog_new(GtkWidget * parent,
						gchar * title)
{
    HostEditorDialog *host_dlg;

    GtkWidget *dialog;
    GtkWidget *dialog_vbox1;
    GtkWidget *vbox1;
    GtkWidget *table1;
    GtkWidget *label4;
    GtkWidget *label5;
    GtkWidget *txt_hostname;
    GtkWidget *alignment2;
    GtkObject *spn_port_adj;
    GtkWidget *spn_port;
    GtkWidget *frame1;
    GtkWidget *alignment3;
    GtkWidget *notebook1;
    GtkWidget *vbox2;
    GtkWidget *hbox2;
    GtkWidget *image1;
    GtkWidget *label6;
    GtkWidget *label1;
    GtkWidget *vbox3;
    GtkWidget *table2;
    GtkWidget *txt_ssh_user;
    GtkWidget *txt_ssh_password;
    GtkWidget *label10;
    GtkWidget *label11;
    GtkWidget *hbox3;
    GtkWidget *image2;
    GtkWidget *label12;
    GtkWidget *label2;
    GtkWidget *hbox1;
    GtkWidget *label3;
    GtkWidget *cmb_type;
    GtkWidget *dialog_action_area1;
    GtkWidget *btn_cancel;
    GtkWidget *btn_save;

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog),
			    GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_gravity(GTK_WINDOW(dialog), GDK_GRAVITY_CENTER);

    dialog_vbox1 = GTK_DIALOG(dialog)->vbox;
    gtk_widget_show(dialog_vbox1);

    vbox1 = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox1);
    gtk_box_pack_start(GTK_BOX(dialog_vbox1), vbox1, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);

    table1 = gtk_table_new(2, 2, FALSE);
    gtk_widget_show(table1);
    gtk_box_pack_start(GTK_BOX(vbox1), table1, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table1), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table1), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 4);

    label4 = gtk_label_new("Host name:");
    gtk_widget_show(label4);
    gtk_table_attach(GTK_TABLE(table1), label4, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label4), 0, 0.5);

    label5 = gtk_label_new("Port:");
    gtk_widget_show(label5);
    gtk_table_attach(GTK_TABLE(table1), label5, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label5), 0, 0.5);

    txt_hostname = gtk_entry_new();
    gtk_widget_show(txt_hostname);
    gtk_table_attach(GTK_TABLE(table1), txt_hostname, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_invisible_char(GTK_ENTRY(txt_hostname), 9679);

    alignment2 = gtk_alignment_new(0, 0.5, 0.03, 1);
    gtk_widget_show(alignment2);
    gtk_table_attach(GTK_TABLE(table1), alignment2, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);

    spn_port_adj = gtk_adjustment_new(4242, 1025, 65535, 1, 10, 0);
    spn_port = gtk_spin_button_new(GTK_ADJUSTMENT(spn_port_adj), 1, 0);
    gtk_widget_show(spn_port);
    gtk_container_add(GTK_CONTAINER(alignment2), spn_port);

    frame1 = gtk_frame_new(NULL);
    gtk_widget_show(frame1);
    gtk_box_pack_start(GTK_BOX(vbox1), frame1, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame1), GTK_SHADOW_NONE);

    alignment3 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment3);
    gtk_container_add(GTK_CONTAINER(frame1), alignment3);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment3), 0, 0, 12, 0);

    notebook1 = gtk_notebook_new();
    gtk_widget_show(notebook1);
    gtk_container_add(GTK_CONTAINER(alignment3), notebook1);
    GTK_WIDGET_UNSET_FLAGS(notebook1, GTK_CAN_FOCUS);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook1), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook1), FALSE);

    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox2);
    gtk_container_add(GTK_CONTAINER(notebook1), vbox2);

    hbox2 = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox2);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox2, TRUE, FALSE, 0);

    image1 =
	gtk_image_new_from_stock("gtk-dialog-info",
				 GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(image1);
    gtk_box_pack_start(GTK_BOX(hbox2), image1, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(image1), 0.5, 0);

    label6 =
	gtk_label_new
	("<small><i>This method provides no authentication and no encryption.</i></small>");
    gtk_widget_show(label6);
    gtk_box_pack_start(GTK_BOX(hbox2), label6, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label6), TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(label6), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label6), 0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(label6), 36);

    vbox3 = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox3);
    gtk_container_add(GTK_CONTAINER(notebook1), vbox3);
    gtk_container_set_border_width(GTK_CONTAINER(vbox3), 5);

    table2 = gtk_table_new(2, 2, FALSE);
    gtk_widget_show(table2);
    gtk_box_pack_start(GTK_BOX(vbox3), table2, TRUE, TRUE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table2), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table2), 4);

    txt_ssh_user = gtk_entry_new();
    gtk_widget_show(txt_ssh_user);
    gtk_table_attach(GTK_TABLE(table2), txt_ssh_user, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_invisible_char(GTK_ENTRY(txt_ssh_user), 9679);

    txt_ssh_password = gtk_entry_new();
    gtk_widget_show(txt_ssh_password);
    gtk_table_attach(GTK_TABLE(table2), txt_ssh_password, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_visibility(GTK_ENTRY(txt_ssh_password), FALSE);
    gtk_entry_set_invisible_char(GTK_ENTRY(txt_ssh_password), 9679);

    label10 = gtk_label_new("User:");
    gtk_widget_show(label10);
    gtk_table_attach(GTK_TABLE(table2), label10, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label10), 0, 0.5);

    label11 = gtk_label_new("Password:");
    gtk_widget_show(label11);
    gtk_table_attach(GTK_TABLE(table2), label11, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label11), 0, 0.5);

    hbox3 = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox3);
    gtk_box_pack_end(GTK_BOX(vbox3), hbox3, TRUE, TRUE, 0);

    image2 =
	gtk_image_new_from_stock("gtk-dialog-warning",
				 GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(image2);
    gtk_box_pack_start(GTK_BOX(hbox3), image2, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(image2), 0.5, 0);

    label12 =
	gtk_label_new
	("<small><i>The password will be saved in clear text on <b>~/.hardinfo/remote.conf</b>. Passwordless RSA keys can be used instead.</i></small>");
    gtk_widget_show(label12);
    gtk_box_pack_start(GTK_BOX(hbox3), label12, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label12), TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(label12), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label12), 0, 0.5);
    gtk_label_set_width_chars(GTK_LABEL(label12), 36);

    hbox1 = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox1);
    gtk_frame_set_label_widget(GTK_FRAME(frame1), hbox1);

    label3 = gtk_label_new("<b>Type:</b>");
    gtk_widget_show(label3);
    gtk_box_pack_start(GTK_BOX(hbox1), label3, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label3), TRUE);

    cmb_type = gtk_combo_box_new_text();
    gtk_widget_show(cmb_type);
    gtk_box_pack_start(GTK_BOX(hbox1), cmb_type, FALSE, TRUE, 0);
    gtk_combo_box_append_text(GTK_COMBO_BOX(cmb_type),
			      "Direct connection");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cmb_type), "SSH tunnel");

    dialog_action_area1 = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog_action_area1);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1),
			      GTK_BUTTONBOX_END);

    btn_cancel = gtk_button_new_from_stock("gtk-cancel");
    gtk_widget_show(btn_cancel);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_cancel,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(btn_cancel, GTK_CAN_DEFAULT);

    btn_save = gtk_button_new_from_stock("gtk-save");
    gtk_widget_show(btn_save);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_save,
				 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(btn_save, GTK_CAN_DEFAULT);

    host_dlg = g_new0(HostEditorDialog, 1);
    host_dlg->dialog = dialog;
    host_dlg->notebook = notebook1;
    host_dlg->txt_hostname = txt_hostname;
    host_dlg->txt_port = spn_port;
    host_dlg->txt_ssh_user = txt_ssh_user;
    host_dlg->txt_ssh_password = txt_ssh_password;
    host_dlg->cmb_type = cmb_type;

    gtk_combo_box_set_active(GTK_COMBO_BOX(host_dlg->cmb_type), 0);
    g_signal_connect(G_OBJECT(cmb_type), "changed",
		     G_CALLBACK(host_editor_combo_changed_cb), host_dlg);

    return host_dlg;
}

static void remote_dialog_add(GtkWidget * button, gpointer data)
{
    RemoteDialog *rd = (RemoteDialog *) data;
    HostEditorDialog *he =
	host_editor_dialog_new(rd->dialog, "Add a host");

    if (gtk_dialog_run(GTK_DIALOG(he->dialog)) == GTK_RESPONSE_ACCEPT) {
	DEBUG("saving");
    }

    gtk_widget_destroy(he->dialog);
    g_free(he);
}

static void remote_dialog_edit(GtkWidget * button, gpointer data)
{
    GtkWidget *dialog;
    RemoteDialog *rd = (RemoteDialog *) data;
    HostEditorDialog *he = host_editor_dialog_new(rd->dialog, "Edit a host");
    gchar *host_type;
    gint host_port;
    
    host_type = g_key_file_get_string(rd->key_file, rd->selected_name, "type", NULL);
    if (!host_type || g_str_equal(host_type, "direct")) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(he->cmb_type), 0);
    } else if (g_str_equal(host_type, "ssh")) {
	gchar *username, *password;

        gtk_combo_box_set_active(GTK_COMBO_BOX(he->cmb_type), 1);

	username = g_key_file_get_string(rd->key_file, rd->selected_name, "username", NULL);
	if (username) {
	    gtk_entry_set_text(GTK_ENTRY(he->txt_ssh_user), username);
	    g_free(username);
	}

	password = g_key_file_get_string(rd->key_file, rd->selected_name, "password", NULL);
	if (password) {
	    gtk_entry_set_text(GTK_ENTRY(he->txt_ssh_password), password);
	    g_free(password);
	}
    } else {
	dialog = gtk_message_dialog_new(GTK_WINDOW(rd->dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Host has invalid type (%s).",
					host_type);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	goto bad;
    }

    gtk_entry_set_text(GTK_ENTRY(he->txt_hostname), rd->selected_name);
    
    host_port = g_key_file_get_integer(rd->key_file, rd->selected_name, "port", NULL);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(he->txt_port), host_port ? host_port : 4242);

    if (gtk_dialog_run(GTK_DIALOG(he->dialog)) == GTK_RESPONSE_ACCEPT) {
	DEBUG("saving");
    }

  bad:
    gtk_widget_destroy(he->dialog);
    g_free(he);
    g_free(host_type);
}

static void remote_dialog_remove(GtkWidget * button, gpointer data)
{
    RemoteDialog *rd = (RemoteDialog *) data;
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(rd->dialog),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_NONE,
						"Remove the host <b>%s</b>?",
						rd->selected_name);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			   GTK_STOCK_NO, GTK_RESPONSE_REJECT,
			   GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	g_key_file_remove_group(rd->key_file, rd->selected_name, NULL);
	gtk_list_store_remove(rd->tree_store, rd->selected_iter);
    }

    gtk_widget_destroy(dialog);
}

static void remote_dialog_tree_sel_changed(GtkTreeSelection * sel,
					   gpointer data)
{
    RemoteDialog *rd = (RemoteDialog *) data;
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
	gchar *name;
	gint id;

	gtk_tree_model_get(model, &iter, 1, &name, 2, &id, -1);

	if (id != -1) {
	    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_edit), TRUE);
	    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_remove), TRUE);
	} else {
	    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_edit), FALSE);
	    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_remove), FALSE);
	}

	g_free(rd->selected_name);

	if (rd->selected_iter)
	    gtk_tree_iter_free(rd->selected_iter);

	rd->selected_id = id;
	rd->selected_name = name;
	rd->selected_iter = gtk_tree_iter_copy(&iter);
    }
}

static void remote_dialog_destroy(RemoteDialog * rd)
{
    gchar *path, *remote_conf;
    gsize length;

    path =
	g_build_filename(g_get_home_dir(), ".hardinfo", "remote.conf",
			 NULL);

    remote_conf = g_key_file_to_data(rd->key_file, &length, NULL);
    g_file_set_contents(path, remote_conf, length, NULL);

    gtk_widget_destroy(rd->dialog);

    g_free(remote_conf);
    g_free(path);
    g_free(rd);
}

static RemoteDialog *remote_dialog_new(GtkWidget * parent)
{
    RemoteDialog *rd;
    gchar *path;
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
    GtkWidget *button2;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkTreeSelection *sel;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cr_text, *cr_pbuf;
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

    button2 = gtk_button_new_with_mnemonic("_Remove");
    gtk_widget_show(button2);
    gtk_container_add(GTK_CONTAINER(vbuttonbox3), button2);
    GTK_WIDGET_SET_FLAGS(button2, GTK_CAN_DEFAULT);
    g_signal_connect(button2, "clicked", G_CALLBACK(remote_dialog_remove),
		     rd);

    dialog1_action_area = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog1_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog1_action_area),
			      GTK_BUTTONBOX_END);

    button8 = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_widget_show(button8);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button8,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);

    button7 = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
    gtk_widget_show(button7);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button7,
				 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(treeview2));

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview2));

    g_signal_connect(G_OBJECT(sel), "changed",
		     (GCallback) remote_dialog_tree_sel_changed, rd);

    rd->dialog = dialog;
    rd->btn_cancel = button8;
    rd->btn_connect = button7;
    rd->btn_add = button3;
    rd->btn_edit = button6;
    rd->btn_remove = button2;
    rd->tree_store = store;

    rd->key_file = g_key_file_new();
    path =
	g_build_filename(g_get_home_dir(), ".hardinfo", "remote.conf",
			 NULL);
    g_key_file_load_from_file(rd->key_file, path, 0, NULL);
    g_free(path);

    populate_store(rd, store);
    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_edit), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_remove), FALSE);

    return rd;
}
