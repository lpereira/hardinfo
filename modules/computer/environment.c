/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 L. A. F. Pereira <l@tia.mat.br>
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

#include "hardinfo.h"
#include "computer.h"

static gchar *_env = NULL;
void scan_env_var(gboolean reload)
{
    SCAN_START();

    gchar **envlist;
    gchar *st;
    gint i;

    g_free(_env);

    _env = g_strdup_printf("[%s]\n", _("Environment Variables") );
    for (i = 0, envlist = g_listenv(); envlist[i]; i++) {
        st=strwrap(g_getenv(envlist[i]),80,':');
        _env = h_strdup_cprintf("%s=%s\n", _env, envlist[i], st);
        g_free(st);
    }
    g_strfreev(envlist);

    SCAN_END();
}

gchar *callback_env_var(void)
{
    return g_strdup(_env);
}
