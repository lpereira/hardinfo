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

double flops_main(); /* flops.c */

static gpointer flops_wrapper(unsigned int start, unsigned int end, void *data, gint thread_number) {
    double *ret = g_malloc(sizeof(double));
    double w = 0;
    *ret = flops_main();
    return ret;
}

void benchmark_flops_do(int threads, int entry)
{
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running Al Aburto's flops.c benchmark...");

    r = benchmark_parallel(threads, flops_wrapper, NULL);
    bench_results[entry] = r;
}

void benchmark_flops_single(void) { benchmark_flops_do(1, BENCHMARK_FLOPS_SINGLE); }
void benchmark_flops_multi(void) { benchmark_flops_do(0, BENCHMARK_FLOPS_MULTI); }
void benchmark_flops_cores(void) { benchmark_flops_do(-1, BENCHMARK_FLOPS_CORES); }
