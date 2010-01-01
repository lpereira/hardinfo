/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    This file is based off xmlrpc-server-test.c from libsoup test suite
 *    Copyright (C) 2008 Red Hat, Inc.
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

#include "config.h"

#include <glib.h>
#ifdef HAS_LIBSOUP
#include <stdio.h>
#include <string.h>
#include <libsoup/soup.h>

#include "shell.h"
#include "hardinfo.h"

#define XMLRPC_SERVER_VERSION		1
/* server namespace */
static void method_get_api_version(SoupMessage * msg,
				   GValueArray * params);
static void method_shutdown_server(SoupMessage * msg,
				   GValueArray * params);
/* module namespace */
static void method_get_module_list(SoupMessage * msg,
				   GValueArray * params);
static void method_get_entry_list(SoupMessage * msg, GValueArray * params);
static void method_entry_reload(SoupMessage * msg, GValueArray * params);
static void method_entry_scan(SoupMessage * msg, GValueArray * params);
static void method_entry_get_field(SoupMessage * msg,
				   GValueArray * params);
static void method_entry_get_moreinfo(SoupMessage * msg,
				      GValueArray * params);
static void method_entry_get_note(SoupMessage * msg, GValueArray * params);
static void method_entry_function(SoupMessage * msg, GValueArray * params);
static void method_get_about_info(SoupMessage * msg, GValueArray * params);
static void method_call_method(SoupMessage * msg, GValueArray * params);
static void method_call_method_param(SoupMessage * msg,
				     GValueArray * params);

/* method handler table */
static const struct {
  gchar	*method_name;
  void	*callback;
} handler_table[] = {
  /* server namespace */
  { "server.getAPIVersion", 		method_get_api_version },
  { "server.shutdownServer",		method_shutdown_server },
  /* module namespace */
  { "module.getModuleList",		method_get_module_list },
  { "module.getEntryList",		method_get_entry_list },
  { "module.entryReload",		method_entry_reload },
  { "module.entryScan",			method_entry_scan },
  { "module.entryFunction",		method_entry_function },
  { "module.entryGetNote",		method_entry_get_note },
  { "module.entryGetField",		method_entry_get_field },
  { "module.entryGetMoreInfo",		method_entry_get_moreinfo },
  { "module.getAboutInfo",		method_get_about_info },
  { "module.callMethod",		method_call_method },
  { "module.callMethodParam",		method_call_method_param },
  { NULL }
};

static GHashTable *handlers = NULL;
static GMainLoop *loop = NULL;

typedef struct _MethodParameter MethodParameter;
struct _MethodParameter {
  int   param_type;
  void *variable;
};

static void
args_error(SoupMessage * msg, GValueArray * params, int expected)
{
    soup_xmlrpc_set_fault(msg,
			  SOUP_XMLRPC_FAULT_SERVER_ERROR_INVALID_METHOD_PARAMETERS,
			  "Wrong number of parameters: expected %d, got %d",
			  expected, params->n_values);
}

static void
type_error(SoupMessage * msg, GType expected, GValueArray * params,
	   int bad_value)
{
    soup_xmlrpc_set_fault(msg,
			  SOUP_XMLRPC_FAULT_SERVER_ERROR_INVALID_METHOD_PARAMETERS,
			  "Bad parameter #%d: expected %s, got %s",
			  bad_value + 1, g_type_name(expected),
			  g_type_name(G_VALUE_TYPE
				      (&params->values[bad_value])));
}

static gboolean validate_parameters(SoupMessage *msg, GValueArray *params,
                                    MethodParameter *method_params, gint n_params)
{
    int i;
    
    if (params->n_values != n_params) {
        args_error(msg, params, n_params);
        return FALSE;
    }
    
    for (i = 0; i < n_params; i++) {
        if (!soup_value_array_get_nth(params, i,
                                      method_params[i].param_type,
                                      method_params[i].variable)) {
            int j;
            
            type_error(msg, method_params[i].param_type, params, i);
            
            for (j = 0; j < i; j++) {
                if (method_params[j].param_type == G_TYPE_STRING) {
                    g_free(method_params[j].variable);
                }
            }
            
            return FALSE;
        }
    }
    
    return TRUE;
}

static void method_get_module_list(SoupMessage * msg, GValueArray * params)
{
    GValueArray *out;
    GSList *modules;

    out = soup_value_array_new();

    for (modules = modules_get_list(); modules; modules = modules->next) {
	ShellModule *module = (ShellModule *) modules->data;
	gchar *icon_file, *tmp;
	GValueArray *tuple;
	
        tuple = soup_value_array_new();

        tmp = g_path_get_basename(g_module_name(module->dll));
        if ((icon_file = g_strrstr(tmp, G_MODULE_SUFFIX))) {
            *icon_file = '\0';
            icon_file = g_strconcat(tmp, "png", NULL);
        } else {
            icon_file = "";
        }

        soup_value_array_append(tuple, G_TYPE_STRING, module->name);
        soup_value_array_append(tuple, G_TYPE_STRING, icon_file);
        
        soup_value_array_append(out, G_TYPE_VALUE_ARRAY, tuple);

        g_value_array_free(tuple);
        g_free(tmp);
    }

    soup_xmlrpc_set_response(msg, G_TYPE_VALUE_ARRAY, out);
    g_value_array_free(out);
}

static void method_get_entry_list(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    ShellModuleEntry *module_entry;
    GSList *entry, *modules;
    GValueArray *out;
    gboolean found = FALSE;
    gchar *module_name;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name }
    };

    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }

    for (modules = modules_get_list(); modules; modules = modules->next) {
	ShellModule *module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }

    out = soup_value_array_new();

    if (found) {
        module = (ShellModule *) modules->data;
        for (entry = module->entries; entry; entry = entry->next) {
            GValueArray *tuple;

            module_entry = (ShellModuleEntry *) entry->data;
              
            if (!(module_entry->flags & MODULE_FLAG_NO_REMOTE)) {
                tuple = soup_value_array_new();

                soup_value_array_append(tuple, G_TYPE_STRING, module_entry->name);
                soup_value_array_append(tuple, G_TYPE_STRING, module_entry->icon_file);
                  
                soup_value_array_append(out, G_TYPE_VALUE_ARRAY, tuple);
                g_value_array_free(tuple);
            }
        }
    }

    soup_xmlrpc_set_response(msg, G_TYPE_VALUE_ARRAY, out);
    g_value_array_free(out);
}

static void method_entry_get_field(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *module_name, *field_name, *answer = NULL;
    gint entry_number;
    gboolean found = FALSE;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name },
        { G_TYPE_INT, &entry_number },
        { G_TYPE_STRING, &field_name }
    };

    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }

    for (modules = modules_get_list(); modules; modules = modules->next) {
	module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }
    
    if (found) {
        if (entry_number < g_slist_length(module->entries)) {
          GSList *entry_node = g_slist_nth(module->entries, entry_number);
          ShellModuleEntry *entry = (ShellModuleEntry *)entry_node->data;

          answer = module_entry_get_field(entry, field_name);
        }
    }
    
    if (!answer) {
        answer = g_strdup("");
    }

    soup_xmlrpc_set_response(msg, G_TYPE_STRING, answer);
    g_free(answer);
}

static void method_entry_get_moreinfo(SoupMessage * msg,
				      GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *module_name, *field_name, *answer = NULL;
    gint entry_number;
    gboolean found = FALSE;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name },
        { G_TYPE_INT, &entry_number },
        { G_TYPE_STRING, &field_name },
    };

    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }

    for (modules = modules_get_list(); modules; modules = modules->next) {
	module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }
    
    if (found) {
        if (entry_number < g_slist_length(module->entries)) {
          GSList *entry_node = g_slist_nth(module->entries, entry_number);
          ShellModuleEntry *entry = (ShellModuleEntry *)entry_node->data;

          answer = module_entry_get_moreinfo(entry, field_name);
        }
    }
    
    if (!answer) {
        answer = g_strdup("");
    }

    soup_xmlrpc_set_response(msg, G_TYPE_STRING, answer);
    g_free(answer);
}

static void method_entry_reload(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *module_name, *field_name;
    gint entry_number;
    gboolean found = FALSE, answer = FALSE;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name },
        { G_TYPE_INT, &entry_number },
    };

    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }

    for (modules = modules_get_list(); modules; modules = modules->next) {
	module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }
    
    if (found) {
        if (entry_number < g_slist_length(module->entries)) {
          GSList *entry_node = g_slist_nth(module->entries, entry_number);
          ShellModuleEntry *entry = (ShellModuleEntry *)entry_node->data;

          module_entry_reload(entry);
          answer = TRUE;
        }
    }
    
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, answer);
}

static void method_entry_scan(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *module_name;
    gint entry_number;
    gboolean found = FALSE, answer = FALSE;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name },
        { G_TYPE_INT, &entry_number },
    };

    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }

    for (modules = modules_get_list(); modules; modules = modules->next) {
	module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }
    
    if (found) {
        if (entry_number < g_slist_length(module->entries)) {
          GSList *entry_node = g_slist_nth(module->entries, entry_number);
          ShellModuleEntry *entry = (ShellModuleEntry *)entry_node->data;

          module_entry_scan(entry);
          answer = TRUE;
        }
    }
    
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, answer);
}

static void method_entry_function(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *module_name, *answer = NULL;
    gboolean found = FALSE;
    gint entry_number;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name },
        { G_TYPE_INT, &entry_number },
    };

    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }
    
    for (modules = modules_get_list(); modules; modules = modules->next) {
	module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }
    
    if (found) {
        if (entry_number < g_slist_length(module->entries)) {
          GSList *entry_node = g_slist_nth(module->entries, entry_number);
          ShellModuleEntry *entry = (ShellModuleEntry *)entry_node->data;

          module_entry_scan(entry);
          answer = module_entry_function(entry);
        }
    }
    
    if (!answer) {
        answer = "";
    }

    soup_xmlrpc_set_response(msg, G_TYPE_STRING, answer);
}


static void method_entry_get_note(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *module_name, *answer = NULL;
    gint entry_number;
    gboolean found = FALSE;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name },
        { G_TYPE_INT, &entry_number },
    };
    
    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }
    
    for (modules = modules_get_list(); modules; modules = modules->next) {
	module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }
    
    if (found) {
        if (entry_number < g_slist_length(module->entries)) {
          GSList *entry_node = g_slist_nth(module->entries, entry_number);
          ShellModuleEntry *entry = (ShellModuleEntry *)entry_node->data;

          answer = g_strdup((gchar *)module_entry_get_note(entry));
        }
    }
    
    if (!answer) {
        answer = g_strdup("");
    }

    soup_xmlrpc_set_response(msg, G_TYPE_STRING, answer);
    g_free(answer);
}

static void method_get_about_info(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *module_name;
    gint entry_number;
    gboolean found = FALSE;
    GValueArray *out;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &module_name },
    };
    
    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }
    
    for (modules = modules_get_list(); modules; modules = modules->next) {
	module = (ShellModule *) modules->data;

	if (!strncmp(module->name, module_name, strlen(module->name))) {
	    found = TRUE;
	    break;
	}
    }
    
    out = soup_value_array_new();

    if (found) {
        ModuleAbout *about = module_get_about(module);

        soup_value_array_append(out, G_TYPE_STRING, about->description);
        soup_value_array_append(out, G_TYPE_STRING, about->author);
        soup_value_array_append(out, G_TYPE_STRING, about->version);
        soup_value_array_append(out, G_TYPE_STRING, about->license);
    }

    soup_xmlrpc_set_response(msg, G_TYPE_VALUE_ARRAY, out);
    g_value_array_free(out);
}

static void method_call_method(SoupMessage * msg, GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *method_name, *answer = NULL;
    gint entry_number;
    gboolean found = FALSE;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &method_name },
    };
    
    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }
    
    if (!(answer = module_call_method(method_name))) {
        answer = g_strdup("");
    }

    soup_xmlrpc_set_response(msg, G_TYPE_STRING, answer);
    g_free(answer);
}

static void method_call_method_param(SoupMessage * msg,
				     GValueArray * params)
{
    ShellModule *module;
    GSList *modules;
    gchar *method_name, *parameter, *answer = NULL;
    gint entry_number;
    gboolean found = FALSE;
    MethodParameter method_params[] = {
        { G_TYPE_STRING, &method_name },
        { G_TYPE_STRING, &parameter },
    };
    
    if (!validate_parameters(msg, params, method_params, G_N_ELEMENTS(method_params))) {
        return;
    }
    
    if (!(answer = module_call_method_param(method_name, parameter))) {
        answer = g_strdup("");
    }

    soup_xmlrpc_set_response(msg, G_TYPE_STRING, answer);
    g_free(answer);
}

static void method_get_api_version(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_INT, XMLRPC_SERVER_VERSION);
}

static void method_shutdown_server(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, TRUE);

    g_main_loop_quit(loop);
}
#endif				/* HAS_LIBSOUP */

void xmlrpc_server_init(void)
{
#ifdef HAS_LIBSOUP
    if (!loop) {
        DEBUG("creating main loop");
	loop = g_main_loop_new(NULL, FALSE);
    } else {
        DEBUG("using main loop instance %p", loop);
    }

    if (!handlers) {
	int i;
	handlers = g_hash_table_new(g_str_hash, g_str_equal);

	DEBUG("registering handlers");

	for (i = 0; handler_table[i].method_name; i++) {
	    g_hash_table_insert(handlers,
				handler_table[i].method_name,
				handler_table[i].callback);
	}
    }
#endif				/* HAS_LIBSOUP */
}

#ifdef HAS_LIBSOUP
static SoupServer *xmlrpc_server_new(void)
{
    SoupServer *server;

    DEBUG("creating server");
    server = soup_server_new(SOUP_SERVER_SSL_CERT_FILE, NULL,
			     SOUP_SERVER_SSL_KEY_FILE, NULL,
			     SOUP_SERVER_ASYNC_CONTEXT, NULL,
			     SOUP_SERVER_PORT, 4242, NULL);
    if (!server) {
	return NULL;
    }

    soup_server_run_async(server);

    return server;
}

static void xmlrpc_server_callback(SoupServer * server,
				   SoupMessage * msg,
				   const char *path,
				   GHashTable * query,
				   SoupClientContext * context,
				   gpointer data)
{
    if (msg->method == SOUP_METHOD_POST) {
	gchar *method_name;
	GValueArray *params;
	void (*callback) (SoupMessage * msg, GValueArray * params);

        DEBUG("POST %s", path);

	if (!soup_xmlrpc_parse_method_call(msg->request_body->data,
					   msg->request_body->length,
					   &method_name, &params)) {
	    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
	    return;
	}

	DEBUG("method: %s", method_name);

	if ((callback = g_hash_table_lookup(handlers, method_name))) {
	    soup_message_set_status(msg, SOUP_STATUS_OK);

	    DEBUG("found callback: %p", callback);
	    callback(msg, params);
	} else {
	    DEBUG("callback not found");
	    soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
	}

	g_value_array_free(params);
	g_free(method_name);
    } else {
	DEBUG("received request of unknown method");
	soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
    }
}

static void icon_server_callback(SoupServer * server,
		 	         SoupMessage * msg,
				 const char *path,
				 GHashTable * query,
				 SoupClientContext * context,
				 gpointer data)
{
    if (msg->method == SOUP_METHOD_GET) {
        path = g_strrstr(path, "/");

        DEBUG("GET %s", path);

        if (!path || !g_str_has_suffix(path, ".png")) {
            DEBUG("not an icon, invalid path, etc");
            soup_message_set_status(msg, SOUP_STATUS_FORBIDDEN);
            soup_message_set_response(msg,
                                      "text/plain",
                                      SOUP_MEMORY_STATIC,
                                      "500 :(", 6);
        } else {
            gchar  *file, *icon;
            gint   size;
            
            file = g_build_filename(params.path_data,
                                    "pixmaps",
                                    path + 1,
                                    NULL);
            
            if (g_file_get_contents(file, &icon, &size, NULL)) {
                DEBUG("icon found");
                soup_message_set_status(msg, SOUP_STATUS_OK);
                soup_message_set_response(msg,
                                          "image/png",
                                          SOUP_MEMORY_TAKE,
                                          icon, size);
            } else {
                DEBUG("icon not found");
                soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
                soup_message_set_response(msg,
                                          "text/plain",
                                          SOUP_MEMORY_STATIC,
                                          "404 :(", 6);
            }

            g_free(file);
        }
    } else {
	DEBUG("received request of unknown method");
	soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
    }
}
#endif				/* HAS_LIBSOUP */

void xmlrpc_server_start(GMainLoop *main_loop)
{
#ifdef HAS_LIBSOUP
    SoupServer *server;

    if (main_loop) {
        loop = main_loop;
    }
    
    if (!loop || !handlers) {
        DEBUG("initializing server");
	xmlrpc_server_init();
    }

    server = xmlrpc_server_new();
    if (!server) {
        if (main_loop) {
            g_warning("Cannot create XML-RPC server.");
            return;
        } else {
            g_error("Cannot create XML-RPC server. Aborting");
        }
    }

    DEBUG("adding soup handlers for /xmlrpc");
    soup_server_add_handler(server, "/xmlrpc", xmlrpc_server_callback,
			    NULL, NULL);
    DEBUG("adding soup handlers for /icon/");
    soup_server_add_handler(server, "/icon/", icon_server_callback,
			    NULL, NULL);

    DEBUG("starting server");
    g_print("XML-RPC server ready\n");
    g_main_loop_run(loop);

    DEBUG("shutting down server");
    g_main_loop_unref(loop);
    soup_server_quit(server);
    g_object_unref(server);
#endif				/* HAS_LIBSOUP */
}

#ifdef XMLRPC_SERVER_TEST
int main(void)
{
#ifdef HAS_LIBSOUP
    g_type_init();

    xmlrpc_server_init();
    xmlrpc_server_start();
#endif				/* HAS_LIBSOUP */
}
#endif				/* XMLRPC_SERVER_TEST */
