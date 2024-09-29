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
#include "math.h"

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 1

static bench_value storage_runtest() {
    bench_value ret = EMPTY_BENCH_VALUE;
    gboolean spawned;
    gchar *out=NULL, *err=NULL,*s=NULL;
    int readbytes=0,writebytes=0,i=0;
    float readspeed=0,writespeed=0;
    float readtime=0,writetime=0;
    char reade[5],writee[5];
    int t=1;
    const char cmd_line[] = "sh -c 'cd ~;dd if=/dev/zero of=hardinfo2_testfile bs=1M count=20 oflag=direct;dd of=/dev/zero if=hardinfo2_testfile bs=1M iflag=direct;rm hardinfo2_testfile'";
    const char cmd_line_long[] = "sh -c 'cd ~;dd if=/dev/zero of=hardinfo2_testfile bs=1M count=400 oflag=direct;dd of=/dev/zero if=hardinfo2_testfile bs=1M iflag=direct;rm hardinfo2_testfile'";

    while(t){
        if(t==1)
	   spawned = g_spawn_command_line_sync(cmd_line, &out, &err, NULL, NULL);
        else
	   spawned = g_spawn_command_line_sync(cmd_line_long, &out, &err, NULL, NULL);
        if (spawned){
	    i=0;
	    if( (s=strstr(err,"\n")) && (s=strstr(s+1,"\n")) ) {
	        i=sscanf(s+1,"%d",&writebytes);
	        if(i==1) {
                    if( (s=strstr(s,")")) && (s=strstr(s+1,", ")) ){
                         i=sscanf(s+2,"%f",&writetime);
                         if((i==1) && (s=strstr(s+2,", ")))
                              i=sscanf(s+2,"%f",&writespeed);
	            }
	        }
            }
	    if( (i==1) && (s=strstr(s+1,"\n")) && (s=strstr(s+1,"\n")) && (s=strstr(s+1,"\n")) ) {
	        i=sscanf(s+1,"%d",&readbytes);
	        if(i==1) {
                    if( (s=strstr(s,")")) && (s=strstr(s+1,", ")) ){
                         i=sscanf(s+2,"%f",&readtime);
                         if((i==1) && (s=strstr(s+2,", ")))
                              i=sscanf(s+2,"%f",&readspeed);
	            }
	        }
            }
            if((i==1) && (readtime!=0) && (writetime!=0)){
	        writespeed=writebytes/writetime;
	        readspeed=readbytes/readtime;
                ret.elapsed_time = writetime+readtime;
		ret.result = (writespeed+readspeed)/2/(1024*1024);
		strcpy(writee,"b/s");
		strcpy(reade,"b/s");
		if(writespeed>1024) {writespeed/=1024;strcpy(writee,"KB/s");}
		if(writespeed>1024) {writespeed/=1024;strcpy(writee,"MB/s");}
		if(writespeed>1024) {writespeed/=1024;strcpy(writee,"GB/s");}
		if(readspeed>1024) {readspeed/=1024;strcpy(reade,"KB/s");}
		if(readspeed>1024) {readspeed/=1024;strcpy(reade,"MB/s");}
		if(readspeed>1024) {readspeed/=1024;strcpy(reade,"GB/s");}
		sprintf(ret.extra,"Read:%0.2lf %s, Write:%0.2lf %s %s",
			readspeed,reade,writespeed,writee,(t==2?"(Long)":"") );
	    }
	}
	g_free(out);
	g_free(err);
      
	if((t==1)&&(ret.elapsed_time<0.2)) t=2; else t=0; //second long run x20 - max 4 sec
    }
    
    ret.threads_used = 1;
    ret.revision = BENCH_REVISION;
    return ret;
}

void benchmark_storage(void) {
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing Storage Benchmark...");

    r = storage_runtest();
    
    bench_results[BENCHMARK_STORAGE] = r;
}

