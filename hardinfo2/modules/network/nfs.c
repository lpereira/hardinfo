/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include "hardinfo.h"
#include "network.h"

gchar *nfs_shares_list = NULL;

void
scan_nfs_shared_directories(void)
{
    FILE *exports;
    gint count = 0;
    gchar buf[512];
    
    if (nfs_shares_list) {
        g_free(nfs_shares_list);
    }

    nfs_shares_list = g_strdup("");
    
    if ((exports = fopen("/etc/exports", "r"))) {
        while (fgets(buf, 512, exports)) {
            if (buf[0] != '/')
                continue;
            
            strend(buf, ' ');
            strend(buf, '\t');

            nfs_shares_list = h_strdup_cprintf("%s=\n", 
                                               buf, nfs_shares_list);
            count++;
        }

        fclose(exports);
    }

    if (!count) {
        g_free(nfs_shares_list);
        
        nfs_shares_list = g_strdup("No NFS exports=\n");
    }
}

