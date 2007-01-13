/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

enum {
    BENCHMARK_ZLIB,
    BENCHMARK_FIB,
    BENCHMARK_MD5,
    BENCHMARK_SHA1,
    BENCHMARK_BLOWFISH,
    BENCHMARK_RAYTRACE
} Entries;

void scan_zlib(gboolean reload);
void scan_raytr(gboolean reload);
void scan_bfsh(gboolean reload);
void scan_md5(gboolean reload);
void scan_fib(gboolean reload);
void scan_sha1(gboolean reload);

gchar *callback_zlib();
gchar *callback_raytr();
gchar *callback_bfsh();
gchar *callback_md5();
gchar *callback_fib();
gchar *callback_sha1();


static ModuleEntry entries[] = {
    {"CPU ZLib", "compress.png", callback_zlib, scan_zlib},
    {"CPU Fibonacci", "module.png", callback_fib, scan_fib},
    {"CPU MD5", "module.png", callback_md5, scan_md5},
    {"CPU SHA1", "module.png", callback_sha1, scan_sha1},
    {"CPU Blowfish", "blowfish.png", callback_bfsh, scan_bfsh},
    {"FPU Raytracing", "raytrace.png", callback_raytr, scan_raytr},
    { NULL }
};

static gchar *__benchmark_include_results(gchar * results,
					  const gchar * benchmark,
					  ShellOrderType order_type)
{
    GKeyFile *conf;
    gchar **machines;
    int i;

    conf = g_key_file_new();
    g_key_file_load_from_file(conf,
                              idle_free(g_build_filename(params.path_data, "benchmark.conf", NULL)),
                              0, NULL);

    machines = g_key_file_get_keys(conf, benchmark, NULL, NULL);
    for (i = 0; machines && machines[i]; i++) {
	gchar *value =
	    g_key_file_get_value(conf, benchmark, machines[i], NULL);
	results =
	    g_strconcat(results, machines[i], "=", value, "\n", NULL);
	g_free(value);
    }

    g_strfreev(machines);
    g_key_file_free(conf);

    return g_strdup_printf("[$ShellParam$]\n"
			   "Zebra=1\n"
			   "OrderType=%d\n"
			   "ViewType=3\n%s", order_type, results);
}

static gchar *benchmark_include_results_reverse(gchar * results,
						const gchar * benchmark)
{
    return __benchmark_include_results(results, benchmark,
				       SHELL_ORDER_DESCENDING);
}

static gchar *benchmark_include_results(gchar * results,
					const gchar * benchmark)
{
    return __benchmark_include_results(results, benchmark,
				       SHELL_ORDER_ASCENDING);
}

#include <arch/common/fib.h>
#include <arch/common/zlib.h>
#include <arch/common/md5.h>
#include <arch/common/sha1.h>
#include <arch/common/blowfish.h>
#include <arch/common/raytrace.h>

static gchar *bench_zlib = NULL,
    *bench_fib = NULL,
    *bench_md5 = NULL,
    *bench_sha1 = NULL,
    *bench_fish = NULL,
    *bench_ray = NULL;

gchar *callback_zlib()
{
    return g_strdup(bench_zlib);
}

gchar *callback_raytr()
{
    return g_strdup(bench_ray);
}

gchar *callback_bfsh()
{
    return g_strdup(bench_fish);
}

gchar *callback_md5()
{
    return g_strdup(bench_md5);
}

gchar *callback_fib()
{
    return g_strdup(bench_fib);
}

gchar *callback_sha1()
{
    return g_strdup(bench_sha1);
}

void scan_zlib(gboolean reload)
{
    SCAN_START();
    g_free(bench_zlib);
    bench_zlib = benchmark_zlib();
    SCAN_END();
}

void scan_raytr(gboolean reload)
{
    SCAN_START();
    g_free(bench_ray);
    bench_ray = benchmark_raytrace();
    SCAN_END();
}

void scan_bfsh(gboolean reload)
{
    SCAN_START();
    g_free(bench_fish);
    bench_fish = benchmark_fish();
    SCAN_END();
}

void scan_md5(gboolean reload)
{
    SCAN_START();
    g_free(bench_md5);
    bench_md5 = benchmark_md5();
    SCAN_END();
}

void scan_fib(gboolean reload)
{
    SCAN_START();
    g_free(bench_fib);
    bench_fib = benchmark_fib();
    SCAN_END();
}

void scan_sha1(gboolean reload)
{
    SCAN_START();
    g_free(bench_sha1);
    bench_sha1 = benchmark_sha1();
    SCAN_END();
}

const gchar *hi_note_func(gint entry)
{
    switch (entry) {
    case BENCHMARK_ZLIB:
    case BENCHMARK_MD5:
    case BENCHMARK_SHA1:
	return "Results in bytes/second. Higher is better.";

    case BENCHMARK_RAYTRACE:
    case BENCHMARK_BLOWFISH:
    case BENCHMARK_FIB:
	return "Results in seconds. Lower is better.";
    }

    return NULL;
}

gchar *hi_module_get_name(void)
{
    return g_strdup("Benchmarks");
}

guchar hi_module_get_weight(void)
{
    return 240;
}

ModuleEntry *hi_module_get_entries(void)
{
    return entries;
}

gchar *get_all_results(void)
{
    return "";
}

ShellModuleMethod*
hi_exported_methods(void)
{
    static ShellModuleMethod m[] = {
      { "getAllResults", get_all_results },
      { NULL }
    };
    
    return m;
}

ModuleAbout *
hi_module_get_about(void)
{
    static ModuleAbout ma[] = {
      {
          .author	= "Leandro A. F. Pereira",
          .description	= "Perform tasks and compare with other systems",
          .version	= VERSION,
          .license	= "GNU GPL version 2"
      }
    };
    
    return ma;
}
