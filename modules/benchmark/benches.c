/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
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

#define BENCH_SCAN_SIMPLE(SN, BF, BID) \
void SN(gboolean reload) { \
    SCAN_START(); \
    do_benchmark(BF, BID); \
    SCAN_END(); \
}

#define BENCH_SIMPLE(BID, BN, BF, R) \
    BENCH_CALLBACK(callback_##BF, BN, BID, R); \
    BENCH_SCAN_SIMPLE(scan_##BF, BF, BID);

// ID, NAME, FUNCTION, R (0 = lower is better, 1 = higher is better)
BENCH_SIMPLE(BENCHMARK_FIB, "CPU Fibonacci", benchmark_fib, 0);
BENCH_SIMPLE(BENCHMARK_NQUEENS, "CPU N-Queens", benchmark_nqueens, 0);
BENCH_SIMPLE(BENCHMARK_FFT, "FPU FFT", benchmark_fft, 0);
BENCH_SIMPLE(BENCHMARK_RAYTRACE, "FPU Raytracing", benchmark_raytrace, 0);
BENCH_SIMPLE(BENCHMARK_BLOWFISH_SINGLE, "CPU Blowfish (Single-thread)", benchmark_bfish_single, 1);
BENCH_SIMPLE(BENCHMARK_BLOWFISH_THREADS, "CPU Blowfish (Multi-thread)", benchmark_bfish_threads, 1);
BENCH_SIMPLE(BENCHMARK_BLOWFISH_CORES, "CPU Blowfish (Multi-core)", benchmark_bfish_cores, 1);
BENCH_SIMPLE(BENCHMARK_ZLIB, "CPU Zlib", benchmark_zlib, 1);
BENCH_SIMPLE(BENCHMARK_CRYPTOHASH, "CPU CryptoHash", benchmark_cryptohash, 1);
BENCH_SIMPLE(BENCHMARK_SBCPU_SINGLE, "SysBench CPU (Single-thread)", benchmark_sbcpu_single, 1);
BENCH_SIMPLE(BENCHMARK_SBCPU_ALL, "SysBench CPU (Multi-thread)", benchmark_sbcpu_all, 1);
BENCH_SIMPLE(BENCHMARK_SBCPU_QUAD, "SysBench CPU (Four threads)", benchmark_sbcpu_quad, 1);
BENCH_SIMPLE(BENCHMARK_MEMORY_SINGLE, "SysBench Memory (Single-thread)", benchmark_memory_single, 1);
BENCH_SIMPLE(BENCHMARK_MEMORY_DUAL, "SysBench Memory (Two threads)", benchmark_memory_dual, 1);
BENCH_SIMPLE(BENCHMARK_MEMORY_QUAD, "SysBench Memory", benchmark_memory_quad, 1);

#if !GTK_CHECK_VERSION(3,0,0)
BENCH_CALLBACK(callback_gui, "GPU Drawing", BENCHMARK_GUI, 1);
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
    [BENCHMARK_BLOWFISH_SINGLE] =
        {
            N_("CPU Blowfish (Single-thread)"),
            "blowfish.png",
            callback_benchmark_bfish_single,
            scan_benchmark_bfish_single,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_BLOWFISH_THREADS] =
        {
            N_("CPU Blowfish (Multi-thread)"),
            "blowfish.png",
            callback_benchmark_bfish_threads,
            scan_benchmark_bfish_threads,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_BLOWFISH_CORES] =
        {
            N_("CPU Blowfish (Multi-core)"),
            "blowfish.png",
            callback_benchmark_bfish_cores,
            scan_benchmark_bfish_cores,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_ZLIB] =
        {
            N_("CPU Zlib"),
            "file-roller.png",
            callback_benchmark_zlib,
            scan_benchmark_zlib,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_CRYPTOHASH] =
        {
            N_("CPU CryptoHash"),
            "cryptohash.png",
            callback_benchmark_cryptohash,
            scan_benchmark_cryptohash,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_FIB] =
        {
            N_("CPU Fibonacci"),
            "nautilus.png",
            callback_benchmark_fib,
            scan_benchmark_fib,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_NQUEENS] =
        {
            N_("CPU N-Queens"),
            "nqueens.png",
            callback_benchmark_nqueens,
            scan_benchmark_nqueens,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_FFT] =
        {
            N_("FPU FFT"),
            "fft.png",
            callback_benchmark_fft,
            scan_benchmark_fft,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_RAYTRACE] =
        {
            N_("FPU Raytracing"),
            "raytrace.png",
            callback_benchmark_raytrace,
            scan_benchmark_raytrace,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_SBCPU_SINGLE] =
        {
            N_("SysBench CPU (Single-thread)"),
            "processor.png",
            callback_benchmark_sbcpu_single,
            scan_benchmark_sbcpu_single,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_SBCPU_ALL] =
        {
            N_("SysBench CPU (Multi-thread)"),
            "processor.png",
            callback_benchmark_sbcpu_all,
            scan_benchmark_sbcpu_all,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_SBCPU_QUAD] =
        {
            N_("SysBench CPU (Four threads)"),
            "processor.png",
            callback_benchmark_sbcpu_quad,
            scan_benchmark_sbcpu_quad,
            MODULE_FLAG_HIDE,
        },
    [BENCHMARK_MEMORY_SINGLE] =
        {
            N_("SysBench Memory (Single-thread)"),
            "memory.png",
            callback_benchmark_memory_single,
            scan_benchmark_memory_single,
            MODULE_FLAG_NONE,
        },
    [BENCHMARK_MEMORY_DUAL] =
        {
            N_("SysBench Memory (Two threads)"),
            "memory.png",
            callback_benchmark_memory_dual,
            scan_benchmark_memory_dual,
            MODULE_FLAG_HIDE,
        },
    [BENCHMARK_MEMORY_QUAD] =
        {
            N_("SysBench Memory"),
            "memory.png",
            callback_benchmark_memory_quad,
            scan_benchmark_memory_quad,
            MODULE_FLAG_NONE,
        },
#if !GTK_CHECK_VERSION(3, 0, 0)
    [BENCHMARK_GUI] =
        {
            N_("GPU Drawing"),
            "module.png",
            callback_gui,
            scan_gui,
            MODULE_FLAG_NO_REMOTE | MODULE_FLAG_HIDE,
        },
#else
    [BENCHMARK_GUI] = {"#"},
#endif
    {NULL}};

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
