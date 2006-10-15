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

#include <iconcache.h>
#include <stock.h>

#include <binreloc.h>

gchar *path_data, *path_lib;

int
main(int argc, char **argv)
{
    GError *error;

    gtk_init(&argc, &argv);
    
    if (!gbr_init(&error)) {
        g_error("BinReloc cannot be initialized: %s", error->message);      
    } else {
        path_data = gbr_find_data_dir(PREFIX);
        path_lib = gbr_find_lib_dir(PREFIX);
    }
    
    icon_cache_init();
    stock_icons_init();
    shell_init();

    gtk_main();

    return 0;
}
