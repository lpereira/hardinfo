/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or Later.
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

/*
 * Functions h_strdup_cprintf and h_strconcat are based on GLib version 2.4.6
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

#include <config.h>

#include <report.h>
#include <string.h>
#include <shell.h>
#include <iconcache.h>
#include <hardinfo.h>
#include <gtk/gtk.h>

#include <binreloc.h>

#include <sys/stat.h>
#include <sys/types.h>

#define KiB 1024
#define MiB 1048576
#define GiB 1073741824
#define TiB 1099511627776
#define PiB 1125899906842624

void sync_manager_clear_entries(void);

gchar *find_program(gchar *program_name)
{
    int i;
    char *temp;
    static GHashTable *cache = NULL;
    const char *path[] = { "/usr/local/bin", "/usr/local/sbin",
		                   "/usr/bin", "/usr/sbin",
		                   "/bin", "/sbin",
		                   NULL };

    /* we don't need to call stat() every time: cache the results */
    if (!cache) {
    	cache = g_hash_table_new(g_str_hash, g_str_equal);
    } else if ((temp = g_hash_table_lookup(cache, program_name))) {
    	return g_strdup(temp);
    }

    for (i = 0; path[i]; i++) {
    	temp = g_build_filename(path[i], program_name, NULL);

    	if (g_file_test(temp, G_FILE_TEST_IS_EXECUTABLE)) {
    		g_hash_table_insert(cache, program_name, g_strdup(temp));
		return temp;
    	}

    	g_free(temp);
    }

    /* our search has failed; use GLib's search (which uses $PATH env var) */
    if ((temp = g_find_program_in_path(program_name))) {
    	g_hash_table_insert(cache, program_name, g_strdup(temp));
    	return temp;
    }

    return NULL;
}

gchar *seconds_to_string(unsigned int seconds)
{
    unsigned int hours, minutes, days;
    const gchar *days_fmt, *hours_fmt, *minutes_fmt, *seconds_fmt;
    gchar *full_fmt, *ret = g_strdup("");

    minutes = seconds / 60;
    seconds %= 60;
    hours = minutes / 60;
    minutes %= 60;
    days = hours / 24;
    hours %= 24;

    days_fmt = ngettext("%d day", "%d days", days);
    hours_fmt = ngettext("%d hour", "%d hours", hours);
    minutes_fmt = ngettext("%d minute", "%d minutes", minutes);
    seconds_fmt = ngettext("%d second", "%d seconds", seconds);

    if (days) {
        full_fmt = g_strdup_printf("%s %s %s %s", days_fmt, hours_fmt, minutes_fmt, seconds_fmt);
        ret = g_strdup_printf(full_fmt, days, hours, minutes, seconds);
    } else if (hours) {
        full_fmt = g_strdup_printf("%s %s %s", hours_fmt, minutes_fmt, seconds_fmt);
        ret = g_strdup_printf(full_fmt, hours, minutes, seconds);
    } else if (minutes) {
        full_fmt = g_strdup_printf("%s %s", minutes_fmt, seconds_fmt);
        ret = g_strdup_printf(full_fmt, minutes, seconds);
    } else {
        ret = g_strdup_printf(seconds_fmt, seconds);
    }
    g_free(full_fmt);
    return ret;
}

gchar *size_human_readable(gfloat size)
{
    if (size < KiB)
        return g_strdup_printf(_("%.1f B"), size);
    if (size < MiB)
        return g_strdup_printf(_("%.1f KiB"), size / KiB);
    if (size < GiB)
        return g_strdup_printf(_("%.1f MiB"), size / MiB);
    if (size < TiB)
        return g_strdup_printf(_("%.1f GiB"), size / GiB);
    if (size < PiB)
        return g_strdup_printf(_("%.1f TiB"), size / TiB);

    return g_strdup_printf(_("%.1f PiB"), size / PiB);
}

char *strend(gchar * str, gchar chr)
{
    if (!str)
	return NULL;

    char *p;
    if ((p = g_utf8_strchr(str, -1, chr)))
	*p = 0;

    return str;
}

void remove_quotes(gchar * str)
{
    if (!str)
	return;

    while (*str == '"')
	*(str++) = ' ';

    strend(str, '"');
}

void remove_linefeed(gchar * str)
{
    strend(str, '\n');
}

void widget_set_cursor(GtkWidget * widget, GdkCursorType cursor_type)
{
    GdkCursor *cursor;
    GdkDisplay *display;
    GdkWindow *gdk_window;

    display = gtk_widget_get_display(widget);
    cursor = gdk_cursor_new_for_display(display, cursor_type);
    gdk_window = gtk_widget_get_window(widget);
    if (cursor) {
        gdk_window_set_cursor(gdk_window, cursor);
        gdk_display_flush(display);
#if GTK_CHECK_VERSION(3, 0, 0)
        g_object_unref(cursor);
#else
        gdk_cursor_unref(cursor);
#endif
    }

    while (gtk_events_pending())
        gtk_main_iteration();
}

static gboolean __nonblock_cb(gpointer data)
{
    gtk_main_quit();
    return FALSE;
}

void nonblock_sleep(guint msec)
{
    g_timeout_add(msec, (GSourceFunc) __nonblock_cb, NULL);
    gtk_main();
}

static void __expand_cb(GtkWidget * widget, gpointer data)
{
    if (GTK_IS_EXPANDER(widget)) {
	gtk_expander_set_expanded(GTK_EXPANDER(widget), TRUE);
    } else if (GTK_IS_CONTAINER(widget)) {
	gtk_container_foreach(GTK_CONTAINER(widget),
			      (GtkCallback) __expand_cb, NULL);
    }
}

void file_chooser_open_expander(GtkWidget * chooser)
{
    gtk_container_foreach(GTK_CONTAINER(chooser),
			  (GtkCallback) __expand_cb, NULL);
}

void file_chooser_add_filters(GtkWidget * chooser, FileTypes * filters)
{
    GtkFileFilter *filter;
    gint i;

    for (i = 0; filters[i].name; i++) {
	filter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(filter, filters[i].mime_type);
	gtk_file_filter_set_name(filter, filters[i].name);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
    }
}

gchar *file_chooser_get_extension(GtkWidget * chooser, FileTypes * filters)
{
    GtkFileFilter *filter;
    const gchar *filter_name;
    gint i;

    filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(chooser));
    filter_name = gtk_file_filter_get_name(filter);
    for (i = 0; filters[i].name; i++) {
	if (g_str_equal(filter_name, filters[i].name)) {
	    return filters[i].extension;
	}
    }

    return NULL;
}

gpointer file_types_get_data_by_name(FileTypes * filters, gchar * filename)
{
    gint i;

    for (i = 0; filters[i].name; i++) {
	if (g_str_has_suffix(filename, filters[i].extension)) {
	    return filters[i].data;
	}
    }

    return NULL;
}

gchar *file_chooser_build_filename(GtkWidget * chooser, gchar * extension)
{
    gchar *filename =
	gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    gchar *retval;

    if (g_str_has_suffix(filename, extension)) {
	return filename;
    }

    retval = g_strconcat(filename, extension, NULL);
    g_free(filename);

    return retval;
}

gboolean binreloc_init(gboolean try_hardcoded)
{
    GError *error = NULL;
    gchar *tmp;

    DEBUG("initializing binreloc (hardcoded = %d)", try_hardcoded);

    /* If the runtime data directories we previously found, don't even try
       to find them again. */
    if (params.path_data && params.path_lib) {
	DEBUG("data and lib path already found.");
	return TRUE;
    }

    if (try_hardcoded || !gbr_init(&error)) {
	/* We were asked to try hardcoded paths or BinReloc failed to initialize. */
	params.path_data = g_strdup(PREFIX);
	params.path_lib = g_strdup(LIBPREFIX);

	if (error) {
	    g_error_free(error);
	}

	DEBUG("%strying hardcoded paths.",
	      try_hardcoded ? "" : "binreloc init failed. ");
    } else {
	/* If we were able to initialize BinReloc, build the default data
	   and library paths. */
	DEBUG("done, trying to use binreloc paths.");

	tmp = gbr_find_data_dir(PREFIX);
	params.path_data = g_build_filename(tmp, "hardinfo2", NULL);
	g_free(tmp);

	tmp = gbr_find_lib_dir(PREFIX);
	params.path_lib = g_build_filename(tmp, "hardinfo2", NULL);
	g_free(tmp);
    }

    DEBUG("searching for runtime data on these locations:");
    DEBUG("  lib: %s", params.path_lib);
    DEBUG(" data: %s", params.path_data);

    /* Try to see if the benchmark test data file isn't missing. This isn't the
       definitive test, but it should do okay for most situations. */
    tmp = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_test(tmp, G_FILE_TEST_EXISTS)) {
	DEBUG("runtime data not found");

	g_free(params.path_data);
	g_free(params.path_lib);
	g_free(tmp);

	params.path_data = params.path_lib = NULL;

	if (try_hardcoded) {
	    /* We tried the hardcoded paths, but still was unable to find the
	       runtime data. Give up. */
	    DEBUG("giving up");
	    return FALSE;
	} else {
	    /* Even though BinReloc worked OK, the runtime data was not found.
	       Try the hardcoded paths. */
	    DEBUG("trying to find elsewhere");
	    return binreloc_init(TRUE);
	}
    }
    g_free(tmp);

    DEBUG("runtime data found!");
    /* We found the runtime data; hope everything is fine */
    return TRUE;
}

static void
log_handler(const gchar * log_domain,
	    GLogLevelFlags log_level,
	    const gchar * message, gpointer user_data)
{
    if (!params.gui_running) {
	/* No GUI running: spit the message to the terminal */
	g_print("\n\n*** %s: %s\n\n",
		(log_level & G_LOG_FLAG_FATAL) ? _("Error") : _("Warning"),
		message);
    } else {
	/* Hooray! We have a GUI running! */
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_MODAL,
						    (log_level &
						     G_LOG_FLAG_FATAL) ?
						    GTK_MESSAGE_ERROR :
						    GTK_MESSAGE_WARNING,
						    GTK_BUTTONS_CLOSE,
						    "<big><b>%s</b></big>\n\n%s",
						    (log_level &
						     G_LOG_FLAG_FATAL) ?
						    _("Fatal Error") :
						    _("Warning"), message);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    }
}

void parameters_init(int *argc, char ***argv, ProgramParameters * param)
{
    static gint create_report = FALSE;
    static gint force_all_details = FALSE;
    static gint show_version = FALSE;
    static gint skip_benchmarks = FALSE;
    static gint quiet = FALSE;
    static gchar *report_format = NULL;
    static gchar *run_benchmark = NULL;
    static gchar *result_format = NULL;
    static gchar *bench_user_note = NULL;
    static gint max_bench_results = 50;

    static GOptionEntry options[] = {
	{
	 .long_name = "quiet",
	 .short_name = 'q',
	 .arg = G_OPTION_ARG_NONE,
	 .arg_data = &quiet,
	 .description = N_("do not print status messages to standard output")},
	{
	 .long_name = "generate-report",
	 .short_name = 'r',
	 .arg = G_OPTION_ARG_NONE,
	 .arg_data = &create_report,
	 .description = N_("creates a report and prints to standard output")},
	{
	 .long_name = "report-format",
	 .short_name = 'f',
	 .arg = G_OPTION_ARG_STRING,
	 .arg_data = &report_format,
	 .description = N_("chooses a report format ([text], html)")},
	{
	 .long_name = "run-benchmark",
	 .short_name = 'b',
	 .arg = G_OPTION_ARG_STRING,
	 .arg_data = &run_benchmark,
	 .description = N_("run benchmark eg. -b 'FPU FFT'")},
	{
	 .long_name = "user-note",
	 .short_name = 'u',
	 .arg = G_OPTION_ARG_STRING,
	 .arg_data = &bench_user_note,
	 .description = N_("note attached to benchmark results")},
	{
	 .long_name = "result-format",
	 .short_name = 'g',
	 .arg = G_OPTION_ARG_STRING,
	 .arg_data = &result_format,
	 .description = N_("benchmark result format ([short], conf, shell)")},
	{
	 .long_name = "max-results",
	 .short_name = 'n',
	 .arg = G_OPTION_ARG_INT,
	 .arg_data = &max_bench_results,
	 .description = N_("maximum number of benchmark results to include (-1 for no limit, default is 50)")},
	{
	 .long_name = "version",
	 .short_name = 'v',
	 .arg = G_OPTION_ARG_NONE,
	 .arg_data = &show_version,
	 .description = N_("shows program version and quit")},
	{
	 .long_name = "skip-benchmarks",
	 .short_name = 's',
	 .arg = G_OPTION_ARG_NONE,
	 .arg_data = &skip_benchmarks,
	 .description = N_("do not run benchmarks")},
	{
	 .long_name = "very-verbose",
	 .short_name = 'w', /* like -vv */
	 .arg = G_OPTION_ARG_NONE,
	 .arg_data = &force_all_details,
	 .description = N_("show all details")},
	{NULL}
    };
    GOptionContext *ctx;

    ctx = g_option_context_new(_("- System Information and Benchmark"));
    g_option_context_set_ignore_unknown_options(ctx, FALSE);
    g_option_context_set_help_enabled(ctx, TRUE);

    g_option_context_add_main_entries(ctx, options, *(argv)[0]);
    g_option_context_parse(ctx, argc, argv, NULL);

    g_option_context_free(ctx);

    if (*argc >= 2) {
	g_print(_("Unrecognized arguments.\n"
		"Try ``%s --help'' for more information.\n"), *(argv)[0]);
	exit(1);
    }

    param->create_report = create_report;
    param->report_format = REPORT_FORMAT_TEXT;
    param->show_version = show_version;
    param->run_benchmark = run_benchmark;
    param->result_format = result_format;
    param->max_bench_results = max_bench_results;
    param->skip_benchmarks = skip_benchmarks;
    param->force_all_details = force_all_details;
    param->quiet = quiet;
    param->argv0 = *(argv)[0];

    if (report_format) {
        if (g_str_equal(report_format, "html"))
            param->report_format = REPORT_FORMAT_HTML;
        if (g_str_equal(report_format, "shell"))
            param->report_format = REPORT_FORMAT_SHELL;
    }

    /* clean user note */
    if (bench_user_note) {
        char *p = NULL;
        while(p = strchr(bench_user_note, ';'))  { *p = ','; }
        param->bench_user_note =
            gg_key_file_parse_string_as_value(bench_user_note, '|');
    }

    /* html ok?
     * gui: yes
     * report html: yes
     * report text: no
     * anything else? */
    param->markup_ok = TRUE;
    if (param->create_report && param->report_format != REPORT_FORMAT_HTML)
        param->markup_ok = FALSE;

    // TODO: fmt_opts: FMT_OPT_ATERM, FMT_OPT_HTML, FMT_OPT_PANGO...
    param->fmt_opts = FMT_OPT_NONE;

    gchar *confdir = g_build_filename(g_get_user_config_dir(), "hardinfo2", NULL);
    if (!g_file_test(confdir, G_FILE_TEST_EXISTS)) {
        mkdir(confdir, 0744);
    }
    g_free(confdir);
}

gint ui_init(int *argc, char ***argv)
{
    DEBUG("initializing gtk+ UI");

    g_set_application_name("HardInfo2");
    g_log_set_handler(NULL,
		      G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL |
		      G_LOG_LEVEL_ERROR, log_handler, NULL);

    return gtk_init_check(argc, argv);
}

/* Copyright: Jens Låås, SLU 2002 */
gchar *strreplacechr(gchar * string, gchar * replace, gchar new_char)
{
    gchar *s;
    for (s = string; *s; s++)
	if (strchr(replace, *s))
	    *s = new_char;

    return string;
}

gchar *strreplace(gchar *string, gchar *replace, gchar *replacement)
{
    gchar **tmp, *ret;

    tmp = g_strsplit(string, replace, 0);
    ret = g_strjoinv(replacement, tmp);
    g_strfreev(tmp);

    return ret;
}

static GHashTable *__module_methods = NULL;

static void module_register_methods(ShellModule * module)
{
    const ShellModuleMethod *(*get_methods)(void);
    gchar *method_name;

    if (__module_methods == NULL) {
	__module_methods = g_hash_table_new(g_str_hash, g_str_equal);
    }

    if (g_module_symbol
	(module->dll, "hi_exported_methods", (gpointer)&get_methods)) {
	const ShellModuleMethod *methods;

	for (methods = get_methods(); methods->name; methods++) {
	    ShellModuleMethod method = *methods;
	    gchar *name = g_path_get_basename(g_module_name(module->dll));
	    gchar *simple_name = strreplace(name, "lib", "");

	    strend(simple_name, '.');

	    method_name = g_strdup_printf("%s::%s", simple_name, method.name);
	    g_hash_table_insert(__module_methods, method_name,
				method.function);
	    g_free(name);
	    g_free(simple_name);
	}
    }

}

gchar *module_call_method(gchar * method)
{
    gchar *(*function) (void);

    if (__module_methods == NULL) {
	return NULL;
    }

    function = g_hash_table_lookup(__module_methods, method);
    return function ? g_strdup(function()) : NULL;
}

/* FIXME: varargs? */
gchar *module_call_method_param(gchar * method, gchar * parameter)
{
    gchar *(*function) (gchar *param);

    if (__module_methods == NULL) {
	return NULL;
    }

    function = g_hash_table_lookup(__module_methods, method);
    return function ? g_strdup(function(parameter)) : NULL;
}

static gboolean remove_module_methods(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, data);
}

static void module_unload(ShellModule * module)
{
    GSList *entry;

    if (module->dll) {
        gchar *name;

        if (module->deinit) {
        	DEBUG("cleaning up module \"%s\"", module->name);
		module->deinit();
	} else {
		DEBUG("module \"%s\" does not need cleanup", module->name);
	}

        name = g_path_get_basename(g_module_name(module->dll));
        g_hash_table_foreach_remove(__module_methods, remove_module_methods, name);

    	g_module_close(module->dll);
    	g_free(name);
    }

    g_free(module->name);
    g_object_unref(module->icon);

    for (entry = module->entries; entry; entry = entry->next) {
	ShellModuleEntry *e = (ShellModuleEntry *)entry->data;

	g_source_remove_by_user_data(e);
    	g_free(e);
    }

    g_slist_free(module->entries);
    g_free(module);
}


void module_unload_all(void)
{
    Shell *shell;
    GSList *module, *merge_id;

    shell = shell_get_main_shell();

    sync_manager_clear_entries();
    shell_clear_timeouts(shell);
    shell_clear_tree_models(shell);
    shell_clear_field_updates();
    shell_set_title(shell, NULL);

    for (module = shell->tree->modules; module; module = module->next) {
    	module_unload((ShellModule *)module->data);
    }

    for (merge_id = shell->merge_ids; merge_id; merge_id = merge_id->next) {
    	gtk_ui_manager_remove_ui(shell->ui_manager,
			         GPOINTER_TO_INT(merge_id->data));
    }
    g_slist_free(shell->tree->modules);
    g_slist_free(shell->merge_ids);

    shell->merge_ids = NULL;
    shell->tree->modules = NULL;
    shell->selected = NULL;
}

static ShellModule *module_load(gchar * filename)
{
    ShellModule *module;
    gchar *tmp;

    module = g_new0(ShellModule, 1);

    if (params.gui_running) {
	gchar *tmpicon, *dot, *simple_name;

	tmpicon = g_strdup(filename);
	dot = g_strrstr(tmpicon, "." G_MODULE_SUFFIX);

	*dot = '\0';

	simple_name = strreplace(tmpicon, "lib", "");

	tmp = g_strdup_printf("%s.png", simple_name);
	module->icon = icon_cache_get_pixbuf(tmp);

	g_free(tmp);
	g_free(tmpicon);
	g_free(simple_name);
    }

    tmp = g_build_filename(params.path_lib, "modules", filename, NULL);
    module->dll = g_module_open(tmp, G_MODULE_BIND_LAZY);
    DEBUG("gmodule resource for ``%s'' is %p (%s)", tmp, module->dll, g_module_error());
    g_free(tmp);

    if (module->dll) {
	void (*init) (void);
	ModuleEntry *(*get_module_entries) (void);
	gint(*weight_func) (void);
	gchar *(*name_func) (void);
	ModuleEntry *entries;
	gint i = 0;

	if (!g_module_symbol(module->dll, "hi_module_get_entries", (gpointer) & get_module_entries) ||
	    !g_module_symbol(module->dll, "hi_module_get_name",	(gpointer) & name_func)) {
	    DEBUG("cannot find needed symbols; is ``%s'' a real module?", filename);
	    goto failed;
	}

	if (g_module_symbol(module->dll, "hi_module_init", (gpointer) & init)) {
	    DEBUG("initializing module ``%s''", filename);
	    init();
	}

	g_module_symbol(module->dll, "hi_module_get_weight",
			(gpointer) & weight_func);

	module->weight = weight_func ? weight_func() : 0;
	module->name = name_func();

        g_module_symbol(module->dll, "hi_module_get_about",
   	 	        (gpointer) & (module->aboutfunc));
        g_module_symbol(module->dll, "hi_module_deinit",
   	 	        (gpointer) & (module->deinit));
        g_module_symbol(module->dll, "hi_module_get_summary",
   	 	        (gpointer) & (module->summaryfunc));

	entries = get_module_entries();
	while (entries[i].name) {
        if (*entries[i].name == '#') { i++; continue; } /* skip */

	    ShellModuleEntry *entry = g_new0(ShellModuleEntry, 1);

	    if (params.gui_running) {
		entry->icon = icon_cache_get_pixbuf(entries[i].icon);
	    }
	    entry->icon_file = entries[i].icon;

	    g_module_symbol(module->dll, "hi_more_info",
			    (gpointer) & (entry->morefunc));
	    g_module_symbol(module->dll, "hi_get_field",
			    (gpointer) & (entry->fieldfunc));
	    g_module_symbol(module->dll, "hi_note_func",
			    (gpointer) & (entry->notefunc));

	    entry->name = _(entries[i].name); //gettext unname N_() in computer.c line 67 etc...
	    entry->scan_func = entries[i].scan_callback;
	    entry->func = entries[i].callback;
	    entry->number = i;
	    entry->flags = entries[i].flags;

	    module->entries = g_slist_append(module->entries, entry);

	    i++;
	}

	DEBUG("registering methods for module ``%s''", filename);
	module_register_methods(module);
    } else {
    	DEBUG("cannot g_module_open(``%s''). permission problem?", filename);
      failed:
	DEBUG("loading module %s failed: %s", filename, g_module_error());

	g_free(module->name);
	g_free(module);
	module = NULL;
    }

    return module;
}

static gboolean module_in_module_list(gchar * module, gchar ** module_list)
{
    int i = 0;

    if (!module_list)
	return TRUE;

    for (; module_list[i]; i++) {
	if (g_str_equal(module_list[i], module))
	    return TRUE;
    }

    return FALSE;
}

static gint module_cmp(gconstpointer m1, gconstpointer m2)
{
    ShellModule *a = (ShellModule *) m1;
    ShellModule *b = (ShellModule *) m2;

    return a->weight - b->weight;
}

static GSList *modules_check_deps(GSList * modules)
{
    GSList *mm;
    ShellModule *module;

    for (mm = modules; mm; mm = mm->next) {
	gchar **(*get_deps) (void);
	gchar **deps;
	gint i;

	module = (ShellModule *) mm->data;

	if (g_module_symbol(module->dll, "hi_module_get_dependencies",
			    (gpointer) & get_deps)) {
	    for (i = 0, deps = get_deps(); deps[i]; i++) {
		GSList *l;
		ShellModule *m;
		gboolean found = FALSE;

		for (l = modules; l && !found; l = l->next) {
		    m = (ShellModule *) l->data;
		    gchar *name = g_path_get_basename(g_module_name(m->dll));

		    found = g_str_equal(name, deps[i]);
		    g_free(name);
		}

		if (!found) {
		  if (0){//params.autoload_deps) {
			ShellModule *mod = module_load(deps[i]);

			if (mod)
			    modules = g_slist_append(modules, mod);
			modules = modules_check_deps(modules);	/* re-check dependencies */

			break;
		    }

		    if (params.gui_running) {
			GtkWidget *dialog;

			dialog = gtk_message_dialog_new(NULL,
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_QUESTION,
							GTK_BUTTONS_NONE,
							_("Module \"%s\" depends on module \"%s\", load it?"),
							module->name,
							deps[i]);
			gtk_dialog_add_buttons(GTK_DIALOG(dialog),
					       "_No",
					       GTK_RESPONSE_REJECT,
					       "_Open",
					       GTK_RESPONSE_ACCEPT, NULL);

			if (gtk_dialog_run(GTK_DIALOG(dialog)) ==
			    GTK_RESPONSE_ACCEPT) {
			    ShellModule *mod = module_load(deps[i]);

			    if (mod)
				modules = g_slist_prepend(modules, mod);
			    modules = modules_check_deps(modules);	/* re-check dependencies */
			} else {
			    g_error("HardInfo2 cannot run without loading the additional module.");
			    exit(1);
			}

			gtk_widget_destroy(dialog);
		    } else {
			g_error(_("Module \"%s\" depends on module \"%s\"."),
				module->name, deps[i]);
		    }
		}
	    }
	}
    }

    return modules;
}

GSList *modules_load_all(void)
{
    GSList *modules = NULL;
    ShellModule *module;
    gchar *filenames[]={"devices.so","computer.so","benchmark.so","network.so"};
    int i=0;

    while(i<4) {
       if (module = module_load(filenames[i])) {
           modules = g_slist_prepend(modules, module);
       }
       i++;
    }

    //modules = modules_check_deps(modules);
    modules = g_slist_sort(modules, module_cmp);
    return modules;
}

gint tree_view_get_visible_height(GtkTreeView * tv)
{
    GtkTreePath *path;
    GdkRectangle rect;
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tv);
    gint nrows = 1;

    path = gtk_tree_path_new_first();
    gtk_tree_view_get_cell_area(GTK_TREE_VIEW(tv), path, NULL, &rect);

    /* FIXME: isn't there any easier way to tell the number of rows? */
    gtk_tree_model_get_iter_first(model, &iter);
    do {
	nrows++;
    } while (gtk_tree_model_iter_next(model, &iter));

    gtk_tree_path_free(path);

    return nrows * rect.height;
}

void module_entry_scan_all_except(ModuleEntry * entries, gint except_entry)
{
    ModuleEntry entry;
    gint i = 0;
    void (*scan_callback) (gboolean reload);
    gchar *text;

    shell_view_set_enabled(FALSE);

    for (entry = entries[0]; entry.name; entry = entries[++i]) {
	if (i == except_entry)
	    continue;

	text = g_strdup_printf(_("Scanning: %s..."), _(entry.name));
	shell_status_update(text);
	g_free(text);

	if ((scan_callback = entry.scan_callback)) {
	    scan_callback(FALSE);
	}
    }

    shell_view_set_enabled(TRUE);
    shell_status_update(_("Done."));
}

void module_entry_scan_all(ModuleEntry * entries)
{
    module_entry_scan_all_except(entries, -1);
}

void module_entry_reload(ShellModuleEntry * module_entry)
{

    if (module_entry->scan_func) {
	module_entry->scan_func(TRUE);
    }
}

void module_entry_scan(ShellModuleEntry * module_entry)
{
    if (module_entry->scan_func) {
	module_entry->scan_func(FALSE);
    }
}

gchar *module_entry_get_field(ShellModuleEntry * module_entry, gchar * field)
{
   if (module_entry->fieldfunc) {
   	return module_entry->fieldfunc(field);
   }

   return NULL;
}

gchar *module_entry_function(ShellModuleEntry * module_entry)
{
    if (module_entry->func) {
	return module_entry->func();
    }

    return NULL;
}

gchar *module_entry_get_moreinfo(ShellModuleEntry * module_entry, gchar * field)
{
    if (module_entry->morefunc) {
	return module_entry->morefunc(field);
    }

    return NULL;
}

const gchar *module_entry_get_note(ShellModuleEntry * module_entry)
{
    if (module_entry->notefunc) {
	return module_entry->notefunc(module_entry->number);
    }

    return NULL;
}

gchar *h_strdup_cprintf(const gchar * format, gchar * source, ...)
{
    gchar *buffer, *retn;
    va_list args;

    va_start(args, source);
    buffer = g_strdup_vprintf(format, args);
    va_end(args);

    if (source) {
	retn = g_strconcat(source, buffer, NULL);
	g_free(buffer);
        g_free(source);
    } else {
	retn = buffer;
    }

    return retn;
}

gchar *h_strconcat(gchar * string1, ...)
{
    gsize l;
    va_list args;
    gchar *s;
    gchar *concat;
    gchar *ptr;

    if (!string1)
	return NULL;

    l = 1 + strlen(string1);
    va_start(args, string1);
    s = va_arg(args, gchar *);
    while (s) {
	l += strlen(s);
	s = va_arg(args, gchar *);
    }
    va_end(args);

    concat = g_new(gchar, l);
    ptr = concat;

    ptr = g_stpcpy(ptr, string1);
    va_start(args, string1);
    s = va_arg(args, gchar *);
    while (s) {
	ptr = g_stpcpy(ptr, s);
	s = va_arg(args, gchar *);
    }
    va_end(args);

    g_free(string1);

    return concat;
}

static gboolean h_hash_table_remove_all_true(gpointer key, gpointer data, gpointer user_data)
{
    return TRUE;
}

void
h_hash_table_remove_all(GHashTable *hash_table)
{
    g_hash_table_foreach_remove(hash_table,
				h_hash_table_remove_all_true,
				NULL);
}

gfloat
h_sysfs_read_float(const gchar *endpoint, const gchar *entry)
{
	gchar *tmp, *buffer;
	gfloat return_value = 0.0f;

	tmp = g_build_filename(endpoint, entry, NULL);
	if (g_file_get_contents(tmp, &buffer, NULL, NULL))
		return_value = atof(buffer);

	g_free(tmp);
	g_free(buffer);

	return return_value;
}

gint
h_sysfs_read_int(const gchar *endpoint, const gchar *entry)
{
	gchar *tmp, *buffer;
	gint return_value = 0;

	tmp = g_build_filename(endpoint, entry, NULL);
	if (g_file_get_contents(tmp, &buffer, NULL, NULL))
		return_value = atoi(buffer);

	g_free(tmp);
	g_free(buffer);

	return return_value;
}

gint
h_sysfs_read_hex(const gchar *endpoint, const gchar *entry)
{
	gchar *tmp, *buffer;
	gint return_value = 0;

	tmp = g_build_filename(endpoint, entry, NULL);
	if (g_file_get_contents(tmp, &buffer, NULL, NULL))
		return_value = (gint) strtoll(buffer, NULL, 16);

	g_free(tmp);
	g_free(buffer);

	return return_value;
}

gchar *
h_sysfs_read_string(const gchar *endpoint, const gchar *entry)
{
	gchar *tmp, *return_value;

	tmp = g_build_filename(endpoint, entry, NULL);
	if (!g_file_get_contents(tmp, &return_value, NULL, NULL)) {
		g_free(return_value);

		return_value = NULL;
	} else {
		return_value = g_strstrip(return_value);
	}

	g_free(tmp);

	return return_value;
}

static GHashTable *_moreinfo = NULL;

void
moreinfo_init(void)
{
	if (G_UNLIKELY(_moreinfo)) {
		DEBUG("moreinfo already initialized");
		return;
	}
	DEBUG("initializing moreinfo");
	_moreinfo = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void
moreinfo_shutdown(void)
{
	if (G_UNLIKELY(!_moreinfo)) {
		DEBUG("moreinfo not initialized");
		return;
	}
	DEBUG("shutting down moreinfo");
	g_hash_table_destroy(_moreinfo);
	_moreinfo = NULL;
}

void
moreinfo_add_with_prefix(gchar *prefix, gchar *key, gchar *value)
{
	if (G_UNLIKELY(!_moreinfo)) {
		DEBUG("moreinfo not initialized");
		return;
	}

	if (prefix) {
		gchar *hashkey = g_strconcat(prefix, ":", key, NULL);
		g_hash_table_insert(_moreinfo, hashkey, value);
		return;
	}

	g_hash_table_insert(_moreinfo, g_strdup(key), value);
}

void
moreinfo_add(gchar *key, gchar *value)
{
	moreinfo_add_with_prefix(NULL, key, value);
}

static gboolean
_moreinfo_del_cb(gpointer key, gpointer value, gpointer data)
{
	return g_str_has_prefix(key, data);
}

void
moreinfo_del_with_prefix(gchar *prefix)
{
	if (G_UNLIKELY(!_moreinfo)) {
		DEBUG("moreinfo not initialized");
		return;
	}

	g_hash_table_foreach_remove(_moreinfo, _moreinfo_del_cb, prefix);
}

void
moreinfo_clear(void)
{
	if (G_UNLIKELY(!_moreinfo)) {
		DEBUG("moreinfo not initialized");
		return;
	}
	h_hash_table_remove_all(_moreinfo);
}

gchar *
moreinfo_lookup_with_prefix(gchar *prefix, gchar *key)
{
	if (G_UNLIKELY(!_moreinfo)) {
		DEBUG("moreinfo not initialized");
		return 0;
	}

	if (prefix) {
		gchar *lookup_key = g_strconcat(prefix, ":", key, NULL);
		gchar *result = g_hash_table_lookup(_moreinfo, lookup_key);
		g_free(lookup_key);
		return result;
	}

	return g_hash_table_lookup(_moreinfo, key);
}

gchar *
moreinfo_lookup(gchar *key)
{
	return moreinfo_lookup_with_prefix(NULL, key);
}

#if !GLIB_CHECK_VERSION(2,44,0)
gboolean g_strv_contains(const gchar * const * strv, const gchar *str) {
    /* g_strv_contains() requires glib>2.44
     * fallback for older versions */
    gint i = 0;
    while(strv[i] != NULL) {
        if (g_strcmp0(strv[i], str) == 0)
            return 1;
        i++;
    }
    return 0;
}
#endif

gchar *hardinfo_clean_grpname(const gchar *v, int replacing) {
    gchar *clean, *p;

    p = clean = g_strdup(v);
    while (*p != 0) {
        switch(*p) {
            case '[':
                *p = '('; break;
            case ']':
                *p = ')'; break;
            default:
                break;
        }
        p++;
    }
    if (replacing)
        g_free((gpointer)v);
    return clean;
}

/* Hardinfo labels that have # are truncated and/or hidden.
 * Labels can't have $ because that is the delimiter in
 * moreinfo. */
gchar *hardinfo_clean_label(const gchar *v, int replacing) {
    gchar *clean, *p;

    p = clean = g_strdup(v);
    while (*p != 0) {
        switch(*p) {
            case '#': case '$':
                *p = '_';
                break;
            default:
                break;
        }
        p++;
    }
    if (replacing)
        g_free((gpointer)v);
    return clean;
}

/* hardinfo uses the values as {ht,x}ml, apparently */
gchar *hardinfo_clean_value(const gchar *v, int replacing) {
    gchar *clean, *tmp;
    gchar **vl;
    if (v == NULL) return NULL;

    vl = g_strsplit(v, "&", -1);
    if (g_strv_length(vl) > 1)
        clean = g_strjoinv("&amp;", vl);
    else
        clean = g_strdup(v);
    g_strfreev(vl);

    vl = g_strsplit(clean, "<", -1);
    if (g_strv_length(vl) > 1) {
        tmp = g_strjoinv("&lt;", vl);
        g_free(clean);
        clean = tmp;
    }
    g_strfreev(vl);

    vl = g_strsplit(clean, ">", -1);
    if (g_strv_length(vl) > 1) {
        tmp = g_strjoinv("&gt;", vl);
        g_free(clean);
        clean = tmp;
    }
    g_strfreev(vl);

    if (replacing)
        g_free((gpointer)v);
    return clean;
}

gboolean hardinfo_spawn_command_line_sync(const gchar *command_line,
                                          gchar **standard_output,
                                          gchar **standard_error,
                                          gint *exit_status,
                                          GError **error)
{
    shell_status_pulse();
    return g_spawn_command_line_sync(command_line, standard_output,
                                     standard_error, exit_status, error);
}
