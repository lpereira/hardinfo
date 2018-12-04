/* uber-range.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "uber-range.h"

UberRange*
uber_range_copy (UberRange *src)
{
	UberRange *dst = g_new0(UberRange, 1);
	memcpy(dst, src, sizeof(UberRange));
	return dst;
}

void
uber_range_free (UberRange *range)
{
	g_free(range);
}

UberRange*
uber_range_new (gdouble begin,
                gdouble end)
{
	UberRange *range;

	range = g_new0(UberRange, 1);
	range->begin = begin;
	range->end = end;
	range->range = range->end - range->begin;
	return range;
}

GType
uber_range_get_type (void)
{
	static gsize initialized = FALSE;
	GType type_id = 0;

	if (G_UNLIKELY(g_once_init_enter(&initialized))) {
		type_id = g_boxed_type_register_static("UberRange",
		                                       (GBoxedCopyFunc)uber_range_copy,
		                                       (GBoxedFreeFunc)uber_range_free);
		g_once_init_leave(&initialized, TRUE);
	}

	return type_id;
}
