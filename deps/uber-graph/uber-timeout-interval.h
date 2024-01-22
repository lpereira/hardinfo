/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Neil Roberts  <neil@linux.intel.com>
 *
 * Copyright (C) 2009  Intel Corporation.
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

#ifndef __UBER_TIMEOUT_INTERVAL_H__
#define __UBER_TIMEOUT_INTERVAL_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _UberTimeoutInterval UberTimeoutInterval;

struct _UberTimeoutInterval
{
  gint64 start_time;
  guint frame_count, fps;
};

void _uber_timeout_interval_init (UberTimeoutInterval *interval,
                                     guint fps);

gboolean _uber_timeout_interval_prepare (gint64 current_time,
                                            UberTimeoutInterval *interval,
                                            gint *delay);

gboolean _uber_timeout_interval_dispatch (UberTimeoutInterval *interval,
                                             GSourceFunc     callback,
                                             gpointer        user_data);

gint _uber_timeout_interval_compare_expiration
                                              (const UberTimeoutInterval *a,
                                               const UberTimeoutInterval *b);

G_END_DECLS

#endif /* __UBER_TIMEOUT_INTERVAL_H__ */
