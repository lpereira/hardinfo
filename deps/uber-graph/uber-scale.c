/* uber-scale.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "uber-scale.h"

/**
 * uber_scale_linear:
 * @range: An #UberRange.
 * @pixel_range: An #UberRange.
 * @value: A pointer to the value to translate.
 * @user_data: user data for scale.
 *
 * An #UberScale function to translate a value to the coordinate system in
 * a linear fashion.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
uber_scale_linear (const UberRange *range,       /* IN */
                   const UberRange *pixel_range, /* IN */
                   gdouble         *value,       /* IN/OUT */
                   gpointer         user_data)   /* IN */
{
	#define A (range->range)
	#define B (pixel_range->range)
	#define C (*value)
	if (*value != 0.) {
		*value = C * B / A;
	}
	#undef A
	#undef B
	#undef C
	return TRUE;
}
