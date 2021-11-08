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

#include "hardinfo.h"
#include "computer.h"
#include "cpu_util.h" /* for UNKIFNULL() */

typedef struct {
    gchar name[32];
    gchar *title,
        *source,
        *address,
        *email,
        *language,
        *territory,
        *revision,
        *date,
        *codeset;
} locale_info;

void locale_info_free(locale_info *s) {
    if (s) {
        g_free(s->title);
        g_free(s->source);
        g_free(s->address);
        g_free(s->email);
        g_free(s->language);
        g_free(s->territory);
        g_free(s->revision);
        g_free(s->date);
        g_free(s->codeset);
        free(s);
    }
}

/* TODO: use info_* */
gchar *locale_info_section(locale_info *s) {
    gchar *name = g_strdup(s->name);
    gchar *title = g_strdup(s->title),
        *source = g_strdup(s->source),
        *address = g_strdup(s->address),
        *email = g_strdup(s->email),
        *language = g_strdup(s->language),
        *territory = g_strdup(s->territory),
        *revision = g_strdup(s->revision),
        *date = g_strdup(s->date),
        *codeset = g_strdup(s->codeset);

    UNKIFNULL(title);
    UNKIFNULL(source);
    UNKIFNULL(address);
    UNKIFNULL(email);
    UNKIFNULL(language);
    UNKIFNULL(territory);
    UNKIFNULL(revision);
    UNKIFNULL(date);
    UNKIFNULL(codeset);

    /* values may have & */
    title = hardinfo_clean_value(title, 1);
    source = hardinfo_clean_value(source, 1);
    address = hardinfo_clean_value(address, 1);
    email = hardinfo_clean_value(email, 1);
    language = hardinfo_clean_value(language, 1);
    territory = hardinfo_clean_value(territory, 1);

    gchar *ret = g_strdup_printf("[%s]\n"
    /* Name */     "%s=%s (%s)\n"
    /* Source */   "%s=%s\n"
    /* Address */  "%s=%s\n"
    /* Email */    "%s=%s\n"
    /* Language */ "%s=%s\n"
    /* Territory */"%s=%s\n"
    /* Revision */ "%s=%s\n"
    /* Date */     "%s=%s\n"
    /* Codeset */  "%s=%s\n",
                 _("Locale Information"),
                 _("Name"), name, title,
                 _("Source"), source,
                 _("Address"), address,
                 _("E-mail"), email,
                 _("Language"), language,
                 _("Territory"), territory,
                 _("Revision"), revision,
                 _("Date"), date,
                 _("Codeset"), codeset );
    g_free(name);
    g_free(title);
    g_free(source);
    g_free(address);
    g_free(email);
    g_free(language);
    g_free(territory);
    g_free(revision);
    g_free(date);
    g_free(codeset);
    return ret;
}

void
scan_languages(OperatingSystem * os)
{
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;

    gchar *ret = NULL;
    locale_info *curr = NULL;
    int last = 0;

    spawned = hardinfo_spawn_command_line_sync("locale -va",
            &out, &err, NULL, NULL);
    if (spawned) {
        ret = g_strdup("");
        p = out;
        while(1) {
            /* `locale -va` doesn't end the last locale block
             * with an \n, which makes this more complicated */
            next_nl = strchr(p, '\n');
            if (next_nl == NULL)
                next_nl = strchr(p, 0);
            last = (*next_nl == 0) ? 1 : 0;
            strend(p, '\n');
            if (strncmp(p, "locale:", 7) == 0) {
                curr = g_new0(locale_info, 1);
                sscanf(p, "locale: %s", curr->name);
                /* TODO: 'directory:' and 'archive:' */
            } else if (strchr(p, '|')) {
                do {/* get_str() has a continue in it,
                     * how fn frustrating that was to figure out */
                    gchar **tmp = g_strsplit(p, "|", 2);
                    tmp[0] = g_strstrip(tmp[0]);
                    if (tmp[1]) {
                        tmp[1] = g_strstrip(tmp[1]);
                        get_str("title", curr->title);
                        get_str("source", curr->source);
                        get_str("address", curr->address);
                        get_str("email", curr->email);
                        get_str("language", curr->language);
                        get_str("territory", curr->territory);
                        get_str("revision", curr->revision);
                        get_str("date", curr->date);
                        get_str("codeset", curr->codeset);
                    }
                    g_strfreev(tmp);
                } while (0);
            } else if (strstr(p, "------")) {
                /* do nothing */
            } else if (curr) {
                /* a blank line is the end of a locale */
                gchar *li_str = locale_info_section(curr);
                gchar *clean_title = hardinfo_clean_value(curr->title, 0); /* may contain & */
                ret = h_strdup_cprintf("$%s$%s=%s\n", ret, curr->name, curr->name, clean_title);
                moreinfo_add_with_prefix("COMP", g_strdup(curr->name), li_str); /* becomes owned by moreinfo */
                locale_info_free(curr);
                curr = NULL;
                g_free(clean_title);
            }
            if (last) break;
            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }
    os->languages = ret;
}
