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
#define ANSWER 25
#define CRUNCH_TIME 5

gulong fib(gulong n)
{
    if (n == 0)
        return 0;
    else if (n <= 2)
        return 1;
    return fib(n - 1) + fib(n - 2);
}

static gpointer fib_for(void *in_data, gint thread_number)
{
    fib(ANSWER);

    return NULL;
}


void
benchmark_fib(void)
{
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Calculating Fibonacci number...");

    r = benchmark_crunch_for(CRUNCH_TIME, 0, fib_for, NULL);
    //r.threads_used = 1;

    r.result /= 100;

    r.revision = BENCH_REVISION;
    snprintf(r.extra, 255, "a:%d", ANSWER);

    bench_results[BENCHMARK_FIB] = r;
}
