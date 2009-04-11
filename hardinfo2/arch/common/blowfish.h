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

#include <blowfish.h>

static gpointer
parallel_blowfish(unsigned int start, unsigned int end, void *data, gint thread_number)
{
    BLOWFISH_CTX ctx;
    unsigned int i;
    unsigned long L, R;

    L = 0xBEBACAFE;
    R = 0xDEADBEEF;

    for (i = start; i <= end; i++) { 
        Blowfish_Init(&ctx, (unsigned char*)data, 65536);
        Blowfish_Encrypt(&ctx, &L, &R);
        Blowfish_Decrypt(&ctx, &L, &R);
    }

    return NULL;
}

static void
benchmark_fish(void)
{
    gdouble elapsed = 0;
    gchar *tmpsrc;

    gchar *bdata_path;

    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, NULL, NULL)) {
        g_free(bdata_path);
        return;
    }

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing Blowfish benchmark...");

    elapsed = benchmark_parallel_for(0, 50000, parallel_blowfish, tmpsrc);

    g_free(bdata_path);
    g_free(tmpsrc);

    bench_results[BENCHMARK_BLOWFISH] = elapsed;
}
