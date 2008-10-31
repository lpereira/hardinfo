/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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
 *
 * Some code from xfce4-mount-plugin, version 0.4.3
 *  Copyright (C) 2005 Jean-Baptiste jb_dul@yahoo.com 
 *  Distributed under the terms of GNU GPL 2. 
 */
#include <sys/vfs.h>

static gchar *fs_list = NULL;

static gboolean
remove_filesystem_entries(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "FS");
}

static void
scan_filesystems(void)
{
    FILE *mtab;
    gchar buf[1024];
    struct statfs sfs;
    int count = 0;

    g_free(fs_list);
    fs_list = g_strdup("");
    g_hash_table_foreach_remove(moreinfo, remove_filesystem_entries, NULL);

    mtab = fopen("/etc/mtab", "r");
    if (!mtab)
	return;

    while (fgets(buf, 1024, mtab)) {
	gfloat size, used, avail;
	gchar **tmp;

	tmp = g_strsplit(buf, " ", 0);
	if (!statfs(tmp[1], &sfs)) {
		gfloat use_ratio;

		size = (float) sfs.f_bsize * (float) sfs.f_blocks;
		avail = (float) sfs.f_bsize * (float) sfs.f_bavail;
		used = size - avail;

		if (size == 0.0f) {
			continue;
		}

		if (avail == 0.0f) {
			use_ratio = 100.0f;
		} else {
			use_ratio = 100.0f * (used / size);
		}

		gchar *strsize = size_human_readable(size),
		      *stravail = size_human_readable(avail),
	  	      *strused = size_human_readable(used);

		gchar *strhash;
		if ((strhash = g_hash_table_lookup(moreinfo, tmp[0]))) {
		    g_hash_table_remove(moreinfo, tmp[0]);
		    g_free(strhash);
		}
		
		strreplace(tmp[0], "#", '_');
		
		strhash = g_strdup_printf("[%s]\n"
					  "Filesystem=%s\n"
					  "Mounted As=%s\n"
					  "Mount Point=%s\n"
					  "Size=%s\n"
					  "Used=%s\n"
					  "Available=%s\n",
					  tmp[0],
					  tmp[2],
					  strstr(tmp[3], "rw") ? "Read-Write" :
					  "Read-Only", tmp[1], strsize, strused,
					  stravail);
		g_hash_table_insert(moreinfo, g_strdup_printf("FS%d", ++count), strhash);

		fs_list = h_strdup_cprintf("$FS%d$%s=%.2f|%s|%s\n",
					  fs_list,
					  count, tmp[0], use_ratio, strsize, stravail);

		g_free(strsize);
		g_free(stravail);
		g_free(strused);
	}
	g_strfreev(tmp);
    }

    fclose(mtab);
}
