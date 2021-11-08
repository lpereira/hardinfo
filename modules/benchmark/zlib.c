/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2017 L. A. F. Pereira <l@tia.mat.br>
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

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 3
#define BENCH_DATA_SIZE 262144
#define BENCH_DATA_MD5 "3753b649c4fa9ea4576fc8f89a773de2"
#define CRUNCH_TIME 7
#define VERIFY_RESULT 1

static unsigned int zlib_errors = 0;

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
    if (VERIFY_RESULT) {
        int cr = memcmp(in_data, uncompressed, BENCH_DATA_SIZE);
        if (!!cr) {
            zlib_errors++;
            bench_msg("zlib error: uncompressed != original");
        }
    }

    free(compressed);

    return NULL;
}

void
benchmark_zlib(void)
{
    bench_value r = EMPTY_BENCH_VALUE;
    gchar *test_data = get_test_data(BENCH_DATA_SIZE);
    if (!test_data)
        return;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running Zlib benchmark...");

    gchar *d = md5_digest_str(test_data, BENCH_DATA_SIZE);
    if (!SEQ(d, BENCH_DATA_MD5))
        bench_msg("test data has different md5sum: expected %s, actual %s", BENCH_DATA_MD5, d);

    r = benchmark_crunch_for(CRUNCH_TIME, 0, zlib_for, test_data);
    r.result /= 100;
    r.revision = BENCH_REVISION;
    snprintf(r.extra, 255, "zlib %s (built against: %s), d:%s, e:%d", zlib_version, ZLIB_VERSION, d, zlib_errors);
    bench_results[BENCHMARK_ZLIB] = r;

    g_free(test_data);
    g_free(d);
}
