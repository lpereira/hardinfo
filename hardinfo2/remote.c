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

#include "config.h"

#ifdef HAS_LIBSOUP

#include "shell.h"
#include "callbacks.h"
#include "iconcache.h"
#include "hardinfo.h"
#include "xmlrpc-client.h"
#include "ssh-conn.h"

#define XMLRPC_SERVER_VERSION		1

/*
 * TODO
 *
 * - Add hi_deinit() to modules (so they can free up their memory)
 * - Import / Export host list
 * - Detect machines on local network that runs SSH
 *   - IP range scan
 *   - mDNS
 * - Allow the user to add/remove/edit a machine
 *   - Use ~/.ssh/known_hosts as a starting point?
 *   - Different icons for different machines?
 * - Make sure SSH can do port forwarding
 * - Make sure the remote host has HardInfo installed
 * - Generate a random username/password to be passed to the XML-RPC server; use
 *   that username/password on the client; this is just to make sure nobody on the
 *   machine will be allowed to obtain information that might be sensitive. This
 *   will be passed with base64, so it can be sniffed; this needs root access anyway,
 *   so not really a problem.
 * - Determine if we're gonna use GIOChannels or create a communication thread
 * - Introduce a flag on the modules, stating their ability to be used locally/remotely
 *   (Benchmarks can't be used remotely; Displays won't work remotely [unless we use
 *    X forwarding, but that'll be local X11 info anyway]).
 */

typedef struct _HostManager HostManager;
typedef struct _HostDialog HostDialog;

typedef enum {
    HOST_DIALOG_MODE_EDIT,
    HOST_DIALOG_MODE_CONNECT
} HostDialogMode;

struct _HostManager {
    GtkWidget *dialog;
    GtkWidget *btn_connect, *btn_cancel;
    GtkWidget *btn_add, *btn_edit, *btn_remove;

    GtkListStore *tree_store;

    gint selected_id;
    gchar *selected_name;
    GtkTreeIter *selected_iter;
};

struct _HostDialog {
    GtkWidget *dialog;
    GtkWidget *notebook;

    GtkWidget *txt_hostname, *txt_port;
    GtkWidget *txt_ssh_user, *txt_ssh_password;

    GtkWidget *cmb_type;
    
    GtkWidget *frm_options;
    GtkWidget *btn_accept, *btn_cancel;
};

static HostManager *host_manager_new(GtkWidget * parent);
static void host_manager_destroy(HostManager * rd);
static HostDialog *host_dialog_new(GtkWidget * parent,
				   gchar * title, HostDialogMode mode);
static void host_dialog_destroy(HostDialog * hd);


static gchar *xmlrpc_server_uri = NULL;
static SSHConn *ssh_conn = NULL;

void remote_disconnect_all(gboolean ssh)
{
    if (ssh && ssh_conn) {
	ssh_close(ssh_conn);
	ssh_conn = NULL;
    }

    if (xmlrpc_server_uri) {
	g_free(xmlrpc_server_uri);
	xmlrpc_server_uri = NULL;
    }
}

static void remote_connection_error(void)
{
    GtkWidget *dialog;
    static gboolean showing_error = FALSE;
    
    if (showing_error || !xmlrpc_server_uri) {
       return;
    }
    
    showing_error = TRUE;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_NONE,
                                    "Connection to %s was lost.", xmlrpc_server_uri);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
       remote_disconnect_all(ssh_conn != NULL);
       cb_local_computer();
    }

    gtk_widget_destroy(dialog);

    showing_error = FALSE;
}

static gboolean remote_version_is_supported(void)
{
    gint remote_ver;
    GtkWidget *dialog;

    shell_status_update("Obtaining remote server API version...");
    remote_ver =
	xmlrpc_get_integer(xmlrpc_server_uri,
			   "server.getAPIVersion", NULL);

    switch (remote_ver) {
    case -1:
	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_NONE,
					"Remote host didn't respond. Try again?");

	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			       "Try again", GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	    gtk_widget_destroy(dialog);
	    return remote_version_is_supported();
	}

	gtk_widget_destroy(dialog);
	break;
    case XMLRPC_SERVER_VERSION:
	return TRUE;
    default:
	dialog = gtk_message_dialog_new(NULL,
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
	xmlrpc_get_string(xmlrpc_server_uri,
			  "module.entryFunction", "%s%i",
			  shell->selected_module->name,
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
	xmlrpc_get_string(xmlrpc_server_uri,
		                     "module.entryReload", "%s%i",
                                     shell->selected_module->name,
                                     shell->selected->number);
    } else {
	xmlrpc_get_string(xmlrpc_server_uri,
		                     "module.entryScan", "%s%i",
                                     shell->selected_module->name,
                                     shell->selected->number);
    }
}

static gchar *remote_module_entry_field_func(gchar * entry)
{
    Shell *shell = shell_get_main_shell();
    gchar *ret;

    ret =
	xmlrpc_get_string(xmlrpc_server_uri,
			  "module.entryGetField", "%s%i%s",
			  shell->selected_module->name,
			  shell->selected->number, entry);
    
    if (!ret) {
        remote_connection_error();
    }
    
    return ret;
}

static gchar *remote_module_entry_more_func(gchar * entry)
{
    Shell *shell = shell_get_main_shell();
    gchar *ret;

    ret =
	xmlrpc_get_string(xmlrpc_server_uri,
			  "module.entryGetMoreInfo", "%s%i%s",
			  shell->selected_module->name,
			  shell->selected->number, entry);

    if (!ret) {
        remote_connection_error();
    }

    return ret;
}

static gchar *remote_module_entry_note_func(gint entry)
{
    Shell *shell = shell_get_main_shell();
    gchar *note;

    note =
	xmlrpc_get_string(xmlrpc_server_uri,
			  "module.entryGetNote", "%s%i",
			  shell->selected_module->name,
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
	xmlrpc_get_array(xmlrpc_server_uri, "module.getModuleList", NULL);
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
	entries = xmlrpc_get_array(xmlrpc_server_uri,
		                   "module.getEntryList", "%s", m->name);
	if (entries && entries->n_values > 0) {
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
	    
            shell->tree->modules = g_slist_append(shell->tree->modules, m);
	} else {
	    g_free(m->name);
	    g_free(m);
	}
    }

    g_slist_foreach(shell->tree->modules, shell_add_modules_to_gui,
		    shell->tree);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->tree->view));

    g_value_array_free(modules);

    return TRUE;
}

static gboolean remote_connect_direct(gchar * hostname, gint port)
{
    gboolean retval = FALSE;

    remote_disconnect_all(FALSE);

    xmlrpc_init();
    xmlrpc_server_uri =
	g_strdup_printf("http://%s:%d/xmlrpc", hostname, port);

    shell_view_set_enabled(FALSE);

    if (remote_version_is_supported()) {
	if (!load_module_list()) {
	    GtkWidget *dialog;

	    dialog = gtk_message_dialog_new(NULL,
					    GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_CLOSE,
					    "Cannot obtain module list from server.");

	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	} else {
	    retval = TRUE;
	}
    }

    shell_status_update("Done.");
    shell_view_set_enabled(TRUE);

    return retval;
}

static gboolean remote_connect_ssh(gchar * hostname,
				   gint port,
				   gchar * username, gchar * password)
{
    GtkWidget *dialog;
    SSHConnResponse ssh_response;
    SoupURI *uri;
    gchar *struri;
    char buffer[32];
    gboolean error = FALSE;

    remote_disconnect_all(TRUE);

    shell_view_set_enabled(FALSE);
    shell_status_update("Establishing SSH tunnel...");
    struri = g_strdup_printf("ssh://%s:%s@%s:%d/?L4343:localhost:4242",
			     username, password, hostname, port);
    uri = soup_uri_new(struri);
    ssh_response = ssh_new(uri, &ssh_conn, "hardinfo -x");

    if (ssh_response != SSH_CONN_OK) {
	error = TRUE;
	ssh_close(ssh_conn);
    } else {
	gint res;
	gint bytes_read;

	memset(buffer, 0, sizeof(buffer));
	res =
	    ssh_read(ssh_conn->fd_read, buffer, sizeof(buffer),
		     &bytes_read);
	if (bytes_read != 0 && res == 1) {
	    if (strncmp(buffer, "XML-RPC server ready", 20) == 0) {
		DEBUG("%s", buffer);

		if (remote_connect_direct("127.0.0.1", 4343)) {
		    DEBUG("connected! :)");
		    goto out;
		}

		DEBUG("unknown error while trying to connect... wtf?");
	    }

	    /* TODO FIXME Perhaps the server is already running; try to fix */
	    DEBUG("hardinfo already running there?");
	}

	error = TRUE;
    }

  out:
    if (error) {
	dialog = gtk_message_dialog_new_with_markup(NULL,
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_ERROR,
						    GTK_BUTTONS_CLOSE,
						    "<b><big>Cannot establish tunnel.</big></b>\n"
						    "<i>%s</i>\n\n"
						    "Please verify that:\n"
						    "\342\200\242 The hostname <b>%s</b> is correct;\n"
						    "\342\200\242 There is a SSH server running on port <b>%d</b>;\n"
						    "\342\200\242 Your username/password combination is correct.",
						    ssh_response ==
						    SSH_CONN_OK ? "" :
						    ssh_conn_errors
						    [ssh_response],
						    hostname, port);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	ssh_close(ssh_conn);
	ssh_conn = NULL;
    }

    soup_uri_free(uri);
    g_free(struri);
    shell_view_set_enabled(TRUE);
    shell_status_update("Done.");

    return !error;
}

gboolean remote_connect_host(gchar * hostname)
{
    Shell *shell = shell_get_main_shell();
    gboolean retval = FALSE;

    if (!g_key_file_has_group(shell->hosts, hostname)) {
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Internal error.");

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    } else {
	const gint port =
	    g_key_file_get_integer(shell->hosts, hostname, "port", NULL);
	gchar *type =
	    g_key_file_get_string(shell->hosts, hostname, "type", NULL);

	if (g_str_equal(type, "ssh")) {
	    gchar *username =
		g_key_file_get_string(shell->hosts, hostname, "username",
				      NULL);
	    gchar *password =
		g_key_file_get_string(shell->hosts, hostname, "password",
				      NULL);

	    retval =
		remote_connect_ssh(hostname, port, username, password);

	    g_free(username);
	    g_free(password);
	} else {
	    retval = remote_connect_direct(hostname, port);
	}

	g_free(type);
    }

    return retval;
}

void connect_dialog_show(GtkWidget * parent)
{
    HostDialog *he =
	host_dialog_new(parent, "Connect to", HOST_DIALOG_MODE_CONNECT);

    if (gtk_dialog_run(GTK_DIALOG(he->dialog)) == GTK_RESPONSE_ACCEPT) {
	gboolean connected;
	gchar *hostname =
	    (gchar *) gtk_entry_get_text(GTK_ENTRY(he->txt_hostname));
	const gint selected_type =
	    gtk_combo_box_get_active(GTK_COMBO_BOX(he->cmb_type));
	const gint port =
	    (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(he->txt_port));

	gtk_widget_set_sensitive(he->dialog, FALSE);

	if (selected_type == 1) {
	    gchar *username =
		(gchar *) gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_user));
	    gchar *password =
		(gchar *)
		gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_password));

	    connected =
		remote_connect_ssh(hostname, port, username, password);
	} else {
	    connected = remote_connect_direct(hostname, port);
	}

	if (connected) {
	    Shell *shell = shell_get_main_shell();
	    gchar *tmp;

	    tmp = g_strdup_printf("Remote: <b>%s</b>", hostname);
	    shell_set_remote_label(shell, tmp);

	    g_free(tmp);
	} else {
	    cb_local_computer();
	}
    }

    host_dialog_destroy(he);
}

void host_manager_show(GtkWidget * parent)
{
    HostManager *rd = host_manager_new(parent);

    gtk_dialog_run(GTK_DIALOG(rd->dialog));

    host_manager_destroy(rd);
}

static void populate_store(HostManager * rd, GtkListStore * store)
{
    Shell *shell;
    GtkTreeIter iter;
    gchar **hosts;
    gint i;
    gsize no_groups;

    gtk_list_store_clear(store);
    shell = shell_get_main_shell();

    hosts = g_key_file_get_groups(shell->hosts, &no_groups);
    DEBUG("%d hosts found", no_groups);
    for (i = 0; i < no_groups; i++) {
	gchar *icon;

	DEBUG("host #%d: %s", i, hosts[i]);

	icon = g_key_file_get_string(shell->hosts, hosts[i], "icon", NULL);

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

static GtkTreeModel *host_dialog_get_completion_model(void)
{
    Shell *shell;
    GtkListStore *store;
    GtkTreeIter iter;
    gchar **groups;
    gint i = 0;

    shell = shell_get_main_shell();

    store = gtk_list_store_new(1, G_TYPE_STRING);
    for (groups = g_key_file_get_groups(shell->hosts, NULL); groups[i];
	 i++) {
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, g_strdup(groups[i]), -1);
    }
    g_strfreev(groups);

    return GTK_TREE_MODEL(store);
}

static void host_combo_changed_cb(GtkComboBox * widget, gpointer user_data)
{
    HostDialog *host_dlg = (HostDialog *) user_data;
    const gint default_ports[] = { 4242, 22 };
    gint index;

    index = gtk_combo_box_get_active(widget);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(host_dlg->notebook), index);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(host_dlg->txt_port),
			      default_ports[index]);

    if (index == 0) {
       gtk_widget_hide(host_dlg->frm_options);
    } else {
       gtk_widget_show(host_dlg->frm_options);
    }
}

static void
host_dialog_hostname_changed (GtkEditable *entry, gpointer user_data)
{
    HostDialog *host_dlg = (HostDialog *)user_data;
    GRegex *regex_ip = NULL, *regex_host = NULL;
    gboolean match;
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    
    /*
     * Regexes from:
     * http://stackoverflow.com/questions/106179
     */
    const gchar *valid_ip_regex = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|" \
                                  "25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}" \
                                  "|2[0-4][0-9]|25[0-5])$";
    const gchar *valid_hostname_regex = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9" \
                                        "\\-]*[a-zA-Z0-9])\\.)*([A-Za-z]|[A-Z" \
                                        "a-z][A-Za-z0-9\\-]*[A-Za-z0-9])$";
    if (regex_ip == NULL) {
        regex_ip = g_regex_new(valid_ip_regex, 0, 0, NULL);
    }
    
    if (regex_host == NULL) {
        regex_host = g_regex_new(valid_hostname_regex, 0, 0, NULL);
    }
    
    match = g_regex_match(regex_ip, text, 0, NULL);
    if (!match) {
        match = g_regex_match(regex_host, text, 0, NULL);
    }
    
    gtk_widget_set_sensitive(host_dlg->btn_accept, match);
}

static void host_dialog_destroy(HostDialog * rd)
{
    gtk_widget_destroy(rd->dialog);
    g_free(rd);
}

static HostDialog *host_dialog_new(GtkWidget * parent,
				   gchar * title, HostDialogMode mode)
{
    HostDialog *host_dlg;
    GtkWidget *dialog;
    GtkWidget *dialog_vbox1;
    GtkWidget *vbox1;
    GtkWidget *frm_remote_host;
    GtkWidget *alignment1;
    GtkWidget *table1;
    GtkWidget *label2;
    GtkWidget *label3;
    GtkWidget *label4;
    GtkWidget *cmb_type;
    GtkWidget *txt_hostname;
    GtkWidget *alignment2;
    GtkWidget *hbox1;
    GtkObject *txt_port_adj;
    GtkWidget *txt_port;
    GtkWidget *label1;
    GtkWidget *frm_options;
    GtkWidget *alignment3;
    GtkWidget *notebook;
    GtkWidget *table2;
    GtkWidget *label8;
    GtkWidget *label9;
    GtkWidget *txt_ssh_user;
    GtkWidget *txt_ssh_password;
    GtkWidget *label10;
    GtkWidget *label5;
    GtkWidget *dialog_action_area1;
    GtkWidget *btn_cancel;
    GtkWidget *btn_save;
    GtkEntryCompletion *completion;
    GtkTreeModel *completion_model;

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

    vbox1 = gtk_vbox_new(FALSE, 3);
    gtk_widget_show(vbox1);
    gtk_box_pack_start(GTK_BOX(dialog_vbox1), vbox1, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);
    gtk_box_set_spacing(GTK_BOX(vbox1), 10);

    frm_remote_host = gtk_frame_new(NULL);
    gtk_widget_show(frm_remote_host);
    gtk_box_pack_start(GTK_BOX(vbox1), frm_remote_host, FALSE, TRUE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frm_remote_host), GTK_SHADOW_NONE);

    alignment1 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment1);
    gtk_container_add(GTK_CONTAINER(frm_remote_host), alignment1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment1), 0, 0, 12, 0);
    
    table1 = gtk_table_new(3, 2, FALSE);
    gtk_widget_show(table1);
    gtk_container_add(GTK_CONTAINER(alignment1), table1);
    gtk_table_set_row_spacings(GTK_TABLE(table1), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 4);

    label2 = gtk_label_new("Protocol:");
    gtk_widget_show(label2);
    gtk_table_attach(GTK_TABLE(table1), label2, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label2), 0, 0.5);

    label3 = gtk_label_new("Host:");
    gtk_widget_show(label3);
    gtk_table_attach(GTK_TABLE(table1), label3, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label3), 0, 0.5);

    label4 = gtk_label_new("Port:");
    gtk_widget_show(label4);
    gtk_table_attach(GTK_TABLE(table1), label4, 0, 1, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label4), 0, 0.5);

    cmb_type = gtk_combo_box_new_text();
    gtk_widget_show(cmb_type);
    gtk_table_attach(GTK_TABLE(table1), cmb_type, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_combo_box_append_text(GTK_COMBO_BOX(cmb_type),
			      "Direct connection");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cmb_type),
			      "Remote tunnel over SSH");

    txt_hostname = gtk_entry_new();
    gtk_widget_show(txt_hostname);
    gtk_table_attach(GTK_TABLE(table1), txt_hostname, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);

    alignment2 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment2);
    gtk_table_attach(GTK_TABLE(table1), alignment2, 1, 2, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);

    hbox1 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox1);
    gtk_container_add(GTK_CONTAINER(alignment2), hbox1);

    txt_port_adj = gtk_adjustment_new(4242, 1, 65535, 1, 10, 0);
    txt_port = gtk_spin_button_new(GTK_ADJUSTMENT(txt_port_adj), 1, 0);
    gtk_widget_show(txt_port);
    gtk_box_pack_start(GTK_BOX(hbox1), txt_port, FALSE, TRUE, 0);

    label1 = gtk_label_new("<b>Remote host</b>");
    gtk_widget_show(label1);
    gtk_frame_set_label_widget(GTK_FRAME(frm_remote_host), label1);
    gtk_label_set_use_markup(GTK_LABEL(label1), TRUE);

    frm_options = gtk_frame_new(NULL);
    gtk_widget_show(frm_options);
    gtk_box_pack_start(GTK_BOX(vbox1), frm_options, FALSE, TRUE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frm_options), GTK_SHADOW_NONE);

    alignment3 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment3);
    gtk_container_add(GTK_CONTAINER(frm_options), alignment3);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment3), 0, 0, 12, 0);

    notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    gtk_container_add(GTK_CONTAINER(alignment3), notebook);
    GTK_WIDGET_UNSET_FLAGS(notebook, GTK_CAN_FOCUS);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    label10 = gtk_label_new("");
    gtk_widget_show(label10);
    gtk_container_add(GTK_CONTAINER(notebook), label10);
    gtk_label_set_use_markup(GTK_LABEL(label10), TRUE);

    table2 = gtk_table_new(2, 2, FALSE);
    gtk_widget_show(table2);
    gtk_container_add(GTK_CONTAINER(notebook), table2);
    gtk_table_set_row_spacings(GTK_TABLE(table2), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table2), 4);

    label8 = gtk_label_new("User:");
    gtk_widget_show(label8);
    gtk_table_attach(GTK_TABLE(table2), label8, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label8), 0, 0.5);

    label9 = gtk_label_new("Password:");
    gtk_widget_show(label9);
    gtk_table_attach(GTK_TABLE(table2), label9, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label9), 0, 0.5);

    txt_ssh_user = gtk_entry_new();
    gtk_widget_show(txt_ssh_user);
    gtk_table_attach(GTK_TABLE(table2), txt_ssh_user, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    txt_ssh_password = gtk_entry_new();
    gtk_widget_show(txt_ssh_password);
    gtk_table_attach(GTK_TABLE(table2), txt_ssh_password, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_invisible_char(GTK_ENTRY(txt_ssh_password), 9679);
    gtk_entry_set_visibility(GTK_ENTRY(txt_ssh_password), FALSE);
    
    label5 = gtk_label_new("<b>Connection options</b>");
    gtk_widget_show(label5);
    gtk_frame_set_label_widget(GTK_FRAME(frm_options), label5);
    gtk_label_set_use_markup(GTK_LABEL(label5), TRUE);

    dialog_action_area1 = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog_action_area1);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1),
			      GTK_BUTTONBOX_END);

    btn_cancel = gtk_button_new_from_stock("gtk-cancel");
    gtk_widget_show(btn_cancel);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_cancel,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(btn_cancel, GTK_CAN_DEFAULT);

    if (mode == HOST_DIALOG_MODE_EDIT) {
	btn_save = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    } else if (mode == HOST_DIALOG_MODE_CONNECT) {
	btn_save = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
    }

    gtk_widget_show(btn_save);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_save,
                                 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(btn_save, GTK_CAN_DEFAULT);

    host_dlg = g_new0(HostDialog, 1);
    host_dlg->dialog = dialog;
    host_dlg->notebook = notebook;
    host_dlg->txt_hostname = txt_hostname;
    host_dlg->txt_port = txt_port;
    host_dlg->txt_ssh_user = txt_ssh_user;
    host_dlg->txt_ssh_password = txt_ssh_password;
    host_dlg->cmb_type = cmb_type;
    host_dlg->frm_options = frm_options;
    host_dlg->btn_accept = btn_save;
    host_dlg->btn_cancel = btn_cancel;
    
    completion = gtk_entry_completion_new();
    gtk_entry_set_completion(GTK_ENTRY(host_dlg->txt_hostname), completion);
    g_object_unref(completion);
    
    completion_model = host_dialog_get_completion_model();
    gtk_entry_completion_set_model(completion, completion_model);
    g_object_unref(completion_model);
    
    gtk_entry_completion_set_text_column(completion, 0);

    gtk_combo_box_set_active(GTK_COMBO_BOX(host_dlg->cmb_type), 0);
    
    g_signal_connect(G_OBJECT(txt_hostname), "changed",
                     G_CALLBACK(host_dialog_hostname_changed), host_dlg);
    g_signal_connect(G_OBJECT(cmb_type), "changed",
		     G_CALLBACK(host_combo_changed_cb), host_dlg);

    host_combo_changed_cb(GTK_COMBO_BOX(cmb_type), host_dlg);
    host_dialog_hostname_changed(GTK_EDITABLE(txt_hostname), host_dlg);
    
    gtk_entry_set_activates_default(GTK_ENTRY(txt_hostname), TRUE);

    return host_dlg;
}

static void host_manager_add(GtkWidget * button, gpointer data)
{
    Shell *shell = shell_get_main_shell();
    HostManager *rd = (HostManager *) data;
    HostDialog *he =
	host_dialog_new(rd->dialog, "Add a host", HOST_DIALOG_MODE_EDIT);

  retry:
    if (gtk_dialog_run(GTK_DIALOG(he->dialog)) == GTK_RESPONSE_ACCEPT) {
	const gchar *hostname =
	    gtk_entry_get_text(GTK_ENTRY(he->txt_hostname));

	if (g_key_file_has_group(shell->hosts, hostname)) {
	    GtkWidget *dialog;

	    dialog =
		gtk_message_dialog_new_with_markup(GTK_WINDOW(rd->dialog),
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_CLOSE,
						   "Hostname <b>%s</b> already exists.",
						   hostname);

	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);

	    goto retry;
	} else {
	    GtkTreeIter iter;
	    const gchar *type[] = { "direct", "ssh" };
	    const gint selected_type =
		gtk_combo_box_get_active(GTK_COMBO_BOX(he->cmb_type));
	    const gint port =
		(int)
		gtk_spin_button_get_value(GTK_SPIN_BUTTON(he->txt_port));

	    g_key_file_set_string(shell->hosts, hostname, "type",
				  type[selected_type]);
	    g_key_file_set_integer(shell->hosts, hostname, "port", port);

	    if (selected_type == 1) {
		const gchar *username =
		    gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_user));
		const gchar *password =
		    gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_password));

		g_key_file_set_string(shell->hosts, hostname, "username",
				      username);
		g_key_file_set_string(shell->hosts, hostname, "password",
				      password);
	    }

	    gtk_list_store_append(rd->tree_store, &iter);
	    gtk_list_store_set(rd->tree_store, &iter,
			       0, icon_cache_get_pixbuf("server.png"),
			       1, g_strdup(hostname), 2, 0, -1);
	}
    }

    host_dialog_destroy(he);
}

static void host_manager_edit(GtkWidget * button, gpointer data)
{
    Shell *shell = shell_get_main_shell();
    GtkWidget *dialog;
    HostManager *rd = (HostManager *) data;
    HostDialog *he =
	host_dialog_new(rd->dialog, "Edit a host", HOST_DIALOG_MODE_EDIT);
    gchar *host_type;
    gint host_port;

    host_type =
	g_key_file_get_string(shell->hosts, rd->selected_name, "type",
			      NULL);
    if (!host_type || g_str_equal(host_type, "direct")) {
	gtk_combo_box_set_active(GTK_COMBO_BOX(he->cmb_type), 0);
    } else if (g_str_equal(host_type, "ssh")) {
	gchar *username, *password;

	gtk_combo_box_set_active(GTK_COMBO_BOX(he->cmb_type), 1);

	username =
	    g_key_file_get_string(shell->hosts, rd->selected_name,
				  "username", NULL);
	if (username) {
	    gtk_entry_set_text(GTK_ENTRY(he->txt_ssh_user), username);
	    g_free(username);
	}

	password =
	    g_key_file_get_string(shell->hosts, rd->selected_name,
				  "password", NULL);
	if (password) {
	    gtk_entry_set_text(GTK_ENTRY(he->txt_ssh_password), password);
	    g_free(password);
	}
    } else {
	dialog = gtk_message_dialog_new(GTK_WINDOW(rd->dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					"Host has invalid type(%s).",
					host_type);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	goto bad;
    }

    gtk_entry_set_text(GTK_ENTRY(he->txt_hostname), rd->selected_name);

    host_port =
	g_key_file_get_integer(shell->hosts, rd->selected_name, "port",
			       NULL);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(he->txt_port),
			      host_port ? host_port : 4242);

    if (gtk_dialog_run(GTK_DIALOG(he->dialog)) == GTK_RESPONSE_ACCEPT) {
	const gchar *type[] = { "direct", "ssh" };
	const gint selected_type =
	    gtk_combo_box_get_active(GTK_COMBO_BOX(he->cmb_type));
	const gint port =
	    (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(he->txt_port));
	const gchar *hostname =
	    gtk_entry_get_text(GTK_ENTRY(he->txt_hostname));

	g_key_file_set_string(shell->hosts, hostname, "type",
			      type[selected_type]);
	g_key_file_set_integer(shell->hosts, hostname, "port", port);

	if (selected_type == 1) {
	    const gchar *username =
		gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_user));
	    const gchar *password =
		gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_password));

	    g_key_file_set_string(shell->hosts, hostname, "username",
				  username);
	    g_key_file_set_string(shell->hosts, hostname, "password",
				  password);
	}
    }

  bad:
    host_dialog_destroy(he);
    g_free(host_type);
}

static void host_manager_remove(GtkWidget * button, gpointer data)
{
    Shell *shell = shell_get_main_shell();
    HostManager *rd = (HostManager *) data;
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
	g_key_file_remove_group(shell->hosts, rd->selected_name, NULL);
	gtk_list_store_remove(rd->tree_store, rd->selected_iter);

	gtk_widget_set_sensitive(rd->btn_edit, FALSE);
	gtk_widget_set_sensitive(rd->btn_remove, FALSE);
    }

    gtk_widget_destroy(dialog);
}

static void host_manager_tree_sel_changed(GtkTreeSelection * sel,
					  gpointer data)
{
    HostManager *rd = (HostManager *) data;
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

static void host_manager_destroy(HostManager * rd)
{
    shell_save_hosts_file();
    shell_update_remote_menu();
    gtk_widget_destroy(rd->dialog);

    g_free(rd);
}

static HostManager *host_manager_new(GtkWidget * parent)
{
    HostManager *rd;
    GtkWidget *dialog;
    GtkWidget *dialog_vbox;
    GtkWidget *scrolledwindow;
    GtkWidget *treeview;
    GtkWidget *vbuttonbox;
    GtkWidget *btn_add;
    GtkWidget *btn_edit;
    GtkWidget *dialog_action_area;
    GtkWidget *btn_cancel;
    GtkWidget *btn_remove;
    GtkWidget *hbox;
    GtkTreeSelection *sel;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cr_text, *cr_pbuf;
    GtkListStore *store;
    GtkTreeModel *model;

    rd = g_new0(HostManager, 1);

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Remote Host Manager");
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog),
			    GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);

    dialog_vbox = GTK_DIALOG(dialog)->vbox;
    gtk_box_set_spacing(GTK_BOX(dialog_vbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(dialog_vbox), 4);
    gtk_widget_show(dialog_vbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dialog_vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow);
    gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow, TRUE, TRUE, 0);
    gtk_widget_set_size_request(scrolledwindow, -1, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scrolledwindow), GTK_SHADOW_IN);

    store =
	gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    model = GTK_TREE_MODEL(store);

    treeview = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_widget_show(treeview);
    gtk_container_add(GTK_CONTAINER(scrolledwindow), treeview);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), TRUE);

    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    cr_pbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, cr_pbuf, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_pbuf, "pixbuf",
				       TREE_COL_PBUF);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, TRUE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       TREE_COL_NAME);

    vbuttonbox = gtk_vbutton_box_new();
    gtk_widget_show(vbuttonbox);
    gtk_box_pack_start(GTK_BOX(hbox), vbuttonbox, FALSE, TRUE, 0);
    gtk_box_set_spacing(GTK_BOX(vbuttonbox), 5);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox),
			      GTK_BUTTONBOX_START);

    btn_add = gtk_button_new_with_mnemonic("_Add");
    gtk_widget_show(btn_add);
    gtk_container_add(GTK_CONTAINER(vbuttonbox), btn_add);
    GTK_WIDGET_SET_FLAGS(btn_add, GTK_CAN_DEFAULT);
    g_signal_connect(btn_add, "clicked", G_CALLBACK(host_manager_add), rd);

    btn_edit = gtk_button_new_with_mnemonic("_Edit");
    gtk_widget_show(btn_edit);
    gtk_container_add(GTK_CONTAINER(vbuttonbox), btn_edit);
    GTK_WIDGET_SET_FLAGS(btn_edit, GTK_CAN_DEFAULT);
    g_signal_connect(btn_edit, "clicked", G_CALLBACK(host_manager_edit),
		     rd);

    btn_remove = gtk_button_new_with_mnemonic("_Remove");
    gtk_widget_show(btn_remove);
    gtk_container_add(GTK_CONTAINER(vbuttonbox), btn_remove);
    GTK_WIDGET_SET_FLAGS(btn_remove, GTK_CAN_DEFAULT);
    g_signal_connect(btn_remove, "clicked",
		     G_CALLBACK(host_manager_remove), rd);

    dialog_action_area = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area),
			      GTK_BUTTONBOX_END);

    btn_cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_widget_show(btn_cancel);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_cancel,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(btn_cancel, GTK_CAN_DEFAULT);

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(treeview));

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    g_signal_connect(G_OBJECT(sel), "changed",
		     (GCallback) host_manager_tree_sel_changed, rd);

    rd->dialog = dialog;
    rd->btn_cancel = btn_cancel;
    rd->btn_add = btn_add;
    rd->btn_edit = btn_edit;
    rd->btn_remove = btn_remove;
    rd->tree_store = store;

    populate_store(rd, store);
    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_edit), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_remove), FALSE);

    return rd;
}

#endif				/* HAS_LIBSOUP */
