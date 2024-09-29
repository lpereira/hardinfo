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
#include <glib/gstdio.h>
#include <stdbool.h>
#include "callbacks.h"
#include "dmi_util.h"

ProgramParameters params = { 0 };

int main(int argc, char **argv)
{
    int exit_code = 0;
    GSList *modules;
    gboolean cleanUserData;
    char *appver;
    gchar *path;
    GKeyFile *key_file = g_key_file_new();
    gchar *conf_path = g_build_filename(g_get_user_config_dir(), "hardinfo2", "settings.ini", NULL);

    DEBUG("Hardinfo2 version " VERSION ". Debug version.");

#if GLIB_CHECK_VERSION(2,32,5)
#else
    //if (!g_thread_supported ()) g_thread_init(NULL);
    g_type_init ();
#endif

    /* parse all command line parameters */
    parameters_init(&argc, &argv, &params);

    params.path_data=g_strdup(PREFIX);
    params.path_lib=g_strdup(LIBPREFIX);
    params.path_locale=g_strdup(LOCALEDIR);

    setlocale(LC_ALL, "");
    bindtextdomain("hardinfo2", params.path_locale);
    textdomain("hardinfo2");

    //Remove ids and json files when starting new program version or first time
    g_key_file_load_from_file(
        key_file, conf_path,
        G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    cleanUserData=false;
    appver = g_key_file_get_string(key_file, "Application", "Version", NULL);
    if(appver){
        if(strcmp(appver,VERSION)) cleanUserData=true;
    } else {appver="OLD";cleanUserData=true;}

    if(cleanUserData){
        DEBUG("Cleaning User Data.... (%s<>%s)\n",appver,VERSION);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","blobs-update-version.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","benchmark.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","cpuflags.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","kernel-module-icons.json", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","arm.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","ieee_oui.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","edid.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","pci.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","sdcard.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","usb.ids", NULL);g_remove(path);g_free(path);
        path = g_build_filename(g_get_user_config_dir(), "hardinfo2","vendor.ids", NULL);g_remove(path);g_free(path);
	//update settings.ini
	g_key_file_set_string(key_file, "Application", "Version", VERSION);
#if GLIB_CHECK_VERSION(2,40,0)
	g_key_file_save_to_file(key_file, conf_path, NULL);
#else
	g2_key_file_save_to_file(key_file, conf_path, NULL);
#endif
    }
    g_free(conf_path);
    g_key_file_free(key_file);

    /* show version information and quit */
    if (params.show_version) {
        g_print("Hardinfo2 version " VERSION "\n");
        g_print
            (_(/*/ %d will be latest year of copyright*/ "Copyright (C) 2003-2023 L. A. F. Pereira. 2024-%d Hardinfo2 Project.\n\n"), HARDINFO2_COPYRIGHT_LATEST_YEAR );

	g_print(N_("Compile-time options:\n"
		"  Release version:  %s (%s)\n"
		"  LibSoup version:  %s\n"
		"  Data           :  %s\n"
		"  Library        :  %s\n"
		"  Locale         :  %s\n"
		"  Compiled for   :  %s\n"),
		RELEASE==1 ? "Yes (" VERSION ")" : (RELEASE==0?"No (" VERSION ")":"Debug (" VERSION ")"), ARCH,
		HARDINFO2_LIBSOUP3 ? _("3.0") : "2.4",
		params.path_data, params.path_lib, params.path_locale,
		PLATFORM);
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
    DEBUG("finished");
    return exit_code;
}
