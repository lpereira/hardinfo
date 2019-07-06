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

#include "hardinfo.h"
#include "callbacks.h"
#include "iconcache.h"

#include "shell.h"
#include "report.h"
#include "syncmanager.h"
#include "xmlrpc-server.h"

#include "config.h"

void cb_sync_manager()
{
    Shell *shell = shell_get_main_shell();

    sync_manager_show(shell->window);
}

void cb_open_web_page()
{
    uri_open("http://www.hardinfo.org");
}

void cb_report_bug()
{
    uri_open("https://github.com/lpereira/hardinfo");
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

	    gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(shell->window));

	    text = g_strdup(sm->name);
#if GTK_CHECK_VERSION(2, 12, 0)
	    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), text);
#else
	    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), text);
#endif
	    g_free(text);

	    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about),
					 ma->version);

	    text = g_strdup_printf(_("Written by %s\nLicensed under %s"),
				   ma->author, ma->license);
	    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), text);
	    g_free(text);

	    if (ma->description)
		gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
					      _(ma->description));

	    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), sm->icon);
	    gtk_dialog_run(GTK_DIALOG(about));
	    gtk_widget_destroy(about);
	} else {
	    g_warning
		(_("No about information is associated with the %s module."),
		 name);
	}

	break;
    }

    g_free(name);
}

void cb_about()
{
    Shell *shell = shell_get_main_shell();
    GtkWidget *about;
    gchar *copyright = NULL;
    const gchar *authors[] = {
        _("Author:"),
        "Leandro A. F. Pereira",
        "",
        _("Contributors:"),
        "Agney Lopes Roth Ferraz",
        "Andrey Esin",
        "Burt P.",
        "Ondrej Čerman",
        "",
        _("Based on work by:"),
        _("MD5 implementation by Colin Plumb (see md5.c for details)"),
        _("SHA1 implementation by Steve Reid (see sha1.c for details)"),
        _("Blowfish implementation by Paul Kocher (see blowfich.c for details)"),
        _("Raytracing benchmark by John Walker (see fbench.c for details)"),
        _("FFT benchmark by Scott Robert Ladd (see fftbench.c for details)"),
        _("Some code partly based on x86cpucaps by Osamu Kayasono"),
        _("Vendor list based on GtkSysInfo by Pissens Sebastien"),
        _("DMI support based on code by Stewart Adam"),
        _("SCSI support based on code by Pascal F. Martin"),
        NULL
    };
    const gchar *artists[] = {
        "Jakub Szypulka",
        _("Tango Project"),
        _("The GNOME Project"),
        _("VMWare, Inc. (USB icon from VMWare Workstation 6)"),
        _("Roundicons (https://roundicons.com)"),
        NULL
    };

    about = gtk_about_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(shell->window));

#if GTK_CHECK_VERSION(2, 12, 0)
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), "HardInfo");
#else
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about), "HardInfo");
#endif

    copyright = g_strdup_printf("Copyright \302\251 2003-%d Leandro A. F. Pereira", HARDINFO_COPYRIGHT_LATEST_YEAR);

    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), copyright);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
				  _("System information and benchmark tool"));
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about),
			      icon_cache_get_pixbuf("hardinfo.png"));

    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about),
				 _("HardInfo is free software; you can redistribute it and/or modify "
				 "it under the terms of the GNU General Public License as published by "
				 "the Free Software Foundation, version 2.\n\n"
				 "This program is distributed in the hope that it will be useful, "
				 "but WITHOUT ANY WARRANTY; without even the implied warranty of "
				 "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
				 "GNU General Public License for more details.\n\n"
				 "You should have received a copy of the GNU General Public License "
				 "along with this program; if not, write to the Free Software "
				 "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA"));
    gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about), TRUE);

    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
    gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), artists);
    gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(about),
        _("translator-credits"));

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(about);

    g_free(copyright);
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

    exit(0);
}
