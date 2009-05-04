/* 
   Remote Client
   HardInfo - Displays System Information
   Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>

   Based on ssh-method.c from GnomeVFS
   Copyright (C) 1999 Free Software Foundation
   Original author: Ian McKellar <yakk@yakk.net>

   ssh-conn.c and ssh-conn.h are free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   ssh-conn.c and ssh-con.h are distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with ssh-conn.c and ssh-conn.h; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "config.h"
#ifdef HAS_LIBSOUP
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/termios.h>

#include "ssh-conn.h"

static const char *errors[] = {
    "OK",
    "No URI",
    "Unknown protocol",
    "Unknown error",
    "Cannot spawn SSH",
    "Bad parameters",
    "Permission denied",
    "Host key verification failed",
    "Connection refused",
    "Invalid username or password"
};

const char *ssh_strerror(SSHConnResponse errno)
{
    return errors[errno];
}

int ssh_write(SSHConn * conn,
	      gconstpointer buffer, gint num_bytes, gint * bytes_written)
{
    int written;
    int count = 0;

    do {
	written = write(conn->fd_write, buffer, (size_t) num_bytes);
	if (written == -1 && errno == EINTR) {
	    count++;
	    usleep(10);
	}
    } while (written == -1 && errno == EINTR && count < 5);

    if (written == -1) {
	return -1;
    }

    *bytes_written = written;

    return 1;
}

int ssh_read(gint fd, gpointer buffer, gint num_bytes, gint * bytes_read)
{
    int retval;
    fd_set fds;
    struct timeval tv = { 5, 0 };

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    *bytes_read = 0;

    if ((retval = select(fd + 1, &fds, NULL, NULL, &tv)) < 0) {
	return retval;
    }

    if ((retval = read(fd, buffer, (size_t) num_bytes)) > 0) {
	*bytes_read = retval;
	return 1;
    }

    return retval;
}

void ssh_close(SSHConn * conn)
{
    if (!conn) {
	return;
    }

    close(conn->fd_read);
    close(conn->fd_write);
    close(conn->fd_error);

    soup_uri_free(conn->uri);
    kill(conn->pid, SIGINT);
    
    if (conn->askpass_path) {
        DEBUG("unlinking %s", conn->askpass_path);
        g_remove(conn->askpass_path);
        g_free(conn->askpass_path);
    }

    g_free(conn);
}

static void ssh_conn_setup(gpointer user_data)
{
    gchar *askpass_path = (gchar *) user_data;
    int fd;
    
    if ((fd = open("/dev/tty", O_RDWR)) != -1) {
        ioctl(fd, TIOCNOTTY, NULL);
        close(fd);
    }

    if (askpass_path) {
	g_setenv("DISPLAY", "none:0.", TRUE);
	g_setenv("SSH_ASKPASS", askpass_path, TRUE);
    } else {
	g_setenv("SSH_ASKPASS", "/bin/false", TRUE);
    }
}

SSHConnResponse ssh_new(SoupURI * uri,
			SSHConn ** conn_return, gchar * command)
{
    int argc, res, bytes_read;
    char **argv, *askpass_path = NULL;
    GString *cmd_line;
    SSHConnResponse response;
    SSHConn *connection;
    gchar buffer[512];

    if (!conn_return) {
	return SSH_CONN_BAD_PARAMS;
    }

    if (!uri) {
	return SSH_CONN_NO_URI;
    }

    if (!g_str_equal(uri->scheme, "ssh")) {
	return SSH_CONN_UNKNOWN_PROTOCOL;
    }

    if (uri->password) {
	int tmp_askpass;

	askpass_path =
	    g_build_filename(g_get_home_dir(), ".hardinfo",
			     "ssh-askpass-XXXXXX", NULL);
	tmp_askpass = g_mkstemp(askpass_path);
	if (tmp_askpass > 0) {
	    gchar *tmp;

	    g_chmod(askpass_path, 0700);

	    tmp = g_strdup_printf("#!/bin/sh\n"
				  "echo '%s'\n", uri->password);
	    write(tmp_askpass, tmp, strlen(tmp));
	    close(tmp_askpass);
	    g_free(tmp);

	    DEBUG("using [%s] as ssh-askpass", askpass_path);
	}
    }

    cmd_line = g_string_new("ssh -x");

    if (uri->user) {
	g_string_append_printf(cmd_line, " -l '%s'", uri->user);
    }

    if (uri->port) {
	g_string_append_printf(cmd_line, " -p %d", uri->port);
    }

    g_string_append_printf(cmd_line,
			   " %s \"LC_ALL=C %s\"", uri->host, command);

    DEBUG("cmd_line = [%s]", cmd_line->str);

    if (!g_shell_parse_argv(cmd_line->str, &argc, &argv, NULL)) {
	response = SSH_CONN_BAD_PARAMS;
	goto end;
    }

    connection = g_new0(SSHConn, 1);

    DEBUG("spawning SSH");

    if (!g_spawn_async_with_pipes(NULL, argv, NULL,
				  G_SPAWN_SEARCH_PATH,
				  ssh_conn_setup, askpass_path,
				  (gint *) & connection->pid,
				  &connection->fd_write,
				  &connection->fd_read,
				  &connection->fd_error, NULL)) {
	response = SSH_CONN_CANNOT_SPAWN_SSH;
	goto end;
    }

    memset(buffer, 0, sizeof(buffer));
    res = ssh_read(connection->fd_error, &buffer, sizeof(buffer),
		   &bytes_read);
    DEBUG("bytes read: %d, result = %d", bytes_read, res);
    if (bytes_read != 0 && res > 0) {
	DEBUG("Received (error channel): [%s]", buffer);

	if (g_str_equal(buffer, "Permission denied")) {
	    response = SSH_CONN_PERMISSION_DENIED;
	} else if (g_str_equal(buffer, "Host key verification failed")) {
	    response = SSH_CONN_HOST_KEY_CHECK_FAIL;
	    goto end;
	} else if (g_str_equal(buffer, "Connection refused")) {
	    response = SSH_CONN_REFUSED;
	    goto end;
	} else if (g_str_has_prefix(buffer, "Host key fingerprint is")) {
	    /* ok */
	} else {
	    response = SSH_CONN_UNKNOWN_ERROR;
	    goto end;
	}
    }

    connection->uri = soup_uri_copy(uri);
    response = SSH_CONN_OK;

  end:
    g_strfreev(argv);
    g_string_free(cmd_line, TRUE);

    if (askpass_path) {
        if (connection) {
            connection->askpass_path = askpass_path;
        } else {
    	    g_free(askpass_path);
        }
    }

    if (response != SSH_CONN_OK) {
	g_free(connection);
	*conn_return = NULL;
    } else {
	*conn_return = connection;
    }

    DEBUG("response = %d (%s)",
	  response, response == SSH_CONN_OK ? "OK" : "Error");

    return response;
}

#ifdef SSH_TEST
int main(int argc, char **argv)
{
    SSHConn *c;
    SSHConnResponse r;
    SoupURI *u;
    char buffer[256];
    
    if (argc < 2) {
        g_print("Usage: %s URI command\n", argv[0]);
        g_print("Example: %s ssh://user:password@host 'ls -la /'\n", argv[0]);
        return 1;
    }

    u = soup_uri_new(argv[1]);
    r = ssh_new(u, &c, argv[2]);
    g_print("Connection result: %s\n", errors[r]);

    if (r == SSH_CONN_OK) {
	int bytes_read;

	while (ssh_read(c->fd_read, &buffer, sizeof(buffer),
			&bytes_read) > 0) {
	    g_print("Bytes read: %d\n", bytes_read);
	    g_print("Contents: %s", buffer);
	}

	g_print("Finished running remote command\n");
    }

    g_print("Closing SSH [ptr = %p]", c);
    ssh_close(c);

    return 0;
}
#endif				/* SSH_TEST */
#endif				/* HAS_LIBSOUP */
