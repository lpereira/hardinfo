/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
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

static gchar *_env = NULL;
void scan_env_var(gboolean reload)
{
    SCAN_START();
    
    gchar **envlist;
    gint i;
    
    g_free(_env);
    
    _env = g_strdup("[Environment Variables]\n");
    for (i = 0, envlist = g_listenv(); envlist[i]; i++) {
      _env = h_strdup_cprintf("%s=%s\n", _env,
                              envlist[i], g_getenv(envlist[i]));
    }
    g_strfreev(envlist);
    
    SCAN_END();
}

gchar *callback_env_var(void)
{
    return _env;
}
