/*
 * ToscoIntl version 0.1
 * Copyright (c) 2002-2003 Leandro Pereira <leandro@linuxmag.com.br>
 * All rights reserved.
 *
 * This script is in the Tosco Public License. It may be copied and/or
 * modified, in whole or in part, provided that all copies and related
 * documentation includes the above copyright notice, this condition
 * and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *
 * (yes, the disclaimer is a copy from the BSD license, eat me!)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "hardinfo.h"

#ifdef ENABLE_NLS

void intl_init(void)
{
	const gchar *by;
	
	g_print("Translation module registered.\n");

	by = intl_translate("translated-by", "Translation");
	if (strcmp(by, "translated-by")) {
		g_print("Translated by: %s\n", by);
	}
}

/*
 * GNU's gettext is cool and all... but hey, this is smaller :)
 */
const gchar *
intl_translate(const gchar * string, const gchar * source) __THROW
{
	FILE *file;
	gchar buffer[256], *keyname, *lang = NULL, *langenv = NULL;
	const gchar *retval, *langvars[] =
		 {"LANG", "LC_MESSAGES", "LC_ALL", NULL};
	gboolean found;
	struct stat st;
	gint i = 0;

	keyname = g_strdup_printf("[%s]", source);

	for (; langvars[i]; i++)
		if (getenv(langvars[i])) {
			langenv = getenv(langvars[i]);
			goto langenv_ok;
		}

	goto not_found;

     langenv_ok:
	lang = g_strconcat(INTL_PREFIX, langenv, ".lang", NULL);
	if (stat(lang, &st)) {
		lang = g_strconcat(INTL_PREFIX, "default.lang", NULL);
		if (stat(lang, &st)) {
		     not_found:
			retval = string;
			goto finished;
		} 
	}

	retval  = string;

	file = fopen(lang, "r");
	if (!file)
		goto finished;
	
	while (fgets(buffer, 256, file)) {
		const gchar *buf = buffer;
		
		buf = g_strstrip(buf);
		
		if (*buf == '[' && !found &&
			!strncmp(buf, keyname, strlen(keyname)))
				found = TRUE;
		if (found && !strncmp(buf, string, strlen(string)) &&
			*(buf + strlen(string)) == '=') {
				walk_until_inclusive('=');
				retval = g_strdup(buf);

				fclose(file);
	
				goto finished;
		}
	}
	fclose(file);
	
     finished:
     	g_free(keyname);
     	g_free(lang);

	return retval;
}

#endif
