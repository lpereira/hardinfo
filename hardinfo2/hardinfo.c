/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include <binreloc.h>

ProgramParameters params = { 0 };

int main(int argc, char **argv)
{
    GSList *modules;

    DEBUG("HardInfo version " VERSION ". Debug version.");

#ifdef HAS_LIBSOUP
    DEBUG("g_thread_init()");
    if (!g_thread_supported())
	g_thread_init(NULL);
#endif				/* HAS_LIBSOUP */

    /* parse all command line parameters */
    parameters_init(&argc, &argv, &params);

    /* show version information and quit */
    if (params.show_version) {
	g_print("HardInfo version " VERSION "\n");
	g_print
	    ("Copyright (C) 2003-2007 Leandro A. F. Pereira. See COPYING for details.\n\n");

	g_print("Compile-time options:\n"
		"  Release version:   %s (%s)\n"
		"  BinReloc enabled:  %s\n"
		"  Data prefix:       %s\n"
		"  Library prefix:    %s\n"
		"  Compiled on:       %s %s (%s)\n",
		RELEASE ? "Yes" : "No (" VERSION ")", ARCH,
		ENABLE_BINRELOC ? "Yes" : "No",
		PREFIX, LIBPREFIX, PLATFORM, KERNEL, HOSTNAME);

	DEBUG("  Debugging is enabled.");

	/* show also available modules */
	params.list_modules = TRUE;
    }

    /* initialize the binreloc library, so we can load program data */
    if (!binreloc_init(FALSE))
	g_error("Failed to find runtime data.\n\n"
		"\342\200\242 Is HardInfo correctly installed?\n"
		"\342\200\242 See if %s and %s exists and you have read permision.",
		PREFIX, LIBPREFIX);

    /* list all module names */
    if (params.list_modules) {
	g_print("Modules:\n"
		"%-20s%-15s%-12s\n", "File Name", "Name", "Version");

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

    if (!params.create_report) {
	/* we only try to open the UI if the user didn't asked for a 
	   report. */
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

    if (params.gui_running) {
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
    }

    DEBUG("finished");
    return 0;
}
