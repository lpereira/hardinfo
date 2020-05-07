#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "hardinfo.h"
#include "util_sysobj.h" /* for SEQ() */

#define BENCH_PTR_BITS ((unsigned int)sizeof(void*) * 8)

extern ProgramParameters params;

enum BenchmarkEntries {
    BENCHMARK_BLOWFISH_SINGLE,
    BENCHMARK_BLOWFISH_THREADS,
    BENCHMARK_BLOWFISH_CORES,
    BENCHMARK_ZLIB,
    BENCHMARK_CRYPTOHASH,
    BENCHMARK_FIB,
    BENCHMARK_NQUEENS,
    BENCHMARK_FFT,
    BENCHMARK_RAYTRACE,
    BENCHMARK_SBCPU_SINGLE,
    BENCHMARK_SBCPU_ALL,
    BENCHMARK_SBCPU_QUAD,
    BENCHMARK_MEMORY_SINGLE,
    BENCHMARK_MEMORY_DUAL,
    BENCHMARK_MEMORY_QUAD,
    BENCHMARK_GUI,
    BENCHMARK_N_ENTRIES
};

void benchmark_bfish_single(void);
void benchmark_bfish_threads(void);
void benchmark_bfish_cores(void);
void benchmark_memory_single(void);
void benchmark_memory_dual(void);
void benchmark_memory_quad(void);
void benchmark_sbcpu_single(void);
void benchmark_sbcpu_all(void);
void benchmark_sbcpu_quad(void);
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
    char user_note[256]; /* no \n, ; or | */
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

/* in bench_util.c */

/* guarantee a minimum size of data
 * or return null */
gchar *get_test_data(gsize min_size);
char *md5_digest_str(const char *data, unsigned int len);
#define bench_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__)

#endif /* __BENCHMARK_H__ */
