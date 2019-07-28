/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _UTIL_IDS_H_
#define _UTIL_IDS_H_

#include <glib.h>

#define IDS_LOOKUP_BUFF_SIZE 220
#define IDS_LOOKUP_MAX_DEPTH 4

/* may be static, all results[] are NULL or point into _strs */
typedef struct {
    gchar *results[IDS_LOOKUP_MAX_DEPTH+1]; /* last always NULL */
    gchar _strs[IDS_LOOKUP_BUFF_SIZE*IDS_LOOKUP_MAX_DEPTH];
} ids_query_result;
#define ids_query_result_new() g_new0(ids_query_result, 1)
#define ids_query_result_free(s) g_free(s);

/* Given a qpath "/X/Y/Z", find names as:
 * X <name> ->result[0]
 * \tY <name> ->result[1]
 * \t\tZ <name> ->result[2]
 *
 * Works with:
 * - pci.ids "<vendor>/<device>/<subvendor> <subdevice>" or "C <class>/<subclass>/<prog-if>"
 *     ... need to query "<subvendor>" separately
 * - arm.ids "<implementer>/<part>"
 * - sdio.ids "<vendor>/<device>", "C <class>"
 * - sdcard.ids "OEMID <code>", "MANFID <code>"
 * - usb.ids "<vendor>/<device>", "C <class>" etc.
 */
long scan_ids_file(const gchar *file, const gchar *qpath, ids_query_result *result, long start_offset);

typedef struct {
    gchar *qpath;
    ids_query_result result;
} ids_query;

ids_query *ids_query_new(const gchar *qpath);
void ids_query_free(ids_query *s);
typedef GSList* ids_query_list;

/* query_list is a GSList of ids_query* */
long scan_ids_file_list(const gchar *file, ids_query_list query_list, long start_offset);
/* after scan_ids_file_list(), count hits */
int query_list_count_found(ids_query_list query_list);

#endif
