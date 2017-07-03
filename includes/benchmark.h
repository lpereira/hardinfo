#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "hardinfo.h"

extern ProgramParameters params;

enum {
    BENCHMARK_BLOWFISH,
    BENCHMARK_CRYPTOHASH,
    BENCHMARK_FIB,
    BENCHMARK_NQUEENS,
    BENCHMARK_ZLIB,
    BENCHMARK_FFT,
    BENCHMARK_RAYTRACE,
    BENCHMARK_GUI,
    BENCHMARK_N_ENTRIES
} BenchmarkEntries;

void benchmark_bfish(void);
void benchmark_cryptohash(void);
void benchmark_fft(void);
void benchmark_fib(void);
void benchmark_fish(void);
void benchmark_gui(void);
void benchmark_nqueens(void);
void benchmark_raytrace(void);
void benchmark_zlib(void);

gdouble benchmark_parallel_for(guint start, guint end,
                               gpointer callback, gpointer callback_data);

extern gdouble bench_results[BENCHMARK_N_ENTRIES];

#endif /* __BENCHMARK_H__ */