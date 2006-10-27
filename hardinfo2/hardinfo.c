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

#include <hardinfo.h>
#include <iconcache.h>
#include <stock.h>

#include <binreloc.h>

gchar	 *path_data = NULL,
         *path_lib = NULL;
gboolean  gui_running = FALSE;

int
main(int argc, char **argv)
{
    gui_running = ui_init(&argc, &argv);

    if (!binreloc_init(FALSE)) {
        g_error("Failed to find runtime data.\n\n"
                "\342\200\242 Is HardInfo correctly installed?\n"
                "\342\200\242 See if %s and %s exists and you have read permision.",
                PREFIX, LIBPREFIX);
    }
    
    icon_cache_init();
    stock_icons_init();
    shell_init();

    gtk_main();

    return 0;
}
