/* uber-scale.h
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

#ifndef __UBER_SCALE_H__
#define __UBER_SCALE_H__

#include "uber-range.h"

G_BEGIN_DECLS

typedef gboolean (*UberScale) (const UberRange *range,
                               const UberRange *pixel_range,
                               gdouble         *value,
                               gpointer         user_data);

gboolean uber_scale_linear (const UberRange *range,
                            const UberRange *pixel_range,
                            gdouble         *value,
                            gpointer         user_data);

G_END_DECLS

#endif /* __UBER_SCALE_H__ */
