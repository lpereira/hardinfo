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

#include <stdlib.h>
#include <gtk/gtk.h>

#include <hardinfo.h>
#include <callbacks.h>
#include <iconcache.h>

#include <shell.h>
#include <report.h>
#include <syncmanager.h>

#include <config.h>

void cb_sync_manager()
{
    Shell *shell = shell_get_main_shell();

    sync_manager_show(shell->window);
}

void cb_connect_to()
{
    Shell *shell = shell_get_main_shell();

    connect_dialog_show(shell->window);
}

void cb_manage_hosts()
{
    Shell *shell = shell_get_main_shell();
    
    host_manager_show(shell->window);
}

void cb_connect_host(GtkAction * action)
{
    gchar *name;
    
    g_object_get(G_OBJECT(action), "name", &name, NULL);
    remote_connect_host(name);

    g_free(name);
}

void cb_local_computer()
{
    Shell *shell = shell_get_main_shell();

    shell_status_update("Unloading modules...");
    module_unload_all();
    
    shell_status_update("Loading local modules...");
    shell->tree->modules = modules_load_all();

    g_slist_foreach(shell->tree->modules, shell_add_modules_to_gui, shell->tree);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(shell->tree->view));
    
    shell_view_set_enabled(TRUE);
}

void cb_save_graphic()
{
    Shell *shell = shell_get_main_shell();
    GtkWidget *dialog;
    gchar *filename;

    /* save the pixbuf to a png file */
    dialog = gtk_file_chooser_dialog_new("Save Image",
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_SAVE,
					 GTK_RESPONSE_ACCEPT, NULL);

    filename = g_strconcat(shell->selected->name, ".png", NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
    g_free(filename);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_widget_destroy(dialog);

	shell_status_update("Saving image...");

	tree_view_save_image(filename);

	shell_status_update("Done.");
	g_free(filename);

	return;
    }

    gtk_widget_destroy(dialog);
}

void cb_open_web_page()
{
    open_url("http://wiki.hardinfo.org");
}

void cb_open_online_docs()
{
    open_url("http://wiki.hardinfo.org/Documentation");
}

void cb_report_bug()
{
    open_url("http://wiki.hardinfo.org/BugReports");
}

void cb_donate()
{
    open_url("http://wiki.hardinfo.org/Donate");
}

void cb_refresh()
{
    shell_do_reload();
}

void cb_copy_to_clipboard()
{
    ShellModuleEntry *entry = shell_get_main_shell()->selected;

    if (entry) {
	gchar *data = module_entry_function(entry);
	GtkClipboard *clip =
	    gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
	ReportContext *ctx = report_context_text_new(NULL);

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

void cb_about_module(GtkAction * action)
{
    Shell *shell = shell_get_main_shell();
    GSList *modules = shell->tree->modules;
    ModuleAbout *ma;
    gchar *name;

    g_object_get(G_OBJECT(action), "tooltip", &name, NULL);

    for (; modules; modules = modules->next) {
	ShellModule *sm = (ShellModule *) modules->data;

	if (!g_str_equal(sm->name, name))
	    continue;

	if ((ma = module_get_about(sm))) {
	    GtkWidget *about;
	    gchar *text;

	    about = gtk_about_dialog_new();

	    text = g_strdup_printf("%s Module", sm->name);
	    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), text);
	    g_free(text);

	    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about),
					 ma->version);

	    text = g_strdup_printf("Written by %s\nLicensed under %s",
				   ma->author, ma->license);
	    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), text);
	    g_free(text);

	    if (ma->description)
		gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
					      ma->description);

	    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), sm->icon);
	    gtk_dialog_run(GTK_DIALOG(about));
	    gtk_widget_destroy(about);
	} else {
	    g_warning
		("No about information is associated with the %s module.",
		 name);
	}

	break;
    }

    g_free(name);
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
	"Andrey Esin",
	"",
	"Based on work by:",
	"MD5 implementation by Colin Plumb (see md5.c for details)",
	"SHA1 implementation by Steve Reid (see sha1.c for details)",
	"Blowfish implementation by Paul Kocher (see blowfich.c for details)",
	"Raytracing benchmark by John Walker (see fbench.c for details)",
	"FFT benchmark by Scott Robert Ladd (see fftbench.c for details)",
	"Some code partly based on x86cpucaps by Osamu Kayasono",
	"Vendor list based on GtkSysInfo by Pissens Sebastien",
	"DMI support based on code by Stewart Adam",
	"SCSI support based on code by Pascal F. Martin",
	NULL
    };
    const gchar *artists[] = {
	"Jakub Szypulka",
	"Tango Project",
	"The GNOME Project",
	"VMWare, Inc. (USB icon from VMWare Workstation 6)",
	NULL
    };

    about = gtk_about_dialog_new();
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), "HardInfo");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
				   "Copyright \302\251 2003-2009 "
				   "Leandro A. F. Pereira");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
				  "System information and benchmark tool");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about),
			      icon_cache_get_pixbuf("logo.png"));

    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about),
				 "HardInfo is free software; you can redistribute it and/or modify "
				 "it under the terms of the GNU General Public License as published by "
				 "the Free Software Foundation, version 2.\n\n"
				 "This program is distributed in the hope that it will be useful, "
				 "but WITHOUT ANY WARRANTY; without even the implied warranty of "
				 "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
				 "GNU General Public License for more details.\n\n"
				 "You should have received a copy of the GNU General Public License "
				 "along with this program; if not, write to the Free Software "
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
    gboolean btn_copy = shell_action_get_enabled("CopyAction");

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
