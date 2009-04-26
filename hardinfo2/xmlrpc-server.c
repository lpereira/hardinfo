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
static void method_entry_scan_all(SoupMessage * msg, GValueArray * params);
static void method_entry_scan_all_except(SoupMessage * msg,
					 GValueArray * params);
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
  { "module.entryScanAll",		method_entry_scan_all },
  { "module.entryScannAllExcept",	method_entry_scan_all_except },
  { "module.getAboutInfo",		method_get_about_info },
  { "module.callMethod",		method_call_method },
  { "module.callMethodParam",		method_call_method_param },
  { NULL }
};

static GHashTable *handlers = NULL;
static GMainLoop *loop = NULL;

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

static void method_get_module_list(SoupMessage * msg, GValueArray * params)
{
    GValueArray *out;
    GSList *modules;

    out = soup_value_array_new();

    for (modules = modules_get_list(); modules; modules = modules->next) {
	ShellModule *module = (ShellModule *) modules->data;

	soup_value_array_append(out, G_TYPE_STRING, module->name);
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

    if (params->n_values != 1) {
	args_error(msg, params, 1);
	return;
    }

    if (!soup_value_array_get_nth(params, 0, G_TYPE_STRING, &module_name)) {
	type_error(msg, G_TYPE_STRING, params, 0);
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
          tuple = soup_value_array_new();

          soup_value_array_append(tuple, G_TYPE_STRING, module_entry->name);
          soup_value_array_append(tuple, G_TYPE_INT, module_entry->number);
          soup_value_array_append(out, G_TYPE_VALUE_ARRAY, tuple);

          g_value_array_free(tuple);
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
    
    if (params->n_values != 3) {
	args_error(msg, params, 2);
	return;
    }
    
    if (!soup_value_array_get_nth(params, 0, G_TYPE_STRING, &module_name)) {
        type_error(msg, G_TYPE_STRING, params, 0);
        return;
    }
    
    if (!soup_value_array_get_nth(params, 1, G_TYPE_INT, &entry_number)) {
        type_error(msg, G_TYPE_INT, params, 1);
        return;
    }
    
    if (!soup_value_array_get_nth(params, 2, G_TYPE_STRING, &field_name)) {
        type_error(msg, G_TYPE_STRING, params, 2);
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
        if (entry_number <= g_slist_length(module->entries)) {
          GSList *entry_node = g_slist_nth(module->entries, entry_number);
          ShellModuleEntry *entry = (ShellModuleEntry *)entry_node->data;

          answer = module_entry_get_field(entry, field_name);
        }
    }
    
    if (!answer) {
        answer = g_strdup("");
    }

    soup_xmlrpc_set_response(msg, G_TYPE_STRING, answer);
}

static void method_entry_get_moreinfo(SoupMessage * msg,
				      GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_entry_reload(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_entry_scan(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_entry_function(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_entry_get_note(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_entry_scan_all(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_entry_scan_all_except(SoupMessage * msg,
					 GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_get_about_info(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_call_method(SoupMessage * msg, GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
}

static void method_call_method_param(SoupMessage * msg,
				     GValueArray * params)
{
    soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, FALSE);
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
	loop = g_main_loop_new(NULL, FALSE);
    }

    if (!handlers) {
	int i;
	handlers = g_hash_table_new(g_str_hash, g_str_equal);

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

	DEBUG("received POST request");

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
#endif				/* HAS_LIBSOUP */

void xmlrpc_server_start(void)
{
#ifdef HAS_LIBSOUP
    SoupServer *server;

    if (!loop || !handlers) {
	xmlrpc_server_init();
    }

    server = xmlrpc_server_new();
    if (!server) {
	g_error("Cannot create XML-RPC server. Aborting");
    }

    soup_server_add_handler(server, "/xmlrpc", xmlrpc_server_callback,
			    NULL, NULL);

    DEBUG("starting server");
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
    g_thread_init(NULL);

    xmlrpc_server_init();
    xmlrpc_server_start();
#endif				/* HAS_LIBSOUP */
}
#endif				/* XMLRPC_SERVER_TEST */
