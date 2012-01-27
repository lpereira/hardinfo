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

#include <string.h>
#include "devices.h"

GHashTable *memlabels = NULL;

void scan_memory_do(void)
{
    gchar **keys, *tmp;
    static gint offset = -1;
    gint i;
    
    if (offset == -1) {
        /* gah. linux 2.4 adds three lines of data we don't need in
           /proc/meminfo */
        gchar *os_kernel = module_call_method("computer::getOSKernel");
        if (os_kernel) {
            offset = strstr(os_kernel, "Linux 2.4") ? 3 : 0;
            g_free(os_kernel);
        } else {
            offset = 0;
        }
    }
    
    g_file_get_contents("/proc/meminfo", &meminfo, NULL, NULL);
    
    keys = g_strsplit(meminfo, "\n", 0);

    g_free(meminfo);
    g_free(lginterval);
    
    meminfo = g_strdup("");
    lginterval = g_strdup("");
    
    for (i = offset; keys[i]; i++) {
        gchar **newkeys = g_strsplit(keys[i], ":", 0);
        
        if (!newkeys[0]) {
            g_strfreev(newkeys);
            break;
        }
        
        g_strstrip(newkeys[1]);
        
        if ((tmp = g_hash_table_lookup(memlabels, newkeys[0]))) {
            g_free(newkeys[0]);
            newkeys[0] = g_strdup(tmp);
        }
        
        moreinfo_add_with_prefix("DEV", newkeys[0], g_strdup(newkeys[1]));

        tmp = g_strconcat(meminfo, newkeys[0], "=", newkeys[1], "\n", NULL);
        g_free(meminfo);
        meminfo = tmp;
        
        tmp = g_strconcat(lginterval,
                          "UpdateInterval$", newkeys[0], "=1000\n", NULL);
        g_free(lginterval);
        lginterval = tmp;

        g_strfreev(newkeys);
    }
    g_strfreev(keys);
}

void init_memory_labels(void)
{
    static struct {
        char *proc_label;
        char *real_label;
    } proc2real[] = {
        { "MemTotal",	"Total Memory"        },
        { "MemFree", 	"Free Memory"         },
        { "SwapCached",	"Cached Swap"         },
        { "HighTotal",	"High Memory"         },
        { "HighFree",   "Free High Memory"    },
        { "LowTotal",	"Low Memory"          },
        { "LowFree",	"Free Low Memory"     },
        { "SwapTotal",	"Virtual Memory"      },
        { "SwapFree",   "Free Virtual Memory" },
        { NULL },
    };
    gint i;

    memlabels = g_hash_table_new(g_str_hash, g_str_equal);
    
    for (i = 0; proc2real[i].proc_label; i++) {
        g_hash_table_insert(memlabels, proc2real[i].proc_label, proc2real[i].real_label);
    }
}
