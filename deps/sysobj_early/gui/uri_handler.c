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

#include <stdio.h>
#include "uri_handler.h"

static uri_handler uri_func = NULL;

void uri_set_function(uri_handler f) {
    uri_func = f;
}

gboolean uri_open(const gchar *uri) {
    gboolean ret = FALSE;
    if (uri_func)
        ret = uri_func(uri);
    if (ret) return TRUE;

    return uri_open_default(uri);
}

gboolean uri_open_default(const gchar *uri) {
    gchar *argv[] = { "/usr/bin/xdg-open", (gchar*)uri, NULL };
    GError *err = NULL;
    g_spawn_async(NULL, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, &err );
    if (err) {
        fprintf(stderr, "Error opening URI %s: %s\n", uri, err->message);
        g_error_free(err);
    }
    return TRUE;
}
