#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "hardinfo.h"

extern ProgramParameters params;

enum {
    BENCHMARK_BLOWFISH_SINGLE,
    BENCHMARK_BLOWFISH_THREADS,
    BENCHMARK_BLOWFISH_CORES,
    BENCHMARK_CRYPTOHASH,
    BENCHMARK_FIB,
    BENCHMARK_NQUEENS,
    BENCHMARK_ZLIB,
    BENCHMARK_FFT,
    BENCHMARK_RAYTRACE,
    BENCHMARK_GUI,
    BENCHMARK_N_ENTRIES
} BenchmarkEntries;

void benchmark_bfish_single(void);
void benchmark_bfish_threads(void);
void benchmark_bfish_cores(void);
void benchmark_cryptohash(void);
void benchmark_fft(void);
void benchmark_fib(void);
void benchmark_fish(void);
void benchmark_gui(void);
void benchmark_nqueens(void);
void benchmark_raytrace(void);
void benchmark_zlib(void);

typedef struct {
    double result;
    double elapsed_time;
    int threads_used;
    int revision;
    char extra[256]; /* no \n, ; or | */
} bench_value;

#define EMPTY_BENCH_VALUE {-1.0f,0,0,-1,""}

char *bench_value_to_str(bench_value r);
bench_value bench_value_from_str(const char* str);

/* Note:
 *    benchmark_parallel_for(): element [start] included, but [end] is excluded.
 *    callback(): expected to processes elements [start] through [end] inclusive.
 */
bench_value benchmark_parallel_for(gint n_threads, guint start, guint end,
                               gpointer callback, gpointer callback_data);

bench_value benchmark_parallel(gint n_threads, gpointer callback, gpointer callback_data);

bench_value benchmark_crunch_for(float seconds, gint n_threads,
                               gpointer callback, gpointer callback_data);

extern bench_value bench_results[BENCHMARK_N_ENTRIES];

#endif /* __BENCHMARK_H__ */
