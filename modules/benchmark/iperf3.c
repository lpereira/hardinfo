/*
 *    HardInfo - System Information and Benchmark
 *    Copyright (C) 2020 Burt P. <pburt0@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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
#include <json-glib/json-glib.h>
#include <math.h>

static int iperf3_version() {
    int ret = -1.0;
    int v1 = 0, v2 = 0, v3 = 0, mc = 0;
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;

    spawned = g_spawn_command_line_sync("iperf3 --version",
            &out, &err, NULL, NULL);
    if (spawned) {
        ret = 0;
        p = out;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            /* version */
            mc = sscanf(p, "iperf %d.%d", &v1, &v2);
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

static gboolean iperf3_server() {
    const char* argv[] = {
        "iperf3", "-s", "--port", "61981", "--one-off", NULL };
    return g_spawn_async(NULL, (char**)argv, NULL,
        G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_SEARCH_PATH,
        NULL, NULL, NULL, NULL);
}

static double _get_double(JsonParser *j, const char* path) {
    double r = NAN;
    GError *e = NULL;
    JsonNode *root = json_parser_get_root(j);
    JsonNode *ra = json_path_query(path, root, &e);
    if (e) {
        fprintf (stderr, "json_path_query(%s) error: %s\n", path, e->message);
    } else {
        JsonArray *arr = json_node_get_array(ra);
        r = json_array_get_double_element(arr, 0);
    }
    json_node_free(ra);
    return r;
}

static bench_value iperf3_client() {
    bench_value ret = EMPTY_BENCH_VALUE;
    int v1 = 0, v2 = 0, v3 = 0, mc = 0;
    gboolean spawned;
    gchar *out, *err;
    GError *e = NULL;

    const char cmd_line[] = "iperf3 -c localhost --port 61981 --json --omit 3 --time 5";

    spawned = g_spawn_command_line_sync(cmd_line,
            &out, &err, NULL, NULL);
    if (spawned) {
        JsonParser *j = json_parser_new();
        if (json_parser_load_from_data(j, out, -1, &e)) {
            if (e) {
                fprintf (stderr, "json_parser_load_from_data error: %s\n", e->message);
                exit(-1);
            }
            strncpy(ret.extra, cmd_line, sizeof(ret.extra)-1);
            ret.threads_used = 1;
            ret.elapsed_time = _get_double(j, "$.end.sum_received.seconds");
            ret.result = _get_double(j, "$.end.sum_received.bits_per_second");
            ret.result /= 1000000.0; // now mega
            ret.result /= 1000.0;    // now giga
            g_object_unref(j);
        }
        g_free(out);
        g_free(err);
    }
    return ret;
}

void benchmark_iperf3_single(void) {
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing iperf3 localhost benchmark (single thread)...");

    int v = iperf3_version();
    if (v > 0) {
        iperf3_server();
        sleep(1);
        r = iperf3_client();
        r.revision = v;
    }
    bench_results[BENCHMARK_IPERF3_SINGLE] = r;
}
