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
    gdouble elapsed = 0;
    int n_cores, i;
    gchar *temp;
    FFTBench **benches;
    
    shell_view_set_enabled(FALSE);
    shell_status_update("Running FFT benchmark...");
        
    /* Pre-allocate all benchmarks */
    temp = module_call_method("devices::getProcessorCount");
    n_cores = temp ? atoi(temp) : 1;
    g_free(temp);
    
    benches = g_new0(FFTBench *, n_cores);
    for (i = 0; i < n_cores; i++) {
      benches[i] = fft_bench_new();
    }
    
    /* Run the benchmark */
    elapsed = benchmark_parallel_for(0, 4, fft_for, benches);
    
    /* Free up the memory */
    for (i = 0; i < n_cores; i++) {
      fft_bench_free(benches[i]);
    }
    g_free(benches);
        
    bench_results[BENCHMARK_FFT] = elapsed;
}
