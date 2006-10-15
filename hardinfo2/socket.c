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

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>

#include "socket.h"

Socket *sock_connect(gchar * host, gint port)
{
    struct sockaddr_in server;
    Socket *s;
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock > 0) {
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(host);
	server.sin_port = htons(port);
	
	if (connect(sock, (struct sockaddr *) (void *) &server, sizeof(server)) < 0) {
	    return NULL;
	}

	s = g_new0(Socket, 1);
	s->sock = sock;
	
	return s;
    }

    return NULL;
}

int sock_write(Socket * s, gchar * str)
{
    return write(s->sock, str, strlen(str));
}

int sock_read(Socket * s, gchar * buffer, gint size)
{
    gint n;

    n = read(s->sock, buffer, size);
    buffer[n] = '\0';
    
    return n;
}

void sock_close(Socket * s)
{
    close(s->sock);
    g_free(s);
}
