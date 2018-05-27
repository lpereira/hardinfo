/*
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

#include "syncmanager.h"
#include "iconcache.h"
#include "hardinfo.h"
#include "config.h"

#ifdef HAS_LIBSOUP
#include <libsoup/soup.h>

#include <stdarg.h>
#include <string.h>

typedef struct _SyncDialog SyncDialog;
typedef struct _SyncNetArea SyncNetArea;
typedef struct _SyncNetAction SyncNetAction;

struct _SyncNetArea {
    GtkWidget *vbox;
};

struct _SyncNetAction {
    gchar *name;
     gboolean(*do_action) (SyncDialog * sd, gpointer sna);

    SyncEntry *entry;
    GError *error;
};

struct _SyncDialog {
    GtkWidget *dialog;
    GtkWidget *label;

    GtkWidget *button_sync;
    GtkWidget *button_cancel;
    GtkWidget *button_close;

    GtkWidget *scroll_box;

    SyncNetArea *sna;

    gboolean flag_cancel:1;
};

static GSList *entries = NULL;
static SoupSession *session = NULL;
static GMainLoop *loop;
static GQuark err_quark;

#define XMLRPC_SERVER_URI   		"https://xmlrpc.hardinfo.org/"
#define XMLRPC_SERVER_API_VERSION	1

#define LABEL_SYNC_DEFAULT  _("<big><b>Synchronize with Central Database</b></big>\n" \
                            "The following information may be synchronized " \
                            "with the HardInfo central database.")
#define LABEL_SYNC_SYNCING  _("<big><b>Synchronizing</b></big>\n" \
                            "This may take some time.")

static SyncDialog *sync_dialog_new(GtkWidget *parent);
static void sync_dialog_destroy(SyncDialog * sd);
static void sync_dialog_start_sync(SyncDialog * sd);

static SyncNetArea *sync_dialog_netarea_new(void);
static void sync_dialog_netarea_destroy(SyncNetArea * sna);
static void sync_dialog_netarea_show(SyncDialog * sd);
#if 0
static void sync_dialog_netarea_hide(SyncDialog * sd);
#endif
static void sync_dialog_netarea_start_actions(SyncDialog * sd,
					      SyncNetAction * sna, gint n);

#define SNA_ERROR(code,message,...) if (!sna->error) {                                         \
                                            sna->error = g_error_new(err_quark, code, message, \
                                                         ##__VA_ARGS__);                       \
                                    }
#endif				/* HAS_LIBSOUP */


gint sync_manager_count_entries(void)
{
#ifdef HAS_LIBSOUP
    return g_slist_length(entries);
#else
    return 0;
#endif
}

void sync_manager_add_entry(SyncEntry * entry)
{
#ifdef HAS_LIBSOUP
    DEBUG("registering syncmanager entry ''%s''", entry->fancy_name);

    entry->selected = TRUE;
    entries = g_slist_append(entries, entry);
#else
    DEBUG("libsoup support is disabled.");
#endif				/* HAS_LIBSOUP */
}

void sync_manager_clear_entries(void)
{
#ifdef HAS_LIBSOUP
    DEBUG("clearing syncmanager entries");

    g_slist_free(entries);
    entries = NULL;
#else
    DEBUG("libsoup support is disabled.");
#endif				/* HAS_LIBSOUP */
}

void sync_manager_show(GtkWidget *parent)
{
#ifndef HAS_LIBSOUP
    g_warning
	(_("HardInfo was compiled without libsoup support. (Network Updater requires it.)"));
#else				/* !HAS_LIBSOUP */
    SyncDialog *sd = sync_dialog_new(parent);

    err_quark = g_quark_from_static_string("syncmanager");

    if (gtk_dialog_run(GTK_DIALOG(sd->dialog)) == GTK_RESPONSE_ACCEPT) {
	shell_view_set_enabled(FALSE);
	shell_status_set_enabled(TRUE);

	sync_dialog_start_sync(sd);

	shell_status_set_enabled(FALSE);
	shell_view_set_enabled(TRUE);
    }

    sync_dialog_destroy(sd);
#endif				/* HAS_LIBSOUP */
}

#ifdef HAS_LIBSOUP
static gint _soup_get_xmlrpc_value_int(SoupMessage * msg,
				       SyncNetAction * sna)
{
    gint int_value = -1;

    sna->error = NULL;

    if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
	SNA_ERROR(1, _("%s (error #%d)"), msg->reason_phrase,
		  msg->status_code);
	goto bad;
    }

    GVariant *value = soup_xmlrpc_parse_response(msg->response_body->data,
                                                 msg->response_body->length,
                                                 "h", NULL);
    if (!value) {
	SNA_ERROR(2, _("Could not parse XML-RPC response"));
	goto bad;
    }

    int_value = g_variant_get_int32(value);
    g_variant_unref(value);

bad:
    return int_value;
}

static gchar *_soup_get_xmlrpc_value_string(SoupMessage * msg,
					    SyncNetAction * sna)
{
    gchar *string = NULL;

    sna->error = NULL;

    if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
	SNA_ERROR(1, _("%s (error #%d)"), msg->reason_phrase,
		  msg->status_code);
	goto bad;
    }

    GVariant *value = soup_xmlrpc_parse_response(msg->response_body->data,
                                                 msg->response_body->length,
                                                 "s", NULL);
    if (!value) {
	SNA_ERROR(2, _("Could not parse XML-RPC response"));
	goto bad;
    }

    string = g_strdup(g_variant_get_string(value, NULL));
    g_variant_unref(value);

bad:
    return string;
}

static gboolean _soup_xmlrpc_call(gchar * method, SyncNetAction * sna,
				  SoupSessionCallback callback)
{
    SoupMessage *msg;

    sna->error = NULL;

    msg = soup_xmlrpc_message_new(XMLRPC_SERVER_URI, method, NULL, NULL);
    if (!msg)
	return FALSE;

    DEBUG("calling xmlrpc method %s", method);

    soup_session_queue_message(session, msg, callback, sna);
    g_main_run(loop);

    return TRUE;
}

static gboolean _soup_xmlrpc_call_with_parameters(gchar * method,
						  SyncNetAction * sna,
						  SoupSessionCallback
						  callback, ...)
{
    SoupMessage *msg;
    GVariantBuilder builder;
    GVariant *parameters;
    gchar *argument, *body;
    va_list ap;

    sna->error = NULL;

    msg = soup_message_new("POST", XMLRPC_SERVER_URI);

    DEBUG("calling xmlrpc method %s", method);
    if (!msg)
	return FALSE;

    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
    va_start(ap, callback);
    while ((argument = va_arg(ap, gchar *))) {
        g_variant_builder_add(&builder, "s", argument);
	DEBUG("with parameter: %s", argument);
    }
    va_end(ap);
    parameters = g_variant_builder_end(&builder);
    g_variant_builder_unref(&builder);

    body = soup_xmlrpc_build_request(method, parameters, NULL);
    g_variant_unref(parameters);
    if (body) {
        soup_message_set_request(msg, "text/xml",
                                 SOUP_MEMORY_TAKE, body, strlen(body));

        soup_session_queue_message(session, msg, callback, sna);
        g_main_run(loop);

        return TRUE;
    }

    return FALSE;
}

static void _action_check_api_version_got_response(SoupSession * session,
						   SoupMessage * msg,
						   gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;
    gint version = _soup_get_xmlrpc_value_int(msg, sna);

    if (version != XMLRPC_SERVER_API_VERSION) {
	SNA_ERROR(5, _("Server says it supports API version %d, but "
		  "this version of HardInfo only supports API "
		  "version %d."), version, XMLRPC_SERVER_API_VERSION);
    }

    g_main_quit(loop);
}

static gboolean _action_check_api_version(SyncDialog * sd,
					  gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;

    if (!_soup_xmlrpc_call("server.getAPIVersion", sna,
			   _action_check_api_version_got_response))
	return FALSE;

    return sna->error ? FALSE : TRUE;
}

static void _action_call_function_got_response(SoupSession * session,
					       SoupMessage * msg,
					       gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;
    gchar *string;

    if ((string = _soup_get_xmlrpc_value_string(msg, sna)) &&
	sna->entry->save_to) {
	DEBUG("received string: %s\n", string);
	gchar *filename = g_build_filename(g_get_user_config_dir(), "hardinfo",
					   sna->entry->save_to, NULL);

	DEBUG("saving to %s", filename);

	g_file_set_contents(filename, string, -1, NULL);
	g_free(filename);
    }

    if (sna->entry->callback) {
	sna->entry->callback(sna->entry, string);
    }

    g_free(string);
    g_main_quit(loop);
}

static gboolean _action_call_function(SyncDialog * sd, gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;

    if (sna->entry) {
	gchar *str_data = NULL;

	if (sna->entry->get_data)
	    str_data = sna->entry->get_data();

	if (!_soup_xmlrpc_call_with_parameters("sync.callFunctionEx", sna,
					       _action_call_function_got_response,
					       VERSION, ARCH,
					       sna->entry->name,
					       str_data, NULL)) {
	    g_free(str_data);

	    return FALSE;
	}

	g_free(str_data);
    }

    return sna->error ? FALSE : TRUE;
}

static gboolean _cancel_sync(GtkWidget * widget, gpointer data)
{
    SyncDialog *sd = (SyncDialog *) data;

    if (session) {
	soup_session_abort(session);
    }

    sd->flag_cancel = TRUE;
    g_main_quit(loop);

    gtk_widget_set_sensitive(widget, FALSE);

    return FALSE;
}

static SyncNetAction *sync_manager_get_selected_actions(gint * n)
{
    gint i;
    GSList *entry;
    SyncNetAction *actions;
    SyncNetAction
      action_check_api = { _("Contacting HardInfo Central Database"), _action_check_api_version },
      action_clean_up = { _("Cleaning up"), NULL};

    actions = g_new0(SyncNetAction, 2 + g_slist_length(entries));

    for (entry = entries, i = 1; entry; entry = entry->next) {
	SyncEntry *e = (SyncEntry *) entry->data;

	if (e->selected) {
	    SyncNetAction sna =
		{ e->fancy_name, _action_call_function, e };

	    actions[i++] = sna;
	}
    }

    actions[0] = action_check_api;
    actions[i++] = action_clean_up;

    *n = i;
    return actions;
}

static SoupURI *sync_manager_get_proxy(void)
{
    const gchar *conf;

    if (!(conf = g_getenv("HTTP_PROXY"))) {
	if (!(conf = g_getenv("http_proxy"))) {
	    return NULL;
	}
    }

    return soup_uri_new(conf);
}

static void sync_dialog_start_sync(SyncDialog * sd)
{
    gint nactions;
    SyncNetAction *actions;

    if (!session) {
	SoupURI *proxy = sync_manager_get_proxy();

	session = soup_session_new_with_options(SOUP_SESSION_TIMEOUT, 10,
						SOUP_SESSION_PROXY_URI,
						proxy, NULL);
	/* Crashes if we unref the proxy? O_o */
	/*if (proxy)
	   g_object_unref(proxy); */
    }

    loop = g_main_loop_new(NULL, TRUE);

    gtk_widget_hide(sd->button_sync);
    sync_dialog_netarea_show(sd);
    g_signal_connect(G_OBJECT(sd->button_cancel), "clicked",
		     (GCallback) _cancel_sync, sd);

    actions = sync_manager_get_selected_actions(&nactions);
    sync_dialog_netarea_start_actions(sd, actions, nactions);
    g_free(actions);

    if (sd->flag_cancel) {
	gtk_widget_hide(sd->button_cancel);
	gtk_widget_show(sd->button_close);

	/* wait for the user to close the dialog */
	g_main_run(loop);
    }

    g_main_loop_unref(loop);
}

static void sync_dialog_netarea_start_actions(SyncDialog * sd,
					      SyncNetAction sna[], gint n)
{
    gint i;
    GtkWidget **labels;
    GtkWidget **status_labels;
    const gchar *done_str  = "\342\234\223";
    const gchar *error_str = "\342\234\227";
    const gchar *curr_str  = "\342\226\266";
    const gchar *empty_str = "\302\240\302\240";

    labels = g_new0(GtkWidget *, n);
    status_labels = g_new0(GtkWidget *, n);

    for (i = 0; i < n; i++) {
	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 5);

	labels[i] = gtk_label_new(sna[i].name);
	status_labels[i] = gtk_label_new(empty_str);

	gtk_label_set_use_markup(GTK_LABEL(labels[i]), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(status_labels[i]), TRUE);

	gtk_misc_set_alignment(GTK_MISC(labels[i]), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(status_labels[i]), 1.0, 0.5);

	gtk_box_pack_start(GTK_BOX(hbox), status_labels[i], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), labels[i], TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(sd->sna->vbox), hbox, FALSE, FALSE, 3);

	gtk_widget_show_all(hbox);
    }

    while (gtk_events_pending())
	gtk_main_iteration();

    for (i = 0; i < n; i++) {
	gchar *markup;

	if (sd->flag_cancel) {
	    markup =
		g_strdup_printf("<s>%s</s> <i>%s</i>",
				sna[i].name, _("(canceled)"));
	    gtk_label_set_markup(GTK_LABEL(labels[i]), markup);
	    g_free(markup);

	    gtk_label_set_markup(GTK_LABEL(status_labels[i]), error_str);
	    break;
	}

	markup = g_strdup_printf("<b>%s</b>", sna[i].name);
	gtk_label_set_markup(GTK_LABEL(labels[i]), markup);
	g_free(markup);

        gtk_label_set_markup(GTK_LABEL(status_labels[i]), curr_str);

	if (sna[i].do_action && !sna[i].do_action(sd, &sna[i])) {
	    markup =
		g_strdup_printf("<b><s>%s</s></b> <i>%s</i>",
				sna[i].name, _("(failed)"));
	    gtk_label_set_markup(GTK_LABEL(labels[i]), markup);
	    g_free(markup);

	    sd->flag_cancel = TRUE;

	    gtk_label_set_markup(GTK_LABEL(status_labels[i]), error_str);
	    if (sna[i].error) {
		if (sna[i].error->code != 1) {
		    /* the user has not cancelled something... */
		    g_warning
			(_("Failed while performing \"%s\". Please file a bug report "
			 "if this problem persists. (Use the Help\342\206\222Report"
			 " bug option.)\n\nDetails: %s"), sna[i].name,
			 sna[i].error->message);
		}

		g_error_free(sna[i].error);
	    } else {
		g_warning
		    (_("Failed while performing \"%s\". Please file a bug report "
		     "if this problem persists. (Use the Help\342\206\222Report"
		     " bug option.)"), sna[i].name);
	    }
	    break;
	}

        gtk_label_set_markup(GTK_LABEL(status_labels[i]), done_str);
	gtk_label_set_markup(GTK_LABEL(labels[i]), sna[i].name);
    }

    g_free(labels);
    g_free(status_labels);
}

static SyncNetArea *sync_dialog_netarea_new(void)
{
    SyncNetArea *sna = g_new0(SyncNetArea, 1);

    sna->vbox = gtk_vbox_new(FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(sna->vbox), 10);

    gtk_widget_show_all(sna->vbox);
    gtk_widget_hide(sna->vbox);

    return sna;
}

static void sync_dialog_netarea_destroy(SyncNetArea * sna)
{
    g_return_if_fail(sna != NULL);

    g_free(sna);
}

static void sync_dialog_netarea_show(SyncDialog * sd)
{
    g_return_if_fail(sd && sd->sna);

    gtk_widget_hide(GTK_WIDGET(sd->scroll_box));
    gtk_widget_show(GTK_WIDGET(sd->sna->vbox));

    gtk_label_set_markup(GTK_LABEL(sd->label), LABEL_SYNC_SYNCING);
    gtk_window_set_default_size(GTK_WINDOW(sd->dialog), 0, 0);
    gtk_window_reshow_with_initial_size(GTK_WINDOW(sd->dialog));
}

#if 0
static void sync_dialog_netarea_hide(SyncDialog * sd)
{
    g_return_if_fail(sd && sd->sna);

    gtk_widget_show(GTK_WIDGET(sd->scroll_box));
    gtk_widget_hide(GTK_WIDGET(sd->sna->vbox));

    gtk_label_set_markup(GTK_LABEL(sd->label), LABEL_SYNC_DEFAULT);
    gtk_window_reshow_with_initial_size(GTK_WINDOW(sd->dialog));
}
#endif

static void populate_store(GtkListStore * store)
{
    GSList *entry;
    SyncEntry *e;

    gtk_list_store_clear(store);

    for (entry = entries; entry; entry = entry->next) {
	GtkTreeIter iter;

	e = (SyncEntry *) entry->data;

	e->selected = TRUE;

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, TRUE, 1, e->fancy_name, 2, e,
			   -1);
    }
}

static void
sel_toggle(GtkCellRendererToggle * cellrenderertoggle,
	   gchar * path_str, GtkTreeModel * model)
{
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    SyncEntry *se;
    gboolean active;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, 0, &active, 2, &se, -1);

    se->selected = !active;

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, se->selected, -1);
    gtk_tree_path_free(path);
}

static void close_clicked(void)
{
    g_main_quit(loop);
}

static SyncDialog *sync_dialog_new(GtkWidget *parent)
{
    SyncDialog *sd;
    GtkWidget *dialog;
    GtkWidget *dialog1_vbox;
    GtkWidget *scrolledwindow2;
    GtkWidget *treeview2;
    GtkWidget *dialog1_action_area;
    GtkWidget *button8;
    GtkWidget *button7;
    GtkWidget *button6;
    GtkWidget *label;
    GtkWidget *hbox;

    GtkTreeViewColumn *column;
    GtkTreeModel *model;
    GtkListStore *store;
    GtkCellRenderer *cr_text, *cr_toggle;

    sd = g_new0(SyncDialog, 1);
    sd->sna = sync_dialog_netarea_new();

    dialog = gtk_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_title(GTK_WINDOW(dialog), _("Network Updater"));
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_icon(GTK_WINDOW(dialog),
			icon_cache_get_pixbuf("syncmanager.png"));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
    gtk_window_set_position(GTK_WINDOW(dialog),
			    GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

#if GTK_CHECK_VERSION(2, 14, 0)
    dialog1_vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
    dialog1_vbox = GTK_DIALOG(dialog)->vbox;
#endif
    gtk_box_set_spacing(GTK_BOX(dialog1_vbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(dialog1_vbox), 4);
    gtk_widget_show(dialog1_vbox);
#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
    hbox = gtk_hbox_new(FALSE, 5);
#endif
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(LABEL_SYNC_DEFAULT);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
#endif

    gtk_box_pack_start(GTK_BOX(hbox),
		       icon_cache_get_image("syncmanager.png"),
		       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show_all(hbox);


    gtk_box_pack_start(GTK_BOX(dialog1_vbox), sd->sna->vbox, TRUE, TRUE,
		       0);

    scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow2);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), scrolledwindow2, TRUE, TRUE,
		       0);
    gtk_widget_set_size_request(scrolledwindow2, -1, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scrolledwindow2), GTK_SHADOW_IN);

    store =
	gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING,
			   G_TYPE_POINTER);
    model = GTK_TREE_MODEL(store);

    treeview2 = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview2), FALSE);
    gtk_widget_show(treeview2);
    gtk_container_add(GTK_CONTAINER(scrolledwindow2), treeview2);

    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), column);

    cr_toggle = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(column, cr_toggle, FALSE);
    g_signal_connect(cr_toggle, "toggled", G_CALLBACK(sel_toggle), model);
    gtk_tree_view_column_add_attribute(column, cr_toggle, "active", 0);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, TRUE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup", 1);

    populate_store(store);

#if GTK_CHECK_VERSION(2, 14, 0)
    dialog1_action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
#else
    dialog1_action_area = GTK_DIALOG(dialog)->action_area;
#endif
    gtk_widget_show(dialog1_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog1_action_area),
			      GTK_BUTTONBOX_END);

    button8 = gtk_button_new_with_mnemonic(_("_Cancel"));
    gtk_widget_show(button8);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button8,
				 GTK_RESPONSE_CANCEL);
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_default(button8, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);
#endif
    button7 = gtk_button_new_with_mnemonic(_("_Synchronize"));
    gtk_widget_show(button7);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button7,
				 GTK_RESPONSE_ACCEPT);
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_default(button7, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);
#endif
    button6 = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect(G_OBJECT(button6), "clicked",
		     (GCallback) close_clicked, NULL);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button6,
				 GTK_RESPONSE_ACCEPT);
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_default(button6, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(button6, GTK_CAN_DEFAULT);
#endif

    sd->dialog = dialog;
    sd->button_sync = button7;
    sd->button_cancel = button8;
    sd->button_close = button6;
    sd->scroll_box = scrolledwindow2;
    sd->label = label;

    return sd;
}

static void sync_dialog_destroy(SyncDialog * sd)
{
    gtk_widget_destroy(sd->dialog);
    sync_dialog_netarea_destroy(sd->sna);
    g_free(sd);
}
#endif				/* HAS_LIBSOUP */
