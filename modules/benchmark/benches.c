/*
 *    HardInfo - System Information and Benchmark
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
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

/* These are parts of modules/benchmark.c where specific benchmarks are defined. */

#define BENCH_CALLBACK(CN, BN, BID, R) \
gchar *CN() { \
    DEBUG("BENCH CALLBACK %s\n",BN); \
    params.aborting_benchmarks=0; \
    if (R) \
        return benchmark_include_results_reverse(bench_results[BID], BN); \
    else \
    return benchmark_include_results(bench_results[BID], BN); \
}

#if(HARDINFO2_QT5)
#define BENCH_SCAN_SIMPLE(SN, BF, BID, BN)	\
void SN(gboolean reload) { \
    static gboolean scanned=FALSE; \
    if(params.aborting_benchmarks) return; \
    if(reload || bench_results[BID].result<=0.0) scanned = FALSE; \
    if(reload){DEBUG("BENCH SCAN RELOAD %s\n",BN);} else if(scanned) {DEBUG("BENCH SCAN OK %s\n",BN);}else{DEBUG("BENCH SCAN %s\n",BN);} \
    if(scanned) return; \
    if(BID!=BENCHMARK_OPENGL || params.gui_running || params.run_benchmark) \
        do_benchmark(BF, BID); \
    scanned = TRUE; \
}
#else
#define BENCH_SCAN_SIMPLE(SN, BF, BID, BN)	\
void SN(gboolean reload) { \
    static gboolean scanned=FALSE; \
    if(params.aborting_benchmarks) return; \
    if(reload || bench_results[BID].result<=0.0) scanned = FALSE; \
    if(reload){DEBUG("BENCH SCAN RELOAD %s\n",BN);} else if(scanned) {DEBUG("BENCH SCAN OK %s\n",BN);}else{DEBUG("BENCH SCAN %s\n",BN);} \
    if(scanned) return; \
    do_benchmark(BF, BID); \
    scanned = TRUE; \
}
#endif

#define BENCH_SIMPLE(BID, BN, BF, R) \
    BENCH_CALLBACK(callback_##BF, BN, BID, R); \
    BENCH_SCAN_SIMPLE(scan_##BF, BF, BID, BN);

// ID, NAME, FUNCTION, R (0 = lower is better, 1 = higher is better)
BENCH_SIMPLE(BENCHMARK_FIB, "CPU Fibonacci", benchmark_fib, 1);
BENCH_SIMPLE(BENCHMARK_NQUEENS, "CPU N-Queens", benchmark_nqueens, 1);
BENCH_SIMPLE(BENCHMARK_FFT, "FPU FFT", benchmark_fft, 1);
BENCH_SIMPLE(BENCHMARK_RAYTRACE, "FPU Raytracing (Single-thread)", benchmark_raytrace, 1);
BENCH_SIMPLE(BENCHMARK_BLOWFISH_SINGLE, "CPU Blowfish (Single-thread)", benchmark_bfish_single, 1);
BENCH_SIMPLE(BENCHMARK_BLOWFISH_THREADS, "CPU Blowfish (Multi-thread)", benchmark_bfish_threads, 1);
BENCH_SIMPLE(BENCHMARK_BLOWFISH_CORES, "CPU Blowfish (Multi-core)", benchmark_bfish_cores, 1);
BENCH_SIMPLE(BENCHMARK_ZLIB, "CPU Zlib", benchmark_zlib, 1);
BENCH_SIMPLE(BENCHMARK_CRYPTOHASH, "CPU CryptoHash", benchmark_cryptohash, 1);
BENCH_SIMPLE(BENCHMARK_IPERF3_SINGLE, "Internal Network Speed", benchmark_iperf3_single, 1);
#if(HARDINFO2_QT5)
BENCH_SIMPLE(BENCHMARK_OPENGL, "GPU OpenGL Drawing", benchmark_opengl, 1);
#endif
BENCH_SIMPLE(BENCHMARK_SBCPU_SINGLE, "SysBench CPU (Single-thread)", benchmark_sbcpu_single, 1);
BENCH_SIMPLE(BENCHMARK_SBCPU_ALL, "SysBench CPU (Multi-thread)", benchmark_sbcpu_all, 1);
BENCH_SIMPLE(BENCHMARK_SBCPU_QUAD, "SysBench CPU (Four threads)", benchmark_sbcpu_quad, 1);
BENCH_SIMPLE(BENCHMARK_MEMORY_SINGLE, "SysBench Memory (Single-thread)", benchmark_memory_single, 1);
BENCH_SIMPLE(BENCHMARK_MEMORY_DUAL, "SysBench Memory (Two threads)", benchmark_memory_dual, 1);
BENCH_SIMPLE(BENCHMARK_MEMORY_QUAD, "SysBench Memory (Quad threads)", benchmark_memory_quad, 1);
BENCH_SIMPLE(BENCHMARK_MEMORY_ALL, "SysBench Memory (Multi-thread)", benchmark_memory_all, 1);
BENCH_SIMPLE(BENCHMARK_STORAGE, "Storage R/W Speed", benchmark_storage, 1);
BENCH_SIMPLE(BENCHMARK_CACHEMEM, "Cache/Memory", benchmark_cachemem, 1);

BENCH_CALLBACK(callback_benchmark_gui, "GPU Drawing", BENCHMARK_GUI, 1);
void scan_benchmark_gui(gboolean reload)
{
    static gboolean scanned=FALSE;
    if(params.aborting_benchmarks) return;
    if (reload || bench_results[BENCHMARK_GUI].result<=0.0) scanned = FALSE;
    if (scanned) return;

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
    scanned = TRUE;
}

//Note: Same order as entries, used for json to server
static char *entries_english_name[] = {
            "CPU Blowfish (Single-thread)",
            "CPU Blowfish (Multi-thread)",
            "CPU Blowfish (Multi-core)",
            "CPU Zlib",
            "CPU CryptoHash",
            "CPU Fibonacci",
            "CPU N-Queens",
            "FPU FFT",
            "FPU Raytracing (Single-thread)",
            "Internal Network Speed",
            "SysBench CPU (Single-thread)",
            "SysBench CPU (Multi-thread)",
            "SysBench CPU (Four threads)",
            "SysBench Memory (Single-thread)",
            "SysBench Memory (Two threads)",
            "SysBench Memory (Quad threads)",
            "SysBench Memory (Multi-thread)",
            "GPU Drawing"
#if(HARDINFO2_QT5)
	    ,"GPU OpenGL Drawing"
#endif
	    ,"Storage R/W Speed"
	    ,"Cache/Memory"
};


static ModuleEntry entries[] = {
    [BENCHMARK_BLOWFISH_SINGLE] =
        {
            N_("CPU Blowfish (Single-thread)"),
            "blowfish.svg",
            callback_benchmark_bfish_single,
            scan_benchmark_bfish_single,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_BLOWFISH_THREADS] =
        {
            N_("CPU Blowfish (Multi-thread)"),
            "blowfish.svg",
            callback_benchmark_bfish_threads,
            scan_benchmark_bfish_threads,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_BLOWFISH_CORES] =
        {
            N_("CPU Blowfish (Multi-core)"),
            "blowfish.svg",
            callback_benchmark_bfish_cores,
            scan_benchmark_bfish_cores,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_ZLIB] =
        {
            N_("CPU Zlib"),
            "compress.svg",
            callback_benchmark_zlib,
            scan_benchmark_zlib,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_CRYPTOHASH] =
        {
            N_("CPU CryptoHash"),
            "cryptohash.svg",
            callback_benchmark_cryptohash,
            scan_benchmark_cryptohash,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_FIB] =
        {
            N_("CPU Fibonacci"),
            "nautilus.svg",
            callback_benchmark_fib,
            scan_benchmark_fib,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_NQUEENS] =
        {
            N_("CPU N-Queens"),
            "nqueens.svg",
            callback_benchmark_nqueens,
            scan_benchmark_nqueens,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_FFT] =
        {
            N_("FPU FFT"),
            "fft.svg",
            callback_benchmark_fft,
            scan_benchmark_fft,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_RAYTRACE] =
        {
            N_("FPU Raytracing (Single-thread)"),
            "raytrace.svg",
            callback_benchmark_raytrace,
            scan_benchmark_raytrace,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_IPERF3_SINGLE] =
        {
            N_("Internal Network Speed"),
            "network.svg",
            callback_benchmark_iperf3_single,
            scan_benchmark_iperf3_single,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_SBCPU_SINGLE] =
        {
            N_("SysBench CPU (Single-thread)"),
            "processor.svg",
            callback_benchmark_sbcpu_single,
            scan_benchmark_sbcpu_single,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_SBCPU_ALL] =
        {
            N_("SysBench CPU (Multi-thread)"),
            "processor.svg",
            callback_benchmark_sbcpu_all,
            scan_benchmark_sbcpu_all,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_SBCPU_QUAD] =
        {
            N_("SysBench CPU (Four threads)"),
            "processor.svg",
            callback_benchmark_sbcpu_quad,
            scan_benchmark_sbcpu_quad,
            MODULE_FLAG_BENCHMARK|MODULE_FLAG_HIDE,
        },
    [BENCHMARK_MEMORY_SINGLE] =
        {
            N_("SysBench Memory (Single-thread)"),
            "memory.svg",
            callback_benchmark_memory_single,
            scan_benchmark_memory_single,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_MEMORY_DUAL] =
        {
            N_("SysBench Memory (Two threads)"),
            "memory.svg",
            callback_benchmark_memory_dual,
            scan_benchmark_memory_dual,
            MODULE_FLAG_BENCHMARK|MODULE_FLAG_HIDE,
        },
    [BENCHMARK_MEMORY_QUAD] =
        {
            N_("SysBench Memory (Quad threads)"),
            "memory.svg",
            callback_benchmark_memory_quad,
            scan_benchmark_memory_quad,
            MODULE_FLAG_BENCHMARK|MODULE_FLAG_HIDE,
        },
    [BENCHMARK_MEMORY_ALL] =
        {
            N_("SysBench Memory (Multi-thread)"),
            "memory.svg",
            callback_benchmark_memory_all,
            scan_benchmark_memory_all,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_GUI] =
        {
            N_("GPU Drawing"),
            "monitor.svg",
            callback_benchmark_gui,
            scan_benchmark_gui,
            MODULE_FLAG_BENCHMARK|MODULE_FLAG_NO_REMOTE,
        },
#if(HARDINFO2_QT5)
    [BENCHMARK_OPENGL] =
        {
            N_("GPU OpenGL Drawing"),
            "gpu.svg",
            callback_benchmark_opengl,
            scan_benchmark_opengl,
            MODULE_FLAG_BENCHMARK|MODULE_FLAG_NO_REMOTE,
        },
#endif
    [BENCHMARK_STORAGE] =
        {
            N_("Storage R/W Speed"),
            "hdd.svg",
            callback_benchmark_storage,
            scan_benchmark_storage,
            MODULE_FLAG_BENCHMARK,
        },
    [BENCHMARK_CACHEMEM] =
        {
            N_("Cache/Memory"),
            "bolt.svg",
            callback_benchmark_cachemem,
            scan_benchmark_cachemem,
            MODULE_FLAG_BENCHMARK,
        },
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
    case BENCHMARK_MEMORY_ALL:
        return _("Alexey Kopytov's <i><b>sysbench</b></i> is required.\n"
                 "Results in MiB/second. Higher is better.");
    case BENCHMARK_IPERF3_SINGLE:
        return _("<i><b>iperf3</b></i> is required.\n"
                 "Results in Gbits/s. Higher is better.");
    case BENCHMARK_CRYPTOHASH:
    case BENCHMARK_BLOWFISH_SINGLE:
    case BENCHMARK_BLOWFISH_THREADS:
    case BENCHMARK_BLOWFISH_CORES:
    case BENCHMARK_ZLIB:
    case BENCHMARK_FFT:
    case BENCHMARK_RAYTRACE:
    case BENCHMARK_FIB:
    case BENCHMARK_NQUEENS:
        return _("Results in HIMarks. Higher is better.");
    case BENCHMARK_GUI:
        return _("Results in HIMarks. Higher is better.\n"
		 "Many Desktop Environments only uses software.");
    case BENCHMARK_STORAGE:
    case BENCHMARK_CACHEMEM:
        return _("Results in MB/s. Higher is better.");
#if(HARDINFO2_QT5)
    case BENCHMARK_OPENGL:
        return _("Results in FPS. Higher is better.");
#endif
    }

    return NULL;
}
