/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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

#ifndef __HI_SOCKET_H__
#define __HI_SOCKET_H__

typedef struct _Socket	Socket;

struct _Socket {
  gint   sock;
};

Socket *sock_connect(gchar * host, gint port);
int	sock_write(Socket * s, gchar * str);
int	sock_read(Socket * s, gchar * buffer, gint size);
void	sock_close(Socket * s);

int	sock_ready_to_read(Socket *s);
int	sock_ready_to_write(Socket *s);

#endif	/* __HI_SOCKET_H__ */
