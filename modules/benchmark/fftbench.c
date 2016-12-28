/*
    fftbench.c

    Written by Scott Robert Ladd (scott@coyotegulch.com)
    No rights reserved. This is public domain software, for use by anyone.

    A number-crunching benchmark using LUP-decomposition to solve a large
    linear equation.

    The code herein is design for the purpose of testing computational
    performance; error handling is minimal.
    
    In fact, this is a weak implementation of the FFT; unfortunately, all
    of my really nifty FFTs are in commercial code, and I haven't had time
    to write a new FFT routine for this benchmark. I may add a Hartley
    transform to the seat, too.

    Actual benchmark results can be found at:
            http://scottrobertladd.net/coyotegulch/

    Please do not use this information or algorithm in any way that might
    upset the balance of the universe or otherwise cause a disturbance in
    the space-time continuum.
*/

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "fftbench.h"

// embedded random number generator; ala Park and Miller
static long seed = 1325;
static const long IA = 16807;
static const long IM = 2147483647;
static const double AM = 4.65661287525E-10;
static const long IQ = 127773;
static const long IR = 2836;
static const long MASK = 123459876;

static double random_double()
{
    long k;
    double result;

    seed ^= MASK;
    k = seed / IQ;
    seed = IA * (seed - k * IQ) - IR * k;

    if (seed < 0)
	seed += IM;

    result = AM * seed;
    seed ^= MASK;

    return result;
}

static const int N = 800;
static const int NM1 = 799;	// N - 1
static const int NP1 = 801;	// N + 1

static void lup_decompose(FFTBench *fftbench)
{
    int i, j, k, k2, t;
    double p, temp, **a;

    int *perm = (int *) malloc(sizeof(double) * N);
    
    fftbench->p = perm;
    a = fftbench->a;
    
    for (i = 0; i < N; ++i)
	perm[i] = i;

    for (k = 0; k < NM1; ++k) {
	p = 0.0;

	for (i = k; i < N; ++i) {
	    temp = fabs(a[i][k]);

	    if (temp > p) {
		p = temp;
		k2 = i;
	    }
	}

	// check for invalid a
	if (p == 0.0)
	    return;

	// exchange rows
	t = perm[k];
	perm[k] = perm[k2];
	perm[k2] = t;

	for (i = 0; i < N; ++i) {
	    temp = a[k][i];
	    a[k][i] = a[k2][i];
	    a[k2][i] = temp;
	}

	for (i = k + 1; i < N; ++i) {
	    a[i][k] /= a[k][k];

	    for (j = k + 1; j < N; ++j)
		a[i][j] -= a[i][k] * a[k][j];
	}
    }
}

static double *lup_solve(FFTBench *fftbench)
{
    int i, j, j2;
    double sum, u;

    double *y = (double *) malloc(sizeof(double) * N);
    double *x = (double *) malloc(sizeof(double) * N);
    
    double **a = fftbench->a;
    double *b = fftbench->b;
    int *perm = fftbench->p;

    for (i = 0; i < N; ++i) {
	y[i] = 0.0;
	x[i] = 0.0;
    }

    for (i = 0; i < N; ++i) {
	sum = 0.0;
	j2 = 0;

	for (j = 1; j <= i; ++j) {
	    sum += a[i][j2] * y[j2];
	    ++j2;
	}

	y[i] = b[perm[i]] - sum;
    }

    i = NM1;

    while (1) {
	sum = 0.0;
	u = a[i][i];

	for (j = i + 1; j < N; ++j)
	    sum += a[i][j] * x[j];

	x[i] = (y[i] - sum) / u;

	if (i == 0)
	    break;

	--i;
    }

    free(y);

    return x;
}

FFTBench *fft_bench_new(void)
{
    FFTBench *fftbench;
    int i, j;
    
    fftbench = g_new0(FFTBench, 1);

    // generate test data            
    fftbench->a = (double **) malloc(sizeof(double *) * N);

    for (i = 0; i < N; ++i) {
	fftbench->a[i] = (double *) malloc(sizeof(double) * N);

	for (j = 0; j < N; ++j)
	    fftbench->a[i][j] = random_double();
    }

    fftbench->b = (double *) malloc(sizeof(double) * N);

    for (i = 0; i < N; ++i)
	fftbench->b[i] = random_double();

    return fftbench;
}

void fft_bench_run(FFTBench *fftbench)
{
    lup_decompose(fftbench);
    double *x = lup_solve(fftbench);
    free(x);
}

void fft_bench_free(FFTBench *fftbench)
{
    int i;
    
    // clean up
    for (i = 0; i < N; ++i)
	free(fftbench->a[i]);

    free(fftbench->a);
    free(fftbench->b);
    free(fftbench->p);
    free(fftbench->r);
    
    g_free(fftbench);
}
