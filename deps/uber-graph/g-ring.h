/* g-ring.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_RING_H__
#define __G_RING_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * g_ring_append_val:
 * @ring: A #GRing.
 * @val: A value to append to the #GRing.
 *
 * Appends a value to the ring buffer.  @val must be a variable as it is
 * referenced to.
 *
 * Returns: None.
 * Side effects: None.
 */
#define g_ring_append_val(ring, val) g_ring_append_vals(ring, &(val), 1)

/**
 * g_ring_get_index:
 * @ring: A #GRing.
 * @type: The type to extract.
 * @i: The index within the #GRing relative to the current position.
 *
 * Retrieves the value at the given index from the #GRing.  The value
 * is cast to @type.  You may retrieve a pointer to the value within the
 * array by using &.
 *
 * [[
 * gdouble *v = &g_ring_get_index(ring, gdouble, 0);
 * gdouble v = g_ring_get_index(ring, gdouble, 0);
 * ]]
 *
 * Returns: The value at the given index.
 * Side effects: None.
 */
#define g_ring_get_index(ring, type, i)                               \
    (((type*)(ring)->data)[(((gint)(ring)->pos - 1 - (i)) >= 0) ?     \
                            ((ring)->pos - 1 - (i)) :                 \
                            ((ring)->len + ((ring)->pos - 1 - (i)))])

typedef struct
{
	guint8 *data;
	guint   len;
	guint   pos;
} GRing;

GType  g_ring_get_type    (void) G_GNUC_CONST;
GRing* g_ring_sized_new   (guint           element_size,
                           guint           reserved_size,
                           GDestroyNotify  element_destroy);
void   g_ring_append_vals (GRing          *ring,
                           gconstpointer   data,
                           guint           len);
void   g_ring_foreach     (GRing          *ring,
                           GFunc           func,
                           gpointer        user_data);
GRing* g_ring_ref         (GRing          *ring);
void   g_ring_unref       (GRing          *ring);

G_END_DECLS

#endif /* __G_RING_H__ */
