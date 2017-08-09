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

#include "hardinfo.h"
#include "computer.h"
#include "cpu_util.h" /* for UNKIFNULL() */

void
scan_languages(OperatingSystem * os)
{
    FILE *locale;
    gchar buf[512], *retval = NULL;

    locale = popen("locale -va && echo", "r");
    if (!locale)
	return;

    gchar name[32];
    gchar *title = NULL,
          *source = NULL,
	  *address = NULL,
	  *email = NULL,
	  *language = NULL,
	  *territory = NULL,
	  *revision = NULL,
	  *date = NULL,
	  *codeset = NULL;

    while (fgets(buf, 512, locale)) {
	if (!strncmp(buf, "locale:", 7)) {
	    sscanf(buf, "locale: %s", name);
	    (void)fgets(buf, 128, locale);
	} else if (strchr(buf, '|')) {
	    gchar **tmp = g_strsplit(buf, "|", 2);

	    tmp[0] = g_strstrip(tmp[0]);

	    if (tmp[1]) {
		tmp[1] = g_strstrip(tmp[1]);

		get_str("title", title);
		get_str("source", source);
		get_str("address", address);
		get_str("email", email);
		get_str("language", language);
		get_str("territory", territory);
		get_str("revision", revision);
		get_str("date", date);
		get_str("codeset", codeset);
	    }

	    g_strfreev(tmp);
	} else {
        gchar *currlocale;

        retval = h_strdup_cprintf("$%s$%s=%s\n", retval, name, name, title);

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

        currlocale = g_strdup_printf("[%s]\n"
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

        moreinfo_add_with_prefix("COMP", name, currlocale);

        g_free(title);
        g_free(source);
        g_free(address);
        g_free(email);
        g_free(language);
        g_free(territory);
        g_free(revision);
        g_free(date);
        g_free(codeset);

        title = source = address = email = language = territory = \
            revision = date = codeset = NULL;
    }
    }

    fclose(locale);

    os->languages = retval;
}
