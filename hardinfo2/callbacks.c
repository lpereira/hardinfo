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

#include <callbacks.h>
#include <iconcache.h>

#include <shell.h>
#include <report.h>

#include <config.h>

void cb_refresh()
{
    shell_do_reload();
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
        "Leandro A. F. Pereira",
        "SCSI support by Pascal F. Martin",
        "MD5 implementation by Colin Plumb",
        "SHA1 implementation by Steve Raid",
        "Blowfish implementation by Paul Kocher",
        "Some code partly based on x86cpucaps by Osamu Kayasono",
        "Artwork by the GNOME Project and Tango Project",
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

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(about);
}

void cb_generate_report()
{
    Shell *shell = shell_get_main_shell();

    report_dialog_show(shell->tree->model, shell->window);
}

void cb_quit(void)
{
    gtk_main_quit();
    exit(0);
}
