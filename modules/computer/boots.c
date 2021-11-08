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

#include <stdio.h>
#include <string.h>
#include "hardinfo.h"
#include "computer.h"

void
scan_boots_real(void)
{
    gchar **tmp;
    gboolean spawned;
    gchar *out, *err, *p, *s, *next_nl;

    scan_os(FALSE);

    if (!computer->os->boots)
      computer->os->boots = strdup("");
    else
      return;

    spawned = hardinfo_spawn_command_line_sync("last",
            &out, &err, NULL, NULL);
    if (spawned && out != NULL) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            if (strstr(p, "system boot")) {
                s = p;
                while (*s) {
                  if (*s == ' ' && *(s + 1) == ' ') {
                    memmove(s, s + 1, strlen(s) + 1);
                    s--;
                  } else {
                    s++;
                  }
                }
                tmp = g_strsplit(p, " ", 0);
                computer->os->boots =
                  h_strdup_cprintf("\n%s %s %s %s=%s",
                    computer->os->boots,
                    tmp[4], tmp[5], tmp[6], tmp[7], tmp[3]);
                g_strfreev(tmp);
            }
            p = next_nl + 1;
        }
      g_free(out);
      g_free(err);
    }
}
