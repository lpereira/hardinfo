/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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
#include <syncmanager.h>

#include <sys/time.h>
#include <sys/resource.h>

enum {
    BENCHMARK_ZLIB,
    BENCHMARK_FIB,
    BENCHMARK_MD5,
    BENCHMARK_SHA1,
    BENCHMARK_BLOWFISH,
    BENCHMARK_RAYTRACE,
    BENCHMARK_N_ENTRIES
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
    {NULL}
};

typedef struct _ParallelBenchTask ParallelBenchTask;

struct _ParallelBenchTask {
    guint	start, end;
    gpointer	data, callback;
};

gpointer benchmark_parallel_for_dispatcher(gpointer data)
{
    ParallelBenchTask 	*pbt = (ParallelBenchTask *)data;
    gpointer 		(*callback)(unsigned int start, unsigned int end, void *data);
    gpointer		return_value;
    
    if ((callback = pbt->callback)) {
        DEBUG("this is thread %p; items %d -> %d, data %p", g_thread_self(),
              pbt->start, pbt->end, pbt->data);
        return_value = callback(pbt->start, pbt->end, pbt->data);
        DEBUG("this is thread %p; return value is %p", g_thread_self(), return_value);
    } else {
        DEBUG("this is thread %p; callback is NULL and it should't be!", g_thread_self());
    }
    
    g_free(pbt);
    
    return return_value;
}

gdouble benchmark_parallel_for(guint start, guint end,
                               gpointer callback, gpointer callback_data) {
    gchar	*temp;
    guint	n_cores, iter_per_core, iter;
    GSList	*threads = NULL, *t;
    GTimer	*timer;
    gdouble	elapsed_time;
    
    timer = g_timer_new();
    
    temp = module_call_method("devices::getProcessorCount");
    n_cores = temp ? atoi(temp) : 1;
    g_free(temp);

    iter_per_core = (end - start) / n_cores;
    
    DEBUG("processor has %d cores; processing %d elements (%d per core)",
          n_cores, (end - start), iter_per_core);
          
    g_timer_start(timer);
    for (iter = start; iter < end; iter += iter_per_core) {
        ParallelBenchTask *pbt = g_new0(ParallelBenchTask, 1);
        GThread *thread;
        
        DEBUG("launching thread %d", 1 + (iter / iter_per_core));
        
        pbt->start    = iter == 0 ? 0 : iter + 1;
        pbt->end      = iter + iter_per_core;
        pbt->data     = callback_data;
        pbt->callback = callback;
        
        if (pbt->end > end)
            pbt->end = end;
        
        thread = g_thread_create((GThreadFunc) benchmark_parallel_for_dispatcher,
                                 pbt, TRUE, NULL);
        threads = g_slist_prepend(threads, thread);
        
        DEBUG("thread %d launched as context %p", 1 + (iter / iter_per_core), threads->data);
    }
    
    DEBUG("waiting for all threads to finish");
    for (t = threads; t; t = t->next) {
        DEBUG("waiting for thread with context %p", t->data);
        g_thread_join((GThread *)t->data);
    }
    
    g_timer_stop(timer);
    elapsed_time = g_timer_elapsed(timer, NULL);
    
    g_slist_free(threads);
    g_timer_destroy(timer);
    
    DEBUG("finishing");
    
    return elapsed_time;
}

static gchar *__benchmark_include_results(gdouble result,
					  const gchar * benchmark,
					  ShellOrderType order_type)
{
    GKeyFile *conf;
    gchar **machines;
    gchar *path, *results = g_strdup("");
    int i;

    conf = g_key_file_new();

    path = g_build_filename(g_get_home_dir(), ".hardinfo", "benchmark.conf", NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
	DEBUG("local benchmark.conf not found, trying system-wide");
	g_free(path);
	path = g_build_filename(params.path_data, "benchmark.conf", NULL);
    }

    g_key_file_load_from_file(conf, path, 0, NULL);

    machines = g_key_file_get_keys(conf, benchmark, NULL, NULL);
    for (i = 0; machines && machines[i]; i++) {
	gchar *value;
	
	value   = g_key_file_get_value(conf, benchmark, machines[i], NULL);
	results = g_strconcat(results, machines[i], "=", value, "\n", NULL);
	
	g_free(value);
    }

    g_strfreev(machines);
    g_free(path);
    g_key_file_free(conf);

    DEBUG("results = %s", results);

    return g_strdup_printf("[$ShellParam$]\n"
			   "Zebra=1\n"
			   "OrderType=%d\n"
			   "ViewType=3\n"
			   "[%s]\n"
			   "<big><b>This Machine</b></big>=%.3f\n"
			   "%s", order_type, benchmark, result, results);
}

static gchar *benchmark_include_results_reverse(gdouble result,
						const gchar * benchmark)
{
    return __benchmark_include_results(result, benchmark,
				       SHELL_ORDER_DESCENDING);
}

static gchar *benchmark_include_results(gdouble result,
					const gchar * benchmark)
{
    return __benchmark_include_results(result, benchmark,
				       SHELL_ORDER_ASCENDING);
}

static gdouble bench_results[BENCHMARK_N_ENTRIES];

#include <arch/common/fib.h>
#include <arch/common/zlib.h>
#include <arch/common/md5.h>
#include <arch/common/sha1.h>
#include <arch/common/blowfish.h>
#include <arch/common/raytrace.h>

gchar *callback_zlib()
{
    return benchmark_include_results_reverse(bench_results[BENCHMARK_ZLIB],
					     "CPU ZLib");
}

gchar *callback_raytr()
{
    return benchmark_include_results(bench_results[BENCHMARK_RAYTRACE],
				     "FPU Raytracing");
}

gchar *callback_bfsh()
{
    return benchmark_include_results(bench_results[BENCHMARK_BLOWFISH],
				     "CPU Blowfish");
}

gchar *callback_md5()
{
    return benchmark_include_results_reverse(bench_results[BENCHMARK_MD5],
					     "CPU MD5");
}

gchar *callback_fib()
{
    return benchmark_include_results(bench_results[BENCHMARK_FIB],
				     "CPU Fibonacci");
}

gchar *callback_sha1()
{
    return benchmark_include_results_reverse(bench_results[BENCHMARK_SHA1],
					     "CPU SHA1");
}

#define RUN_WITH_HIGH_PRIORITY(fn)			\
  do {							\
    int old_priority = getpriority(PRIO_PROCESS, 0);	\
    setpriority(PRIO_PROCESS, 0, -20);			\
    fn();						\
    setpriority(PRIO_PROCESS, 0, old_priority);		\
  } while (0);

void scan_zlib(gboolean reload)
{
    SCAN_START();
    RUN_WITH_HIGH_PRIORITY(benchmark_zlib);
    SCAN_END();
}

void scan_raytr(gboolean reload)
{
    SCAN_START();
    RUN_WITH_HIGH_PRIORITY(benchmark_raytrace);
    SCAN_END();
}

void scan_bfsh(gboolean reload)
{
    SCAN_START();
    RUN_WITH_HIGH_PRIORITY(benchmark_fish);
    SCAN_END();
}

void scan_md5(gboolean reload)
{
    SCAN_START();
    RUN_WITH_HIGH_PRIORITY(benchmark_md5);
    SCAN_END();
}

void scan_fib(gboolean reload)
{
    SCAN_START();
    RUN_WITH_HIGH_PRIORITY(benchmark_fib);
    SCAN_END();
}

void scan_sha1(gboolean reload)
{
    SCAN_START();
    RUN_WITH_HIGH_PRIORITY(benchmark_sha1);
    SCAN_END();
}

const gchar *hi_note_func(gint entry)
{
    switch (entry) {
    case BENCHMARK_ZLIB:
	return "Results in KiB/second. Higher is better.";

    case BENCHMARK_MD5:
    case BENCHMARK_SHA1:
	return "Results in MiB/second. Higher is better.";

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

ModuleAbout *hi_module_get_about(void)
{
    static ModuleAbout ma[] = {
	{
	 .author = "Leandro A. F. Pereira",
	 .description = "Perform tasks and compare with other systems",
	 .version = VERSION,
	 .license = "GNU GPL version 2"}
    };

    return ma;
}

static gchar *get_benchmark_results()
{
    void (*scan_callback) (gboolean rescan);

    gint i = G_N_ELEMENTS(entries) - 1;
    gchar *machine = module_call_method("devices::getProcessorName");
    gchar *param = g_strdup_printf("[param]\n"
				   "machine=%s\n" "nbenchmarks=%d\n",
				   machine, i);
    gchar *result = param;

    for (; i >= 0; i--) {
	if ((scan_callback = entries[i].scan_callback)) {
	    scan_callback(FALSE);

	    result = g_strdup_printf("%s\n"
				     "[bench%d]\n"
				     "name=%s\n"
				     "value=%f\n",
				     result,
				     i, entries[i].name, bench_results[i]);
	}
    }

    g_free(machine);
    g_free(param);

    return result;
}

void hi_module_init(void)
{
    static SyncEntry se[] = {
	{
	 .fancy_name = "Send Benchmark Results",
	 .name = "SendBenchmarkResults",
	 .save_to = NULL,
	 .get_data = get_benchmark_results},
	{
	 .fancy_name = "Receive Benchmark Results",
	 .name = "RecvBenchmarkResults",
	 .save_to = "benchmark.conf",
	 .get_data = NULL}
    };

    sync_manager_add_entry(&se[0]);
    sync_manager_add_entry(&se[1]);
}

gchar **hi_module_get_dependencies(void)
{
    static gchar *deps[] = { "devices.so", NULL };

    return deps;
}
