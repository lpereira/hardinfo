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

void inline md5_step(char *data, glong srclen); /* cryptohash.c */
void inline sha1_step(char *data, glong srclen); /* cryptohash.c */

static gpointer cruncher(void *data, gint thread_number)
{
    md5_step(data, 65536);
    sha1_step(data, 65536);

    BLOWFISH_CTX ctx;
    unsigned long L, R;
    L = 0xBEBACAFE;
    R = 0xDEADBEEF;
    Blowfish_Init(&ctx, (unsigned char*)data, 65536);
    Blowfish_Encrypt(&ctx, &L, &R);
    Blowfish_Decrypt(&ctx, &L, &R);

    return NULL;
}

#define CRUNCH_TIME 7

static gchar *get_data()
{
    gchar *tmpsrc, *bdata_path;

    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, NULL, NULL)) {
        g_free(bdata_path);
        return NULL;
    }

    return tmpsrc;
}

void benchmark_crunch_do(int threads, int entry)
{
    bench_value r = EMPTY_BENCH_VALUE;
    gchar *test_data = get_data();

    r = benchmark_crunch_for(CRUNCH_TIME, threads, cruncher, test_data);
    bench_results[entry] = r;

    g_free(test_data);
}

void benchmark_crunch(void) { benchmark_crunch_do(0, BENCHMARK_CRUNCH); }
void benchmark_crunch_single(void) { benchmark_crunch_do(1, BENCHMARK_CRUNCH_SINGLE); }
void benchmark_crunch_cores(void) { benchmark_crunch_do(-1, BENCHMARK_CRUNCH_CORES); }
