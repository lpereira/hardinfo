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

#ifndef _FORMAT_EARLY_H_
#define _FORMAT_EARLY_H_

#include <glib.h>
#include <strings.h>
#include "appf.h"
#include "util_sysobj.h"
#include "vendor.h"

enum {
    FMT_OPT_NONE   = 0,
    FMT_OPT_ATERM  = 1<<16,  /* ANSI color terminal */
    FMT_OPT_PANGO  = 1<<17,  /* pango markup for gtk */
    FMT_OPT_HTML   = 1<<18,  /* html */
};

gchar *safe_ansi_color(gchar *ansi_color, gboolean free_in); /* verify the ansi color */
const gchar *color_lookup(int ansi_color); /* ansi_color to html color */
/* wrap the input str with color based on fmt_opts (none,term,html,pango) */
gchar *format_with_ansi_color(const gchar *str, const gchar *ansi_color, int fmt_opts);

void tag_vendor(gchar **str, guint offset, const gchar *vendor_str, const char *ansi_color, int fmt_opts);
gchar *vendor_match_tag(const gchar *vendor_str, int fmt_opts);

gchar *vendor_list_ribbon(const vendor_list vl_in, int fmt_opts);

#endif
