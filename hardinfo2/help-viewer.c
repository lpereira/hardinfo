/*
 *    HelpViewer - Simple Help file browser
 *    Copyright (C) 2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#define _GNU_SOURCE		/* for strcasestr() */
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "config.h"
#include "shell.h"
#include "markdown-text-view.h"
#include "help-viewer.h"
#include "hardinfo.h"

static void do_search(HelpViewer *hv, gchar *text);

static void forward_clicked(GtkWidget *widget, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    GSList *temp;
    
    /* puts the current file on the back stack */
    hv->back_stack = g_slist_prepend(hv->back_stack, g_strdup(hv->current_file));
    
    /* enables the back button */
    gtk_widget_set_sensitive(hv->btn_back, TRUE);

    /* loads the new current file */
    if (g_str_has_prefix(hv->forward_stack->data, "search://")) {
        do_search(hv, hv->forward_stack->data + sizeof("search://") - 1);
    } else {
        markdown_textview_load_file(MARKDOWN_TEXTVIEW(hv->text_view), hv->forward_stack->data);
    }
    
    /* pops the stack */
    temp = hv->forward_stack->next;
    g_free(hv->forward_stack->data);
    g_slist_free1(hv->forward_stack);
    hv->forward_stack = temp;
    
    /* if there aren't items on forward stack anymore, disables the button */
    if (!hv->forward_stack) {
        gtk_widget_set_sensitive(hv->btn_forward, FALSE);
    }
}

static void back_clicked(GtkWidget *widget, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    GSList *temp;
    
    /* puts the current file on the forward stack */
    hv->forward_stack = g_slist_prepend(hv->forward_stack, g_strdup(hv->current_file));
    
    /* enables the forward button */
    gtk_widget_set_sensitive(hv->btn_forward, TRUE);

    /* loads the new current file */
    if (g_str_has_prefix(hv->back_stack->data, "search://")) {
        do_search(hv, hv->back_stack->data + sizeof("search://") - 1);
    } else {
        markdown_textview_load_file(MARKDOWN_TEXTVIEW(hv->text_view), hv->back_stack->data);
    }
    
    /* pops the stack */
    temp = hv->back_stack->next;
    g_free(hv->back_stack->data);
    g_slist_free1(hv->back_stack);
    hv->back_stack = temp;
    
    /* if there aren't items on back stack anymore, disables the button */
    if (!hv->back_stack) {
        gtk_widget_set_sensitive(hv->btn_back, FALSE);
    }
}

static void link_clicked(MarkdownTextView *text_view, gchar *link, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    
    if (g_str_has_prefix(link, "http://")) {
        open_url(link);
    } else {
        /* adds the current file to the back stack (before loading the new file */
        hv->back_stack = g_slist_prepend(hv->back_stack, g_strdup(hv->current_file));
        gtk_widget_set_sensitive(hv->btn_back, TRUE);

        gtk_statusbar_pop(GTK_STATUSBAR(hv->status_bar), 1);
        markdown_textview_load_file(text_view, link);
    }
}

static void file_load_complete(MarkdownTextView *text_view, gchar *file, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    
    /* sets the currently-loaded file */
    g_free(hv->current_file);
    hv->current_file = g_strdup(file);
    
    gtk_statusbar_push(GTK_STATUSBAR(hv->status_bar), 1, "Done.");
}

static void hovering_over_link(MarkdownTextView *text_view, gchar *link, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    gchar *temp;
    
    temp = g_strconcat("Link to ", link, NULL);
    
    gtk_statusbar_push(GTK_STATUSBAR(hv->status_bar), 1, temp);
    
    g_free(temp);
}

static void hovering_over_text(MarkdownTextView *text_view, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    
    gtk_statusbar_pop(GTK_STATUSBAR(hv->status_bar), 1);
}

static void do_search(HelpViewer *hv, gchar *text)
{
    GString *markdown;
    GDir *dir;
    gchar **terms;
    gint no_results = 0;
    int term;
    
    /*
     * FIXME: This search is currently pretty slow; think on a better way to do search. 
     * Ideas:
     * - Build a index the first time the help file is opened
     * - Search only titles and subtitles
     */
    
    terms = g_strsplit(text, " ", 0);
    markdown = g_string_new("# Search Results\n");
    g_string_append_printf(markdown, "Search terms: *%s*\n****\n", text);
    
    gtk_widget_set_sensitive(hv->window, FALSE);
    
    if ((dir = g_dir_open(hv->help_directory, 0, NULL))) {
        const gchar *name;
        
        while ((name = g_dir_read_name(dir))) {
#if GTK_CHECK_VERSION(2,16,0)
            gtk_entry_progress_pulse(GTK_ENTRY(hv->text_search));
#endif	/* GTK_CHECK_VERSION(2,16,0) */
            
            if (g_str_has_suffix(name, ".hlp")) {
                FILE *file;
                gchar *path;
                gchar buffer[256];
                
                path = g_build_filename(hv->help_directory, name, NULL);
                if ((file = fopen(path, "rb"))) {
                    gboolean found = FALSE;
                    gchar *title = NULL;
                    
                    while (!found && fgets(buffer, sizeof buffer, file)) {
                        if (!title && (g_str_has_prefix(buffer, "# ") || g_str_has_prefix(buffer, " # "))) {
                            title = g_strstrip(strchr(buffer, '#') + 1);
                            title = g_strdup(title);
                        }
                        
                        for (term = 0; !found && terms[term]; term++) {
#ifdef strcasestr
                            found = strcasestr(buffer, terms[term]) != NULL;
#else
                            gchar *upper1, *upper2;
                            
                            upper1 = g_utf8_strup(buffer, -1);
                            upper2 = g_utf8_strup(terms[term], -1);
                            
                            found = strstr(upper1, upper2) != NULL;
                            
                            g_free(upper1);
                            g_free(upper2);
#endif
                        }
                    }
                    
                    if (found) {
                        no_results++;
                        
                        if (title) {
                              g_string_append_printf(markdown,
                                                     "* [%s %s]\n", name, title);
                        } else {
                              g_string_append_printf(markdown,
                                                     "* [%s %s]\n", name, name);
                        }
                    }
                    
                    g_free(title);
                    fclose(file);
                }
                
                g_free(path);
            }
        }
        
        g_dir_close(dir);
    }
    
    
    if (no_results == 0) {
        g_string_append_printf(markdown,
                               "Search returned no results.");
    } else {
        g_string_append_printf(markdown,
                               "****\n%d results found.", no_results);
    }
    
    /* shows the results inside the textview */
    markdown_textview_set_text(MARKDOWN_TEXTVIEW(hv->text_view), markdown->str);

    g_free(hv->current_file);
    hv->current_file = g_strdup_printf("search://%s", text);

#if GTK_CHECK_VERSION(2,16,0)
    gtk_entry_set_progress_fraction(GTK_ENTRY(hv->text_search), 0.0f);
#endif	/* GTK_CHECK_VERSION(2,16,0) */
    gtk_widget_set_sensitive(hv->window, TRUE);

    g_string_free(markdown, TRUE);
    g_strfreev(terms);
}

static void activate(GtkEntry *entry, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    
    /* adds the current file to the back stack (before loading the new file */
    hv->back_stack = g_slist_prepend(hv->back_stack, g_strdup(hv->current_file));
    gtk_widget_set_sensitive(hv->btn_back, TRUE);

    do_search((HelpViewer *)data, (gchar *)gtk_entry_get_text(entry));
}

#if GTK_CHECK_VERSION(2,16,0)
static void icon_press(GtkEntry *entry, gint position,
                       GdkEventButton *event, gpointer data)
{
    if (position == GTK_ENTRY_ICON_SECONDARY)
        activate(entry, data);
}
#endif	/* GTK_CHECK_VERSION(2,16,0) */ 

static void home_clicked(GtkWidget *button, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    
    help_viewer_open_page(hv, "index.hlp");
}

void help_viewer_open_page(HelpViewer *hv, const gchar *page)
{
    /* adds the current file to the back stack (before loading the new file */
    hv->back_stack = g_slist_prepend(hv->back_stack, g_strdup(hv->current_file));
    gtk_widget_set_sensitive(hv->btn_back, TRUE);

    markdown_textview_load_file(MARKDOWN_TEXTVIEW(hv->text_view), page);
    
    gtk_window_present(GTK_WINDOW(hv->window));
}

void help_viewer_destroy(HelpViewer *hv)
{
    Shell *shell;
    GSList *item;

    for (item = hv->back_stack; item; item = item->next) {
        g_free(item->data);
    }

    for (item = hv->forward_stack; item; item = item->next) {
        g_free(item->data);
    }
    
    g_slist_free(hv->back_stack);
    g_slist_free(hv->forward_stack);

    g_free(hv->current_file);
    g_free(hv->help_directory);
    
    shell = shell_get_main_shell();
    shell->help_viewer = NULL;
}

static gboolean destroy_me(GtkWidget *widget, gpointer data)
{
    HelpViewer *hv = (HelpViewer *)data;
    
    help_viewer_destroy(hv);

    return FALSE;
}

HelpViewer *
help_viewer_new (const gchar *help_dir, const gchar *help_file)
{
    Shell *shell;
    HelpViewer *hv;
    GtkWidget *help_viewer;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *toolbar1;
    GtkIconSize tmp_toolbar_icon_size;
    GtkWidget *btn_back;
    GtkWidget *btn_forward;
    GtkWidget *separatortoolitem1;
    GtkWidget *toolbar2;
    GtkWidget *toolitem3;
#if !GTK_CHECK_VERSION(2,16,0)
    GtkWidget *label1;
#endif	/* GTK_CHECK_VERSION(2,16,0) */
    GtkWidget *toolitem4;
    GtkWidget *txt_search;
    GtkWidget *scrolledhelp_viewer;
    GtkWidget *markdown_textview;
    GtkWidget *status_bar;
    GtkWidget *separatortoolitem;
    GtkWidget *btn_home;
    GdkPixbuf *icon;
    
    shell = shell_get_main_shell();

    help_viewer = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(help_viewer, 300, 200);
    gtk_window_set_default_size(GTK_WINDOW(help_viewer), 640, 480);
    gtk_window_set_title(GTK_WINDOW(help_viewer), "Help Viewer");
    gtk_window_set_transient_for(GTK_WINDOW(help_viewer), GTK_WINDOW(shell->window));
    
    icon = gtk_widget_render_icon(help_viewer, GTK_STOCK_HELP, 
                                  GTK_ICON_SIZE_DIALOG,
                                  NULL);
    gtk_window_set_icon(GTK_WINDOW(help_viewer), icon);
    g_object_unref(icon);
    
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (help_viewer), vbox);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    toolbar1 = gtk_toolbar_new ();
    gtk_widget_show (toolbar1);
    gtk_box_pack_start (GTK_BOX (hbox), toolbar1, TRUE, TRUE, 0);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_BOTH_HORIZ);
    tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1));

    btn_back = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-go-back");
    gtk_widget_show (btn_back);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_back);
    gtk_tool_item_set_is_important (GTK_TOOL_ITEM (btn_back), TRUE);
    gtk_widget_set_sensitive(btn_back, FALSE);

    btn_forward = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-go-forward");
    gtk_widget_show (btn_forward);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_forward);
    gtk_widget_set_sensitive(btn_forward, FALSE);

    separatortoolitem1 = (GtkWidget*) gtk_separator_tool_item_new ();
    gtk_widget_show (separatortoolitem1);
    gtk_container_add (GTK_CONTAINER (toolbar1), separatortoolitem1);

    btn_home = (GtkWidget*) gtk_tool_button_new_from_stock ("gtk-home");
    gtk_widget_show (btn_home);
    gtk_container_add (GTK_CONTAINER (toolbar1), btn_home);

    toolbar2 = gtk_toolbar_new ();
    gtk_widget_show (toolbar2);
    gtk_box_pack_end (GTK_BOX (hbox), toolbar2, FALSE, TRUE, 0);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar2), FALSE);
    tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar2));

    toolitem3 = (GtkWidget*) gtk_tool_item_new ();
    gtk_widget_show (toolitem3);
    gtk_container_add (GTK_CONTAINER (toolbar2), toolitem3);

#if !GTK_CHECK_VERSION(2,16,0)
    label1 = gtk_label_new_with_mnemonic ("_Search:");
    gtk_widget_show (label1);
    gtk_container_add (GTK_CONTAINER (toolitem3), label1);
#endif	/* GTK_CHECK_VERSION(2,16,0) */

    toolitem4 = (GtkWidget*) gtk_tool_item_new ();
    gtk_widget_show (toolitem4);
    gtk_container_add (GTK_CONTAINER (toolbar2), toolitem4);

    txt_search = gtk_entry_new ();
    gtk_widget_show (txt_search);
    gtk_container_add (GTK_CONTAINER (toolitem4), txt_search);
    gtk_entry_set_invisible_char (GTK_ENTRY (txt_search), 9679);
#if GTK_CHECK_VERSION(2,16,0)
    gtk_entry_set_icon_from_stock(GTK_ENTRY(txt_search),
                                  GTK_ENTRY_ICON_SECONDARY,
                                  GTK_STOCK_FIND);
#endif	/* GTK_CHECK_VERSION(2,16,0) */

    scrolledhelp_viewer = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledhelp_viewer);
    gtk_box_pack_start (GTK_BOX (vbox), scrolledhelp_viewer, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledhelp_viewer), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    markdown_textview = markdown_textview_new();
    markdown_textview_set_image_directory(MARKDOWN_TEXTVIEW(markdown_textview), help_dir);
    gtk_widget_show (markdown_textview);
    gtk_container_add (GTK_CONTAINER (scrolledhelp_viewer), markdown_textview);

    status_bar = gtk_statusbar_new ();
    gtk_widget_show (status_bar);
    gtk_box_pack_start (GTK_BOX (vbox), status_bar, FALSE, FALSE, 0);

    hv = g_new0(HelpViewer, 1);
    hv->window = help_viewer;
    hv->status_bar = status_bar;
    hv->btn_back = btn_back;
    hv->btn_forward = btn_forward;
    hv->text_view = markdown_textview;
    hv->text_search = txt_search;
    hv->help_directory = g_strdup(help_dir);
    hv->back_stack = NULL;
    hv->forward_stack = NULL;

    g_signal_connect(markdown_textview, "link-clicked", G_CALLBACK(link_clicked), hv);
    g_signal_connect(markdown_textview, "hovering-over-link", G_CALLBACK(hovering_over_link), hv);
    g_signal_connect(markdown_textview, "hovering-over-text", G_CALLBACK(hovering_over_text), hv);
    g_signal_connect(markdown_textview, "file-load-complete", G_CALLBACK(file_load_complete), hv);

    g_signal_connect(btn_back, "clicked", G_CALLBACK(back_clicked), hv);
    g_signal_connect(btn_forward, "clicked", G_CALLBACK(forward_clicked), hv);
    g_signal_connect(btn_home, "clicked", G_CALLBACK(home_clicked), hv);

    g_signal_connect(help_viewer, "delete-event", G_CALLBACK(destroy_me), hv);
    g_signal_connect(txt_search, "activate", G_CALLBACK(activate), hv);

#if GTK_CHECK_VERSION(2,16,0)
    g_signal_connect(txt_search, "icon-press", G_CALLBACK(icon_press), hv);
#endif	/* GTK_CHECK_VERSION(2,16,0) */
                            
    if (!markdown_textview_load_file(MARKDOWN_TEXTVIEW(markdown_textview), help_file ? help_file : "index.hlp")) {
        GtkWidget	*dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(shell->window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        "Cannot open help file (%s).",
                                        help_file ? help_file : "index.hlp");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        gtk_widget_destroy(hv->window);
        g_free(hv);
        
        return NULL;
    }
    
    gtk_widget_show_all(hv->window);

    return hv;
}

#ifdef HELPVIEWER_TEST
int main(int argc, char **argv)
{
    HelpViewer *hv;
    
    gtk_init(&argc, &argv);
    
    hv = help_viewer_new("documentation", NULL);
    
    gtk_main();
}
#endif	/* HELPVIEWER_TEST */
