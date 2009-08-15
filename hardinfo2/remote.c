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
#include "shell.h"
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
};

static HostManager *host_manager_new(GtkWidget * parent);
static void host_manager_destroy(HostManager * rd);
static HostDialog *host_dialog_new(GtkWidget * parent,
                                   gchar * title,
                                   HostDialogMode mode);
static void host_dialog_destroy(HostDialog *hd);


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

static gboolean remote_version_is_supported()
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
	xmlrpc_get_string(xmlrpc_server_uri,
			  "module.entryReload", "%s%i",
			  shell->selected_module_name,
			  shell->selected->number);
    } else {
	xmlrpc_get_string(xmlrpc_server_uri,
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
	xmlrpc_get_string(xmlrpc_server_uri,
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
	xmlrpc_get_string(xmlrpc_server_uri,
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
	xmlrpc_get_string(xmlrpc_server_uri,
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
	xmlrpc_get_array(xmlrpc_server_uri,
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
	    xmlrpc_get_array(xmlrpc_server_uri,
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

static gboolean remote_connect_direct(gchar *hostname,
                                      gint port)
{
    gboolean retval = FALSE;
    
    remote_disconnect_all(FALSE);

    xmlrpc_init();
    xmlrpc_server_uri = g_strdup_printf("http://%s:%d/xmlrpc", hostname, port);
    
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

static gboolean remote_connect_ssh(gchar *hostname, 
                                   gint port,
                                   gchar *username,
                                   gchar *password)
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
                             username, password,
                             hostname, port);
    uri = soup_uri_new(struri);
    ssh_response = ssh_new(uri, &ssh_conn, "hardinfo -x");
    
    if (ssh_response != SSH_CONN_OK) {
        error = TRUE;
        ssh_close(ssh_conn);
    } else {
        gint res;
        gint bytes_read;
        
        memset(buffer, 0, sizeof(buffer));
        res = ssh_read(ssh_conn->fd_read, buffer, sizeof(buffer), &bytes_read);
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
                                                    ssh_response == SSH_CONN_OK ? "" : ssh_conn_errors[ssh_response],
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

gboolean remote_connect_host(gchar *hostname)
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
        
        return;
    } else {
        const gint port = g_key_file_get_integer(shell->hosts, hostname, "port", NULL);
        gchar *type = g_key_file_get_string(shell->hosts, hostname, "type", NULL);
        
        if (g_str_equal(type, "ssh")) {
            gchar *username = g_key_file_get_string(shell->hosts, hostname, "username", NULL);
            gchar *password = g_key_file_get_string(shell->hosts, hostname, "password", NULL);
            
            retval = remote_connect_ssh(hostname, port, username, password);
            
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
    HostDialog *he = host_dialog_new(parent, "Connect to", HOST_DIALOG_MODE_CONNECT);

    if (gtk_dialog_run(GTK_DIALOG(he->dialog)) == GTK_RESPONSE_ACCEPT) {
        gboolean connected;
        gchar *hostname = (gchar *)gtk_entry_get_text(GTK_ENTRY(he->txt_hostname));
        const gint selected_type = gtk_combo_box_get_active(GTK_COMBO_BOX(he->cmb_type));
        const gint port = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(he->txt_port));
        
        gtk_widget_set_sensitive(he->dialog, FALSE);

        if (selected_type == 1) {
            gchar *username = (gchar *)gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_user));
            gchar *password = (gchar *)gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_password));
            
            connected = remote_connect_ssh(hostname, port, username, password);
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
    gboolean success;
    HostManager *rd = host_manager_new(parent);

    gtk_dialog_run(GTK_DIALOG(rd->dialog));

    host_manager_destroy(rd);
}

static void populate_store(HostManager * rd, GtkListStore * store)
{
    Shell *shell;
    GtkTreeIter iter;
    gchar *path;
    gchar **hosts;
    gint i, no_groups;

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
    for (groups = g_key_file_get_groups(shell->hosts, NULL); groups[i]; i++) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, g_strdup(groups[i]), -1);
    }
    g_strfreev(groups);
    
    return GTK_TREE_MODEL(store);
}

static void host_combo_changed_cb(GtkComboBox * widget,
					 gpointer user_data)
{
    HostDialog *host_dlg = (HostDialog *) user_data;
    const gint default_ports[] = { 4242, 22 };
    gint index;

    index = gtk_combo_box_get_active(widget);
    
    gtk_notebook_set_current_page(GTK_NOTEBOOK(host_dlg->notebook), index);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(host_dlg->txt_port), default_ports[index]);
}

static void host_dialog_destroy(HostDialog *rd)
{
    gtk_widget_destroy(rd->dialog);
    g_free(rd);
}

static HostDialog *host_dialog_new(GtkWidget * parent,
                                   gchar * title,
                                   HostDialogMode mode)
{
    HostDialog *host_dlg;
    GtkEntryCompletion *completion;
    GtkTreeModel *completion_model;
    GtkWidget *dialog;
    GtkWidget *dialog_vbox1;
    GtkWidget *vbox1;
    GtkWidget *table1;
    GtkWidget *label4;
    GtkWidget *label5;
    GtkWidget *txt_hostname;
    GtkWidget *alignment2;
    GtkObject *txt_port_adj;
    GtkWidget *txt_port;
    GtkWidget *frame1;
    GtkWidget *alignment3;
    GtkWidget *notebook;
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

    txt_port_adj = gtk_adjustment_new(4242, 1, 65535, 1, 10, 0);
    txt_port = gtk_spin_button_new(GTK_ADJUSTMENT(txt_port_adj), 1, 0);
    gtk_widget_show(txt_port);
    gtk_container_add(GTK_CONTAINER(alignment2), txt_port);

    frame1 = gtk_frame_new(NULL);
    gtk_widget_show(frame1);
    gtk_box_pack_start(GTK_BOX(vbox1), frame1, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame1), GTK_SHADOW_ETCHED_IN);

    alignment3 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment3);
    gtk_container_add(GTK_CONTAINER(frame1), alignment3);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment3), 0, 0, 12, 0);

    notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    gtk_container_add(GTK_CONTAINER(alignment3), notebook);
    GTK_WIDGET_UNSET_FLAGS(notebook, GTK_CAN_FOCUS);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox2);
    gtk_container_add(GTK_CONTAINER(notebook), vbox2);

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
    gtk_container_add(GTK_CONTAINER(notebook), vbox3);
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

    if (mode == HOST_DIALOG_MODE_EDIT) {
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
    }

    hbox1 = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox1);
    gtk_frame_set_label_widget(GTK_FRAME(frame1), hbox1);

    label3 = gtk_label_new("Type:");
    gtk_widget_show(label3);
    gtk_box_pack_start(GTK_BOX(hbox1), label3, FALSE, FALSE, 0);

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

    btn_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
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

    completion = gtk_entry_completion_new();
    gtk_entry_set_completion(GTK_ENTRY(host_dlg->txt_hostname), completion);
    g_object_unref(completion);
    
    completion_model = host_dialog_get_completion_model();
    gtk_entry_completion_set_model(completion, completion_model);
    g_object_unref(completion_model);
    
    gtk_entry_completion_set_text_column(completion, 0);
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(host_dlg->cmb_type), 0);
    g_signal_connect(G_OBJECT(cmb_type), "changed",
		     G_CALLBACK(host_combo_changed_cb), host_dlg);

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
        const gchar *hostname = gtk_entry_get_text(GTK_ENTRY(he->txt_hostname));

        if (g_key_file_has_group(shell->hosts, hostname)) {
            GtkWidget *dialog;
            
            dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(rd->dialog),
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_ERROR,
                                                        GTK_BUTTONS_CLOSE,
                                                        "Hostname <b>%s</b> already exists.", hostname);

            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            
            goto retry;
        } else {
            GtkTreeIter iter;
            const gchar *type[] = { "direct", "ssh" };
            const gint selected_type = gtk_combo_box_get_active(GTK_COMBO_BOX(he->cmb_type));
            const gint port = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(he->txt_port));

            g_key_file_set_string(shell->hosts, hostname, "type", type[selected_type]);
            g_key_file_set_integer(shell->hosts, hostname, "port", port);
            
            if (selected_type == 1) {
                const gchar *username = gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_user));
                const gchar *password = gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_password));
                
                g_key_file_set_string(shell->hosts, hostname, "username", username);
                g_key_file_set_string(shell->hosts, hostname, "password", password);
            }

            gtk_list_store_append(rd->tree_store, &iter);
            gtk_list_store_set(rd->tree_store, &iter,
                               0, icon_cache_get_pixbuf("server.png"),
                               1, g_strdup(hostname),
                               2, 0,
                               -1);
        }
    }

    host_dialog_destroy(he);
}

static void host_manager_edit(GtkWidget * button, gpointer data)
{
    Shell *shell = shell_get_main_shell();
    GtkWidget *dialog;
    HostManager *rd = (HostManager *) data;
    HostDialog *he = host_dialog_new(rd->dialog, "Edit a host", HOST_DIALOG_MODE_EDIT);
    gchar *host_type;
    gint host_port;
    
    host_type = g_key_file_get_string(shell->hosts, rd->selected_name, "type", NULL);
    if (!host_type || g_str_equal(host_type, "direct")) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(he->cmb_type), 0);
    } else if (g_str_equal(host_type, "ssh")) {
	gchar *username, *password;

        gtk_combo_box_set_active(GTK_COMBO_BOX(he->cmb_type), 1);

	username = g_key_file_get_string(shell->hosts, rd->selected_name, "username", NULL);
	if (username) {
	    gtk_entry_set_text(GTK_ENTRY(he->txt_ssh_user), username);
	    g_free(username);
	}

	password = g_key_file_get_string(shell->hosts, rd->selected_name, "password", NULL);
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
    
    host_port = g_key_file_get_integer(shell->hosts, rd->selected_name, "port", NULL);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(he->txt_port), host_port ? host_port : 4242);

    if (gtk_dialog_run(GTK_DIALOG(he->dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar *type[] = { "direct", "ssh" };
        const gint selected_type = gtk_combo_box_get_active(GTK_COMBO_BOX(he->cmb_type));
        const gint port = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(he->txt_port));
        const gchar *hostname = gtk_entry_get_text(GTK_ENTRY(he->txt_hostname));

        g_key_file_set_string(shell->hosts, hostname, "type", type[selected_type]);
        g_key_file_set_integer(shell->hosts, hostname, "port", port);
        
        if (selected_type == 1) {
            const gchar *username = gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_user));
            const gchar *password = gtk_entry_get_text(GTK_ENTRY(he->txt_ssh_password));
            
            g_key_file_set_string(shell->hosts, hostname, "username", username);
            g_key_file_set_string(shell->hosts, hostname, "password", password);
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
    gchar *path;
    GtkWidget *dialog;
    GtkWidget *dialog1_vbox;
    GtkWidget *scrolledwindow;
    GtkWidget *treeview;
    GtkWidget *vbuttonbox;
    GtkWidget *button3;
    GtkWidget *button6;
    GtkWidget *dialog1_action_area;
    GtkWidget *button8;
    GtkWidget *button2;
    GtkWidget *label;
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

    dialog1_vbox = GTK_DIALOG(dialog)->vbox;
    gtk_box_set_spacing(GTK_BOX(dialog1_vbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(dialog1_vbox), 4);
    gtk_widget_show(dialog1_vbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbox, TRUE, TRUE, 0);
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

    button3 = gtk_button_new_with_mnemonic("_Add");
    gtk_widget_show(button3);
    gtk_container_add(GTK_CONTAINER(vbuttonbox), button3);
    GTK_WIDGET_SET_FLAGS(button3, GTK_CAN_DEFAULT);
    g_signal_connect(button3, "clicked",
		     G_CALLBACK(host_manager_add), rd);

    button6 = gtk_button_new_with_mnemonic("_Edit");
    gtk_widget_show(button6);
    gtk_container_add(GTK_CONTAINER(vbuttonbox), button6);
    GTK_WIDGET_SET_FLAGS(button6, GTK_CAN_DEFAULT);
    g_signal_connect(button6, "clicked", G_CALLBACK(host_manager_edit),
		     rd);

    button2 = gtk_button_new_with_mnemonic("_Remove");
    gtk_widget_show(button2);
    gtk_container_add(GTK_CONTAINER(vbuttonbox), button2);
    GTK_WIDGET_SET_FLAGS(button2, GTK_CAN_DEFAULT);
    g_signal_connect(button2, "clicked", G_CALLBACK(host_manager_remove),
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

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(treeview));

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    g_signal_connect(G_OBJECT(sel), "changed",
		     (GCallback) host_manager_tree_sel_changed, rd);

    rd->dialog = dialog;
    rd->btn_cancel = button8;
    rd->btn_add = button3;
    rd->btn_edit = button6;
    rd->btn_remove = button2;
    rd->tree_store = store;

    populate_store(rd, store);
    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_edit), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(rd->btn_remove), FALSE);

    return rd;
}
