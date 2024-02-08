/*
 *    HardInfo - System Information and Benchmark
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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

#include "benchmark.h"

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 2
#define CRUNCH_TIME 5

void fbench();	/* fbench.c */

static gpointer parallel_raytrace(void *in_data, gint thread_number)
{
    unsigned int i;

    fbench();

    return NULL;
}

void
benchmark_raytrace(void)
{
    bench_value r = EMPTY_BENCH_VALUE;
    gchar *test_data = get_test_data(1000);

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing John Walker's FBENCH...");

    r = benchmark_crunch_for(CRUNCH_TIME, 1, parallel_raytrace, test_data);

    r.revision = BENCH_REVISION;
    snprintf(r.extra, 255, "r:%d", 500);//niter from fbench

    g_free(test_data);

    r.result /= 10;
    
    bench_results[BENCHMARK_RAYTRACE] = r;
}

