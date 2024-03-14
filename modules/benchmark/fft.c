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

#include "hardinfo.h"
#include "benchmark.h"
#include "fftbench.h"

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 3
#define CRUNCH_TIME 5

static gpointer fft_for(void *in_data, gint thread_number)
{
    unsigned int i;
    FFTBench **benches = (FFTBench **)in_data;
    FFTBench *fftbench = (FFTBench *)(benches[thread_number]);

    fft_bench_run(benches[thread_number]);

    return NULL;
}

void
benchmark_fft(void)
{
    int cpu_procs, cpu_cores, cpu_threads, cpu_nodes;
    bench_value r = EMPTY_BENCH_VALUE;

    int n_cores, i;
    gchar *temp;
    FFTBench **benches=NULL;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running FFT benchmark...");

    cpu_procs_cores_threads_nodes(&cpu_procs, &cpu_cores, &cpu_threads, &cpu_nodes);

    /* Pre-allocate all benchmarks */
    benches = g_new0(FFTBench *, cpu_threads);
    for (i = 0; i < cpu_threads; i++) {benches[i] = fft_bench_new();}

    /* Run the benchmark */
    r = benchmark_crunch_for(CRUNCH_TIME, 0, fft_for, benches);

    /* Free up the memory */
    for (i = 0; i < cpu_threads; i++) {
        fft_bench_free(benches[i]);
    }
    g_free(benches);

    r.result /= 100;
    
    r.revision = BENCH_REVISION;
    bench_results[BENCHMARK_FFT] = r;
}
