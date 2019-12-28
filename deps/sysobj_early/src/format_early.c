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

#include "format_early.h"

#define ANSI_COLOR_RESET   "\x1b[0m"

const gchar *color_lookup(int ansi_color) {
    static struct { int ansi; const gchar *html; } tab[] = {
        { 30,  "#010101" }, { 31,  "#de382b" }, { 32,  "#39b54a" }, { 33,  "#ffc706" },
        { 34,  "#006fb8" }, { 35,  "#762671" }, { 36,  "#2cb5e9" }, { 37,  "#cccccc" },
        { 90,  "#808080" }, { 91,  "#ff0000" }, { 92,  "#00ff00" }, { 93,  "#ffff00" },
        { 94,  "#0000ff" }, { 95,  "#ff00ff" }, { 96,  "#00ffff" }, { 97,  "#ffffff" },
        { 40,  "#010101" }, { 41,  "#de382b" }, { 42,  "#39b54a" }, { 43,  "#ffc706" },
        { 44,  "#006fb8" }, { 45,  "#762671" }, { 46,  "#2cb5e9" }, { 47,  "#cccccc" },
        { 100, "#808080" }, { 101, "#ff0000" }, { 102, "#00ff00" }, { 103, "#ffff00" },
        { 104, "#0000ff" }, { 105, "#ff00ff" }, { 106, "#00ffff" }, { 107, "#ffffff" },
    };
    for (int i = 0; i<(int)G_N_ELEMENTS(tab); i++)
        if (tab[i].ansi == ansi_color)
            return tab[i].html;
    return NULL;
}

gchar *safe_ansi_color(gchar *ansi_color, gboolean free_in) {
    if (!ansi_color) return NULL;
    gchar *ret = NULL;
    gchar **codes = g_strsplit(ansi_color, ";", -1);
    if (free_in)
        g_free(ansi_color);
    int len = g_strv_length(codes);
    for(int i = 0; i < len; i++) {
        int c = atoi(codes[i]);
        if (c == 0 || c == 1
            || ( c >= 30 && c <= 37)
            || ( c >= 40 && c <= 47)
            || ( c >= 90 && c <= 97)
            || ( c >= 100 && c <= 107) ) {
                ret = appf(ret, ";", "%s", codes[i]);
        }
    }
    g_strfreev(codes);
    return ret;
}

gchar *format_with_ansi_color(const gchar *str, const gchar *ansi_color, int fmt_opts) {
    gchar *ret = NULL;

    gchar *safe_color = g_strdup(ansi_color);
    util_strstrip_double_quotes_dumb(safe_color);

    if (fmt_opts & FMT_OPT_ATERM) {
        safe_color = safe_ansi_color(safe_color, TRUE);
        ret = g_strdup_printf("\x1b[%sm%s" ANSI_COLOR_RESET, safe_color, str);
        goto format_with_ansi_color_end;
    }

    if (fmt_opts & FMT_OPT_PANGO || fmt_opts & FMT_OPT_HTML) {
        int fgc = 37, bgc = 40;
        gchar **codes = g_strsplit(safe_color, ";", -1);
        int len = g_strv_length(codes);
        for(int i = 0; i < len; i++) {
            int c = atoi(codes[i]);
            if ( (c >= 30 && c <= 37)
               || ( c >= 90 && c <= 97 ) ) {
                fgc = c;
            }
            if ( (c >= 40 && c <= 47)
               || ( c >= 100 && c <= 107) ) {
                bgc = c;
            }
        }
        g_strfreev(codes);
        const gchar *html_color_fg = color_lookup(fgc);
        const gchar *html_color_bg = color_lookup(bgc);
        if (fmt_opts & FMT_OPT_PANGO)
            ret = g_strdup_printf("<span background=\"%s\" color=\"%s\"><b> %s </b></span>", html_color_bg, html_color_fg, str);
        else if (fmt_opts & FMT_OPT_HTML)
            ret = g_strdup_printf("<span style=\"background-color: %s; color: %s;\"><b>&nbsp;%s&nbsp;</b></span>", html_color_bg, html_color_fg, str);
    }

format_with_ansi_color_end:
    g_free(safe_color);
    if (!ret)
        ret = g_strdup(str);
    return ret;
}

void tag_vendor(gchar **str, guint offset, const gchar *vendor_str, const char *ansi_color, int fmt_opts) {
    if (!str || !*str) return;
    if (!vendor_str || !ansi_color) return;
    gchar *work = *str, *new = NULL;
    if (g_str_has_prefix(work + offset, vendor_str)
        || strncasecmp(work + offset, vendor_str, strlen(vendor_str)) == 0) {
        gchar *cvs = format_with_ansi_color(vendor_str, ansi_color, fmt_opts);
        *(work+offset) = 0;
        new = g_strdup_printf("%s%s%s", work, cvs, work + offset + strlen(vendor_str) );
        g_free(work);
        *str = new;
        g_free(cvs);
    }
}

gchar *vendor_match_tag(const gchar *vendor_str, int fmt_opts) {
    const Vendor *v = vendor_match(vendor_str, NULL);
    if (v) {
        gchar *ven_tag = v->name_short ? g_strdup(v->name_short) : g_strdup(v->name);
        tag_vendor(&ven_tag, 0, ven_tag, v->ansi_color, fmt_opts);
        return ven_tag;
    }
    return NULL;
}

gchar *vendor_list_ribbon(const vendor_list vl_in, int fmt_opts) {
    gchar *ret = NULL;
    vendor_list vl = g_slist_copy(vl_in); /* shallow is fine */
    vl = vendor_list_remove_duplicates(vl);
    if (vl) {
        GSList *l = vl, *n = l ? l->next : NULL;
        /* replace each vendor with the vendor tag */
        for(; l; l = n) {
            n = l->next;
            const Vendor *v = l->data;
            if (!v) {
                vl = g_slist_delete_link(vl, l);
                continue;
            }
            gchar *ven_tag = v->name_short ? g_strdup(v->name_short) : g_strdup(v->name);
            if(ven_tag) {
                tag_vendor(&ven_tag, 0, ven_tag, v->ansi_color, fmt_opts);
                l->data = ven_tag;
            }
        }
        /* vl is now a regular GSList of formatted vendor tag strings */
        vl = gg_slist_remove_duplicates_custom(vl, (GCompareFunc)g_strcmp0);
        for(l = vl; l; l = l->next)
            ret = appfsp(ret, "%s", (gchar*)l->data);
    }
    g_slist_free_full(vl, g_free);
    return ret;
}
