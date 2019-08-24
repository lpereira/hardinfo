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

#ifndef __STRSTR_WORD_H__
#define __STRSTR_WORD_H__

/* versions of strstr() and strcasestr() where the match must be preceded and
 * succeded by a non-alpha-numeric character. */
char *strstr_word(const char *haystack, const char *needle);
char *strcasestr_word(const char *haystack, const char *needle);

/* word boundary at start only (prefix), or end only (suffix) */
char *strstr_word_prefix(const char *haystack, const char *needle);
char *strcasestr_word_prefix(const char *haystack, const char *needle);
char *strstr_word_suffix(const char *haystack, const char *needle);
char *strcasestr_word_suffix(const char *haystack, const char *needle);

#endif
