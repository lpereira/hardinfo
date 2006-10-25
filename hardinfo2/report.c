/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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
 
#include <report.h>
#include <stdio.h>
#include <string.h>
#include <shell.h>
#include <hardinfo.h>

static ReportDialog	*report_dialog_new(GtkTreeModel *model, GtkWidget *parent);
static void		 set_all_active(ReportDialog *rd, gboolean setting);

static FileTypes file_types[] = {
  { "HTML (*.html)",		"text/html",	".html",	report_context_html_new },
  { "Plain Text (*.txt)",	"text/plain",	".txt",		report_context_text_new },
  { NULL,			NULL,		NULL,		NULL }
};


void
report_header(ReportContext *ctx)
{
    ctx->header(ctx);
}

void
report_footer(ReportContext *ctx)
{
    ctx->footer(ctx);
}

void
report_title(ReportContext *ctx, gchar *text)
{
    ctx->title(ctx, text);
}

void
report_subtitle(ReportContext *ctx, gchar *text)
{
    ctx->subtitle(ctx, text);
}

void
report_subsubtitle(ReportContext *ctx, gchar *text)
{
    ctx->subsubtitle(ctx, text);
}

void
report_key_value(ReportContext *ctx, gchar *key, gchar *value)
{
    ctx->keyvalue(ctx, key, value);
}

void
report_table(ReportContext *ctx, gchar *text)
{
    GKeyFile	 *key_file = g_key_file_new();
    gchar	**groups;
    gint          i;

    g_key_file_load_from_data(key_file, text, strlen(text), 0, NULL);
    groups = g_key_file_get_groups(key_file, NULL);
    
    for (i = 0; groups[i]; i++) {
	gchar   *group, *tmpgroup;
	gchar **keys;
	gint    j;
	
	if (groups[i][0] == '$')
	    continue;
	
        group = groups[i];
        keys = g_key_file_get_keys(key_file, group, NULL, NULL);

        tmpgroup = g_strdup(group);
        strend(group, '#');

        report_subsubtitle(ctx, group);
	
	for (j = 0; keys[j]; j++) {
            gchar *key = keys[j];
            gchar *value;

            value = g_key_file_get_value(key_file, tmpgroup, key, NULL);
       
            if (g_utf8_validate(key, -1, NULL) && g_utf8_validate(value, -1, NULL)) {
                    strend(key, '#');

                    if (g_str_equal(value, "...")) {
                        g_free(value);
                        if (!(value = ctx->entry->fieldfunc(key))) {
                            value = g_strdup("...");
                        }
                    }

                    if (*key == '$') {
                        report_key_value(ctx, strchr(key + 1, '$') + 1, value);
                    } else {
                        report_key_value(ctx, key, value);
                    }
                                        
            }

            g_free(value);
	}
	
	g_free(tmpgroup);
	g_strfreev(keys);
    }

    g_strfreev(groups);
    g_key_file_free(key_file);
}

static void
report_html_header(ReportContext *ctx)
{
    if (ctx->output)
        g_free(ctx->output);
    
    ctx->output = g_strdup("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Final//EN\">\n" \
                           "<html><head>\n" \
              	           "<title>HardInfo System Report</title>\n" \
                           "<style>\n" \
        	           "body    { background: #fff }\n" \
        	           ".title  { font: bold 130%% serif; color: #0066FF; padding: 30px 0 10px 0 }\n" \
        	           ".stitle { font: bold 100%% sans-serif; color: #0044DD; padding: 30px 0 10px 0 }\n" \
        	           ".sstitle{ font: bold 80%% serif; color: #000000; background: #efefef }\n" \
        	           ".field  { font: 80%% sans-serif; color: #000000; padding: 2px; padding-left: 50px }\n" \
        	           ".value  { font: 80%% sans-serif; color: #505050 }\n" \
        	           "</style>\n" \
        	           "</head><body>\n" \
                           "<table width=\"100%%\"><tbody>");
}

static void
report_html_footer(ReportContext *ctx)
{
    ctx->output = g_strconcat(ctx->output,
                              "</tbody></table></body></html>",
                              NULL);
}

static void
report_html_title(ReportContext *ctx, gchar *text)
{
    ctx->output = g_strdup_printf("%s" \
                                  "<tr><td colspan=\"2\" class=\"titl" \
                                  "e\">%s</td></tr>\n",
                                  ctx->output, text);
}

static void
report_html_subtitle(ReportContext *ctx, gchar *text)
{
    ctx->output = g_strdup_printf("%s" \
                                  "<tr><td colspan=\"2\" class=\"stit" \
                                  "le\">%s</td></tr>\n",
                                  ctx->output, text);
}

static void
report_html_subsubtitle(ReportContext *ctx, gchar *text)
{
    ctx->output = g_strdup_printf("%s" \
                                  "<tr><td colspan=\"2\" class=\"ssti" \
                                  "tle\">%s</td></tr>\n",
                                  ctx->output, text);
}

static void
report_html_key_value(ReportContext *ctx, gchar *key, gchar *value)
{
    ctx->output = g_strdup_printf("%s" \
                                  "<tr><td class=\"field\">%s</td>" \
                                  "<td class=\"value\">%s</td></tr>\n",
                                  ctx->output, key, value);
}

static void
report_text_header(ReportContext *ctx)
{
    if (ctx->output)
        g_free(ctx->output);
        
    ctx->output = g_strdup("");
}

static void
report_text_footer(ReportContext *ctx)
{
}

static void
report_text_title(ReportContext *ctx, gchar *text)
{
    gchar *str = (gchar*)ctx->output;
    int i = strlen(text);
    
    str = g_strdup_printf("%s\n%s\n", str, text);
    for (; i; i--)
        str = g_strconcat(str, "*", NULL);
        
    str = g_strconcat(str, "\n\n", NULL);
    ctx->output = str;
}

static void
report_text_subtitle(ReportContext *ctx, gchar *text)
{
    gchar *str = ctx->output;
    int i = strlen(text);
    
    str = g_strdup_printf("%s\n%s\n", str, text);
    for (; i; i--)
        str = g_strconcat(str, "-", NULL);
        
    str = g_strconcat(str, "\n\n", NULL);
    ctx->output = str;
}

static void
report_text_subsubtitle(ReportContext *ctx, gchar *text)
{
    ctx->output = g_strdup_printf("%s-%s-\n", ctx->output, text);
}

static void
report_text_key_value(ReportContext *ctx, gchar *key, gchar *value)
{
    if (strlen(value))
        ctx->output = g_strdup_printf("%s%s\t\t: %s\n", ctx->output, key, value);
    else
        ctx->output = g_strdup_printf("%s%s\n", ctx->output, key);
}

static void
report_generate_child(ReportContext *ctx, GtkTreeIter *iter)
{
    ShellModuleEntry *entry;
    gboolean selected;
    
    gtk_tree_model_get(ctx->rd->model, iter, TREE_COL_SEL, &selected, -1);
    if (!selected)
        return;
    
    gtk_tree_model_get(ctx->rd->model, iter, TREE_COL_DATA, &entry, -1);
    
    ctx->entry = entry;

    report_subtitle(ctx, entry->name);
    report_table(ctx, entry->func(entry->number));
}

static void
report_generate_children(ReportContext *ctx, GtkTreeIter *iter)
{
    GtkTreeModel *model = ctx->rd->model;
    gchar *name;

    gtk_tree_model_get(model, iter, TREE_COL_NAME, &name, -1);
    report_title(ctx, name);

    if (gtk_tree_model_iter_has_child(model, iter)) {
        gint children = gtk_tree_model_iter_n_children(model, iter);
        gint i;

        for (i = 0; i < children; i++) {
            GtkTreeIter child;
            
            gtk_tree_model_iter_nth_child(model, &child, iter, i);
            report_generate_child(ctx, &child);
        }
    }
}

static gchar *
report_get_filename(void)
{
    GtkWidget *dialog;
    gchar *filename = NULL;

    dialog = gtk_file_chooser_dialog_new("Save File",
	                                 NULL,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);
                                                   
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "hardinfo report");
    
    file_chooser_add_filters(dialog, file_types);
    file_chooser_open_expander(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *ext = file_chooser_get_extension(dialog, file_types);
        filename = file_chooser_build_filename(dialog, ext);        
    }
  
    gtk_widget_destroy (dialog);
    return filename;
}

ReportContext*
report_context_html_new(ReportDialog *rd)
{
    ReportContext *ctx;
    
    ctx = g_new0(ReportContext, 1);
    ctx->header      = report_html_header;
    ctx->footer      = report_html_footer;
    ctx->title       = report_html_title;
    ctx->subtitle    = report_html_subtitle;
    ctx->subsubtitle = report_html_subsubtitle;
    ctx->keyvalue    = report_html_key_value;
    
    ctx->rd     = rd;
    ctx->output = g_strdup("");
    
    return ctx;
}

ReportContext*
report_context_text_new(ReportDialog *rd)
{
    ReportContext *ctx;
    
    ctx = g_new0(ReportContext, 1);
    ctx->header      = report_text_header;
    ctx->footer      = report_text_footer;
    ctx->title       = report_text_title;
    ctx->subtitle    = report_text_subtitle;
    ctx->subsubtitle = report_text_subsubtitle;
    ctx->keyvalue    = report_text_key_value;
    
    ctx->rd     = rd;
    ctx->output = g_strdup("");
    
    return ctx;
}

void
report_context_free(ReportContext *ctx)
{
    g_free(ctx->output);
    g_free(ctx);
}

static gboolean
report_generate(ReportDialog *rd)
{
    GtkTreeIter		 iter;
    ReportContext	*ctx;
    ReportContext	*(*ctx_gen)(ReportDialog *rd);
    gchar		*file;
    FILE		*stream;
    
    if (!(file = report_get_filename()))
        return FALSE;

    if (!(stream = fopen(file, "w+")))
        return FALSE;
    
    ctx_gen = file_types_get_data_by_name(file_types, file);
    if (ctx_gen) {
        ctx = ctx_gen(rd);
        
        report_header(ctx);
        
        gtk_tree_model_get_iter_first(rd->model, &iter);
        do {
             report_generate_children(ctx, &iter);
        } while (gtk_tree_model_iter_next(rd->model, &iter));

        report_footer(ctx);
        
        fputs(ctx->output, stream);
        fclose(stream);
        
        report_context_free(ctx);
        g_free(file);
        
        return TRUE;
    }
    
    g_free(file);
    return FALSE;
}

void
report_dialog_show(GtkTreeModel *model, GtkWidget *parent)
{
    gboolean success;
    ReportDialog *rd = report_dialog_new(model, parent);

    if (gtk_dialog_run(GTK_DIALOG(rd->dialog)) == GTK_RESPONSE_ACCEPT) {
	shell_status_update("Generating report...");
        gtk_widget_hide(rd->dialog);
        shell_view_set_enabled(FALSE);
        shell_status_set_enabled(TRUE);

	success = report_generate(rd);

        shell_status_set_enabled(FALSE);
        
        if (success) 
  	    shell_status_update("Report saved.");
        else
            shell_status_update("Error while creating the report.");
    }

    set_all_active(rd, FALSE);
    gtk_widget_destroy(rd->dialog);
    g_free(rd);
}

static void
set_children_active(GtkTreeModel *model, GtkTreeIter *iter, gboolean setting)
{
    if (gtk_tree_model_iter_has_child(model, iter)) {
        gint children = gtk_tree_model_iter_n_children(model, iter);

        gtk_tree_store_set(GTK_TREE_STORE(model), iter, TREE_COL_SEL, setting, -1);
        
        for (children--; children >= 0; children--) {
            GtkTreeIter child;
            
            gtk_tree_model_iter_nth_child(model, &child, iter, children);
            gtk_tree_store_set(GTK_TREE_STORE(model), &child, TREE_COL_SEL, setting, -1);
        }
    }
}

static void
set_all_active(ReportDialog *rd, gboolean setting)
{
    GtkTreeIter iter;
    GtkTreeModel *model = rd->model;
    
    gtk_tree_model_get_iter_first(model, &iter);
    
    do {
         set_children_active(model, &iter, setting);
    } while (gtk_tree_model_iter_next(model, &iter));
}

static void
report_dialog_sel_none(GtkWidget *widget, ReportDialog *rd)
{
    set_all_active(rd, FALSE);
}

static void
report_dialog_sel_all(GtkWidget *widget, ReportDialog *rd)
{
    set_all_active(rd, TRUE);
}

static void
report_dialog_sel_toggle(GtkCellRendererToggle *cellrenderertoggle,
			 gchar		       *path_str,
			 ReportDialog          *rd)
{
    GtkTreeModel *model = rd->model;
    GtkTreeIter  iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean active;
    
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, TREE_COL_SEL, &active, -1);
    
    active = !active;
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, TREE_COL_SEL, active, -1);
    set_children_active(model, &iter, active);

    gtk_tree_path_free(path);
}

static ReportDialog
*report_dialog_new(GtkTreeModel *model, GtkWidget *parent)
{
    ReportDialog *rd;
    GtkWidget *dialog;
    GtkWidget *dialog1_vbox;
    GtkWidget *scrolledwindow2;
    GtkWidget *treeview2;
    GtkWidget *hbuttonbox3;
    GtkWidget *button3;
    GtkWidget *button6;
    GtkWidget *dialog1_action_area;
    GtkWidget *button8;
    GtkWidget *button7;
    GtkWidget *label;
    
    GtkTreeViewColumn *column;
    GtkCellRenderer *cr_text, *cr_pbuf, *cr_toggle; 

    rd = g_new0(ReportDialog, 1);

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Generate Report");
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);

    dialog1_vbox = GTK_DIALOG(dialog)->vbox;
    gtk_box_set_spacing(GTK_BOX(dialog1_vbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(dialog1_vbox), 4);
    gtk_widget_show(dialog1_vbox);
    
    label = gtk_label_new("<big><b>Generate Report</b></big>\n" \
                          "Please choose the information that you wish " \
                          "to view in your report:");
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), label, FALSE, FALSE, 0);
    
    scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow2);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), scrolledwindow2, TRUE, TRUE, 0);
    gtk_widget_set_size_request(scrolledwindow2, -1, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scrolledwindow2), GTK_SHADOW_IN);

    treeview2 = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview2), FALSE);
    gtk_widget_show(treeview2);
    gtk_container_add(GTK_CONTAINER(scrolledwindow2), treeview2);
    
    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), column);

    cr_toggle = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(column, cr_toggle, FALSE);
    g_signal_connect(cr_toggle, "toggled", G_CALLBACK(report_dialog_sel_toggle), rd);
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
    
    hbuttonbox3 = gtk_hbutton_box_new();
    gtk_widget_show(hbuttonbox3);
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbuttonbox3, FALSE, TRUE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox3),
			      GTK_BUTTONBOX_SPREAD);

    button3 = gtk_button_new_with_mnemonic("Select _None");
    gtk_widget_show(button3);
    gtk_container_add(GTK_CONTAINER(hbuttonbox3), button3);
    GTK_WIDGET_SET_FLAGS(button3, GTK_CAN_DEFAULT);
    g_signal_connect(button3, "clicked", G_CALLBACK(report_dialog_sel_none), rd);

    button6 = gtk_button_new_with_mnemonic("Select _All");
    gtk_widget_show(button6);
    gtk_container_add(GTK_CONTAINER(hbuttonbox3), button6);
    GTK_WIDGET_SET_FLAGS(button6, GTK_CAN_DEFAULT);
    g_signal_connect(button6, "clicked", G_CALLBACK(report_dialog_sel_all), rd);

    dialog1_action_area = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog1_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog1_action_area),
			      GTK_BUTTONBOX_END);

    button8 = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_widget_show(button8);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button8,
				 GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);

    button7 = gtk_button_new_with_mnemonic("_Generate");
    gtk_widget_show(button7);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button7,
				 GTK_RESPONSE_ACCEPT);
    GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);

    rd->dialog		= dialog;
    rd->btn_cancel	= button8;
    rd->btn_generate	= button7;
    rd->btn_sel_all	= button6;
    rd->btn_sel_none	= button3;
    rd->treeview	= treeview2;
    rd->model		= model;

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(treeview2));
    set_all_active(rd, TRUE);

    return rd;
}
