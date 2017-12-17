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

#include "benchmark.h"

gulong fib(gulong n)
{
    if (n == 0)
        return 0;
    else if (n <= 2)
        return 1;
    return fib(n - 1) + fib(n - 2);
}

void
benchmark_fib(void)
{
    GTimer *timer = g_timer_new();
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Calculating the 42nd Fibonacci number...");

    g_timer_reset(timer);
    g_timer_start(timer);

    fib(42);

    g_timer_stop(timer);
    r.elapsed_time = g_timer_elapsed(timer, NULL);
    g_timer_destroy(timer);

    r.threads_used = 1;
    r.result = r.elapsed_time;

    bench_results[BENCHMARK_FIB] = r;
}
