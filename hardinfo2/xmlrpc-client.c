/*
 *    XMLRPC Client
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

#include <string.h>
#include "config.h"
#include "xmlrpc-client.h"

static GMainLoop	*loop = NULL;
static SoupSession	*session = NULL;
static gboolean		 lock = FALSE;

void xmlrpc_init(void)
{
    if (!loop) {
      loop = g_main_loop_new(FALSE, FALSE);
    }
    
    if (!session) {
      session = soup_session_async_new_with_options(SOUP_SESSION_TIMEOUT, 10, NULL);
    }
}    

static void xmlrpc_response_get_integer(SoupSession *s,
                                        SoupMessage *m,
                                        gpointer user_data)
{
    gint *response = user_data;
    
    *response = -1;
    
    if (SOUP_STATUS_IS_SUCCESSFUL(m->status_code)) {
        soup_xmlrpc_extract_method_response(m->response_body->data,
                                            m->response_body->length,
                                            NULL,
                                            G_TYPE_INT, response);
    }
    
    g_main_quit(loop);
    lock = FALSE;
}                                

gint xmlrpc_get_integer(gchar *addr,
                               gchar *method,
                               const gchar *param_types,
                               ...)
{
    gint integer;
    GValueArray *params;
    SoupMessage *msg;
    gchar *body;

    msg = soup_message_new("POST", addr);

    params = g_value_array_new(1);
    
    if (param_types && *param_types) {
        va_list ap;

        va_start(ap, param_types);
        while (*param_types) {
            switch (*param_types) {
            case '%':
              break;
            case 'i':
              soup_value_array_append(params, G_TYPE_INT, va_arg(ap, int));
              break;
            case 's':
            default:
              soup_value_array_append(params, G_TYPE_STRING, va_arg(ap, char *));
              break;
            }
            
            param_types++;
        }
        
        va_end(ap);
    }

    body = soup_xmlrpc_build_method_call(method, params->values, params->n_values);
    g_value_array_free(params);

    soup_message_set_request(msg, "text/xml",
			     SOUP_MEMORY_TAKE, body, strlen(body));
    
    while (lock)
      g_main_iteration(FALSE);

    lock = TRUE;
    soup_session_queue_message(session, msg, xmlrpc_response_get_integer, &integer);
    g_main_run(loop);
    
    return integer;
}

static void xmlrpc_response_get_string(SoupSession *s,
                                       SoupMessage *m,
                                       gpointer user_data)
{
    if (SOUP_STATUS_IS_SUCCESSFUL(m->status_code)) {
        soup_xmlrpc_extract_method_response(m->response_body->data,
                                            m->response_body->length,
                                            NULL,
                                            G_TYPE_STRING, user_data);
    }
    
    g_main_quit(loop);
    lock = FALSE;
}                                

gchar *xmlrpc_get_string(gchar *addr,
                                gchar *method,
                                const gchar *param_types,
                                ...)
{
    GValueArray *params;
    SoupMessage *msg;
    gchar *body, *string = NULL;

    msg = soup_message_new("POST", addr);

    params = g_value_array_new(1);
    
    if (param_types && *param_types) {
        va_list ap;

        va_start(ap, param_types);
        while (*param_types) {
            switch (*param_types) {
            case '%':
              break;
            case 'i':
              soup_value_array_append(params, G_TYPE_INT, va_arg(ap, int));
              break;
            case 's':
            default:
              soup_value_array_append(params, G_TYPE_STRING, va_arg(ap, char *));
              break;
            }
            
            param_types++;
        }
        
        va_end(ap);
    }

    body = soup_xmlrpc_build_method_call(method, params->values, params->n_values);
    g_value_array_free(params);

    soup_message_set_request(msg, "text/xml",
			     SOUP_MEMORY_TAKE, body, strlen(body));
    
    while (lock)
      g_main_iteration(FALSE);

    lock = TRUE;
    soup_session_queue_message(session, msg, xmlrpc_response_get_string, &string);
    g_main_run(loop);
    
    return string;
}

static void xmlrpc_response_get_array(SoupSession *s,
                                      SoupMessage *m,
                                      gpointer user_data)
{
    if (SOUP_STATUS_IS_SUCCESSFUL(m->status_code)) {
        soup_xmlrpc_extract_method_response(m->response_body->data,
                                            m->response_body->length,
                                            NULL,
                                            G_TYPE_VALUE_ARRAY, user_data);
    }
    
    g_main_quit(loop);
    lock = FALSE;
}                                

GValueArray *xmlrpc_get_array(gchar *addr,
                              gchar *method,
                              const gchar *param_types,
                              ...)
{
    GValueArray *params, *answer = NULL;
    SoupMessage *msg;
    gchar *body;

    msg = soup_message_new("POST", addr);

    params = g_value_array_new(1);
    
    if (param_types && *param_types) {
        va_list ap;

        va_start(ap, param_types);
        while (*param_types) {
            switch (*param_types) {
            case '%':
              break;
            case 'i':
              soup_value_array_append(params, G_TYPE_INT, va_arg(ap, int));
              break;
            case 's':
            default:
              soup_value_array_append(params, G_TYPE_STRING, va_arg(ap, char *));
              break;
            }
            
            param_types++;
        }
        
        va_end(ap);
    }

    body = soup_xmlrpc_build_method_call(method, params->values, params->n_values);
    g_value_array_free(params);

    soup_message_set_request(msg, "text/xml",
			     SOUP_MEMORY_TAKE, body, strlen(body));
    
    while (lock)
      g_main_iteration(FALSE);

    lock = TRUE;
    soup_session_queue_message(session, msg, xmlrpc_response_get_array, &answer);
    g_main_run(loop);
    
    return answer;
}
