/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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

void
scan_boots_real(void)
{
    FILE *last;
    char buffer[256];

    scan_os(FALSE);

    if (!computer->os->boots)
      computer->os->boots = g_strdup("[Boots]\n");
    else
      return;
    
    last = popen("last", "r");
    if (last) {
      while (fgets(buffer, 256, last)) {
        if (strstr(buffer, "system boot")) {
          gchar **tmp, *buf = buffer;
          
          strend(buffer, '\n');
          
          while (*buf) {
            if (*buf == ' ' && *(buf + 1) == ' ') {
              strcpy(buf, buf + 1);
              
              buf--;
            } else {
              buf++;
            }
          }

          tmp = g_strsplit(buffer, " ", 5);
          computer->os->boots = h_strdup_cprintf("\n%s=Kernel %s",
                                                computer->os->boots,
                                                tmp[4],
                                                tmp[3]);
          g_strfreev(tmp);
        }
      }
      
      pclose(last);
    }
}
