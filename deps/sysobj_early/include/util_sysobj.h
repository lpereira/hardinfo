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

#ifndef _UTIL_SYSOBJ_H_
#define _UTIL_SYSOBJ_H_

#include <glib.h>
#include "appf.h"

/* string eq */
#define SEQ(s1, s2) (g_strcmp0((s1), (s2)) == 0)

/* handy for static halp */
#define BULLET "\u2022"
#define REFLINK(URI) "<a href=\"" URI "\">" URI "</a>"
#define REFLINKT(TEXT, URI) "<a href=\"" URI "\">" TEXT "</a>"

gboolean util_have_root();
void util_null_trailing_slash(gchar *str); /* in-place */
void util_compress_space(gchar *str); /* in-place, multiple whitespace replaced by one space */
void util_strstrip_double_quotes_dumb(gchar *str); /* in-place, strips any double-quotes from the start and end of str */
gchar *util_build_fn(const gchar *base, const gchar *name); /* returns "<base>[/<name>]" */
gchar *util_canonicalize_path(const gchar *path); /* resolve . and .., but not symlinks */
gchar *util_normalize_path(const gchar *path, const gchar *relto); /* resolve . and .., and symlinks */
gsize util_count_lines(const gchar *str); /* doesn't count empty last line */
gchar *util_escape_markup(gchar *v, gboolean replacing);
int util_get_did(gchar *str, const gchar *lbl); /* ("cpu6", "cpu") -> 6, returns -1 if error */
int util_maybe_num(gchar *str); /* returns the guessed base, 0 for not num */
gchar *util_find_line_value(gchar *data, gchar *key, gchar delim);
gchar *util_strchomp_float(gchar* str_float); /* in-place, must use , or . for decimal sep */
gchar *util_safe_name(const gchar *name, gboolean lower_case); /* make a string into a name nice and safe for file name */

/* to quiet -Wunused-parameter nagging.  */
#define PARAM_NOT_UNUSED(p) (void)p

#endif
