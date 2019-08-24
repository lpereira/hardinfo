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

/* versions of strstr() and strcasestr() where the match must be preceded and
 * succeded by a non-alpha-numeric character. */

#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>

static char *_strstr(const char *haystack, const char *needle, int anycase) {
    return anycase
        ? strcasestr(haystack, needle)
        : strstr(haystack, needle);
}

static char *_strstr_word(const char *haystack, const char *needle,
                          int anycase, int prefix_ok, int suffix_ok) {

    if (!haystack || !needle)
        return NULL;

    char *c;
    const char *p = haystack;
    size_t l = strlen(needle);
    while((c = _strstr(p, needle, anycase))) {
        const char *before = (c == haystack) ? NULL : c-1;
        const char *after = c + l;
        int ok = 1, wbs = 1, wbe = 1;
        if (isalnum(*after)) wbe = 0;
        if (before && isalnum(*before)) wbs = 0;
        if (!wbe && !prefix_ok) ok = 0;
        if (!wbs && !suffix_ok) ok = 0;
        if (!(wbs || wbe)) ok = 0;
        if (ok) return c;
        p++;
    }
    return NULL;
}

char *strstr_word(const char *haystack, const char *needle) {
    return _strstr_word(haystack, needle, 0, 0, 0);
}

char *strcasestr_word(const char *haystack, const char *needle) {
    return _strstr_word(haystack, needle, 1, 0, 0);
}

char *strstr_word_prefix(const char *haystack, const char *needle) {
    return _strstr_word(haystack, needle, 0, 1, 0);
}

char *strcasestr_word_prefix(const char *haystack, const char *needle) {
    return _strstr_word(haystack, needle, 1, 1, 0);
}

char *strstr_word_suffix(const char *haystack, const char *needle) {
    return _strstr_word(haystack, needle, 0, 0, 1);
}

char *strcasestr_word_suffix(const char *haystack, const char *needle) {
    return _strstr_word(haystack, needle, 1, 0, 1);
}
