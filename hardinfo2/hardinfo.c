/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 L. A. F. Pereira <l@tia.mat.br>
 *
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License v2.0 or later.
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


#include <config.h>
#include <shell.h>

#include <report.h>
#include <hardinfo.h>
#include <iconcache.h>
#include <stock.h>
#include <vendor.h>
#include <syncmanager.h>
#include <gio/gio.h>
#include "callbacks.h"

#include <binreloc.h>
#include "dmi_util.h"

ProgramParameters params = { 0 };

#if GTK_CHECK_VERSION(3,0,0)
gulong interface_changed_sh;
GSettings *settings=NULL;

void interface_changed_cb(GSettings *settings, gchar *name, gpointer user_data) {
    gchar *theme = g_settings_get_string(settings, name);
    //g_print("theme_changed: %s:%s\n", name, theme);
    params.darkmode=0;
    if(strstr(theme,"Dark")||strstr(theme,"dark")) params.darkmode=1;
    if(params.theme==-1) cb_disable_theme();
    if(params.theme==1) cb_theme1();
    if(params.theme==2) cb_theme2();
    if(params.theme==3) cb_theme3();
    if(params.theme==4) cb_theme4();
    if(params.theme==5) cb_theme5();
    if(params.theme==6) cb_theme6();
    g_free(theme);
}
#endif

int main(int argc, char **argv)
{
    int exit_code = 0;
    GSList *modules;

    DEBUG("Hardinfo2 version " VERSION ". Debug version.");

#if GLIB_CHECK_VERSION(2,32,5)
#else
    //if (!g_thread_supported ()) g_thread_init(NULL);
    g_type_init ();
#endif

    /* parse all command line parameters */
    parameters_init(&argc, &argv, &params);

    /* initialize the binreloc library, so we can load program data */
#if(ENABLE_BINRELOC)
    if (!binreloc_init(FALSE))
         g_error("Failed to find runtime data. PREFIX=%s, LIB=%s, LOCALE=%s\n",PREFIX,LIBPREFIX,LOCALEDIR);
#else
    params.path_data=g_strdup(PREFIX);
    params.path_lib=g_strdup(LIBPREFIX);
    params.path_locale=g_strdup(LOCALEDIR);
#endif

    setlocale(LC_ALL, "");
    bindtextdomain("hardinfo2", params.path_locale);
    textdomain("hardinfo2");

    /* show version information and quit */
    if (params.show_version) {
        g_print("Hardinfo2 version " VERSION "\n");
        g_print
            (_(/*/ %d will be latest year of copyright*/ "Copyright (C) 2003-2023 L. A. F. Pereira. 2024-%d Hardinfo2 Project.\n\n"), HARDINFO2_COPYRIGHT_LATEST_YEAR );

	g_print(_("Compile-time options:\n"
		"  Release version:   %s (%s)\n"
		"  LibSoup version:   %s\n"
		"  BinReloc enabled:  %s\n"
		"  Data prefix:       %s\n"
		"  Library prefix:    %s\n"
		"  Locale prefix :    %s\n"
		"  Data hardcoded:    %s\n"
		"  Library hardcoded: %s\n"
		"  Locale hardcoded:  %s\n"
		"  Compiled for:      %s\n"),
		RELEASE==1 ? "Yes (" VERSION ")" : (RELEASE==0?"No (" VERSION ")":"Debug (" VERSION ")"), ARCH,
		HARDINFO2_LIBSOUP3 ? _("3.0") : "2.4",
		ENABLE_BINRELOC ? _("Yes") : _("No"),
		params.path_data, params.path_lib, params.path_locale, PREFIX, LIBPREFIX,LOCALEDIR, PLATFORM);
        return 0;
    }

    if (!params.create_report && !params.run_benchmark) {
        /* we only try to open the UI if the user didn't ask for a report. */
        params.gui_running = ui_init(&argc, &argv);

        /* as a fallback, if GTK+ initialization failed, run in report
           generation mode. */
        if (!params.gui_running) {
            params.create_report = TRUE;
            /* ... it is possible to -f html without -r */
            if (params.report_format != REPORT_FORMAT_HTML)
                params.markup_ok = FALSE;
        }
    }

    //Get DarkMode state from system
    if(params.gui_running) {
        //get darkmode via gtk-theme has (d/D)ark as part of theme name from gsettings
	params.darkmode=0;
#if GTK_CHECK_VERSION(3,0,0)
        settings=g_settings_new("org.gnome.desktop.interface");
        interface_changed_sh = g_signal_connect(settings, "changed", G_CALLBACK(interface_changed_cb), NULL);
	char *theme=g_settings_get_string(settings,"gtk-theme");
	if(strstr(theme,"Dark")||strstr(theme,"dark")) params.darkmode=1;
	g_free(theme);
#endif	
	//get darkmode override from gtk-3.0/settings.ini - gtksettings
	gint dark=-1;
        g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &dark, NULL);
	if(dark==1) params.darkmode=1;
	//if(dark==0) params.darkmode=0;

    }

    /* load all modules */
    DEBUG("loading all modules");
    modules = modules_load_all();

    /* initialize vendor database */
    vendor_init();

    /* initialize moreinfo */
    moreinfo_init();

    if (params.run_benchmark) {
        gchar *result;

        result = module_call_method_param("benchmark::runBenchmark", params.run_benchmark);
        if (!result) {
          fprintf(stderr, _("Unknown benchmark ``%s''\n"), params.run_benchmark);
          exit_code = 1;
        } else {
          fprintf(stderr, "\n");
          g_print("%s\n", result);
          g_free(result);
        }
    } else if (params.gui_running) {
	/* initialize gui and start gtk+ main loop */
	icon_cache_init();
	stock_icons_init();

	shell_init(modules);

	DEBUG("entering gtk+ main loop");

	gtk_main();
    } else if (params.create_report) {
	/* generate report */
	gchar *report;

	if(params.bench_user_note) {//synchronize without sending benchmarks
	    sync_manager_update_on_startup(0);
	}

	DEBUG("generating report");

	report = report_create_from_module_list_format(modules,
						       params.
						       report_format);
	g_print("%s", report);

	if(params.bench_user_note) {//synchronize
	    if(!params.skip_benchmarks)
	       sync_manager_update_on_startup(1);
	}

	g_free(report);
    } else {
        g_error(_("Don't know what to do. Exiting."));
    }

    moreinfo_shutdown();
    vendor_cleanup();
    dmidecode_cache_free();
    free_auto_free_final();
#if GTK_CHECK_VERSION(3,0,0)
    g_object_unref(settings);
#endif
    DEBUG("finished");
    return exit_code;
}
