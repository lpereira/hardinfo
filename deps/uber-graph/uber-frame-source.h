/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2008 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UBER_FRAME_SOURCE_H__
#define __UBER_FRAME_SOURCE_H__

#include <glib.h>

G_BEGIN_DECLS

guint uber_frame_source_add (guint          fps,
				GSourceFunc    func,
				gpointer       data);

guint uber_frame_source_add_full (gint           priority,
				     guint          fps,
				     GSourceFunc    func,
				     gpointer       data,
				     GDestroyNotify notify);

G_END_DECLS

#endif /* __UBER_FRAME_SOURCE_H__ */
