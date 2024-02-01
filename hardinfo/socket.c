/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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
	     goto cleanup;
	}

	s = g_new0(Socket, 1);
	s->sock = sock;

	return s;
    }

cleanup:    
    close(sock);

    return NULL;
}

/* From: http://www.erlenstar.demon.co.uk/unix/faq_3.html#SEC26 */
static inline int __sock_is_ready(Socket * s, int mode)
{
    int rc, fd = s->sock;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = tv.tv_usec = 0;

    if (mode == 0) {
	/* read */
	rc = select(fd + 1, &fds, NULL, NULL, &tv);
    } else {
	/* write */
	rc = select(fd + 1, NULL, &fds, NULL, &tv);
    }

    if (rc < 0)
	return -1;

    return FD_ISSET(fd, &fds) ? 1 : 0;
}

int sock_ready_to_read(Socket * s)
{
    return __sock_is_ready(s, 0);
}

int sock_ready_to_write(Socket * s)
{
    return __sock_is_ready(s, 1);
}

int sock_read(Socket * s, gchar * buffer, gint size)
{
    if (size > 2 && sock_ready_to_read(s)) {
	gint n;

	n = read(s->sock, buffer, size - 1);
	if (n > 0) {
            buffer[n] = '\0';
        } else {
            return 0;
        }

	return n;
    }

    return 0;
}

int sock_write(Socket * s, gchar * str)
{
    while (!sock_ready_to_write(s));

    return write(s->sock, str, strlen(str));
}

void sock_close(Socket * s)
{
    shutdown(s->sock, 2);
    close(s->sock);
    g_free(s);
}
