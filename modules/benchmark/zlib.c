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

/* zip/unzip 256KB blocks for 7 seconds
 * result is number of full completions / 100 */
#define BENCH_DATA_SIZE 262144
#define CRUNCH_TIME 7

static gpointer zlib_for(void *in_data, gint thread_number) {
    char *compressed;
    uLong bound = compressBound(BENCH_DATA_SIZE);
    unsigned int i;

    compressed = malloc(bound);
    if (!compressed)
        return NULL;

    char uncompressed[BENCH_DATA_SIZE];
    uLong compressedBound = bound;
    uLong destBound = sizeof(uncompressed);

    compress(compressed, &compressedBound, in_data, BENCH_DATA_SIZE);
    uncompress(uncompressed, &destBound, compressed, compressedBound);

    free(compressed);

    return NULL;
}

static gchar *get_test_data(gsize min_size) {
    gchar *bdata_path, *data;
    gsize data_size;

    gchar *exp_data, *p;
    gsize sz;

    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &data, &data_size, NULL)) {
        g_free(bdata_path);
        return NULL;
    }

    if (data_size < min_size) {
        DEBUG("expanding %lu bytes of test data to %lu bytes", data_size, min_size);
        exp_data = g_malloc(min_size + 1);
        memcpy(exp_data, data, data_size);
        p = exp_data + data_size;
        sz = data_size;
        while (sz < (min_size - data_size) ) {
            memcpy(p, data, data_size);
            p += data_size;
            sz += data_size;
        }
        strncpy(p, data, min_size - sz);
        g_free(data);
        data = exp_data;
    }
    g_free(bdata_path);
    return data;
}

void
benchmark_zlib(void)
{
    bench_value r = EMPTY_BENCH_VALUE;
    gchar *data = get_test_data(BENCH_DATA_SIZE);
    if (!data)
        return;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running Zlib benchmark...");

    r = benchmark_crunch_for(CRUNCH_TIME, 0, zlib_for, data);
    r.result /= 100;
    bench_results[BENCHMARK_ZLIB] = r;

    g_free(data);
}
