/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include <sys/types.h>
#include <signal.h>

enum {
    BENCHMARK_BLOWFISH,
    BENCHMARK_CRYPTOHASH,
    BENCHMARK_FIB,
    BENCHMARK_NQUEENS,
    BENCHMARK_FFT,
    BENCHMARK_RAYTRACE,
    BENCHMARK_GUI,
    BENCHMARK_N_ENTRIES
} Entries;

void scan_fft(gboolean reload);
void scan_raytr(gboolean reload);
void scan_bfsh(gboolean reload);
void scan_cryptohash(gboolean reload);
void scan_fib(gboolean reload);
void scan_nqueens(gboolean reload);
void scan_gui(gboolean reload);

gchar *callback_fft();
gchar *callback_raytr();
gchar *callback_bfsh();
gchar *callback_fib();
gchar *callback_cryptohash();
gchar *callback_nqueens();
gchar *callback_gui();

static ModuleEntry entries[] = {
    {"CPU Blowfish", "blowfish.png", callback_bfsh, scan_bfsh, MODULE_FLAG_NO_REMOTE},
    {"CPU CryptoHash", "cryptohash.png", callback_cryptohash, scan_cryptohash, MODULE_FLAG_NO_REMOTE},
    {"CPU Fibonacci", "nautilus.png", callback_fib, scan_fib, MODULE_FLAG_NO_REMOTE},
    {"CPU N-Queens", "nqueens.png", callback_nqueens, scan_nqueens, MODULE_FLAG_NO_REMOTE},
    {"FPU FFT", "fft.png", callback_fft, scan_fft, MODULE_FLAG_NO_REMOTE},
    {"FPU Raytracing", "raytrace.png", callback_raytr, scan_raytr, MODULE_FLAG_NO_REMOTE},
    {"GPU Drawing", "module.png", callback_gui, scan_gui, MODULE_FLAG_NO_REMOTE},
    {NULL}
};


static gboolean sending_benchmark_results = FALSE;

typedef struct _ParallelBenchTask ParallelBenchTask;

struct _ParallelBenchTask {
    gint	thread_number;
    guint	start, end;
    gpointer	data, callback;
};

gpointer benchmark_parallel_for_dispatcher(gpointer data)
{
    ParallelBenchTask 	*pbt = (ParallelBenchTask *)data;
    gpointer 		(*callback)(unsigned int start, unsigned int end, void *data, gint thread_number);
    gpointer		return_value;

    if ((callback = pbt->callback)) {
        DEBUG("this is thread %p; items %d -> %d, data %p", g_thread_self(),
              pbt->start, pbt->end, pbt->data);
        return_value = callback(pbt->start, pbt->end, pbt->data, pbt->thread_number);
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
    guint	n_cores, iter_per_core, iter, thread_number = 0;
    gdouble	elapsed_time;
    GSList	*threads = NULL, *t;
    GTimer	*timer;

    timer = g_timer_new();

    temp = module_call_method("devices::getProcessorCount");
    n_cores = temp ? atoi(temp) : 1;
    g_free(temp);

    while (1) {
        iter_per_core = (end - start) / n_cores;

        if (iter_per_core == 0) {
          DEBUG("not enough items per core; disabling one");
          n_cores--;
        } else {
          break;
        }
    }

    DEBUG("processor has %d cores; processing %d elements (%d per core)",
          n_cores, (end - start), iter_per_core);

    g_timer_start(timer);
    for (iter = start; iter < end; iter += iter_per_core) {
        ParallelBenchTask *pbt = g_new0(ParallelBenchTask, 1);
        GThread *thread;

        DEBUG("launching thread %d", 1 + (iter / iter_per_core));

        pbt->thread_number = thread_number++;
        pbt->start    = iter == 0 ? 0 : iter;
        pbt->end      = iter + iter_per_core - 1;
        pbt->data     = callback_data;
        pbt->callback = callback;

        if (pbt->end > end)
            pbt->end = end;

        thread = g_thread_create((GThreadFunc) benchmark_parallel_for_dispatcher,
                                 pbt, TRUE, NULL);
        threads = g_slist_append(threads, thread);

        DEBUG("thread %d launched as context %p", thread_number, thread);
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

    DEBUG("finishing; all threads took %f seconds to finish", elapsed_time);

    return elapsed_time;
}

static gchar *clean_cpuname(gchar *cpuname)
{
    gchar *ret = NULL, *tmp;
    gchar *remove[] = {
        "(R)", "(r)", "(TM)", "(tm)", "Processor",
        "Technology", "processor", "CPU",
        "cpu", "Genuine", "Authentic", NULL
    };
    gint i;

    ret = g_strdup(cpuname);
    for (i = 0; remove[i]; i++) {
      tmp = strreplace(ret, remove[i], "");
      g_free(ret);
      ret = tmp;
    }

    ret = strend(ret, '@');
    ret = g_strstrip(ret);

    tmp = g_strdup(ret);
    g_free(ret);

    return tmp;
}

static gchar *__benchmark_include_results(gdouble result,
					  const gchar * benchmark,
					  ShellOrderType order_type)
{
    GKeyFile *conf;
    gchar **machines;
    gchar *path, *results = g_strdup(""), *return_value, *processor_frequency;
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
	gchar *value, *cleaned_machine;

	value   = g_key_file_get_value(conf, benchmark, machines[i], NULL);
	cleaned_machine = clean_cpuname(machines[i]);
	results = h_strconcat(results, cleaned_machine, "=", value, "\n", NULL);

	g_free(value);
	g_free(cleaned_machine);
    }

    g_strfreev(machines);
    g_free(path);
    g_key_file_free(conf);

    if (result > 0.0f) {
        processor_frequency = module_call_method("devices::getProcessorFrequency");
        return_value = g_strdup_printf("[$ShellParam$]\n"
			       	   "Zebra=1\n"
			       	   "OrderType=%d\n"
	       			   "ViewType=3\n"
	       			   "ColumnTitle$Extra1=CPU Clock\n"
			       	   "ColumnTitle$Progress=Results\n"
			       	   "ColumnTitle$TextValue=CPU\n"
			       	   "ShowColumnHeaders=true\n"
	       			   "[%s]\n"
		       		   "<big><b>This Machine</b></big>=%.3f|%s MHz\n"
			       	   "%s", order_type, benchmark, result, processor_frequency, results);
        g_free(processor_frequency);
    } else {
        return_value = g_strdup_printf("[$ShellParam$]\n"
			       	   "Zebra=1\n"
			       	   "OrderType=%d\n"
	       			   "ViewType=3\n"
	       			   "ColumnTitle$Extra1=CPU Clock\n"
			       	   "ColumnTitle$Progress=Results\n"
			       	   "ColumnTitle$TextValue=CPU\n"
			       	   "ShowColumnHeaders=true\n"
	       			   "[%s]\n"
			       	   "%s", order_type, benchmark, results);
    }
    return return_value;
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
#include <arch/common/cryptohash.h>
#include <arch/common/blowfish.h>
#include <arch/common/raytrace.h>
#include <arch/common/nqueens.h>
#include <arch/common/fft.h>
#include <arch/common/drawing.h>

gchar *callback_gui()
{
    return benchmark_include_results_reverse(bench_results[BENCHMARK_GUI],
                                             "GPU Drawing");
}

gchar *callback_fft()
{
    return benchmark_include_results(bench_results[BENCHMARK_FFT],
				     "FPU FFT");
}

gchar *callback_nqueens()
{
    return benchmark_include_results(bench_results[BENCHMARK_NQUEENS],
				     "CPU N-Queens");
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

gchar *callback_cryptohash()
{
    return benchmark_include_results_reverse(bench_results[BENCHMARK_CRYPTOHASH],
					     "CPU CryptoHash");
}

gchar *callback_fib()
{
    return benchmark_include_results(bench_results[BENCHMARK_FIB],
				     "CPU Fibonacci");
}

typedef struct _BenchmarkDialog BenchmarkDialog;
struct _BenchmarkDialog {
    GtkWidget *dialog;
    double result;
};

static gboolean do_benchmark_handler(GIOChannel *source,
                                     GIOCondition condition,
                                     gpointer data)
{
    BenchmarkDialog *bench_dialog = (BenchmarkDialog*)data;
    GIOStatus status;
    gchar *result;

    status = g_io_channel_read_line(source, &result, NULL, NULL, NULL);
    if (status != G_IO_STATUS_NORMAL) {
        DEBUG("error while reading benchmark result");

        bench_dialog->result = -1.0f;
        gtk_widget_destroy(bench_dialog->dialog);
        return FALSE;
    }

    bench_dialog->result = atof(result);
    gtk_widget_destroy(bench_dialog->dialog);
    g_free(result);

    return FALSE;
}

static void do_benchmark(void (*benchmark_function)(void), int entry)
{
    int old_priority = 0;
    
    if (params.gui_running && !sending_benchmark_results) {
       gchar *argv[] = { params.argv0, "-b", entries[entry].name,
                         "-m", "benchmark.so", "-a", NULL };
       GPid bench_pid;
       gint bench_stdout;
       GtkWidget *bench_dialog;
       GtkWidget *bench_image;
       BenchmarkDialog *benchmark_dialog;
       GSpawnFlags spawn_flags = G_SPAWN_STDERR_TO_DEV_NULL;
       gchar *bench_status;

       bench_status = g_strdup_printf("Benchmarking: <b>%s</b>.", entries[entry].name);

       shell_view_set_enabled(FALSE);
       shell_status_update(bench_status);
       
       g_free(bench_status);

       bench_image = icon_cache_get_image("benchmark.png");
       gtk_widget_show(bench_image);

       bench_dialog = gtk_message_dialog_new(GTK_WINDOW(shell_get_main_shell()->window),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_NONE,
                                             "Benchmarking. Please do not move your mouse " \
                                             "or press any keys.");
       g_object_set_data(G_OBJECT(bench_dialog), "result", "0.0");
       gtk_dialog_add_buttons(GTK_DIALOG(bench_dialog),
                              "Cancel", GTK_RESPONSE_ACCEPT, NULL);
       gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(bench_dialog), bench_image);

       while (gtk_events_pending()) {
         gtk_main_iteration();
       }

       benchmark_dialog = g_new0(BenchmarkDialog, 1);
       benchmark_dialog->dialog = bench_dialog;
       benchmark_dialog->result = -1.0f;
       
       if (!g_path_is_absolute(params.argv0)) {
          spawn_flags |= G_SPAWN_SEARCH_PATH;
       }

       if (g_spawn_async_with_pipes(NULL,
                                    argv, NULL,
                                    spawn_flags,
                                    NULL, NULL,
                                    &bench_pid,
                                    NULL, &bench_stdout, NULL,
                                    NULL)) {
          GIOChannel *channel;
          guint watch_id;

          channel = g_io_channel_unix_new(bench_stdout);
          watch_id = g_io_add_watch(channel, G_IO_IN, do_benchmark_handler,
                                    benchmark_dialog);

          switch (gtk_dialog_run(GTK_DIALOG(bench_dialog))) {
            case GTK_RESPONSE_NONE:
              DEBUG("benchmark finished");
              break;
            case GTK_RESPONSE_ACCEPT:
              DEBUG("cancelling benchmark");

              gtk_widget_destroy(bench_dialog);
              g_source_remove(watch_id);
              kill(bench_pid, SIGINT);
          }

          bench_results[entry] = benchmark_dialog->result;

          g_io_channel_unref(channel);
          shell_status_set_enabled(TRUE);
          g_free(benchmark_dialog);

          shell_status_update("Done.");

          return;
       }
       
       gtk_widget_destroy(bench_dialog);
       g_free(benchmark_dialog);
       shell_status_set_enabled(TRUE);
       shell_status_update("Done.");
    }

    setpriority(PRIO_PROCESS, 0, -20);
    benchmark_function();
    setpriority(PRIO_PROCESS, 0, old_priority);
}

void scan_gui(gboolean reload)
{
    SCAN_START();

    if (params.run_benchmark) {
        int argc = 0;

        ui_init(&argc, NULL);
    }

    if (params.gui_running || params.run_benchmark) {
        do_benchmark(benchmark_gui, BENCHMARK_GUI);
    } else {
        bench_results[BENCHMARK_GUI] = 0.0f;
    }
    SCAN_END();
}

void scan_fft(gboolean reload)
{
    SCAN_START();
    do_benchmark(benchmark_fft, BENCHMARK_FFT);
    SCAN_END();
}

void scan_nqueens(gboolean reload)
{
    SCAN_START();
    do_benchmark(benchmark_nqueens, BENCHMARK_NQUEENS);
    SCAN_END();
}

void scan_raytr(gboolean reload)
{
    SCAN_START();
    do_benchmark(benchmark_raytrace, BENCHMARK_RAYTRACE);
    SCAN_END();
}

void scan_bfsh(gboolean reload)
{
    SCAN_START();
    do_benchmark(benchmark_fish, BENCHMARK_BLOWFISH);
    SCAN_END();
}

void scan_cryptohash(gboolean reload)
{
    SCAN_START();
    do_benchmark(benchmark_cryptohash, BENCHMARK_CRYPTOHASH);
    SCAN_END();
}

void scan_fib(gboolean reload)
{
    SCAN_START();
    do_benchmark(benchmark_fib, BENCHMARK_FIB);
    SCAN_END();
}

const gchar *hi_note_func(gint entry)
{
    switch (entry) {
    case BENCHMARK_CRYPTOHASH:
	return "Results in MiB/second. Higher is better.";

    case BENCHMARK_GUI:
        return "Results in HIMarks. Higher is better.";

    case BENCHMARK_FFT:
    case BENCHMARK_RAYTRACE:
    case BENCHMARK_BLOWFISH:
    case BENCHMARK_FIB:
    case BENCHMARK_NQUEENS:
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

    sending_benchmark_results = TRUE;

    gint i = G_N_ELEMENTS(entries) - 1;
    gchar *machine = module_call_method("devices::getProcessorName");
    gchar *machineclock = module_call_method("devices::getProcessorFrequency");
    gchar *machineram = module_call_method("devices::getMemoryTotal");
    gchar *result = g_strdup_printf("[param]\n"
				    "machine=%s\n"
				    "machineclock=%s\n"
				    "machineram=%s\n"
				    "nbenchmarks=%d\n",
				    machine,
				    machineclock,
				    machineram, i);
    for (; i >= 0; i--) {
	if ((scan_callback = entries[i].scan_callback)) {
	    if (bench_results[i] < 0.0) {
	       /* benchmark was cancelled */
	       scan_callback(TRUE);
            } else {
  	       scan_callback(FALSE);
            }

	    result = h_strdup_cprintf("[bench%d]\n"
				      "name=%s\n"
				      "value=%f\n",
				      result,
				      i, entries[i].name, bench_results[i]);
	}
    }

    g_free(machine);
    g_free(machineclock);
    g_free(machineram);

    sending_benchmark_results = FALSE;
    
    return result;
}

static gchar *run_benchmark(gchar *name)
{
    int i;

    DEBUG("name = %s", name);

    for (i = 0; entries[i].name; i++) {
      if (g_str_equal(entries[i].name, name)) {
        void (*scan_callback)(gboolean rescan);

        if ((scan_callback = entries[i].scan_callback)) {
          scan_callback(FALSE);

          return g_strdup_printf("%f", bench_results[i]);
        }
      }
    }

    return NULL;
}

ShellModuleMethod *hi_exported_methods(void)
{
    static ShellModuleMethod m[] = {
        {"runBenchmark", run_benchmark},
	{NULL}
    };

    return m;
}

void hi_module_init(void)
{
    static SyncEntry se[] = {
	{
	 .fancy_name = "Send benchmark results",
	 .name = "SendBenchmarkResults",
	 .save_to = NULL,
	 .get_data = get_benchmark_results},
	{
	 .fancy_name = "Receive benchmark results",
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

