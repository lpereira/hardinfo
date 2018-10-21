/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include "hardinfo.h"
#include "benchmark.h"
#include "blowfish.h"

#define CRUNCH_TIME 7

/* must be less than or equal to
 * file size of ( params.path_data + "benchmark.data" ) */
#define BENCH_DATA_SIZE 65536

static gpointer bfish_exec(void *in_data, gint thread_number)
{
    unsigned char key[] = "Has my shampoo arrived?";
    unsigned char *data = NULL;
    unsigned long data_len = BENCH_DATA_SIZE, i = 0;
    BLOWFISH_CTX ctx;

    data = malloc(BENCH_DATA_SIZE);
    memcpy(data, in_data, BENCH_DATA_SIZE);

    Blowfish_Init(&ctx, key, strlen(key));
    for(i = 0; i < data_len; i += 8) {
        Blowfish_Encrypt(&ctx, (unsigned long*)&data[i], (unsigned long*)&data[i+4]);
    }
    for(i = 0; i < data_len; i += 8) {
        Blowfish_Decrypt(&ctx, (unsigned long*)&data[i], (unsigned long*)&data[i+4]);
    }

    free(data);
    return NULL;
}

static gchar *get_data()
{
    gchar *tmpsrc, *bdata_path;
    gsize length;

    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, &length, NULL)) {
        g_free(bdata_path);
        return NULL;
    }
    if (length < BENCH_DATA_SIZE) {
        g_free(tmpsrc);
        return NULL;
    }

    return tmpsrc;
}

void benchmark_bfish_do(int threads, int entry, const char *status)
{
    bench_value r = EMPTY_BENCH_VALUE;
    gchar *test_data = get_data();

    shell_view_set_enabled(FALSE);
    shell_status_update(status);

    r = benchmark_crunch_for(CRUNCH_TIME, threads, bfish_exec, test_data);
    r.result /= 100;
    bench_results[entry] = r;

    g_free(test_data);
}

void benchmark_bfish_threads(void) { benchmark_bfish_do(0, BENCHMARK_BLOWFISH_THREADS, "Performing Blowfish benchmark (multi-thread)..."); }
void benchmark_bfish_single(void) { benchmark_bfish_do(1, BENCHMARK_BLOWFISH_SINGLE, "Performing Blowfish benchmark (single-thread)..."); }
void benchmark_bfish_cores(void) { benchmark_bfish_do(-1, BENCHMARK_BLOWFISH_CORES, "Performing Blowfish benchmark (multi-core)..."); }
