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

#include <string.h>

#include "hardinfo.h"
#include "computer.h"

static void
get_glx_info(DisplayInfo *di)
{
    gchar *output;
    if (g_spawn_command_line_sync("glxinfo", &output, NULL, NULL, NULL)) {
	gchar **output_lines;
	gint i = 0;
	gint cpd = 0;

	for (output_lines = g_strsplit(output, "\n", 0);
	     output_lines && output_lines[i];
	     i++) {
	    if (strstr(output_lines[i], "OpenGL")) 
	    {
		if ( strstr(output_lines[i], "profile") ) {    cpd = 1;    }
		gchar **tmp = g_strsplit(output_lines[i], ":", 0);

		tmp[1] = g_strchug(tmp[1]);

		get_str("OpenGL vendor str", di->ogl_vendor);
		get_str("OpenGL renderer str", di->ogl_renderer);
		if ( cpd > 0 ) {    get_str("OpenGL core profile version str", di->ogl_version);    }
                else             {    get_str("OpenGL version str", di->ogl_version);    }

		g_strfreev(tmp);
	    } else if (strstr(output_lines[i], "direct rendering: Yes")) {
	        di->dri = TRUE;
	    }
	}

	g_free(output);
	g_strfreev(output_lines);

	if (!di->ogl_vendor)
	    di->ogl_vendor = "Unknown";
	if (!di->ogl_renderer)
	    di->ogl_renderer = "Unknown";
	if (!di->ogl_version)
	    di->ogl_version = "Unknown";
    } else {
	di->ogl_vendor = di->ogl_renderer = di->ogl_version = "Unknown";
    }

}

static void
get_x11_info(DisplayInfo *di)
{
    gchar *output;
    
    if (g_spawn_command_line_sync("xdpyinfo", &output, NULL, NULL, NULL)) {
	gchar **output_lines, **old;

	output_lines = g_strsplit(output, "\n", 0);
	g_free(output);

	old = output_lines;
	while (*(output_lines++)) {
            gchar **tmp = g_strsplit(*output_lines, ":", 0);

            if (tmp[1] && tmp[0]) {
              tmp[1] = g_strchug(tmp[1]);

              get_str("vendor string", di->vendor);
              get_str("X.Org version", di->version);
              get_str("XFree86 version", di->version);

              if (g_str_has_prefix(tmp[0], "number of extensions")) {
                int n;
                
                di->extensions = g_strdup("");
                
                for (n = atoi(tmp[1]); n; n--) {
                  di->extensions = h_strconcat(di->extensions, 
                                               g_strstrip(*(++output_lines)),
                                               "=\n",
                                               NULL);
                }
                g_strfreev(tmp);
                
                break;
              }
            }

            g_strfreev(tmp);
	}

	g_strfreev(old);
    }
    
    GdkScreen *screen = gdk_screen_get_default();
    
    if (screen && GDK_IS_SCREEN(screen)) {
        gint n_monitors = gdk_screen_get_n_monitors(screen);
        gint i;
        
        di->monitors = NULL;
        for (i = 0; i < n_monitors; i++) {
            GdkRectangle rect;
            
            gdk_screen_get_monitor_geometry(screen, i, &rect);
            
            di->monitors = h_strdup_cprintf(_("Monitor %d=%dx%d pixels\n"),
                                            di->monitors, i, rect.width, rect.height);
        }
      } else {
          di->monitors = "";
      }
}

DisplayInfo *
computer_get_display(void)
{
    DisplayInfo *di = g_new0(DisplayInfo, 1);
    
    GdkScreen *screen = gdk_screen_get_default();
    
    if (screen && GDK_IS_SCREEN(screen)) {
        di->width = gdk_screen_get_width(screen);
        di->height = gdk_screen_get_height(screen);
    } else {
        di->width = di->height = 0;
    }

    get_glx_info(di);
    get_x11_info(di);

    return di;
}
