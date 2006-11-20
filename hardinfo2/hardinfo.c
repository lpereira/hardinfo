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

#include <config.h>
#include <shell.h>

#include <report.h>
#include <hardinfo.h>
#include <iconcache.h>
#include <stock.h>

#include <binreloc.h>

ProgramParameters params = { 0 };

int
main(int argc, char **argv)
{
    GSList *modules;
    
    parameters_init(&argc, &argv, &params);
    
    if (params.show_version) {
       g_print("HardInfo version " VERSION "\n");
       g_print("Copyright (C) 2003-2006 Leandro A. F. Pereira. See COPYING for details.\n");
       return 0;
    }
    
    if (!params.create_report) {
       /* we only try to open the UI if the user didn't asked for a 
          report. */
       params.gui_running = ui_init(&argc, &argv);

       /* if GTK+ initialization failed, assume the user wants a 
          report. */
       if (!params.gui_running)
           params.create_report = TRUE;
    }

    if (!binreloc_init(FALSE))
        g_error("Failed to find runtime data.\n\n"
                "\342\200\242 Is HardInfo correctly installed?\n"
                "\342\200\242 See if %s and %s exists and you have read permision.",
                PREFIX, LIBPREFIX);
    
    modules = modules_load();
    
    if (params.gui_running) {
        icon_cache_init();
        stock_icons_init();
    
        shell_init(modules);
  
        gtk_main();
    } else if (params.create_report) {
        gchar *report;
        
        report = report_create_from_module_list_format(modules,
                                                       params.report_format);
        g_print("%s", report);
        
        g_free(report);
    }

    return 0;
}
