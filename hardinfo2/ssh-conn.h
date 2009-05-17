/* 
   Remote Client
   HardInfo - Displays System Information
   Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>

   Based on ssh-method.c from GnomeVFS
   Copyright (C) 1999 Free Software Foundation
   Original author: Ian McKellar <yakk@yakk.net>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __SSH_CONN_H__
#define __SSH_CONN_H__

#include <libsoup/soup.h>

typedef struct _SSHConn SSHConn;

typedef enum {
    SSH_CONN_OK,
    SSH_CONN_NO_URI,
    SSH_CONN_UNKNOWN_PROTOCOL,
    SSH_CONN_UNKNOWN_ERROR,
    SSH_CONN_CANNOT_SPAWN_SSH,
    SSH_CONN_BAD_PARAMS,
    SSH_CONN_PERMISSION_DENIED,
    SSH_CONN_HOST_KEY_CHECK_FAIL,
    SSH_CONN_REFUSED,
    SSH_CONN_INVALID_USER_PASS,
} SSHConnResponse;

struct _SSHConn {
    SoupURI *uri;
    int fd_read, fd_write, fd_error;
    GPid pid;
    gchar *askpass_path;
    
    gint exit_status;
};

SSHConnResponse ssh_new(SoupURI * uri,
			SSHConn ** conn_return, gchar * command);
void ssh_close(SSHConn * conn);

int ssh_write(SSHConn * conn,
	      gconstpointer buffer, gint num_bytes, gint * bytes_written);
int ssh_read(gint fd, gpointer buffer, gint num_bytes, gint * bytes_read);

const char *ssh_conn_errors[10];

#endif				/* __SSH_CONN_H__ */
