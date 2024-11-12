/*
 *    hardinfo2 - System Information and Benchmark
 *    Copyright (C) 2024 hardinfo2 project
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
#define BENCH_REVISION 1

#define SZ 128L*1024*1024
#define ALIGN (1024*1024)

void __attribute__ ((noinline)) mcpy(void *dst, void *src, size_t sz) {memcpy(dst, src, sz);}

void cachemem_do_benchmark(char *dst, char *src, long sz, double *time, double *res){
    double sec;
    unsigned long long repeat;
    repeat=1;
    while(repeat<=(1LL<<60)){
        clock_t start = clock();
        unsigned long long i=0;
	while(i<repeat){ mcpy(dst, src, sz*0.8);i++;}
        sec = (clock() - start) / (double)CLOCKS_PER_SEC;
        if (sec > 0.05) break;
	repeat<<=1;
    }    
    *time=sec/repeat;
    *res=(sz*0.8)/(sec*(1024*1024))*repeat;
}

static bench_value cacchemem_runtest(){
    bench_value ret = EMPTY_BENCH_VALUE;
    char *buf;//,cn=1;
    int clsfound=0,i,lasti,ncn=1000,l1max=0,ramres=0,llast=0,l2max=0;
    double res[30],time[30],lastcres=0,maxres=0;
    long sz, l=0;
    buf=g_malloc(SZ+SZ+ALIGN);
    if(!buf) return ret;
    char *foo = (char*)((((unsigned long)buf) + (ALIGN-1)) & ~(ALIGN-1));
    char *bar = foo + SZ;
    while(l<SZ){
         long rnd = random() & 0xff;
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
    i=1;res[0]=0;
    sz=4;
    while(sz <= SZ) {
        cachemem_do_benchmark(bar, foo, sz, &time[i], &res[i]);
	i++;
	sz<<=1;
    }
    res[i]=res[i-1];
    res[i+1]=res[i-1];
    lasti=i-1;
    i=1;
    while(i<lasti){
      if(maxres && (res[i]>(maxres*0.92)) && (l1max==(i-1))) l1max=i;
      if(res[i]>maxres) {maxres=res[i];l1max=i;}
      i++;
    }
    ramres=i;
    i=lasti;
    while(i>0) {
      if(!llast && (res[i]>(res[ramres]*1.5))) llast=i;
      i--;
    }
    i=1;sz=4;
    while(i<=lasti){
        //printf("%09ld: %010.3f => %06.0f", sz,time[i]*1000000,res[i]);
        if(!clsfound && (res[i+1]<(1.5*res[i]))) {/*printf(" CacheLineSize %lu",sz);*/clsfound=1;}
        if(i==l1max) {//L1
	    //printf(" L%d CacheSize %luKB",cn++,sz>>10);
	    lastcres=res[i];ncn=i+2;
	} else if(i==llast) {//Llast
	    //printf(" L%d CacheSize %luKB",cn++,sz>>10);cn=10;
	} else if(i==ramres) {//Ram
	    //printf(" Ram Speed %0.1fGB/s",res[i]/1024);
	} else if(!l2max && (i>=ncn) && (res[i+1]<(0.75*lastcres)) && (abs((int)res[i]-(int)res[i+1])>(res[i]*0.2)) ) {
	    //printf(" L%d CacheSize %luKB",cn++,sz>>10);
	    l2max=i;
	    lastcres=res[i+2];ncn=i+2;
	}
        //printf("\n");
        i++;
	sz<<=1;
    }
    //printf("\n");
    g_free(buf);
    
    ret.elapsed_time = 5;
    ret.result = (res[l1max]+res[l2max]+res[llast]+res[ramres])/4;
    sprintf(ret.extra,"L1-Cache: %0.1lfGB/s, L2-Cache: %0.1lfGB/s, Llast-Cache: %0.1lfGB/s, Memory: %0.1lfGB/s", res[l1max]/1024, res[l2max]/1024, res[llast]/1024, res[ramres]/1024);
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

