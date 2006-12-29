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

#include <stdlib.h>
#include <gtk/gtk.h>

#include <hardinfo.h>
#include <callbacks.h>
#include <iconcache.h>

#include <shell.h>
#include <report.h>

#include <config.h>

void cb_save_graphic()
{
    /* this is ridiculously complicated :/ why in the hell gtk+ makes this kind of thing so difficult? 
       FIXME: it still don't save the image with the right size; we should do this sometime soon (tm) */
    Shell		*shell = shell_get_main_shell();
    GtkWidget		*widget = shell->info->view;
    GtkWidget		*dialog;

    PangoLayout		*layout;
    PangoContext	*context;
    PangoRectangle	 rect;

    GdkPixmap		*pm;
    GdkPixbuf		*pb;
    GdkGC		*gc;
    GdkColor	 	 black = { 0, 0, 0, 0 };
    GdkColor		 white = { 0, 65535, 65535, 65535 };

    gint		 w, h;
    gchar		*filename = NULL;
    
    /* */
    shell_status_update("Creating image...");

    /* initialize stuff */
    gc = gdk_gc_new(widget->window);
    context = gtk_widget_get_pango_context (widget);
    layout = pango_layout_new(context);

    /* draw the title */    
    pango_layout_set_markup(layout, shell->selected->name, -1);
    pango_layout_set_width(layout, widget->allocation.width * PANGO_SCALE);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    
    pango_layout_get_pixel_extents(layout, NULL, &rect);
    
    w = widget->allocation.width;
    h = widget->allocation.height + rect.height;
    
    pm = gdk_pixmap_new(widget->window, w, h, -1);
    gdk_draw_layout_with_colors (GDK_DRAWABLE(pm), gc, 0, 0, layout, &white, &black);
    
    /* copy the pixmap from the treeview and from the title */
    pb = gdk_pixbuf_get_from_drawable(NULL,
                                      widget->window,
				      NULL,
				      0, 0,
				      0, 0,
				      w, h);
    pb = gdk_pixbuf_get_from_drawable(pb,
                                      pm,
                                      NULL,
                                      0, 0,
                                      0, widget->allocation.height,
                                      widget->allocation.width, rect.height);

    /* save the pixbuf to a png file */
    dialog = gtk_file_chooser_dialog_new("Save Image",
	                                 NULL,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);
                                                   
    filename = g_strconcat(shell->selected->name, ".png", NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
    g_free(filename);
    
    if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        shell_status_update("Saving image...");
        gdk_pixbuf_save(pb, filename, "png", NULL,
                        "compression", "9",
                        "tEXt::hardinfo::version", VERSION,
                        "tEXt::hardinfo::arch", ARCH,
                        NULL);
        
        g_free(filename);
    }
  
    /* unref */
    gtk_widget_destroy (dialog);
    g_object_unref(pb);
    g_object_unref(layout);
    g_object_unref(pm);
    g_object_unref(gc);
    
    shell_status_update("Done.");
}

void cb_open_web_page()
{
    open_url("http://hardinfo.berlios.de");
}

void cb_report_bug()
{
    open_url("http://hardinfo.berlios.de/web/BugReports");
}

void cb_refresh()
{
    shell_do_reload();
}

void cb_copy_to_clipboard()
{
    ShellModuleEntry *entry = shell_get_main_shell()->selected;
    
    if (entry) {
        gchar         *data = entry->func(entry->number);
        GtkClipboard  *clip = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
        ReportContext *ctx  = report_context_text_new(NULL);

        ctx->entry = entry;

        report_header(ctx);
        report_table(ctx, data);
        report_footer(ctx);
        
        gtk_clipboard_set_text(clip, ctx->output, -1);

        g_free(data);
        report_context_free(ctx);
    }
}

void cb_side_pane()
{
    gboolean visible;
    
    visible = shell_action_get_active("SidePaneAction");
    shell_set_side_pane_visible(visible);
}

void cb_toolbar()
{
    gboolean visible;
    
    visible = shell_action_get_active("ToolbarAction");
    shell_ui_manager_set_visible("/MainMenuBarAction", visible);
}

void cb_about()
{
    GtkWidget *about;
    const gchar *authors[] = {
        "Author:",
        "Leandro A. F. Pereira",
        "",
        "Contributors:",
        "Agney Lopes Roth Ferraz",
        "SCSI support by Pascal F. Martin",
        "",
        "Based on work by:",
        "MD5 implementation by Colin Plumb (see md5.c for details)",
        "SHA1 implementation by Steve Raid (see sha1.c for details)",
        "Blowfish implementation by Paul Kocher (see blowfich.c for details)",
        "Raytracing benchmark by John Walker (see fbench.c for details)",
        "Some code partly based on x86cpucaps by Osamu Kayasono",
        "Vendor list based on GtkSysInfo by Pissens Sebastien",
        NULL
    };
    const gchar *artists[] = {
        "The GNOME Project",
        "Tango Project",
        NULL
    };

    about = gtk_about_dialog_new();
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), "HardInfo");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
				   "Copyright \302\251 2003-2006 " 
				   "Leandro A. F. Pereira");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
				  "System information and benchmark tool");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about),
			      icon_cache_get_pixbuf("logo.png"));

    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about),
       "HardInfo is free software; you can redistribute it and/or modify " \
       "it under the terms of the GNU General Public License as published by " \
       "the Free Software Foundation, version 2.\n\n"
       "This program is distributed in the hope that it will be useful, " \
       "but WITHOUT ANY WARRANTY; without even the implied warranty of " \
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the " \
       "GNU General Public License for more details.\n\n"
       "You should have received a copy of the GNU General Public License " \
       "along with this program; if not, write to the Free Software " \
       "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA");
#if GTK_CHECK_VERSION(2,8,0)
    gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about), TRUE);
#endif
    
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
    gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), artists);

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(about);
}

void cb_generate_report()
{
    Shell *shell = shell_get_main_shell();
    gboolean btn_refresh = shell_action_get_enabled("RefreshAction");
    gboolean btn_copy    = shell_action_get_enabled("CopyAction");

    report_dialog_show(shell->tree->model, shell->window);
    
    shell_action_set_enabled("RefreshAction", btn_refresh);
    shell_action_set_enabled("CopyAction", btn_copy);
}

void cb_quit(void)
{
    do {
        gtk_main_quit();
    } while (gtk_main_level() > 1);
}
