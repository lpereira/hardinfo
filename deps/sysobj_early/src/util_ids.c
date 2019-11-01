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
static int ids_tracing = 0;
void ids_trace_start() { ids_tracing = 1; }
void ids_trace_stop() { ids_tracing = 0; }

ids_query *ids_query_new(const gchar *qpath) {
    ids_query *s = g_new0(ids_query, 1);
    s->qpath = qpath ? g_strdup(qpath) : NULL;
    return s;
}

void ids_query_free(ids_query *s) {
    if (s) g_free(s->qpath);
    g_free(s);
}

void ids_query_result_cpy(ids_query_result *dest, ids_query_result *src) {
    if (!dest || !src) return;
    memcpy(dest, src, sizeof(ids_query_result));
    for(int i = 0; dest->results[i]; i++)
        dest->results[i] = dest->_strs + (src->results[i] - src->_strs);
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

static void ids_query_result_set_str(ids_query_result *ret, int tabs, gchar *p) {
    if (!p) {
        ret->results[tabs] = p;
    } else {
        if (tabs == 0) {
            ret->results[tabs] = ret->_strs;
            strncpy(ret->results[tabs], p, IDS_LOOKUP_BUFF_SIZE-1);
        } else {
            ret->results[tabs] = ret->results[tabs-1] + strlen(ret->results[tabs-1]) + 1;
            strncpy(ret->results[tabs], p, IDS_LOOKUP_BUFF_SIZE-1);
        }
    }
    /* all following strings become invalid */
    while(tabs < IDS_LOOKUP_MAX_DEPTH)
        ret->results[++tabs] = NULL;
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
 * - edid.ids "<3letter_vendor>"
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
        if (ret.results[tabs])
            goto ids_lookup_done; /* answered at this level */

        if (ids_tracing) ids_msg("[%s] looking at (%d) %s...", file, tabs, p);

        if (g_str_has_prefix(p, qparts[tabs])
            && isspace(*(p + qpartlen[tabs])) ) {
            /* found */
            p += qpartlen[tabs];
            while(isspace((unsigned char)*p)) p++; /* ffwd */

            if (tabs == 0) last_root_fpos = fpos;
            ids_query_result_set_str(&ret, tabs, p);

            if (ids_tracing) {
                int i = 0;
                for(; i < IDS_LOOKUP_MAX_DEPTH; i++) {
                    if (!qparts[i]) break;
                    ids_msg(" ...[%d]: %s\t--> %s", i, qparts[i], ret.results[i]);
                }
            }
            continue;
        }

        if (ids_cmp(p, qparts[tabs]) == 1) {
            if (ids_tracing)
                ids_msg("will not be found qparts[tabs] = %s, p = %s", qparts[tabs], p);
            goto ids_lookup_done; /* will not be found */
        }

    } /* for each line */

ids_lookup_done:
    if (ids_tracing)
        ids_msg("bailed at line %ld...", line);
    fclose(fd);

    if (result) {
        ids_query_result_cpy(result, &ret);
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

static gchar *split_loc_default(const char *line) {
    return g_utf8_strchr(line, -1, ' ');
}

GSList *ids_file_all_get_all(const gchar *file, split_loc_function split_loc_func) {
    GSList *ret = NULL;
    gchar buff[IDS_LOOKUP_BUFF_SIZE] = "";
    gchar *p = NULL, *name = NULL;

    FILE *fd;
    int tabs = 0, tabs_last = 0;
    long fpos, line = -1;

    fd = fopen(file, "r");
    if (!fd) {
        ids_msg("file could not be read: %s", file);
        return ret;
    }

    ids_query_result *working = g_new0(ids_query_result, 1);
    gchar **qparts = g_new0(gchar*, IDS_LOOKUP_MAX_DEPTH + 1);
    for(tabs = IDS_LOOKUP_MAX_DEPTH-1; tabs>=0; tabs--)
        qparts[tabs] = g_malloc0(IDS_LOOKUP_BUFF_SIZE);
    tabs = 0;

    if (!split_loc_func) split_loc_func = split_loc_default;

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

        if (tabs >= IDS_LOOKUP_MAX_DEPTH) continue; /* too deep */
        if (tabs > tabs_last + 1) {
            /* jump too big, there's a qpath part that is "" */
            ids_msg("jump too big from depth %d to %d, file: %s, offset: %ld", tabs_last, tabs, file, fpos);
            continue;
        }

        name = split_loc_func(p);
        if (!name) {
            ids_msg("expected name/value split not found, file: %s, offset: %ld", file, fpos);
            continue;
        }
        *name = 0; name++; /* split ptr is the first char of split string */
        g_strstrip(p);
        g_strstrip(name);

        // now  p = id, name = name
        // ids_msg("p: %s -- name: %s", p, name);

        strncpy(qparts[tabs], p, IDS_LOOKUP_BUFF_SIZE-1);
        ids_query_result_set_str(working, tabs, name);
        if (tabs < tabs_last)
            for(;tabs_last > tabs; tabs_last--) {
                qparts[tabs_last][0] = 0;
                working->results[tabs_last] = NULL;
        }

        ids_query *found = ids_query_new(NULL);
        ids_query_result_cpy(&found->result, working);
        found->qpath = g_strjoinv("/", qparts);
        p = found->qpath + strlen(found->qpath) - 1;
        while(*p == '/') { *p = 0; p--; }
        ret = g_slist_append(ret, found);

        tabs_last = tabs;
    } /* for each line */

    fclose(fd);
    g_strfreev(qparts);
    ids_query_result_free(working);

    return ret;
}
