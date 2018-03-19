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

#include "benchmark.h"

#include "benchmark/bench_results.c"

bench_value bench_results[BENCHMARK_N_ENTRIES];

static void do_benchmark(void (*benchmark_function)(void), int entry);
static gchar *benchmark_include_results_reverse(bench_value result, const gchar * benchmark);
static gchar *benchmark_include_results(bench_value result, const gchar * benchmark);

/* ModuleEntry entries, scan_*(), callback_*(), etc. */
#include "benchmark/benches.c"

static gboolean sending_benchmark_results = FALSE;

char *bench_value_to_str(bench_value r) {
    return g_strdup_printf("%lf; %lf; %d", r.result, r.elapsed_time, r.threads_used);
}

bench_value bench_value_from_str(const char* str) {
    bench_value ret = EMPTY_BENCH_VALUE;
    double r, e;
    int t, c;
    if (str) {
        c = sscanf(str, "%lf; %lf; %d", &r, &e, &t);
        if (c >= 3) {
            ret.result = r;
            ret.elapsed_time = e;
            ret.threads_used = t;
        }
    }
    return ret;
}

typedef struct _ParallelBenchTask ParallelBenchTask;

struct _ParallelBenchTask {
    gint	thread_number;
    guint	start, end;
    gpointer	data, callback;
    int *stop;
};

static gpointer benchmark_crunch_for_dispatcher(gpointer data)
{
    ParallelBenchTask 	*pbt = (ParallelBenchTask *)data;
    gpointer (*callback)(void *data, gint thread_number);
    gpointer return_value = g_malloc(sizeof(int));
    int count = 0;

    if ((callback = pbt->callback)) {
        while(!*pbt->stop) {
            callback(pbt->data, pbt->thread_number);
            /* don't count if didn't finish in time */
            if (!*pbt->stop)
                count++;
        }
    } else {
        DEBUG("this is thread %p; callback is NULL and it should't be!", g_thread_self());
    }

    g_free(pbt);

    *(double*)return_value = (double)count;
    return return_value;
}

bench_value benchmark_crunch_for(float seconds, gint n_threads,
                               gpointer callback, gpointer callback_data) {
    int cpu_procs, cpu_cores, cpu_threads, thread_number, stop = 0;
    GSList *threads = NULL, *t;
    GTimer *timer;
    bench_value ret = EMPTY_BENCH_VALUE;

    timer = g_timer_new();

    cpu_procs_cores_threads(&cpu_procs, &cpu_cores, &cpu_threads);
    if (n_threads > 0)
        ret.threads_used = n_threads;
    else if (n_threads < 0)
        ret.threads_used = cpu_cores;
    else
        ret.threads_used = cpu_threads;

    g_timer_start(timer);
    for (thread_number = 0; thread_number < ret.threads_used; thread_number++) {
        ParallelBenchTask *pbt = g_new0(ParallelBenchTask, 1);
        GThread *thread;

        DEBUG("launching thread %d", thread_number);

        pbt->thread_number = thread_number;
        pbt->data     = callback_data;
        pbt->callback = callback;
        pbt->stop = &stop;

        thread = g_thread_new("dispatcher",
            (GThreadFunc)benchmark_crunch_for_dispatcher, pbt);
        threads = g_slist_prepend(threads, thread);

        DEBUG("thread %d launched as context %p", thread_number, thread);
    }

    /* wait for time */
    //while ( g_timer_elapsed(timer, NULL) < seconds ) { }
    g_usleep(seconds * 1000000);

    /* signal all threads to stop */
    stop = 1;
    g_timer_stop(timer);

    ret.result = 0;
    DEBUG("waiting for all threads to finish");
    for (t = threads; t; t = t->next) {
        DEBUG("waiting for thread with context %p", t->data);
        gpointer *rv = g_thread_join((GThread *)t->data);
        ret.result += *(double*)rv;
        g_free(rv);
    }

    ret.elapsed_time = g_timer_elapsed(timer, NULL);

    g_slist_free(threads);
    g_timer_destroy(timer);

    return ret;
}

static gpointer benchmark_parallel_for_dispatcher(gpointer data)
{
    ParallelBenchTask 	*pbt = (ParallelBenchTask *)data;
    gpointer 		(*callback)(unsigned int start, unsigned int end, void *data, gint thread_number);
    gpointer		return_value = NULL;

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

/* one call for each thread to be used */
bench_value benchmark_parallel(gint n_threads, gpointer callback, gpointer callback_data) {
    int cpu_procs, cpu_cores, cpu_threads;
    cpu_procs_cores_threads(&cpu_procs, &cpu_cores, &cpu_threads);
    if (n_threads == 0) n_threads = cpu_threads;
    else if (n_threads == -1) n_threads = cpu_cores;
    return  benchmark_parallel_for(n_threads, 0, n_threads, callback, callback_data);
}

/* Note:
 *    benchmark_parallel_for(): element [start] included, but [end] is excluded.
 *    callback(): expected to processes elements [start] through [end] inclusive.
 */
bench_value benchmark_parallel_for(gint n_threads, guint start, guint end,
                               gpointer callback, gpointer callback_data) {
    gchar	*temp;
    int 	cpu_procs, cpu_cores, cpu_threads;
    guint	iter_per_thread, iter, thread_number = 0;
    GSList	*threads = NULL, *t;
    GTimer	*timer;

    bench_value ret = EMPTY_BENCH_VALUE;

    timer = g_timer_new();

    cpu_procs_cores_threads(&cpu_procs, &cpu_cores, &cpu_threads);

    if (n_threads > 0)
        ret.threads_used = n_threads;
    else if (n_threads < 0)
        ret.threads_used = cpu_cores;
    else
        ret.threads_used = cpu_threads;

    while (ret.threads_used > 0) {
        iter_per_thread = (end - start) / ret.threads_used;

        if (iter_per_thread == 0) {
          DEBUG("not enough items per thread; disabling one thread");
          ret.threads_used--;
        } else {
          break;
        }
    }

    DEBUG("Using %d threads across %d logical processors; processing %d elements (%d per thread)",
          ret.threads_used, cpu_threads, (end - start), iter_per_thread);

    g_timer_start(timer);
    for (iter = start; iter < end; ) {
        ParallelBenchTask *pbt = g_new0(ParallelBenchTask, 1);
        GThread *thread;

        guint ts = iter, te = iter + iter_per_thread;
        /* add the remainder of items/iter_per_thread to the last thread */
        if (end - te < iter_per_thread)
            te = end;
        iter = te;

        DEBUG("launching thread %d", 1 + thread_number);

        pbt->thread_number = thread_number++;
        pbt->start    = ts;
        pbt->end      = te - 1;
        pbt->data     = callback_data;
        pbt->callback = callback;

        thread = g_thread_new("dispatcher",
            (GThreadFunc)benchmark_parallel_for_dispatcher, pbt);
        threads = g_slist_prepend(threads, thread);

        DEBUG("thread %d launched as context %p", thread_number, thread);
    }

    DEBUG("waiting for all threads to finish");
    for (t = threads; t; t = t->next) {
        DEBUG("waiting for thread with context %p", t->data);
        gpointer *rv = g_thread_join((GThread *)t->data);
        if (rv) {
            if (ret.result == -1.0) ret.result = 0;
            ret.result += *(double*)rv;
        }
        g_free(rv);
    }

    g_timer_stop(timer);
    ret.elapsed_time = g_timer_elapsed(timer, NULL);

    g_slist_free(threads);
    g_timer_destroy(timer);

    DEBUG("finishing; all threads took %f seconds to finish", ret.elapsed_time);

    return ret;
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

gchar *hi_more_info(gchar * entry)
{
    gchar *info = moreinfo_lookup_with_prefix("BENCH", entry);
    if (info)
        return g_strdup(info);
    return g_strdup("?");
}

gchar *hi_get_field(gchar * field)
{
    gchar *info = moreinfo_lookup_with_prefix("BENCH", field);
    if (info)
        return g_strdup(info);
    return g_strdup(field);
}

static void br_mi_add(char **results_list, bench_result *b, gboolean select) {
    gchar *ckey, *rkey;

    ckey = hardinfo_clean_label(b->machine->cpu_name, 0);
    rkey = strdup(b->machine->mid);

    *results_list = h_strdup_cprintf("$%s%s$%s=%.2f|%s\n", *results_list,
        select ? "*" : "", rkey, ckey,
        b->bvalue.result, b->machine->cpu_config);

    moreinfo_add_with_prefix("BENCH", rkey, bench_result_more_info(b) );

    g_free(ckey);
    g_free(rkey);
}

static gchar *__benchmark_include_results(bench_value r,
					  const gchar * benchmark,
					  ShellOrderType order_type)
{
    bench_result *b = NULL;
    GKeyFile *conf;
    gchar **machines;
    gchar *path, *results = g_strdup(""), *return_value, *processor_frequency, *processor_name;
    int i, n_threads;

    moreinfo_del_with_prefix("BENCH");

    if (r.result > 0.0) {
        b = bench_result_this_machine(benchmark, r);
        br_mi_add(&results, b, 1);
    }

    conf = g_key_file_new();

    path = g_build_filename(g_get_home_dir(), ".hardinfo", "benchmark.conf", NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        DEBUG("local benchmark.conf not found, trying system-wide");
        g_free(path);
        path = g_build_filename(params.path_data, "benchmark.conf", NULL);
    }

    g_key_file_load_from_file(conf, path, 0, NULL);
    g_key_file_set_list_separator(conf, '|');

    machines = g_key_file_get_keys(conf, benchmark, NULL, NULL);
    for (i = 0; machines && machines[i]; i++) {
        gchar **values;
        bench_result *sbr;

        values = g_key_file_get_string_list(conf, benchmark, machines[i], NULL, NULL);

        sbr = bench_result_benchmarkconf(benchmark, machines[i], values);
        br_mi_add(&results, sbr, 0);

        bench_result_free(sbr);
        g_strfreev(values);
    }

    g_strfreev(machines);
    g_free(path);
    g_key_file_free(conf);

    return_value = g_strdup_printf("[$ShellParam$]\n"
                   "Zebra=1\n"
                   "OrderType=%d\n"
                   "ViewType=4\n"
                   "ColumnTitle$Extra1=%s\n" /* CPU Clock */
                   "ColumnTitle$Progress=%s\n" /* Results */
                   "ColumnTitle$TextValue=%s\n" /* CPU */
                   "ShowColumnHeaders=true\n"
                   "[%s]\n%s",
                   order_type,
                   _("CPU Config"), _("Results"), _("CPU"),
                   benchmark, results);

    bench_result_free(b);
    return return_value;
}

static gchar *benchmark_include_results_reverse(bench_value result, const gchar * benchmark)
{
    return __benchmark_include_results(result, benchmark, SHELL_ORDER_DESCENDING);
}

static gchar *benchmark_include_results(bench_value result, const gchar * benchmark)
{
    return __benchmark_include_results(result, benchmark, SHELL_ORDER_ASCENDING);
}

typedef struct _BenchmarkDialog BenchmarkDialog;
struct _BenchmarkDialog {
    GtkWidget *dialog;
    bench_value r;
};

static gboolean do_benchmark_handler(GIOChannel *source,
                                     GIOCondition condition,
                                     gpointer data)
{
    BenchmarkDialog *bench_dialog = (BenchmarkDialog*)data;
    GIOStatus status;
    gchar *result;
    bench_value r = EMPTY_BENCH_VALUE;

    status = g_io_channel_read_line(source, &result, NULL, NULL, NULL);
    if (status != G_IO_STATUS_NORMAL) {
        DEBUG("error while reading benchmark result");
        r.result = -1.0f;
        bench_dialog->r = r;
        gtk_widget_destroy(bench_dialog->dialog);
        return FALSE;
    }

    r = bench_value_from_str(result);
    bench_dialog->r = r;

    gtk_widget_destroy(bench_dialog->dialog);
    g_free(result);

    return FALSE;
}

static void do_benchmark(void (*benchmark_function)(void), int entry)
{
    int old_priority = 0;

    if (params.skip_benchmarks) return;

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

       bench_value r = EMPTY_BENCH_VALUE;
       bench_results[entry] = r;

       bench_status = g_strdup_printf(_("Benchmarking: <b>%s</b>."), entries[entry].name);

       shell_view_set_enabled(FALSE);
       shell_status_update(bench_status);

       g_free(bench_status);

       bench_image = icon_cache_get_image("benchmark.png");
       gtk_widget_show(bench_image);

#if GTK_CHECK_VERSION(3, 0, 0)
       GtkWidget *button;
       GtkWidget *content_area;
       GtkWidget *hbox;
       GtkWidget *label;

       bench_dialog = gtk_dialog_new_with_buttons("",
                                                  NULL,
                                                  GTK_DIALOG_MODAL,
                                                  _("Cancel"),
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
       content_area = gtk_dialog_get_content_area(GTK_DIALOG(bench_dialog));
       label = gtk_label_new(_("Benchmarking. Please do not move your mouse " \
                                             "or press any keys."));
       hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
       gtk_box_pack_start(GTK_BOX(hbox), bench_image, TRUE, TRUE, 5);
       gtk_box_pack_end(GTK_BOX(hbox), label, TRUE, TRUE, 5);
       gtk_container_add(GTK_CONTAINER (content_area), hbox);
       gtk_widget_show_all(bench_dialog);
#else
       bench_dialog = gtk_message_dialog_new(GTK_WINDOW(shell_get_main_shell()->window),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_NONE,
                                             _("Benchmarking. Please do not move your mouse " \
                                             "or press any keys."));
       gtk_dialog_add_buttons(GTK_DIALOG(bench_dialog),
                              _("Cancel"), GTK_RESPONSE_ACCEPT, NULL);
       gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(bench_dialog), bench_image);
#endif

       while (gtk_events_pending()) {
         gtk_main_iteration();
       }

       benchmark_dialog = g_new0(BenchmarkDialog, 1);
       benchmark_dialog->dialog = bench_dialog;
       benchmark_dialog->r = r;

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

          DEBUG("spawning benchmark; pid=%d", bench_pid);

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

          bench_results[entry] = benchmark_dialog->r;

          g_io_channel_unref(channel);
          shell_view_set_enabled(TRUE);
          shell_status_set_enabled(TRUE);
          g_free(benchmark_dialog);

          shell_status_update(_("Done."));

          return;
       }

       gtk_widget_destroy(bench_dialog);
       g_free(benchmark_dialog);
       shell_status_set_enabled(TRUE);
       shell_status_update(_("Done."));
    }

    setpriority(PRIO_PROCESS, 0, -20);
    benchmark_function();
    setpriority(PRIO_PROCESS, 0, old_priority);
}

gchar *hi_module_get_name(void)
{
    return g_strdup(_("Benchmarks"));
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
	 .description = N_("Perform tasks and compare with other systems"),
	 .version = VERSION,
	 .license = "GNU GPL version 2"}
    };

    return ma;
}

static gchar *get_benchmark_results()
{
    gint i;
    void (*scan_callback) (gboolean rescan);

    sending_benchmark_results = TRUE;

    gchar *machine = module_call_method("devices::getProcessorName");
    gchar *machineclock = module_call_method("devices::getProcessorFrequency");
    gchar *machineram = module_call_method("devices::getMemoryTotal");
    gchar *result = g_strdup_printf("[param]\n"
				    "machine=%s\n"
				    "machineclock=%s\n"
				    "machineram=%s\n"
				    "nbenchmarks=%zu\n",
				    machine,
				    machineclock,
				    machineram,
				    G_N_ELEMENTS(entries) - 1);
    for (i = 0; i < G_N_ELEMENTS(entries); i++) {
        scan_callback = entries[i].scan_callback;
        if (!scan_callback)
            continue;

        if (bench_results[i].result < 0.0) {
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

#define CHK_RESULT_FORMAT(F) (params.result_format && strcmp(params.result_format, F) == 0)

          if (params.run_benchmark) {
            if (CHK_RESULT_FORMAT("conf") ) {
               bench_result *b = bench_result_this_machine(name, bench_results[i]);
               char *temp = bench_result_benchmarkconf_line(b);
               bench_result_free(b);
               return temp;
            } else if (CHK_RESULT_FORMAT("shell") ) {
               bench_result *b = bench_result_this_machine(name, bench_results[i]);
               char *temp = bench_result_more_info_complete(b);
               bench_result_free(b);
               return temp;
            }
            /* defaults to "short" which is below*/
          }

          return bench_value_to_str(bench_results[i]);
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
	 .fancy_name = N_("Send benchmark results"),
	 .name = "SendBenchmarkResults",
	 .save_to = NULL,
	 .get_data = get_benchmark_results},
	{
	 .fancy_name = N_("Receive benchmark results"),
	 .name = "RecvBenchmarkResults",
	 .save_to = "benchmark.conf",
	 .get_data = NULL}
    };

    sync_manager_add_entry(&se[0]);
    sync_manager_add_entry(&se[1]);

    bench_value er = EMPTY_BENCH_VALUE;
    int i;
    for (i = 0; i < G_N_ELEMENTS(entries) - 1; i++) {
         bench_results[i] = er;
    }
}

gchar **hi_module_get_dependencies(void)
{
    static gchar *deps[] = { "devices.so", NULL };

    return deps;
}
