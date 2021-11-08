/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
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

#define STATMSG "Performing Alexey Kopytov's sysbench memory benchmark"

/* known to work with:
 * sysbench 0.4.12 --> r:4012
 * sysbench 1.0.11 --> r:1000011
 * sysbench 1.0.15 --> r:1000015
 */

struct sysbench_ctx {
    char *test;
    int threads;
    int max_time;
    char *parms_test;
    bench_value r;
};

int sysbench_version() {
    int ret = -1;
    int v1 = 0, v2 = 0, v3 = 0, mc = 0;
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;

    spawned = g_spawn_command_line_sync("sysbench --version",
            &out, &err, NULL, NULL);
    if (spawned) {
        ret = 0;
        p = out;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            /* version */
            mc = sscanf(p, "sysbench %d.%d.%d", &v1, &v2, &v3);
            if (mc >= 1) {
                ret += v1 * 1000000;
                ret += v2 * 1000;
                ret += v3;
                break;
            }
            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }
    return ret;
}

static gboolean sysbench_run(struct sysbench_ctx *ctx, int expecting_version) {
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;

    int v1 = 0, v2 = 0, v3 = 0, mc = 0;
    char *pp = NULL;

    if (!ctx) return FALSE;
    if (!ctx->test) return FALSE;
    if (!ctx->parms_test) return FALSE;
    if (!ctx->threads) ctx->threads = 1;
    ctx->r.threads_used = ctx->threads;
    if (!ctx->max_time) ctx->max_time = 7;

    gchar *cmd_line = NULL;
    snprintf(ctx->r.extra, 255, "--time=%d %s", ctx->max_time, ctx->parms_test);
    util_compress_space(ctx->r.extra);

    if (!expecting_version)
        expecting_version = sysbench_version();
    if (expecting_version < 1000000) {
        /* v0.x.x: sysbench [general-options]... --test=<test-name> [test-options]... command */
        cmd_line = g_strdup_printf("sysbench --num-threads=%d --max-time=%d --test=%s %s run", ctx->threads, ctx->max_time, ctx->test, ctx->parms_test);
    } else {
        /* v1.x.x: sysbench [options]... [testname] [command] */
        cmd_line = g_strdup_printf("sysbench --threads=%d --time=%d %s %s run", ctx->threads, ctx->max_time, ctx->parms_test, ctx->test);
    }
    //bench_msg("\ncmd_line: %s", cmd_line);

    spawned = g_spawn_command_line_sync(cmd_line,
            &out, &err, NULL, NULL);
    g_free(cmd_line);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;

            if (strstr(p, "Usage:")) {
                /* We're hosed */
                goto sysbench_failed;
            }

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
            if (SEQ(ctx->test, "cpu") ) {
                if (ctx->r.revision < 1000000) {
                    // there is not a nice result line
                    // to grab in version 0.x...
                    // total time:                          7.0016s
                    // total number of events:              873

                    /* should already have "total time:" */
                    if (pp = strstr(p, " total number of events:")) {
                        pp = strchr(pp, ':') + 1;
                        ctx->r.result = strtof(pp, NULL);
                        ctx->r.result /= ctx->r.elapsed_time;
                    }
                }
                if (ctx->r.revision >= 1000000) {
                    //  events per second:  1674.97
                    if (pp = strstr(p, " events per second:")) {
                        pp = strchr(pp, ':') + 1;
                        ctx->r.result = strtof(pp, NULL);
                    }
                }
            }

            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    } else {
        bench_msg("\nfailed to spawn sysbench");
        sleep(5);
    }

    if (ctx->r.result == -1)
        goto sysbench_failed;

    return spawned;

sysbench_failed:
    bench_msg("\nfailed to configure sysbench");
    g_free(out);
    g_free(err);
    return 0;
}

void benchmark_memory_run(int threads, int result_index) {
    struct sysbench_ctx ctx = {
        .test = "memory",
        .threads = threads,
        .parms_test = "",
        .r = EMPTY_BENCH_VALUE};

    int sbv = sysbench_version();
    if (BENCH_PTR_BITS > 32 && sbv >= 1000011) {
        ctx.parms_test =
           " --memory-block-size=1K"
           " --memory-total-size=100G"
           " --memory-scope=global"
           " --memory-hugetlb=off"
           " --memory-oper=write"
           " --memory-access-mode=seq";
    } else {
        /* safer set */
        ctx.parms_test =
           " --memory-block-size=1K"
           " --memory-total-size=3056M"
           " --memory-scope=global"
           " --memory-hugetlb=off"
           " --memory-oper=write"
           " --memory-access-mode=seq";
    }

    shell_view_set_enabled(FALSE);
    char msg[128] = "";
    snprintf(msg, 128, "%s (threads: %d)", STATMSG, threads);
    shell_status_update(msg);

    sysbench_run(&ctx, sbv);
    bench_results[result_index] = ctx.r;
}

void benchmark_memory_single(void) { benchmark_memory_run(1, BENCHMARK_MEMORY_SINGLE); }
void benchmark_memory_dual(void) { benchmark_memory_run(2, BENCHMARK_MEMORY_DUAL); }
void benchmark_memory_quad(void) {  benchmark_memory_run(4, BENCHMARK_MEMORY_QUAD); }

void benchmark_sbcpu_single(void) {
    struct sysbench_ctx ctx = {
        .test = "cpu",
        .threads = 1,
        .parms_test =
           "--cpu-max-prime=10000",
        .r = EMPTY_BENCH_VALUE};

    shell_view_set_enabled(FALSE);
    shell_status_update(STATMSG " (single thread)...");

    sysbench_run(&ctx, 0);
    bench_results[BENCHMARK_SBCPU_SINGLE] = ctx.r;
}

void benchmark_sbcpu_all(void) {
    int cpu_procs, cpu_cores, cpu_threads, cpu_nodes;

    cpu_procs_cores_threads_nodes(&cpu_procs, &cpu_cores, &cpu_threads, &cpu_nodes);

    struct sysbench_ctx ctx = {
        .test = "cpu",
        .threads = cpu_threads,
        .parms_test =
           "--cpu-max-prime=10000",
        .r = EMPTY_BENCH_VALUE};

    shell_view_set_enabled(FALSE);
    shell_status_update(STATMSG " (Multi-thread)...");

    sysbench_run(&ctx, 0);
    bench_results[BENCHMARK_SBCPU_ALL] = ctx.r;
}

void benchmark_sbcpu_quad(void) {
    struct sysbench_ctx ctx = {
        .test = "cpu",
        .threads = 4,
        .parms_test =
           "--cpu-max-prime=10000",
        .r = EMPTY_BENCH_VALUE};

    shell_view_set_enabled(FALSE);
    shell_status_update(STATMSG " (Four thread)...");

    sysbench_run(&ctx, 0);
    bench_results[BENCHMARK_SBCPU_QUAD] = ctx.r;
}
