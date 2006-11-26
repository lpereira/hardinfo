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
#include <config.h>

#include <report.h>
#include <string.h>
#include <shell.h>
#include <iconcache.h>
#include <hardinfo.h>
#include <gtk/gtk.h>

#include <binreloc.h>

#define KiB 1024
#define MiB 1048576
#define GiB 1073741824

inline gchar *size_human_readable(gfloat size)
{
    if (size < KiB)
	return g_strdup_printf("%.1f B", size);
    if (size < MiB)
	return g_strdup_printf("%.1f KiB", size / KiB);
    if (size < GiB)
	return g_strdup_printf("%.1f MiB", size / MiB);

    return g_strdup_printf("%.1f GiB", size / GiB);
}

inline void strend(gchar * str, gchar chr)
{
    if (!str)
	return;

    char *p;
    if ((p = strchr(str, chr)))
	*p = 0;
}

inline void remove_quotes(gchar * str)
{
    if (!str)
	return;

    while (*str == '"')
	*(str++) = ' ';

    strend(str, '"');
}

inline void remove_linefeed(gchar * str)
{
    strend(str, '\n');
}

void widget_set_cursor(GtkWidget * widget, GdkCursorType cursor_type)
{
    GdkCursor *cursor;

    cursor = gdk_cursor_new(cursor_type);
    gdk_window_set_cursor(GDK_WINDOW(widget->window), cursor);
    gdk_cursor_unref(cursor);

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

gchar
    * file_chooser_get_extension(GtkWidget * chooser, FileTypes * filters)
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

gchar * file_chooser_build_filename(GtkWidget * chooser, gchar * extension)
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

    /* If the runtime data directories we previously found, don't even try
       to find them again. */
    if (params.path_data && params.path_lib) {
	return TRUE;
    }

    if (try_hardcoded || !gbr_init(&error)) {
	/* We were asked to try hardcoded paths or BinReloc failed to initialize. */
	params.path_data = g_strdup(PREFIX);
	params.path_lib = g_strdup(LIBPREFIX);

	if (error) {
	    g_error_free(error);
	}
    } else {
	/* If we were able to initialize BinReloc, build the default data
	   and library paths. */
	tmp = gbr_find_data_dir(PREFIX);
	params.path_data = g_build_filename(tmp, "hardinfo", NULL);
	g_free(tmp);

	tmp = gbr_find_lib_dir(PREFIX);
	params.path_lib = g_build_filename(tmp, "hardinfo", NULL);
	g_free(tmp);
    }

    /* Try to see if the uidefs.xml file isn't missing. This isn't the
       definitive test, but it should do okay for most situations. */
    tmp = g_build_filename(params.path_data, "uidefs.xml", NULL);
    if (!g_file_test(tmp, G_FILE_TEST_EXISTS)) {
	g_free(params.path_data);
	g_free(params.path_lib);
	g_free(tmp);

	params.path_data = params.path_lib = NULL;

	if (try_hardcoded) {
	    /* We tried the hardcoded paths, but still was unable to find the
	       runtime data. Give up. */
	    return FALSE;
	} else {
	    /* Even though BinReloc worked OK, the runtime data was not found.
	       Try the hardcoded paths. */
	    return binreloc_init(TRUE);
	}
    }
    g_free(tmp);

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
		(log_level & G_LOG_FLAG_FATAL) ? "Error" : "Warning",
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
						    "Fatal Error" :
						    "Warning", message);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    }
}

void parameters_init(int *argc, char ***argv, ProgramParameters * param)
{
    static gboolean  create_report = FALSE;
    static gboolean  show_version  = FALSE;
    static gboolean  list_modules  = FALSE;
    static gchar    *report_format = NULL;
    static gchar   **use_modules   = NULL;

    static GOptionEntry options[] = {
	{
	 .long_name   = "generate-report",
	 .short_name  = 'r',
	 .arg         = G_OPTION_ARG_NONE,
	 .arg_data    = &create_report,
	 .description = "creates a report and prints to standard output"
        },
	{
	 .long_name   = "report-format",
	 .short_name  = 'f',
	 .arg         = G_OPTION_ARG_STRING,
	 .arg_data    = &report_format,
	 .description = "chooses a report format (text, html)"
        },
	{
	 .long_name   = "list-modules",
	 .short_name  = 'l',
	 .arg         = G_OPTION_ARG_NONE,
	 .arg_data    = &list_modules,
	 .description = "lists modules"
        },
	{
	 .long_name   = "load-module",
	 .short_name  = 'm',
	 .arg         = G_OPTION_ARG_STRING_ARRAY,
	 .arg_data    = &use_modules,
	 .description = "specify module file name to load; may be used more than once"
        },
	{
	 .long_name   = "version",
	 .short_name  = 'v',
	 .arg         = G_OPTION_ARG_NONE,
	 .arg_data    = &show_version,
	 .description = "shows program version and quit"
        },
	{ NULL }	
    };
    GOptionContext *ctx;

    ctx = g_option_context_new("- System Profiler and Benchmark tool");
    g_option_context_set_ignore_unknown_options(ctx, FALSE);
    g_option_context_set_help_enabled(ctx, TRUE);
 
    g_option_context_add_main_entries(ctx, options, *(argv)[0]);
    g_option_context_parse(ctx, argc, argv, NULL);
 
    g_option_context_free(ctx);

    param->create_report = create_report;
    param->report_format = REPORT_FORMAT_TEXT;
    param->show_version  = show_version;
    param->list_modules  = list_modules;
    param->use_modules   = use_modules;
    
    if (report_format && g_str_equal(report_format, "html"))
        param->report_format = REPORT_FORMAT_HTML;
}

gboolean ui_init(int *argc, char ***argv)
{
    g_set_application_name("HardInfo");
    g_log_set_handler(NULL,
		      G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL |
		      G_LOG_LEVEL_ERROR, log_handler, NULL);

    return gtk_init_check(argc, argv);
}

void open_url(gchar * url)
{
    const gchar *browsers[] =
	{ "xdg-open", "gnome-open", "kfmclient openURL", "sensible-browser",
	"firefox", "epiphany", "galeon", "mozilla", "opera", "konqueror",
	"netscape", "links -g", NULL };
    gint i;

    for (i = 0; browsers[i]; i++) {
	gchar *cmdline = g_strdup_printf("%s '%s'", browsers[i], url);

	if (g_spawn_command_line_async(cmdline, NULL)) {
	    g_free(cmdline);
	    return;
	}

	g_free(cmdline);
    }

    g_warning("Couldn't find a Web browser to open URL %s.", url);
}

/* Copyright: Jens Låås, SLU 2002 */
gchar *strreplace(gchar * string, gchar * replace, gchar new_char)
{
    gchar *s;
    for (s = string; *s; s++)
	if (strchr(replace, *s))
	    *s = new_char;

    return string;
}

static ShellModule *module_load(gchar *filename) {
    ShellModule *module;
    gchar *tmp;
    
    module = g_new0(ShellModule, 1);
    
    if (params.gui_running) {
        gchar *dot = g_strrstr(filename, G_MODULE_SUFFIX) - 1;
        
        *dot = '\0';
        
        tmp = g_strdup_printf("%s.png", filename);
        module->icon = icon_cache_get_pixbuf(tmp);
        g_free(tmp);
        
        *dot = '.';
    }

    tmp = g_build_filename(params.path_lib, "modules", filename, NULL);
    module->dll = g_module_open(tmp, G_MODULE_BIND_LAZY);
    g_free(tmp);
    
    if (module->dll) {
        gint   (*n_entries)   (void);
        gint   (*weight_func) (void);
        gchar *(*name_func)   (void);
        gint i;

        if (!g_module_symbol(module->dll, "hi_n_entries", (gpointer) & n_entries) ||
            !g_module_symbol(module->dll, "hi_module_name", (gpointer) & name_func))
            goto failed;
            
        g_module_symbol(module->dll, "hi_module_weight", (gpointer) & weight_func);

        module->weight = weight_func ? weight_func() : 0;
        module->name   = name_func();

        gint j = n_entries();
        for (i = 0; i <= j; i++) {
            GdkPixbuf *(*shell_icon) (gint);
            const gchar *(*shell_name) (gint);
            ShellModuleEntry *entry = g_new0(ShellModuleEntry, 1);

            if (params.gui_running
                && g_module_symbol(module->dll, "hi_icon",
                                   (gpointer) & (shell_icon))) {
                entry->icon = shell_icon(i);
            }
            if (g_module_symbol
                (module->dll, "hi_name", (gpointer) & (shell_name))) {
                entry->name = g_strdup(shell_name(i));
            }
            g_module_symbol(module->dll, "hi_info",
                            (gpointer) & (entry->func));
            g_module_symbol(module->dll, "hi_reload",
                            (gpointer) & (entry->reloadfunc));
            g_module_symbol(module->dll, "hi_more_info",
                            (gpointer) & (entry->morefunc));
            g_module_symbol(module->dll, "hi_get_field",
                            (gpointer) & (entry->fieldfunc));

            entry->number = i;
            
            module->entries = g_slist_append(module->entries, entry);
        }
    } else {
      failed:
        g_free(module->name);
        g_free(module);
        module = NULL;
    }
    
    return module;
}

static gboolean module_in_module_list(gchar *module, gchar **module_list)
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
    ShellModule *a = (ShellModule *)m1;
    ShellModule *b = (ShellModule *)m2;
    
    return a->weight - b->weight;
}

static GSList *modules_load(gchar **module_list)
{
    GDir *dir;
    GSList *modules = NULL;
    ShellModule *module;
    gchar *filename;
    
    filename = g_build_filename(params.path_lib, "modules", NULL);
    dir = g_dir_open(filename, 0, NULL);
    g_free(filename);
    
    if (dir) {
        while ((filename = (gchar*)g_dir_read_name(dir))) {
            if (module_in_module_list(filename, module_list) &&
                ((module = module_load(filename)))) {
                    modules = g_slist_append(modules, module);
            }
        }

        g_dir_close(dir);
    }

    if (g_slist_length(modules) == 0) {
        if (params.use_modules == NULL) {
    	    g_error("No module could be loaded. Check permissions on \"%s\" and try again.",
	            params.path_lib);
        } else {
            g_error("No module could be loaded. Please use hardinfo -l to list all avai"
                    "lable modules and try again with a valid module list.");
                    
        }
    }
    
    return g_slist_sort(modules, module_cmp);
}

GSList *modules_load_selected(void)
{
    return modules_load(params.use_modules);
}

GSList *modules_load_all(void)
{
    return modules_load(NULL);
}
