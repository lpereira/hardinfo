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

static gchar *
benchmark_zlib(void)
{
    GModule *libz;
    static gulong (*compressBound) (glong srclen) = NULL;
    static gint (*compress) (gchar *dst, glong *dstlen,
                             const gchar *src, glong srclen) = NULL;

    if (!(compress && compressBound)) {
	libz = g_module_open("libz", G_MODULE_BIND_LAZY);
	if (!libz) {
            libz = g_module_open("/usr/lib/libz.so", G_MODULE_BIND_LAZY);
            if (!libz) {
                g_warning("Cannot load ZLib: %s", g_module_error());
                return g_strdup("[Error]\n"
                       "ZLib not found=");
            }
	}

	if (!g_module_symbol(libz, "compress", (gpointer) & compress)
	    || !g_module_symbol(libz, "compressBound", (gpointer) & compressBound)) {
	    
            g_module_close(libz);
	    return g_strdup("[Error]\n"
	           "Invalid Z-Lib found=");
	}
    }

    shell_view_set_enabled(FALSE);

    int i;
    GTimer *timer = g_timer_new();
    gdouble elapsed = 0;
    gchar src[65536], *tmpsrc;
    glong srclen = 65536;
    gchar *bdata_path;
    
    bdata_path = g_build_filename(path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, NULL, NULL)) {
        g_free(bdata_path);
        return g_strdup("[Error]\n"
                        PREFIX "benchmark.data not found=\n");
    }     
    
    shell_status_update("Compressing 64MB with default options...");
    
    for (i = 0; i <= 1000; i++) { 
        g_timer_start(timer);
        
        gchar *dst;
        glong dstlen = compressBound(srclen);
        
        dst = g_new0(gchar, dstlen);
        compress(dst, &dstlen, src, srclen);

        g_timer_stop(timer);
        elapsed += g_timer_elapsed(timer, NULL);
        g_free(dst);
        
        shell_status_set_percentage(i/10);
    }
    
    g_timer_destroy(timer);
    g_free(bdata_path);

    gchar *retval = g_strdup_printf("[Results <i>(in seconds; lower is better)</i>]\n"
                           "<b>This Machine</b>=<b>%.2f</b>\n", elapsed);
    return benchmark_include_results(retval, "ZLib");
}

