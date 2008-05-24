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
#include <sha1.h>

static void
benchmark_sha1(void)
{
    SHA1_CTX ctx;
    guchar checksum[20];
    int i;
    GTimer *timer = g_timer_new();
    gdouble elapsed = 0;
    gchar src[65536], *tmpsrc;
    glong srclen = 65536;
    
    tmpsrc = src;

    gchar *bdata_path;
    
    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, NULL, NULL)) {
        g_free(bdata_path);
        return;
    }     
    
    shell_view_set_enabled(FALSE);
    shell_status_update("Generating SHA1 sum for 312MiB of data...");
    
    for (i = 0; i <= 5000; i++) { 
        g_timer_start(timer);

        SHA1Init(&ctx);
        SHA1Update(&ctx, (guchar*)tmpsrc, srclen);
        SHA1Final(checksum, &ctx);
        
        g_timer_stop(timer);
        elapsed += g_timer_elapsed(timer, NULL);
        
        shell_status_set_percentage(i/50);
    }
    
    g_timer_destroy(timer);
    g_free(bdata_path);
    
    bench_results[BENCHMARK_SHA1] = 312.0 / elapsed;
}

