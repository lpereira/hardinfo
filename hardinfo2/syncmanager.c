#include "syncmanager.h"
#include "iconcache.h"
#include "hardinfo.h"
#include "config.h"

#include <libsoup/soup.h>
#include <libsoup/soup-xmlrpc-message.h>
#include <libsoup/soup-xmlrpc-response.h>

typedef struct _SyncDialog	SyncDialog;
typedef struct _SyncNetArea	SyncNetArea;
typedef struct _SyncNetAction	SyncNetAction;

struct _SyncNetArea {
    GtkWidget *vbox;
};

struct _SyncNetAction {
    gchar	*name;
    gboolean	(*do_action)(SyncDialog *sd, gpointer sna);
    
    GError	*error;
};

struct _SyncDialog {
    GtkWidget		*dialog;
    GtkWidget		*label;
    
    GtkWidget		*button_sync;
    GtkWidget		*button_cancel;
    GtkWidget		*button_close;
    
    GtkWidget		*scroll_box;
    
    SyncNetArea		*sna;
    
    gboolean		 flag_cancel : 1;
};

static GSList		*entries = NULL;
static SoupSession	*session = NULL;
static GQuark		 err_quark;

#define XMLRPC_SERVER_URI   		"http://hardinfo.berlios.de/xmlrpc/"
#define XMLRPC_SERVER_API_VERSION	1

#define LABEL_SYNC_DEFAULT  "<big><b>Synchronize with Central Database</b></big>\n" \
                            "The following information may be synchronized " \
                            "with the HardInfo central database. <i>No information " \
                            "that ultimately identify this computer will be " \
                            "sent.</i>"
#define LABEL_SYNC_SYNCING  "<big><b>Synchronizing</b></big>\n" \
                            "This may take some time."

static SyncDialog	*sync_dialog_new(void);
static void		 sync_dialog_destroy(SyncDialog *sd);
static void		 sync_dialog_start_sync(SyncDialog *sd);

static SyncNetArea	*sync_dialog_netarea_new(void);
static void		 sync_dialog_netarea_destroy(SyncNetArea *sna);
static void		 sync_dialog_netarea_show(SyncDialog *sd);
static void		 sync_dialog_netarea_hide(SyncDialog *sd);
static void		 sync_dialog_netarea_start_actions(SyncDialog *sd, SyncNetAction *sna,
                                                           gint n);

void sync_manager_add_entry(SyncEntry *entry)
{
    entries = g_slist_prepend(entries, entry);
}

void sync_manager_show(void)
{
    SyncDialog *sd = sync_dialog_new();
    
    err_quark = g_quark_from_static_string("syncmanager");

    if (gtk_dialog_run(GTK_DIALOG(sd->dialog)) == GTK_RESPONSE_ACCEPT) {
        sync_dialog_start_sync(sd);
    }   

    sync_dialog_destroy(sd);
}

static SoupXmlrpcValue *_soup_get_xmlrpc_value(SoupMessage *msg, SyncNetAction *sna)
{
    SoupXmlrpcResponse *response = NULL;
    SoupXmlrpcValue *value = NULL;
    
    sna->error = NULL;
    
    if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
        sna->error = g_error_new(err_quark, 1, "%s (error #%d)",
                                 msg->reason_phrase, msg->status_code);
        goto bad;
    }
    
    response = soup_xmlrpc_message_parse_response(SOUP_XMLRPC_MESSAGE(msg));
    if (!response) {
        sna->error = g_error_new(err_quark, 2, "Could not parse XMLRPC response");
        goto bad;
    }
    
    value = soup_xmlrpc_response_get_value(response);
    if (!value) {
        sna->error = g_error_new(err_quark, 3, "No response value in XMLRPC response");
    }
    
bad:
    return value;
}

static long _soup_get_xmlrpc_value_int(SoupMessage *msg, SyncNetAction *sna)
{
    SoupXmlrpcValue *value;
    long int_value = -1;
    
    value = _soup_get_xmlrpc_value(msg, sna);
    if (!value) {
        if (!sna->error)
            sna->error = GINT_TO_POINTER(1);
        goto very_bad;
    }

    if (!soup_xmlrpc_value_get_int(value, &int_value)) {
        if (!sna->error)
            sna->error = g_error_new(err_quark, 4,
                                     "Could not extract result from XMLRPC response");
            
        goto bad;
    }
    
bad:
    /* FIXME: free value? */
very_bad:
    return int_value;
}

static gboolean _soup_xmlrpc_call(gchar *method, SyncNetAction *sna,
                                  SoupMessageCallbackFn callback)
{
    SoupXmlrpcMessage *msg;
    
    sna->error = NULL;
    
    msg = soup_xmlrpc_message_new(XMLRPC_SERVER_URI);
    if (!msg)
        return FALSE;
        
    soup_xmlrpc_message_start_call(msg, method);
    soup_xmlrpc_message_end_call(msg);
    
    soup_xmlrpc_message_persist(msg);
    
    soup_session_queue_message(session, SOUP_MESSAGE(msg),
                               callback, sna);
    gtk_main();
    
    return TRUE;
}

static void _action_check_api_version_got_response(SoupMessage *msg, gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;
    long version = _soup_get_xmlrpc_value_int(msg, sna);
    
    if (version != XMLRPC_SERVER_API_VERSION) {
        if (!sna->error)
            sna->error = g_error_new(err_quark, 5, "Server says it supports API version %ld, but " \
                                                   "this version of HardInfo only supports API "  \
                                                   "version %d.", version, XMLRPC_SERVER_API_VERSION);
    }

    gtk_main_quit();
}

static gboolean _action_check_api_version(SyncDialog *sd, gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;

    if (!_soup_xmlrpc_call("server.getAPIVersion", sna,
                           _action_check_api_version_got_response))
        return FALSE;
    
    return sna->error ? FALSE : TRUE;
}

static void _action_get_benchmarks_got_response(SoupMessage *msg, gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;
    
    _soup_get_xmlrpc_value_int(msg, sna);

    gtk_main_quit();
}

static gboolean _action_get_benchmarks(SyncDialog *sd, gpointer user_data)
{
    SyncNetAction *sna = (SyncNetAction *) user_data;

    if (!_soup_xmlrpc_call("benchmark.getResults", sna,
                           _action_get_benchmarks_got_response))
        return FALSE;
    
    return sna->error ? FALSE : TRUE;
}

static gboolean _cancel_sync(GtkWidget *widget, gpointer data)
{
    SyncDialog *sd = (SyncDialog *) data;
    
    if (session) {
        soup_session_abort(session);
    }
    
    sd->flag_cancel = TRUE;
    gtk_main_quit();
    
    return FALSE;
}

static void sync_dialog_start_sync(SyncDialog *sd)
{
    SyncNetAction actions[] = {
        { "Contacting HardInfo Central Database",	_action_check_api_version },
        { "Receiving benchmark results",		_action_get_benchmarks },
        { "Sync finished",				NULL },
    };
    
    if (!session) {
        session = soup_session_async_new_with_options(SOUP_SESSION_TIMEOUT, 5, NULL);
    }

    gtk_widget_hide(sd->button_sync);
    sync_dialog_netarea_show(sd);
    g_signal_connect(G_OBJECT(sd->button_cancel), "clicked",
                     (GCallback)_cancel_sync, sd);

    sync_dialog_netarea_start_actions(sd, actions, G_N_ELEMENTS(actions));
    
    gtk_widget_hide(sd->button_cancel);
    gtk_widget_show(sd->button_close);

    /* wait for the user to close the dialog */    
    gtk_main();
}

static void sync_dialog_netarea_start_actions(SyncDialog *sd, SyncNetAction sna[], gint n)
{
    gint i;
    GtkWidget **labels;
    GtkWidget **icons;
    GdkPixbuf *done_icon = icon_cache_get_pixbuf("status-done.png");
    GdkPixbuf *curr_icon = icon_cache_get_pixbuf("status-curr.png");
    
    labels = g_new0(GtkWidget *, n);
    icons  = g_new0(GtkWidget *, n);
    
    for (i = 0; i < n; i++) {
        GtkWidget *hbox;
        
        hbox = gtk_hbox_new(FALSE, 5);
        
        labels[i] = gtk_label_new(sna[i].name);
        icons[i]  = gtk_image_new();
        
        gtk_widget_set_size_request(icons[i],
                                    gdk_pixbuf_get_width(done_icon),
                                    gdk_pixbuf_get_height(done_icon));

        gtk_label_set_use_markup(GTK_LABEL(labels[i]), TRUE);
        gtk_misc_set_alignment(GTK_MISC(labels[i]), 0.0, 0.5);

        gtk_box_pack_start(GTK_BOX(hbox), icons[i], FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), labels[i], TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(sd->sna->vbox), hbox, FALSE, FALSE, 3);
        
        gtk_widget_show_all(hbox);
    }
    
    while (gtk_events_pending())
        gtk_main_iteration();
    
    for (i = 0; i < n; i++) {
        gchar *markup;
        
        if (sd->flag_cancel) {
            markup = g_strdup_printf("<s>%s</s> <i>(canceled)</i>", sna[i].name);
            gtk_label_set_markup(GTK_LABEL(labels[i]), markup);
            g_free(markup);

            gtk_image_set_from_pixbuf(GTK_IMAGE(icons[i]),
                                      icon_cache_get_pixbuf("dialog-error.png"));
            break;
        }
        
        markup = g_strdup_printf("<b>%s</b>", sna[i].name);
        gtk_label_set_markup(GTK_LABEL(labels[i]), markup);
        g_free(markup);
        
        gtk_image_set_from_pixbuf(GTK_IMAGE(icons[i]), curr_icon);
    
        if (sna[i].do_action && !sna[i].do_action(sd, &sna[i])) {
            markup = g_strdup_printf("<b><s>%s</s></b> <i>(failed)</i>", sna[i].name);
            gtk_label_set_markup(GTK_LABEL(labels[i]), markup);
            g_free(markup);

            gtk_image_set_from_pixbuf(GTK_IMAGE(icons[i]),
                                      icon_cache_get_pixbuf("dialog-error.png"));
            if (sna[i].error) {
                g_warning("Failed while performing \"%s\". Please file a bug report " \
                          "if this problem persists. (Use the Help\342\206\222Report" \
                          " bug option.)\n\nDetails: %s",
                          sna[i].name, sna[i].error->message);
                g_error_free(sna[i].error);
            } else {
                g_warning("Failed while performing \"%s\". Please file a bug report " \
                          "if this problem persists. (Use the Help\342\206\222Report" \
                          " bug option.)", sna[i].name);
            }
            break;
        }

        gtk_image_set_from_pixbuf(GTK_IMAGE(icons[i]), done_icon);
        gtk_label_set_markup(GTK_LABEL(labels[i]), sna[i].name);
    }
    
    g_free(labels);
    g_free(icons);
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

static void sync_dialog_netarea_destroy(SyncNetArea *sna)
{
    g_return_if_fail(sna != NULL);

    g_free(sna);
}

static void sync_dialog_netarea_show(SyncDialog *sd)
{
    g_return_if_fail(sd && sd->sna);
    
    gtk_widget_hide(GTK_WIDGET(sd->scroll_box));
    gtk_widget_show(GTK_WIDGET(sd->sna->vbox));
    
    gtk_label_set_markup(GTK_LABEL(sd->label), LABEL_SYNC_SYNCING);
    gtk_window_set_default_size(GTK_WINDOW(sd->dialog), 0, 0);
    gtk_window_reshow_with_initial_size(GTK_WINDOW(sd->dialog));
}

static void sync_dialog_netarea_hide(SyncDialog *sd)
{
    g_return_if_fail(sd && sd->sna);
    
    gtk_widget_show(GTK_WIDGET(sd->scroll_box));
    gtk_widget_hide(GTK_WIDGET(sd->sna->vbox));

    gtk_label_set_markup(GTK_LABEL(sd->label), LABEL_SYNC_DEFAULT);
    gtk_window_reshow_with_initial_size(GTK_WINDOW(sd->dialog));
}

static SyncDialog *sync_dialog_new(void)
{
    SyncDialog *sd;
    GtkWidget *dialog;
    GtkWidget *dialog1_vbox;
    GtkWidget *scrolledwindow2;
//    GtkWidget *treeview2;
    GtkWidget *dialog1_action_area;
    GtkWidget *button8;
    GtkWidget *button7;
    GtkWidget *button6;
    GtkWidget *label;
    GtkWidget *hbox;
    
//    GtkTreeViewColumn *column;
//    GtkCellRenderer *cr_text, *cr_pbuf, *cr_toggle; 
    
    sd = g_new0(SyncDialog, 1);
    sd->sna = sync_dialog_netarea_new();

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "SyncManager");
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
//    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);

    dialog1_vbox = GTK_DIALOG(dialog)->vbox;
    gtk_box_set_spacing(GTK_BOX(dialog1_vbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(dialog1_vbox), 4);
    gtk_widget_show(dialog1_vbox);
    
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbox, FALSE, FALSE, 0);
    
    label = gtk_label_new(LABEL_SYNC_DEFAULT);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    
    gtk_box_pack_start(GTK_BOX(hbox),
                       icon_cache_get_image("syncmanager.png"),
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show_all(hbox);

    gtk_box_pack_start(GTK_BOX(dialog1_vbox), sd->sna->vbox, TRUE, TRUE, 0);

    scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow2);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), scrolledwindow2, TRUE, TRUE, 0);
    gtk_widget_set_size_request(scrolledwindow2, -1, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scrolledwindow2), GTK_SHADOW_IN);
/*
    treeview2 = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview2), FALSE);
    gtk_widget_show(treeview2);
    gtk_container_add(GTK_CONTAINER(scrolledwindow2), treeview2);
    
    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), column);

    cr_toggle = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(column, cr_toggle, FALSE);
    g_signal_connect(cr_toggle, "toggled", G_CALLBACK(report_dialog_sel_toggle), sd);
    gtk_tree_view_column_add_attribute(column, cr_toggle, "active",
				       TREE_COL_SEL);

    cr_pbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, cr_pbuf, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_pbuf, "pixbuf",
				       TREE_COL_PBUF);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, TRUE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       TREE_COL_NAME);
  */  

    dialog1_action_area = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog1_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog1_action_area),
			      GTK_BUTTONBOX_END);

    button8 = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_widget_show(button8);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button8,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);

    button7 = gtk_button_new_with_mnemonic("_Synchronize");
    gtk_widget_show(button7);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button7,
				 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);

    button6 = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect(G_OBJECT(button6), "clicked", (GCallback)gtk_main_quit, NULL);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button6,
				 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(button6, GTK_CAN_DEFAULT);

    sd->dialog	= dialog;
    sd->button_sync = button7;
    sd->button_cancel = button8;
    sd->button_close = button6;
    sd->scroll_box = scrolledwindow2;
    sd->label = label;
    //gtk_tree_view_collapse_all(GTK_TREE_VIEW(treeview2));
    //set_all_active(sd, TRUE);

    return sd;
}

static void sync_dialog_destroy(SyncDialog *sd)
{
    gtk_widget_destroy(sd->dialog);
    sync_dialog_netarea_destroy(sd->sna);
    g_free(sd);
}
