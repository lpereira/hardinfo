/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    Copyright (C) 2019 Burt P. <pburt0@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
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

struct sysbench_ctx {
    char *test;
    char *parms;
    bench_value r;
};

static gboolean sysbench_run(struct sysbench_ctx *ctx) {
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;

    int v1 = 0, v2 = 0, v3 = 0, mc = 0;
    char *pp = NULL;

    if (!ctx) return FALSE;
    if (!ctx->test) return FALSE;
    if (!ctx->parms) return FALSE;
    snprintf(ctx->r.extra, 255, "%s", ctx->parms);
    gchar *cmd_line = g_strdup_printf("sysbench %s %s run", ctx->parms, ctx->test);
    spawned = g_spawn_command_line_sync(cmd_line,
            &out, &err, NULL, NULL);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;

            /* version */
            mc = sscanf(p, "sysbench %d.%d.%d", &v1, &v2, &v3);
            if (mc >= 1) {
                ctx->r.revision = 0;
                ctx->r.revision += v1 * 1000000;
                ctx->r.revision += v2 * 1000;
                ctx->r.revision += v3;
            }

            /* total_time */
            if (pp = strstr(p, "total time:")) {
                pp = strchr(pp, ':') + 1;
                ctx->r.elapsed_time = strtof(pp, NULL);
            }

            /* result */
            if (SEQ(ctx->test, "memory") ) {
                // 57894.30 MiB transferred (5787.59 MiB/sec)
                if (pp = strstr(p, " transferred (")) {
                    pp = strchr(pp, '(') + 1;
                    ctx->r.result = strtof(pp, NULL);
                }
            }

            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }

    g_free(cmd_line);
    return spawned;
}

void benchmark_memory_single(void)
{
    struct sysbench_ctx ctx = {
        .test = "memory",
        .parms = "--threads=1 --time=7"
           " --memory-block-size=1K"
           " --memory-total-size=100G"
           " --memory-scope=global"
           " --memory-hugetlb=off"
           " --memory-oper=write"
           " --memory-access-mode=seq",
        .r = EMPTY_BENCH_VALUE};
    ctx.r.threads_used = 1;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing Alexey Kopytov's sysbench memory benchmark (single thread)...");

    sysbench_run(&ctx);
    bench_results[BENCHMARK_MEMORY_SINGLE] = ctx.r;
}

void benchmark_memory_dual(void)
{
    struct sysbench_ctx ctx = {
        .test = "memory",
        .parms = "--threads=2 --time=7"
           " --memory-block-size=1K"
           " --memory-total-size=100G"
           " --memory-scope=global"
           " --memory-hugetlb=off"
           " --memory-oper=write"
           " --memory-access-mode=seq",
        .r = EMPTY_BENCH_VALUE};
    ctx.r.threads_used = 2;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing Alexey Kopytov's sysbench memory benchmark (two threads)...");

    sysbench_run(&ctx);
    bench_results[BENCHMARK_MEMORY_DUAL] = ctx.r;
}

void benchmark_memory_quad(void)
{
    struct sysbench_ctx ctx = {
        .test = "memory",
        .parms = "--threads=4 --time=7"
           " --memory-block-size=1K"
           " --memory-total-size=100G"
           " --memory-scope=global"
           " --memory-hugetlb=off"
           " --memory-oper=write"
           " --memory-access-mode=seq",
        .r = EMPTY_BENCH_VALUE};
    ctx.r.threads_used = 4;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing Alexey Kopytov's sysbench memory benchmark (four threads)...");

    sysbench_run(&ctx);
    bench_results[BENCHMARK_MEMORY_QUAD] = ctx.r;
}
