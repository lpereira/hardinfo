#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "hardinfo.h"

extern ProgramParameters params;

enum {
    BENCHMARK_BLOWFISH,
    BENCHMARK_CRYPTOHASH,
    BENCHMARK_FIB,
    BENCHMARK_NQUEENS,
    BENCHMARK_FFT,
    BENCHMARK_RAYTRACE,
    BENCHMARK_GUI,
    BENCHMARK_N_ENTRIES
} BenchmarkEntries;

extern gdouble bench_results[BENCHMARK_N_ENTRIES];

#endif /* __BENCHMARK_H__ */