/*
 *    hardinfo2 - System Information and Benchmark
 *    Copyright (C) 2024 hardinfo2 project
 *    Written by: hwspeedy
 *    License: GPL2+
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License v2.0 or later.
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

#include "hardinfo.h"
#include "benchmark.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 2

#define SZ 128L*1024*1024
#define ALIGN (1024*1024)

void __attribute__ ((noinline)) mcpy(void *dst, void *src, size_t sz) {memcpy(dst, src, sz);}

void cachemem_do_benchmark(char *dst, char *src, long sz, double *res){
    double sec;
    unsigned long long repeat;
    repeat=1;
    while(repeat<=(1LL<<60)){
        unsigned long long i=0;
        clock_t start = clock();
	while(i<repeat){ mcpy(dst, src, sz);i++;}
        sec = (clock() - start) / (double)CLOCKS_PER_SEC;
	if(sec>0.01) break;
	if(sec<0.00001) repeat<<=10; else
	  if(sec<0.0001) repeat<<=7; else
	    if(sec<0.001) repeat<<=4; else
	      repeat<<=1;
    }    
    *res=(sz)/(sec*1024*1024*1024)*repeat;
}

static bench_value cacchemem_runtest(){
    bench_value ret = EMPTY_BENCH_VALUE;
    char *buf;
    int i,cachespeed;
    double res[30];
    long sz, l=0;
    clock_t start=clock();

    buf=g_malloc(SZ+SZ+ALIGN);
    if(!buf) return ret;
    char *foo = (char*)((((unsigned long)buf) + (ALIGN-1)) & ~(ALIGN-1));
    char *bar = foo + SZ;
    while(l<SZ){
         long rnd = l & 0xff; //random() & 0xff;
         foo[l++] = rnd;
    }
    memcpy(bar, foo, SZ);

    l=0;
    while(l<SZ){
         if (bar[l] != foo[l]){
             free(buf);
             return ret;
         }
	 l++;
    }

    i=1;while(i<30) res[i++]=0;

    i=1;sz=4;
    while((sz <= SZ) && (((clock()-start)/(double)CLOCKS_PER_SEC)<5) ) {
        cachemem_do_benchmark(bar, foo, sz, &res[i]);
        i++;
	sz<<=1;
    }

    g_free(buf);
    
    ret.elapsed_time = ((clock()-start)/(double)CLOCKS_PER_SEC);
    cachespeed=(res[8]+res[10]+res[12]+res[14])/4;
    ret.result = (cachespeed+((res[16]+res[18]+res[20]+res[22])/4-cachespeed)/2)*1024;
    sprintf(ret.extra,"%0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf", res[1],res[2],res[3],res[4],res[5],res[6],res[7],res[8],res[9],res[10],res[11],res[12],res[13],res[14],res[15],res[16],res[17],res[18],res[19],res[20],res[21],res[22],res[23],res[24],res[25],res[26]);
    ret.threads_used = 1;
    ret.revision = BENCH_REVISION;
    return ret;
}

void benchmark_cachemem(void) {
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing Cache/Memory Benchmark...");

    r = cacchemem_runtest();
    
    bench_results[BENCHMARK_CACHEMEM] = r;
}

