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

#include <string.h>
#include "devices.h"

GHashTable *memlabels = NULL;

void scan_memory_do(void)
{
    gchar **keys, *tmp, *tmp_label, *trans_val;
    static gint offset = -1;
    gint i;

    if (offset == -1) {
        /* gah. linux 2.4 adds three lines of data we don't need in
         * /proc/meminfo.
         * The lines look something like this:
         *  total: used: free: shared: buffers: cached:
         *  Mem: 3301101568 1523159040 1777942528 0 3514368 1450356736
         *  Swap: 0 0 0
         */
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

        g_strstrip(newkeys[0]);
        g_strstrip(newkeys[1]);

        /* try to find a localizable label */
        tmp = g_hash_table_lookup(memlabels, newkeys[0]);
        if (tmp)
            tmp_label = _(tmp);
        else
            tmp_label = ""; /* or newkeys[0] */
        /* although it doesn't matter... */

        if (strstr(newkeys[1], "kB")) {
            trans_val = g_strdup_printf("%d %s", atoi(newkeys[1]), _("KiB") );
        } else {
            trans_val = strdup(newkeys[1]);
        }

        moreinfo_add_with_prefix("DEV", newkeys[0], g_strdup(trans_val));

        tmp = g_strconcat(meminfo, newkeys[0], "=", trans_val, "|", tmp_label, "\n", NULL);
        g_free(meminfo);
        meminfo = tmp;

        g_free(trans_val);

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
    static const struct {
        char *proc_label;
        char *real_label;
    } proc2real[] = {
        { "MemTotal",   N_("Total Memory") },
        { "MemFree",    N_("Free Memory") },
        { "SwapCached", N_("Cached Swap") },
        { "HighTotal",  N_("High Memory") },
        { "HighFree",   N_("Free High Memory") },
        { "LowTotal",   N_("Low Memory") },
        { "LowFree",    N_("Free Low Memory") },
        { "SwapTotal",  N_("Virtual Memory") },
        { "SwapFree",   N_("Free Virtual Memory") },
        { NULL },
    };
    gint i;

    memlabels = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; proc2real[i].proc_label; i++) {
        g_hash_table_insert(memlabels, proc2real[i].proc_label,
            _(proc2real[i].real_label));
    }
}
