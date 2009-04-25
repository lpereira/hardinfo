#include <stdio.h>
#include <libsoup/soup.h>

#include "config.h"

#define XMLRPC_SERVER_VERSION		1

static void method_get_api_version(SoupMessage *msg, GValueArray *params);
static void method_shutdown_server(SoupMessage *msg, GValueArray *params);

static const struct {
  gchar	*method_name;
  void	*callback;
} handler_table[] = {
  { "getAPIVersion", 	method_get_api_version },
  { "shutdownServer",	method_shutdown_server },
  { NULL }
};
  
static GHashTable *handlers = NULL;
static GMainLoop *loop = NULL;

static void method_get_api_version(SoupMessage *msg, GValueArray *params)
{
  soup_xmlrpc_set_response(msg, G_TYPE_INT, 1);
}

static void method_shutdown_server(SoupMessage *msg, GValueArray *params)
{
  soup_xmlrpc_set_response(msg, G_TYPE_BOOLEAN, TRUE);
  
  g_main_loop_quit(loop);
}

void xmlrpc_server_init(void)
{
  if (!loop) {
    loop = g_main_loop_new(NULL, FALSE);
  }
  
  if (!handlers) {
    int i;
    handlers = g_hash_table_new(g_str_hash, g_str_equal);
    
    for (i = 0; handler_table[i].method_name; i++) {
      g_hash_table_insert(handlers,
                          handler_table[i].method_name, handler_table[i].callback);
    }
  }
}

static SoupServer *xmlrpc_server_new(void)
{
  SoupServer *server;
  
  DEBUG("creating server");
  server = soup_server_new(SOUP_SERVER_SSL_CERT_FILE, NULL,
                           SOUP_SERVER_SSL_KEY_FILE, NULL,
                           SOUP_SERVER_ASYNC_CONTEXT, NULL,
                           NULL);
  if (!server) {
    return NULL;
  }
  
  soup_server_run_async(server);
  
  return server;
}

static void xmlrpc_server_callback(SoupServer *server,
                                   SoupMessage *msg,
                                   const char *path,
                                   GHashTable *query,
                                   SoupClientContext *context,
                                   gpointer data)
{
  if (msg->method == SOUP_METHOD_POST) {
    gchar *method_name;
    GValueArray *params;
    void (*callback)(SoupMessage *msg, GValueArray *params);
    
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
      soup_message_set_status(msg, SOUP_STATUS_NOT_ACCEPTABLE);
    }
    
    g_free(method_name);
    g_value_array_free(params);
  } else {
    DEBUG("received request of unknown method");
    soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
  }
}

void xmlrpc_server_start(void)
{
  SoupServer *server;
  
  if (!loop || !handlers) {
    xmlrpc_server_init();
  }
  
  server = xmlrpc_server_new();
  if (!server) {
    g_error("Cannot create XML-RPC server. Aborting");
  }
  
  soup_server_add_handler(server, "/xmlrpc", xmlrpc_server_callback, NULL, NULL);
  
  g_print("XML-RPC server listening on port: %d\n", soup_server_get_port(server));
  
  DEBUG("starting server");
  g_main_loop_run(loop);
  DEBUG("shutting down server");
  g_main_loop_unref(loop);
}

#ifdef XMLRPC_SERVER_TEST
int main(void)
{
  g_type_init();
  g_thread_init(NULL);
  
  xmlrpc_server_init();
  xmlrpc_server_start();
}
#endif /* XMLRPC_SERVER_TEST */ 
