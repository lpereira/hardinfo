/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

static void __scan_memory()
{
    gchar **keys, *tmp;
    gint i;
    
    g_file_get_contents("/proc/meminfo", &meminfo, NULL, NULL);
    
    keys = g_strsplit(meminfo, "\n", 0);

    g_free(meminfo);
    g_free(lginterval);
    
    meminfo = g_strdup("");
    lginterval = g_strdup("");
    
    for (i = 0; keys[i]; i++) {
        gchar **newkeys = g_strsplit(keys[i], ":", 0);
        
        if (!newkeys[0]) {
            g_strfreev(newkeys);
            break;
        }
        
        g_strstrip(newkeys[1]);
        
        g_hash_table_replace(moreinfo, g_strdup(newkeys[0]), g_strdup(newkeys[1]));
        tmp = g_strconcat(meminfo, newkeys[0], "=", newkeys[1], "\n", NULL);
        g_free(meminfo);
        meminfo = tmp;
        
        tmp = g_strconcat(lginterval,
                          "LoadGraphInterval$", newkeys[0], "=500\n",
                          "UpdateInterval$", newkeys[0], "=1000\n", NULL);
        g_free(lginterval);
        lginterval = tmp;

        g_strfreev(newkeys);
    }
    g_strfreev(keys);
}
