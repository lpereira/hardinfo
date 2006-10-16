/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

#include <hardinfo.h>
#include <iconcache.h>
#include <shell.h>
#include <config.h>
#include <binreloc.h>

enum {
    BENCHMARK_ZLIB,
    BENCHMARK_FIB,
    BENCHMARK_MD5,
    BENCHMARK_SHA1,
    BENCHMARK_BLOWFISH
} Entries;

static ModuleEntry hi_entries[] = {
    {"CPU ZLib",	"compress.png"},
    {"CPU Fibonacci",	"module.png"},
    {"CPU MD5",		"module.png"},
    {"CPU SHA1",	"module.png"},
    {"CPU Blowfish",	"blowfish.png"}
};

static gchar *
benchmark_include_results(gchar *results, const gchar *benchmark)
{
    GKeyFile *conf;
    gchar **machines, *bconf_path;
    int i;
    
    conf = g_key_file_new();
    bconf_path = g_build_filename(path_data, "benchmark.conf", NULL);
    g_key_file_load_from_file(conf, bconf_path, 0, NULL);
    
    machines = g_key_file_get_keys(conf, benchmark, NULL, NULL);
    for (i = 0; machines && machines[i]; i++) {
        gchar *value = g_key_file_get_value(conf, benchmark, machines[i], NULL);
        results = g_strconcat(results, machines[i], "=", value, "\n", NULL);
        g_free(value);
    }
    
    g_strfreev(machines);
    g_key_file_free(conf);
    g_free(bconf_path);
    
    return g_strconcat(results, "[$ShellParam$]\n"
                                "Zebra=1\n", NULL);
}

#include <arch/common/fib.h>
#include <arch/common/zlib.h>
#include <arch/common/md5.h>
#include <arch/common/sha1.h>
#include <arch/common/blowfish.h>

static gchar *bench_zlib = NULL,
             *bench_fib  = NULL,
             *bench_md5  = NULL,
             *bench_sha1 = NULL,
             *bench_fish = NULL;

gchar *
hi_info(gint entry)
{
    switch (entry) {
        case BENCHMARK_ZLIB:
            if (bench_zlib)
                return g_strdup(bench_zlib);
            
            bench_zlib = benchmark_zlib();
            return g_strdup(bench_zlib);

        case BENCHMARK_BLOWFISH:
            if (bench_fish)
                return g_strdup(bench_fish);
                
            bench_fish = benchmark_fish();
            return g_strdup(bench_fish);

        case BENCHMARK_MD5:
            if (bench_md5)
                return g_strdup(bench_md5);
            
            bench_md5 = benchmark_md5();
            return g_strdup(bench_md5);

        case BENCHMARK_FIB:
            if (bench_fib)
                return g_strdup(bench_fib);
            
            bench_fib = benchmark_fib();
            return g_strdup(bench_fib);

        case BENCHMARK_SHA1:
            if (bench_sha1)
                return g_strdup(bench_sha1);
            
            bench_sha1 = benchmark_sha1();
            return g_strdup(bench_sha1);

        default:
            return g_strdup("[Empty]\n");
    }
}

void
hi_reload(gint entry)
{
    switch (entry) {
        case BENCHMARK_ZLIB:
            if (bench_zlib) g_free(bench_zlib);
            bench_zlib = benchmark_zlib();
            break;
        case BENCHMARK_BLOWFISH:
            if (bench_fish) g_free(bench_fish);
            bench_fish = benchmark_fish();
            break;
        case BENCHMARK_MD5:
            if (bench_md5) g_free(bench_md5);
            bench_md5 = benchmark_md5();
            break;
        case BENCHMARK_FIB:
            if (bench_fib) g_free(bench_fib);
            bench_fib = benchmark_fib();
            break;
        case BENCHMARK_SHA1:
            if (bench_sha1) g_free(bench_sha1);
            bench_sha1 = benchmark_sha1();
            break;
    }
}

gint
hi_n_entries(void)
{
    return G_N_ELEMENTS(hi_entries) - 1;
}

GdkPixbuf *
hi_icon(gint entry)
{
    return icon_cache_get_pixbuf(hi_entries[entry].icon);
}

gchar *
hi_name(gint entry)
{
    return hi_entries[entry].name;
}
