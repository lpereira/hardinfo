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

static gpointer
parallel_zlib(unsigned int start, unsigned int end, void *data)
{
    GModule *libz;
    gint i;
    glong srclen = 65536;
    
    static gulong (*compressBound) (glong srclen) = NULL;
    static gint (*compress) (gchar *dst, glong *dstlen,
                             const gchar *src, glong srclen) = NULL;

    if (!(compress && compressBound)) {
	libz = g_module_open("libz", G_MODULE_BIND_LAZY);
	if (!libz) {
            libz = g_module_open("/usr/lib/libz.so", G_MODULE_BIND_LAZY);
            if (!libz) {
                g_warning("Cannot load ZLib: %s", g_module_error());
                return NULL;
            }
	}

	if (!g_module_symbol(libz, "compress", (gpointer) & compress)
	    || !g_module_symbol(libz, "compressBound", (gpointer) & compressBound)) {
	    
            g_module_close(libz);
	    return NULL;
	}
    }

    for (i = start; i <= end; i++) {
        gchar *dst;
        glong dstlen = compressBound(srclen);
        
        dst = g_new0(gchar, dstlen);
        compress(dst, &dstlen, data, srclen);
        g_free(dst);
    }
    
    return NULL;
}


static void
benchmark_zlib(void)
{
    GTimer *timer = g_timer_new();
    gdouble elapsed = 0;
    gchar *tmpsrc;
    gchar *bdata_path;
    
    shell_view_set_enabled(FALSE);

    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, NULL, NULL)) {
        g_free(bdata_path);
        return;
    }     
    
    shell_status_update("Compressing 64MB with default options...");
    
    g_timer_start(timer);
    benchmark_parallel_for(0, 1000, parallel_zlib, tmpsrc);
    g_timer_stop(timer);
    
    elapsed = g_timer_elapsed(timer, NULL);
    
    g_timer_destroy(timer);
    g_free(bdata_path);
    g_free(tmpsrc);
    
    bench_results[BENCHMARK_ZLIB] = 65536.0 / elapsed;
}
