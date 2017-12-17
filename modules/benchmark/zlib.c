/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2017 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include <glib.h>
#include <stdlib.h>
#include <zlib.h>

#include "benchmark.h"

static gpointer zlib_for(unsigned int start, unsigned int end, void *data, gint thread_number)
{
    char *compressed;
    uLong bound = compressBound(bound);
    unsigned int i;

    compressed = malloc(bound);
    if (!compressed)
        return NULL;

    for (i = start; i <= end; i++) {
        char uncompressed[65536];
        uLong compressedBound = bound;
        uLong destBound = sizeof(uncompressed);

        compress(compressed, &compressedBound, data, 65536);
        uncompress(uncompressed, &destBound, compressed, compressedBound);
    }

    free(compressed);

    return NULL;
}

void
benchmark_zlib(void)
{
    bench_value r = EMPTY_BENCH_VALUE;
    gchar *tmpsrc, *bdata_path;

    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, NULL, NULL)) {
        g_free(bdata_path);
        return;
    }

    shell_view_set_enabled(FALSE);
    shell_status_update("Running Zlib benchmark...");

    r = benchmark_parallel_for(0, 0, 50000, zlib_for, tmpsrc);

    g_free(bdata_path);
    g_free(tmpsrc);

    //TODO: explain in code comments
    gdouble marks = (50000. * 65536.) / (r.elapsed_time * 840205128.);
    r.result = marks;
    bench_results[BENCHMARK_ZLIB] = r;
}
