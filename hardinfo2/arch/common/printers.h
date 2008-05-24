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

void
__scan_printers(void)
{
    static GModule *cups = NULL;
    static int (*cupsGetPrinters) (char ***printers) = NULL;
    static char *(*cupsGetDefault) (void) = NULL;
    
    static char *libcups[] = { "libcups",
                               "libcups.so",
                               "libcups.so.1",
                               "libcups.so.2",
                               NULL };

    if (printer_list)
	g_free(printer_list);

    if (!(cupsGetPrinters && cupsGetDefault)) {
        int i;
        
        for (i = 0; libcups[i] != NULL; i++) {
            cups = g_module_open(libcups[i], G_MODULE_BIND_LAZY);
            if (cups)
                break;
        }
        
        if (!cups) {
	    printer_list = g_strdup("[Printers]\n"
	                            "CUPS libraries cannot be found=");
	    return;
	}

	if (!g_module_symbol(cups, "cupsGetPrinters", (gpointer) & cupsGetPrinters)
	    || !g_module_symbol(cups, "cupsGetDefault", (gpointer) & cupsGetDefault)) {
	    printer_list = g_strdup("[Printers]\n"
                                    "No suitable CUPS library found=");
            g_module_close(cups);
	    return;
	}
    }

    gchar **printers;
    int noprinters, i;
    const char *default_printer;

    noprinters = cupsGetPrinters(&printers);
    default_printer = cupsGetDefault();
    
    if (!default_printer) {
        default_printer = "";
    }
    
    if (noprinters > 0) {
	printer_list = g_strdup_printf("[Printers (CUPS)]\n");
	for (i = 0; i < noprinters; i++) {
	    printer_list = h_strdup_cprintf("\n$PRN%d$%s=%s\n",
					    printer_list,
					    i,						
					    printers[i],
                                            g_str_equal(default_printer, printers[i]) ?
				            "<i>(Default)</i>" : "");
	    g_free(printers[i]);
	}
	
	g_free(printers);
    } else {
	printer_list = g_strdup("[Printers]\n"
	                        "No printers found=\n");
    }
}
