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

#include "util_ids.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define ids_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

ids_query *ids_query_new(const gchar *qpath) {
    ids_query *s = g_new0(ids_query, 1);
    s->qpath = qpath ? g_strdup(qpath) : NULL;
    return s;
}

void ids_query_free(ids_query *s) {
    if (s) g_free(s->qpath);
    g_free(s);
}

/* c001 < C 01 */
//TODO: compare more than the first char
static int ids_cmp(const char *s1, const char *s2) {
    int cmp = (s2 ? 1 : 0) - (s1 ? 1 : 0);
    if (cmp == 0 && isalpha(*s1) && isalpha(*s2))
        cmp = (islower(*s2) ? 1 : 0) - (islower(*s1) ? 1 : 0);
    if (cmp == 0)
        return g_strcmp0(s1, s2);
    else
        return cmp;
}

/* Given a qpath "/X/Y/Z", find names as:
 * X <name> ->result[0]
 * \tY <name> ->result[1]
 * \t\tZ <name> ->result[2]
 *
 * Works with:
 * - pci.ids "<vendor>/<device>/<subvendor> <subdevice>" or "C <class>/<subclass>/<prog-if>"
 * - arm.ids "<implementer>/<part>"
 * - sdio.ids "<vendor>/<device>", "C <class>"
 * - usb.ids "<vendor>/<device>", "C <class>" etc
 */
long scan_ids_file(const gchar *file, const gchar *qpath, ids_query_result *result, long start_offset) {
    gchar **qparts = NULL;
    gchar buff[IDS_LOOKUP_BUFF_SIZE] = "";
    ids_query_result ret = {};
    gchar *p = NULL;

    FILE *fd;
    int tabs;
    int qdepth;
    int qpartlen[IDS_LOOKUP_MAX_DEPTH];
    long last_root_fpos = -1, fpos, line = -1;

    if (!qpath)
        return -1;

    fd = fopen(file, "r");
    if (!fd) {
        ids_msg("file could not be read: %s", file);
        return -1;
    }

    qparts = g_strsplit(qpath, "/", -1);
    qdepth = g_strv_length(qparts);
    if (qdepth > IDS_LOOKUP_MAX_DEPTH) {
        ids_msg("qdepth (%d) > ids_max_depth (%d) for %s", qdepth, IDS_LOOKUP_MAX_DEPTH, qpath);
        qdepth = IDS_LOOKUP_MAX_DEPTH;
    }
    for(int i = 0; i < qdepth; i++)
        qpartlen[i] = strlen(qparts[i]);

    if (start_offset > 0)
        fseek(fd, start_offset, SEEK_SET);

    for (fpos = ftell(fd); fgets(buff, IDS_LOOKUP_BUFF_SIZE, fd); fpos = ftell(fd)) {
        p = strchr(buff, '\n');
        if (!p)
            ids_msg("line longer than IDS_LOOKUP_BUFF_SIZE (%d), file: %s, offset: %ld", IDS_LOOKUP_BUFF_SIZE, file, fpos);
        line++;

        /* line ends at comment */
        p = strchr(buff, '#');
        if (p) *p = 0;
        /* trim trailing white space */
        if (!p) p = buff + strlen(buff);
        p--;
        while(p > buff && isspace((unsigned char)*p)) p--;
        *(p+1) = 0;
        p = buff;

        if (buff[0] == 0)    continue; /* empty line */
        if (buff[0] == '\n') continue; /* empty line */

        /* scan for fields */
        tabs = 0;
        while(*p == '\t') { tabs++; p++; }

        if (tabs >= qdepth) continue; /* too deep */
        if (tabs != 0 && !ret.results[tabs-1])
            continue; /* not looking at this depth, yet */

        //ids_msg("looking at (%d) %s...", tabs, p);

        if (g_str_has_prefix(p, qparts[tabs])
            && isspace(*(p + qpartlen[tabs])) ) {
            /* found */
            p += qpartlen[tabs];
            while(isspace((unsigned char)*p)) p++; /* ffwd */

            if (tabs == 0) {
                last_root_fpos = fpos;
                ret.results[tabs] = ret._strs;
                strncpy(ret.results[tabs], p, IDS_LOOKUP_BUFF_SIZE-1);
            } else {
                ret.results[tabs] = ret.results[tabs-1] + strlen(ret.results[tabs-1]) + 1;
                strncpy(ret.results[tabs], p, IDS_LOOKUP_BUFF_SIZE-1);
            }
            continue;
        }

        if (ids_cmp(p, qparts[tabs]) == 1) {
            //ids_msg("will not be found p = %s (%d) qparts[tabs] = %s", p, qparts[tabs]);
            goto ids_lookup_done; /* will not be found */
        }

    } /* for each line */

ids_lookup_done:
    //ids_msg("bailed at line %ld...", line);
    fclose(fd);

    if (result) {
        memcpy(result, &ret, sizeof(ids_query_result));
        for(int i = 0; result->results[i]; i++)
            result->results[i] = result->_strs + (ret.results[i] - ret._strs);

        return last_root_fpos;
    }
    return last_root_fpos;
}

static gint _ids_query_list_cmp(const ids_query *ql1, const ids_query *ql2) {
    return g_strcmp0(ql1->qpath, ql2->qpath);
}

long scan_ids_file_list(const gchar *file, ids_query_list query_list, long start_offset) {
    GSList *tmp = g_slist_copy(query_list);
    tmp = g_slist_sort(tmp, (GCompareFunc)_ids_query_list_cmp);

    long offset = start_offset;
    for (GSList *l = query_list; l; l = l->next) {
        ids_query *q = l->data;
        offset = scan_ids_file(file, q->qpath, &(q->result), offset);
        if (offset == -1)
            break;
    }
    g_slist_free(tmp);
    return offset;
}

int query_list_count_found(ids_query_list query_list) {
    long count = 0;
    for (GSList *l = query_list; l; l = l->next) {
        ids_query *q = l->data;
        if (q->result.results[0]) count++;
    }
    return count;
}
