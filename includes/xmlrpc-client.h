/*
 *    XMLRPC Client
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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

#ifndef __XMLRPC_CLIENT_H__
#define __XMLRPC_CLIENT_H__

#include "config.h"

#ifdef HAS_LIBSOUP
#include <libsoup/soup.h>

void xmlrpc_init(void);
gint xmlrpc_get_integer(gchar *addr,
                        gchar *method,
                        const gchar *param_types,
                        ...);
gchar *xmlrpc_get_string(gchar *addr,
                         gchar *method,
                         const gchar *param_types,
                         ...);
GValueArray *xmlrpc_get_array(gchar *addr,
                              gchar *method,
                              const gchar *param_types,
                              ...);
#endif /* HAS_LIBSOUP */

#endif	/* __XMLRPC_CLIENT_H__ */
