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

gchar *
file_chooser_get_extension(GtkWidget * chooser, FileTypes * filters)
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
	
	DEBUG("%strying hardcoded paths.", try_hardcoded ? "" : "binreloc init failed. ");
    } else {
	/* If we were able to initialize BinReloc, build the default data
	   and library paths. */
        DEBUG("done, trying to use binreloc paths.");
	   
	tmp = gbr_find_data_dir(PREFIX);
	params.path_data = g_build_filename(tmp, "hardinfo", NULL);
	g_free(tmp);

	tmp = gbr_find_lib_dir(PREFIX);
	params.path_lib = g_build_filename(tmp, "hardinfo", NULL);
	g_free(tmp);
    }

    DEBUG("searching for runtime data on these locations:");
    DEBUG("  lib: %s", params.path_lib);
    DEBUG(" data: %s", params.path_data);

    /* Try to see if the uidefs.xml file isn't missing. This isn't the
       definitive test, but it should do okay for most situations. */
    tmp = g_build_filename(params.path_data, "uidefs.xml", NULL);
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
    static gboolean  autoload_deps = FALSE;
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
	 .description = "specify module to load"
        },
	{
	 .long_name   = "autoload-deps",
	 .short_name  = 'a',
	 .arg         = G_OPTION_ARG_NONE,
	 .arg_data    = &autoload_deps,
	 .description = "automatically load module dependencies"
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

    if (*argc >= 2) {
        g_print("Unrecognized arguments.\n"
                "Try ``%s --help'' for more information.\n",
                *(argv)[0]);
        exit(1);
    }

    param->create_report = create_report;
    param->report_format = REPORT_FORMAT_TEXT;
    param->show_version  = show_version;
    param->list_modules  = list_modules;
    param->use_modules   = use_modules;
    param->autoload_deps = autoload_deps;
    
    if (report_format && g_str_equal(report_format, "html"))
        param->report_format = REPORT_FORMAT_HTML;
        
    gchar *confdir = g_build_filename(g_get_home_dir(), ".hardinfo", NULL);
    if (!g_file_test(confdir, G_FILE_TEST_EXISTS)) {
        mkdir(confdir, 0744);
    }
    g_free(confdir);    
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
    const gchar *browsers[] = {
	  "xdg-open", "gnome-open", "kfmclient openURL",
	  "sensible-browser", "firefox", "epiphany",
	  "galeon", "mozilla", "opera", "konqueror",
	  "netscape", "links -g", NULL
    };
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

static GHashTable *__module_methods = NULL;

static void module_register_methods(ShellModule *module)
{
    ShellModuleMethod *(*get_methods) (void);
    gchar *method_name;

    if (__module_methods == NULL) {
        __module_methods = g_hash_table_new(g_str_hash, g_str_equal);
    }

    if (g_module_symbol(module->dll, "hi_exported_methods", (gpointer) &get_methods)) {
        ShellModuleMethod *methods = get_methods();
        
        while (TRUE) {
            ShellModuleMethod method = *methods;
            gchar *name = g_path_get_basename(g_module_name(module->dll));
            
            strend(name, '.');
            
            method_name = g_strdup_printf("%s::%s", name, method.name);
            g_hash_table_insert(__module_methods, method_name, method.function);
            g_free(name);
            
            if (!(*(++methods)).name)
                break;
        }
    }

}

gchar *module_call_method(gchar *method)
{
    gchar *(*function) (void);
    
    if (__module_methods == NULL) {
        return NULL;
    }
    
    function = g_hash_table_lookup(__module_methods, method);
    return function ? g_strdup(function()) :
                      g_strdup_printf("{Unknown method: \"%s\"}", method);
}

static ShellModule *module_load(gchar *filename)
{
    ShellModule *module;
    gchar *tmp;
    
    module = g_new0(ShellModule, 1);
    
    if (params.gui_running) {
        gchar *tmpicon;
        
        tmpicon = g_strdup(filename);
        gchar *dot = g_strrstr(tmpicon, "." G_MODULE_SUFFIX);
        
        *dot = '\0';
        
        tmp = g_strdup_printf("%s.png", tmpicon);
        module->icon = icon_cache_get_pixbuf(tmp);
        
        g_free(tmp);
        g_free(tmpicon);
    }

    tmp = g_build_filename(params.path_lib, "modules", filename, NULL);
    module->dll = g_module_open(tmp, G_MODULE_BIND_LAZY);
    g_free(tmp);
    
    if (module->dll) {
        void         (*init)               (void);
        ModuleEntry *(*get_module_entries) (void);
        gint         (*weight_func)        (void);
        gchar       *(*name_func)          (void);
        ModuleEntry *entries;
        gint i = 0;

        if (!g_module_symbol(module->dll, "hi_module_get_entries", (gpointer) & get_module_entries) ||
            !g_module_symbol(module->dll, "hi_module_get_name", (gpointer) & name_func)) {
            goto failed;
        }
            
        if (g_module_symbol(module->dll, "hi_module_init", (gpointer) & init)) {
            init();
        }

        g_module_symbol(module->dll, "hi_module_get_weight", (gpointer) & weight_func);

        module->weight = weight_func ? weight_func() : 0;
        module->name   = name_func();
        
        entries = get_module_entries();
        while (entries[i].name) {
            ShellModuleEntry *entry = g_new0(ShellModuleEntry, 1);

            if (params.gui_running) {
                entry->icon = icon_cache_get_pixbuf(entries[i].icon);
            }

            g_module_symbol(module->dll, "hi_more_info",
                            (gpointer) & (entry->morefunc));
            g_module_symbol(module->dll, "hi_get_field",
                            (gpointer) & (entry->fieldfunc));
            g_module_symbol(module->dll, "hi_note_func",
                            (gpointer) & (entry->notefunc));

            entry->name      = entries[i].name;
            entry->scan_func = entries[i].scan_callback;
            entry->func      = entries[i].callback;
            entry->number    = i;

            module->entries = g_slist_append(module->entries, entry);
            
            i++;
        }

        module_register_methods(module);
    } else {
      failed:
        DEBUG("loading module %s failed", filename);
        
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

static void module_entry_free(gpointer data, gpointer user_data)
{
    ShellModuleEntry *entry = (ShellModuleEntry *)data;

    if (entry) {
        /*g_free(entry->name);*/
        g_object_unref(entry->icon);

        g_free(entry);
    }
}

static void module_free(ShellModule *module)
{
    g_free(module->name);
    g_object_unref(module->icon);
    /*g_module_close(module->dll);*/

    DEBUG("module_free: module->entries, %p\n", module->entries);
    g_slist_foreach(module->entries, (GFunc)module_entry_free, NULL);
    g_slist_free(module->entries);

    g_free(module);
}

ModuleAbout *module_get_about(ShellModule *module)
{
    ModuleAbout *(*get_about)(void);
    
    if (g_module_symbol(module->dll, "hi_module_get_about", (gpointer) &get_about)) {
        return get_about();
    }
    
    return NULL;
}

static GSList *modules_check_deps(GSList *modules)
{
    GSList      *mm;
    ShellModule *module;
    
    for (mm = modules; mm; mm = mm->next) {
        gchar    **(*get_deps)(void);
        gchar    **deps;
        gint 	 i;

        module = (ShellModule *) mm->data;
        
        if (g_module_symbol(module->dll, "hi_module_get_dependencies",
                            (gpointer) & get_deps)) {
            for (i = 0, deps = get_deps(); deps[i]; i++) {
                GSList      *l;
                ShellModule *m;
                gboolean    found = FALSE;
                
                for (l = modules; l; l = l->next) {
                    m = (ShellModule *)l->data;
                    gchar *name = g_path_get_basename(g_module_name(m->dll));
                    
                    if (g_str_equal(name, deps[i]))  {
                        found = TRUE;
                        break;
                    }
                
                    g_free(name);
                }
                
                if (!found) {
                    if (params.autoload_deps) {
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
                                                        "Module \"%s\" depends on module \"%s\", load it?",
                                                        module->name, deps[i]);
                        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                                               GTK_STOCK_NO, GTK_RESPONSE_REJECT,
                                               GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                               NULL);
                                               
                        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
                            ShellModule *mod = module_load(deps[i]);
                            
                            if (mod)
                                modules = g_slist_append(modules, mod);
                            modules = modules_check_deps(modules);	/* re-check dependencies */
                        } else {
                            modules = g_slist_remove(modules, module);
                            module_free(module);
                        }

                        gtk_widget_destroy(dialog);
                    } else {
                        g_error("Module \"%s\" depends on module \"%s\".", module->name, deps[i]);
                    }
                }
            }
        }
    }
    
    return modules;
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
            if (g_strrstr(filename, "." G_MODULE_SUFFIX) &&
                module_in_module_list(filename, module_list) &&
                ((module = module_load(filename)))) {
                    modules = g_slist_append(modules, module);
            }
        }

        g_dir_close(dir);
    }

    modules = modules_check_deps(modules);

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

gint tree_view_get_visible_height(GtkTreeView *tv)
{
    GtkTreePath  *path;
    GdkRectangle  rect;
    GtkTreeIter   iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tv);
    gint          nrows = 1;

    path = gtk_tree_path_new_first();
    gtk_tree_view_get_cell_area(GTK_TREE_VIEW(tv),
                                path, NULL, &rect);

    /* FIXME: isn't there any easier way to tell the number of rows? */
    gtk_tree_model_get_iter_first(model, &iter);
    do {
        nrows++;
    } while (gtk_tree_model_iter_next(model, &iter));
    
    gtk_tree_path_free(path);
    
    return nrows * rect.height;
}

void tree_view_save_image(gchar *filename)
{
    /* this is ridiculously complicated :/ why in the hell gtk+ makes this kind of
       thing so difficult?  */
    
    /* FIXME: this does not work if the window (or part of it) isn't visible. does
              anyone know how to fix this? :/ */
    Shell		*shell = shell_get_main_shell();
    GtkWidget		*widget = shell->info->view;

    PangoLayout		*layout;
    PangoContext	*context;
    PangoRectangle	 rect;

    GdkPixmap		*pm;
    GdkPixbuf		*pb;
    GdkGC		*gc;
    GdkColor	 	 black = { 0, 0, 0, 0 };
    GdkColor		 white = { 0, 65535, 65535, 65535 };

    gint		 w, h, visible_height;
    gchar		*tmp;
    
    gboolean		 tv_enabled;

    /* present the window */
    gtk_window_present(GTK_WINDOW(shell->window));
    
    /* if the treeview is disabled, we need to enable it so we get the
       correct colors when saving. we make it insensitive later on if it
       was this way before entering this function */    
    tv_enabled = GTK_WIDGET_IS_SENSITIVE(widget);
    gtk_widget_set_sensitive(widget, TRUE);
    
    gtk_widget_queue_draw(widget);

    /* unselect things in the information treeview */
    gtk_range_set_value(GTK_RANGE
                        (GTK_SCROLLED_WINDOW(shell->info->scroll)->
                         vscrollbar), 0.0);
    gtk_tree_selection_unselect_all(
          gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)));
    while (gtk_events_pending())
        gtk_main_iteration();

    /* initialize stuff */
    gc = gdk_gc_new(widget->window);
    gdk_gc_set_background(gc, &black);
    gdk_gc_set_foreground(gc, &white);
    
    context = gtk_widget_get_pango_context (widget);
    layout = pango_layout_new(context);
    
    visible_height = tree_view_get_visible_height(GTK_TREE_VIEW(widget));
                                   
    /* draw the title */    
    tmp = g_strdup_printf("<b><big>%s</big></b>\n<small>%s</small>",
                          shell->selected->name,
                          shell->selected->notefunc(shell->selected->number));
    pango_layout_set_markup(layout, tmp, -1);
    pango_layout_set_width(layout, widget->allocation.width * PANGO_SCALE);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_get_pixel_extents(layout, NULL, &rect);
    
    w = widget->allocation.width;
    h = visible_height + rect.height;
    
    pm = gdk_pixmap_new(widget->window, w, rect.height, -1);
    gdk_draw_rectangle(GDK_DRAWABLE(pm), gc, TRUE, 0, 0, w, rect.height);
    gdk_draw_layout_with_colors (GDK_DRAWABLE(pm), gc, 0, 0, layout,
                                 &white, &black);
    
    /* copy the pixmap from the treeview and from the title */
    pb = gdk_pixbuf_get_from_drawable(NULL,
                                      widget->window,
				      NULL,
				      0, 0,
				      0, 0,
				      w, h);
    pb = gdk_pixbuf_get_from_drawable(pb,
                                      pm,
                                      NULL,
                                      0, 0,
                                      0, visible_height,
                                      w, rect.height);

    /* save the pixbuf to a png file */
    gdk_pixbuf_save(pb, filename, "png", NULL,
                    "compression", "9",
                    "tEXt::hardinfo::version", VERSION,
                    "tEXt::hardinfo::arch", ARCH,
                    NULL);
  
    /* unref */
    g_object_unref(pb);
    g_object_unref(layout);
    g_object_unref(pm);
    g_object_unref(gc);
    g_free(tmp);
    
    gtk_widget_set_sensitive(widget, tv_enabled);
}


static gboolean __idle_free_do(gpointer ptr)
{
    if (ptr) {
        g_free(ptr);
    }

    return FALSE;
}

gpointer idle_free(gpointer ptr)
{
    if (ptr) {
        g_timeout_add(10000, __idle_free_do, ptr);
    }
    
    return ptr;
}

void module_entry_scan_all_except(ModuleEntry *entries, gint except_entry)
{
    ModuleEntry entry;
    gint        i = 0;

    void        (*scan_callback)(gboolean reload);
    
    for (entry = entries[0]; entry.name; entry = entries[++i]) {
        if (i == except_entry)
            continue;
            
        shell_status_update(idle_free(g_strdup_printf("Scanning: %s...", entry.name)));
        
        if ((scan_callback = entry.scan_callback)) {
            scan_callback(FALSE);
        }
    }
    
    shell_status_update("Done.");
}

void module_entry_scan_all(ModuleEntry *entries)
{
    module_entry_scan_all_except(entries, -1);
}

void module_entry_reload(ShellModuleEntry *module_entry)
{
    if (module_entry->scan_func) {
        module_entry->scan_func(TRUE);
    }
}

void module_entry_scan(ShellModuleEntry *module_entry)
{
    if (module_entry->scan_func) {
        module_entry->scan_func(FALSE);
    }
}

gchar *module_entry_function(ShellModuleEntry *module_entry)
{
    if (module_entry->func) {
        return g_strdup(module_entry->func());
    }
    
    return g_strdup("[Error]\n"
                    "Invalid module=");
}

const gchar *module_entry_get_note(ShellModuleEntry *module_entry)
{
    return module_entry->notefunc(module_entry->number);
}
