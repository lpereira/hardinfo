/*
 *    hardinfo2 - System Information and Benchmark
 *    Copyright (C) 2024 hardinfo2 project
 *    License: GPL2+
 *
 *    Based on qgears2 by Zack Rusin (Public Domain)
 *    Based on cairogears by David Reveman & Peter Nilsson (Public Domain)
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
#include <math.h>

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 1

static bench_value opengl_bench(int opengl, int darkmode) {
    bench_value ret = EMPTY_BENCH_VALUE;
    gboolean spawned;
    gchar *out=NULL, *err=NULL;
    int count, ms,ver,gl;
    float fps;
    char *cmd_line;
    
    if(opengl) {
        cmd_line=g_strdup_printf("%s/modules/qgears2 -gl %s",params.path_lib, (darkmode ? "-dark" : ""));
    } else {
        cmd_line=g_strdup_printf("%s/modules/qgears2 %s",params.path_lib, (darkmode ? "-dark" : ""));
    }

    spawned = g_spawn_command_line_sync(cmd_line, &out, &err, NULL, NULL);
    g_free(cmd_line);
    if (spawned && (sscanf(out,"Ver=%d, GL=%d, Result:%d/%d=%f", &ver, &gl, &count, &ms, &fps)==5)) {
            strncpy(ret.extra, out, sizeof(ret.extra)-1);
	    ret.extra[sizeof(ret.extra)-1]=0;
            ret.threads_used = 1;
            ret.elapsed_time = (double)ms/1000;
	    ret.revision = (BENCH_REVISION*100) + ver;
            ret.result = fps;
    }
    g_free(out);
    g_free(err);

    return ret;
}

void benchmark_opengl(void) {
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing opengl benchmark (single thread)...");

    r = opengl_bench(1,(params.max_bench_results==1?1:0));

    if(r.threads_used!=1) {
        r = opengl_bench(0,(params.max_bench_results==1?1:0));
    }
    
    bench_results[BENCHMARK_OPENGL] = r;
}
