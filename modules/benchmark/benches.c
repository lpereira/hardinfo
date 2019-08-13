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

/* These are parts of modules/benchmark.c where specific benchmarks are defined. */

#define BENCH_CALLBACK(CN, BN, BID, R) \
gchar *CN() { \
    if (R)    \
        return benchmark_include_results_reverse(bench_results[BID], BN); \
    else      \
        return benchmark_include_results(bench_results[BID], BN); \
}

/* lower is better R = 0 */
BENCH_CALLBACK(callback_fib, "CPU Fibonacci", BENCHMARK_FIB, 0);
BENCH_CALLBACK(callback_nqueens, "CPU N-Queens", BENCHMARK_NQUEENS, 0);
BENCH_CALLBACK(callback_fft, "FPU FFT", BENCHMARK_FFT, 0);
BENCH_CALLBACK(callback_raytr, "FPU Raytracing", BENCHMARK_RAYTRACE, 0);
/* higher is better R = 1 */
BENCH_CALLBACK(callback_bfsh_single, "CPU Blowfish (Single-thread)", BENCHMARK_BLOWFISH_SINGLE, 1);
BENCH_CALLBACK(callback_bfsh_threads, "CPU Blowfish (Multi-thread)", BENCHMARK_BLOWFISH_THREADS, 1);
BENCH_CALLBACK(callback_bfsh_cores, "CPU Blowfish (Multi-core)", BENCHMARK_BLOWFISH_CORES, 1);
BENCH_CALLBACK(callback_sbcpu_single, "SysBench CPU (Single-thread)", BENCHMARK_SBCPU_SINGLE, 1);
BENCH_CALLBACK(callback_sbcpu_all, "SysBench CPU (Multi-thread)", BENCHMARK_SBCPU_ALL, 1);
BENCH_CALLBACK(callback_sbcpu_quad, "SysBench CPU (Four threads)", BENCHMARK_SBCPU_QUAD, 1);
BENCH_CALLBACK(callback_memory_single, "SysBench Memory (Single-thread)", BENCHMARK_MEMORY_SINGLE, 1);
BENCH_CALLBACK(callback_memory_dual, "SysBench Memory (Two threads)", BENCHMARK_MEMORY_DUAL, 1);
BENCH_CALLBACK(callback_memory_quad, "SysBench Memory (Four threads)", BENCHMARK_MEMORY_QUAD, 1);
BENCH_CALLBACK(callback_cryptohash, "CPU CryptoHash", BENCHMARK_CRYPTOHASH, 1);
BENCH_CALLBACK(callback_zlib, "CPU Zlib", BENCHMARK_ZLIB, 1);
BENCH_CALLBACK(callback_gui, "GPU Drawing", BENCHMARK_GUI, 1);

#define BENCH_SCAN_SIMPLE(SN, BF, BID) \
void SN(gboolean reload) { \
    SCAN_START(); \
    do_benchmark(BF, BID); \
    SCAN_END(); \
}

BENCH_SCAN_SIMPLE(scan_fft, benchmark_fft, BENCHMARK_FFT);
BENCH_SCAN_SIMPLE(scan_nqueens, benchmark_nqueens, BENCHMARK_NQUEENS);
BENCH_SCAN_SIMPLE(scan_raytr, benchmark_raytrace, BENCHMARK_RAYTRACE);
BENCH_SCAN_SIMPLE(scan_bfsh_single, benchmark_bfish_single, BENCHMARK_BLOWFISH_SINGLE);
BENCH_SCAN_SIMPLE(scan_bfsh_threads, benchmark_bfish_threads, BENCHMARK_BLOWFISH_THREADS);
BENCH_SCAN_SIMPLE(scan_bfsh_cores, benchmark_bfish_cores, BENCHMARK_BLOWFISH_CORES);
BENCH_SCAN_SIMPLE(scan_sbcpu_single, benchmark_sbcpu_single, BENCHMARK_SBCPU_SINGLE);
BENCH_SCAN_SIMPLE(scan_sbcpu_quad, benchmark_sbcpu_quad, BENCHMARK_SBCPU_QUAD);
BENCH_SCAN_SIMPLE(scan_sbcpu_all, benchmark_sbcpu_all, BENCHMARK_SBCPU_ALL);
BENCH_SCAN_SIMPLE(scan_memory_single, benchmark_memory_single, BENCHMARK_MEMORY_SINGLE);
BENCH_SCAN_SIMPLE(scan_memory_dual, benchmark_memory_dual, BENCHMARK_MEMORY_DUAL);
BENCH_SCAN_SIMPLE(scan_memory_quad, benchmark_memory_quad, BENCHMARK_MEMORY_QUAD);
BENCH_SCAN_SIMPLE(scan_cryptohash, benchmark_cryptohash, BENCHMARK_CRYPTOHASH);
BENCH_SCAN_SIMPLE(scan_fib, benchmark_fib, BENCHMARK_FIB);
BENCH_SCAN_SIMPLE(scan_zlib, benchmark_zlib, BENCHMARK_ZLIB);

#if !GTK_CHECK_VERSION(3,0,0)
void scan_gui(gboolean reload)
{
    SCAN_START();

    bench_value er = EMPTY_BENCH_VALUE;

    if (params.run_benchmark) {
        int argc = 0;

        ui_init(&argc, NULL);
    }

    if (params.gui_running || params.run_benchmark) {
        do_benchmark(benchmark_gui, BENCHMARK_GUI);
    } else {
        bench_results[BENCHMARK_GUI] = er;
    }
    SCAN_END();
}
#endif

static ModuleEntry entries[] = {
    {N_("CPU Blowfish (Single-thread)"), "blowfish.png", callback_bfsh_single, scan_bfsh_single, MODULE_FLAG_NONE},
    {N_("CPU Blowfish (Multi-thread)"), "blowfish.png", callback_bfsh_threads, scan_bfsh_threads, MODULE_FLAG_NONE},
    {N_("CPU Blowfish (Multi-core)"), "blowfish.png", callback_bfsh_cores, scan_bfsh_cores, MODULE_FLAG_NONE},
    {N_("CPU Zlib"), "file-roller.png", callback_zlib, scan_zlib, MODULE_FLAG_NONE},
    {N_("CPU CryptoHash"), "cryptohash.png", callback_cryptohash, scan_cryptohash, MODULE_FLAG_NONE},
    {N_("CPU Fibonacci"), "nautilus.png", callback_fib, scan_fib, MODULE_FLAG_NONE},
    {N_("CPU N-Queens"), "nqueens.png", callback_nqueens, scan_nqueens, MODULE_FLAG_NONE},
    {N_("FPU FFT"), "fft.png", callback_fft, scan_fft, MODULE_FLAG_NONE},
    {N_("FPU Raytracing"), "raytrace.png", callback_raytr, scan_raytr, MODULE_FLAG_NONE},
    {N_("SysBench CPU (Single-thread)"), "processor.png", callback_sbcpu_single, scan_sbcpu_single, MODULE_FLAG_NONE},
    {N_("SysBench CPU (Multi-thread)"), "processor.png", callback_sbcpu_all, scan_sbcpu_all, MODULE_FLAG_NONE},
    {N_("SysBench CPU (Four threads)"), "processor.png", callback_sbcpu_quad, scan_sbcpu_quad, MODULE_FLAG_NONE},
    {N_("SysBench Memory (Single-thread)"), "memory.png", callback_memory_single, scan_memory_single, MODULE_FLAG_NONE},
    {N_("SysBench Memory (Two threads)"), "memory.png", callback_memory_dual, scan_memory_dual, MODULE_FLAG_NONE},
    {N_("SysBench Memory (Four threads)"), "memory.png", callback_memory_quad, scan_memory_quad, MODULE_FLAG_NONE},
#if !GTK_CHECK_VERSION(3,0,0)
    {N_("GPU Drawing"), "module.png", callback_gui, scan_gui, MODULE_FLAG_NO_REMOTE},
#endif
    {NULL}
};

const gchar *hi_note_func(gint entry)
{
    switch (entry) {
    case BENCHMARK_SBCPU_SINGLE:
    case BENCHMARK_SBCPU_QUAD:
    case BENCHMARK_SBCPU_ALL:
        return _("Alexey Kopytov's <i><b>sysbench</b></i> is required.\n"
                 "Results in events/second. Higher is better.");

    case BENCHMARK_MEMORY_SINGLE:
    case BENCHMARK_MEMORY_DUAL:
    case BENCHMARK_MEMORY_QUAD:
        return _("Alexey Kopytov's <i><b>sysbench</b></i> is required.\n"
                 "Results in MiB/second. Higher is better.");

    case BENCHMARK_CRYPTOHASH:
        return _("Results in MiB/second. Higher is better.");

    case BENCHMARK_BLOWFISH_SINGLE:
    case BENCHMARK_BLOWFISH_THREADS:
    case BENCHMARK_BLOWFISH_CORES:
    case BENCHMARK_ZLIB:
    case BENCHMARK_GUI:
        return _("Results in HIMarks. Higher is better.");

    case BENCHMARK_FFT:
    case BENCHMARK_RAYTRACE:
    case BENCHMARK_FIB:
    case BENCHMARK_NQUEENS:
        return _("Results in seconds. Lower is better.");
    }

    return NULL;
}
