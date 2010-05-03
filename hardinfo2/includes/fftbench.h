#ifndef __FFTBENCH_H__ 
#define __FFTBENCH_H__

#include <glib.h>

typedef struct _FFTBench	FFTBench;

struct _FFTBench {
  double **a, *b, *r;
  int *p;
};

FFTBench *fft_bench_new(void);
void fft_bench_run(FFTBench *fftbench);
void fft_bench_free(FFTBench *fftbench);

#endif /* __FFTBENCH_H__ */


