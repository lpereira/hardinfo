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

BENCH_CALLBACK(callback_gui, "GPU Drawing", BENCHMARK_GUI, 1);
BENCH_CALLBACK(callback_fft, "FPU FFT", BENCHMARK_FFT, 0);
BENCH_CALLBACK(callback_nqueens, "CPU N-Queens", BENCHMARK_NQUEENS, 0);
BENCH_CALLBACK(callback_raytr, "FPU Raytracing", BENCHMARK_RAYTRACE, 0);
BENCH_CALLBACK(callback_bfsh, "CPU Blowfish", BENCHMARK_BLOWFISH, 0);
BENCH_CALLBACK(callback_cryptohash, "CPU CryptoHash", BENCHMARK_CRYPTOHASH, 1);
BENCH_CALLBACK(callback_fib, "CPU Fibonacci", BENCHMARK_FIB, 0);
BENCH_CALLBACK(callback_zlib, "CPU Zlib", BENCHMARK_ZLIB, 0);

#define BENCH_SCAN_SIMPLE(SN, BF, BID) \
void SN(gboolean reload) { \
    SCAN_START(); \
    do_benchmark(BF, BID); \
    SCAN_END(); \
}

BENCH_SCAN_SIMPLE(scan_fft, benchmark_fft, BENCHMARK_FFT);
BENCH_SCAN_SIMPLE(scan_nqueens, benchmark_nqueens, BENCHMARK_NQUEENS);
BENCH_SCAN_SIMPLE(scan_raytr, benchmark_raytrace, BENCHMARK_RAYTRACE);
BENCH_SCAN_SIMPLE(scan_bfsh, benchmark_fish, BENCHMARK_BLOWFISH);
BENCH_SCAN_SIMPLE(scan_cryptohash, benchmark_cryptohash, BENCHMARK_CRYPTOHASH);
BENCH_SCAN_SIMPLE(scan_fib, benchmark_fib, BENCHMARK_FIB);
BENCH_SCAN_SIMPLE(scan_zlib, benchmark_zlib, BENCHMARK_ZLIB);

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

static ModuleEntry entries[] = {
    {N_("CPU Blowfish"), "blowfish.png", callback_bfsh, scan_bfsh, MODULE_FLAG_NONE},
    {N_("CPU CryptoHash"), "cryptohash.png", callback_cryptohash, scan_cryptohash, MODULE_FLAG_NONE},
    {N_("CPU Fibonacci"), "nautilus.png", callback_fib, scan_fib, MODULE_FLAG_NONE},
    {N_("CPU N-Queens"), "nqueens.png", callback_nqueens, scan_nqueens, MODULE_FLAG_NONE},
    {N_("CPU Zlib"), "file-roller.png", callback_zlib, scan_zlib, MODULE_FLAG_NONE},
    {N_("FPU FFT"), "fft.png", callback_fft, scan_fft, MODULE_FLAG_NONE},
    {N_("FPU Raytracing"), "raytrace.png", callback_raytr, scan_raytr, MODULE_FLAG_NONE},
#if !GTK_CHECK_VERSION(3,0,0)
    {N_("GPU Drawing"), "module.png", callback_gui, scan_gui, MODULE_FLAG_NO_REMOTE},
#endif
    {NULL}
};

const gchar *hi_note_func(gint entry)
{
    switch (entry) {
    case BENCHMARK_CRYPTOHASH:
        return _("Results in MiB/second. Higher is better.");

    case BENCHMARK_ZLIB:
    case BENCHMARK_GUI:
        return _("Results in HIMarks. Higher is better.");

    case BENCHMARK_FFT:
    case BENCHMARK_RAYTRACE:
    case BENCHMARK_BLOWFISH:
    case BENCHMARK_FIB:
    case BENCHMARK_NQUEENS:
        return _("Results in seconds. Lower is better.");
    }

    return NULL;
}
