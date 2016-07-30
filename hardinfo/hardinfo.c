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

#include <config.h>
#include <shell.h>

#include <report.h>
#include <hardinfo.h>
#include <iconcache.h>
#include <stock.h>
#include <vendor.h>

#include <binreloc.h>

ProgramParameters params = { 0 };

int main(int argc, char **argv)
{
	setlocale( LC_ALL, "" );
    bindtextdomain( "hardinfo", "/usr/share/locale" );
    textdomain( "hardinfo" );
    
    GSList *modules;

    DEBUG("HardInfo version " VERSION ". Debug version.");

    DEBUG("g_thread_init()");
    if (!g_thread_supported())
	g_thread_init(NULL);

    /* parse all command line parameters */
    parameters_init(&argc, &argv, &params);

    /* show version information and quit */
    if (params.show_version) {
	g_print("HardInfo version " VERSION "\n");
	g_print
	    (_("Copyright (C) 2003-2009 Leandro A. F. Pereira. See COPYING for details.\n\n"));

	g_print(_("Compile-time options:\n"
		"  Release version:   %s (%s)\n"
		"  BinReloc enabled:  %s\n"
		"  Data prefix:       %s\n"
		"  Library prefix:    %s\n"
		"  Compiled on:       %s %s (%s)\n"),
		RELEASE ? _("Yes") : "No (" VERSION ")", ARCH,
		ENABLE_BINRELOC ? _("Yes") : _("No"),
		PREFIX, LIBPREFIX, PLATFORM, KERNEL, HOSTNAME);

	DEBUG("  Debugging is enabled.");

	/* show also available modules */
	params.list_modules = TRUE;
    }

    /* initialize the binreloc library, so we can load program data */
    if (!binreloc_init(FALSE))
	g_error(_("Failed to find runtime data.\n\n"
		"\342\200\242 Is HardInfo correctly installed?\n"
		"\342\200\242 See if %s and %s exists and you have read permision."),
		PREFIX, LIBPREFIX);

    /* list all module names */
    if (params.list_modules) {
	g_print(_("Modules:\n"
		"%-20s%-15s%-12s\n"), _("File Name"), _("Name"), _("Version"));

	for (modules = modules_load_all(); modules;
	     modules = modules->next) {
	    ShellModule *module = (ShellModule *) modules->data;
	    ModuleAbout *ma = module_get_about(module);
	    gchar *name = g_path_get_basename(g_module_name(module->dll));

	    g_print("%-20s%-15s%-12s\n", name, module->name, ma->version);

	    g_free(name);
	}

	return 0;
    }

    if (!params.create_report && !params.run_benchmark) {
	/* we only try to open the UI if the user didn't ask for a report. */
	params.gui_running = ui_init(&argc, &argv);

	/* as a fallback, if GTK+ initialization failed, run in report
	   generation mode. */
	if (!params.gui_running)
	    params.create_report = TRUE;
    }

    if (params.use_modules) {
	/* load only selected modules */
	DEBUG("loading user-selected modules");
	modules = modules_load_selected();
    } else {
	/* load all modules */
	DEBUG("loading all modules");
	modules = modules_load_all();
    }

    /* initialize vendor database */
    vendor_init();
    
    /* initialize moreinfo */
    moreinfo_init();

    if (params.run_benchmark) {
        gchar *result;
        
        result = module_call_method_param("benchmark::runBenchmark", params.run_benchmark);
        if (!result) {
          g_error(_("Unknown benchmark ``%s'' or libbenchmark.so not loaded"), params.run_benchmark);
        } else {
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

	DEBUG("generating report");

	report = report_create_from_module_list_format(modules,
						       params.
						       report_format);
	g_print("%s", report);

	g_free(report);
    } else {
        g_error(_("Don't know what to do. Exiting."));
    }

    moreinfo_shutdown();

    DEBUG("finished");
    return 0;
}
