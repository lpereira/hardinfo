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

#include "hardinfo.h"
#include "benchmark.h"
#include "fftbench.h"

/* if anything changes in this block, increment revision */
#define BENCH_REVISION -1
#define FFT_MAXT 4

static gpointer fft_for(unsigned int start, unsigned int end, void *data, gint thread_number)
{
    unsigned int i;
    FFTBench **benches = (FFTBench **)data;
    FFTBench *fftbench = (FFTBench *)(benches[thread_number]);

    for (i = start; i <= end; i++) {
        fft_bench_run(fftbench);
    }

    return NULL;
}

void
benchmark_fft(void)
{
    bench_value r = EMPTY_BENCH_VALUE;

    int n_cores, i;
    gchar *temp;
    FFTBench **benches;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running FFT benchmark...");

    /* Pre-allocate all benchmarks */
    benches = g_new0(FFTBench *, FFT_MAXT);
    for (i = 0; i < FFT_MAXT; i++) {
        benches[i] = fft_bench_new();
    }

    /* Run the benchmark */
    r = benchmark_parallel_for(FFT_MAXT, 0, FFT_MAXT, fft_for, benches);

    /* Free up the memory */
    for (i = 0; i < FFT_MAXT; i++) {
        fft_bench_free(benches[i]);
    }
    g_free(benches);

    r.result = r.elapsed_time;
    r.revision = BENCH_REVISION;
    bench_results[BENCHMARK_FFT] = r;
}
