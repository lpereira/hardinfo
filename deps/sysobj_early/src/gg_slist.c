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

#include <glib.h>

GSList *gg_slist_remove_duplicates(GSList *sl) {
    for (GSList *l = sl; l; l = l->next) {
        GSList *d = NULL;
        while(d = g_slist_find(l->next, l->data) )
            sl = g_slist_delete_link(sl, d);
    }
    return sl;
}

GSList *gg_slist_remove_duplicates_custom(GSList *sl, GCompareFunc func) {
    for (GSList *l = sl; l; l = l->next) {
        GSList *d = NULL;
        while(d = g_slist_find_custom(l->next, l->data, func) )
            sl = g_slist_delete_link(sl, d);
    }
    return sl;
}

GSList *gg_slist_remove_null(GSList *sl) {
    GSList *n = sl ? sl->next : NULL;
    for (GSList *l = sl; l; l = n) {
        n = l->next;
        if (l->data == NULL)
            sl = g_slist_delete_link(sl, l);
    }
    return sl;
}
