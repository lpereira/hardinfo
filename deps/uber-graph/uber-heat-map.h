/* uber-heat-map.h
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

#ifndef __UBER_HEAT_MAP_H__
#define __UBER_HEAT_MAP_H__

#include "uber-graph.h"

G_BEGIN_DECLS

#define UBER_TYPE_HEAT_MAP            (uber_heat_map_get_type())
#define UBER_HEAT_MAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_HEAT_MAP, UberHeatMap))
#define UBER_HEAT_MAP_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_HEAT_MAP, UberHeatMap const))
#define UBER_HEAT_MAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  UBER_TYPE_HEAT_MAP, UberHeatMapClass))
#define UBER_IS_HEAT_MAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UBER_TYPE_HEAT_MAP))
#define UBER_IS_HEAT_MAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  UBER_TYPE_HEAT_MAP))
#define UBER_HEAT_MAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  UBER_TYPE_HEAT_MAP, UberHeatMapClass))

typedef struct _UberHeatMap        UberHeatMap;
typedef struct _UberHeatMapClass   UberHeatMapClass;
typedef struct _UberHeatMapPrivate UberHeatMapPrivate;

typedef gboolean (*UberHeatMapFunc) (UberHeatMap  *map,
                                     GArray      **values,
                                     gpointer      user_data);

struct _UberHeatMap
{
	UberGraph parent;

	/*< private >*/
	UberHeatMapPrivate *priv;
};

struct _UberHeatMapClass
{
	UberGraphClass parent_class;
};

GType      uber_heat_map_get_type      (void) G_GNUC_CONST;
GtkWidget* uber_heat_map_new           (void);
void       uber_heat_map_set_fg_color  (UberHeatMap     *map,
                                        const GdkRGBA   *color);
void       uber_heat_map_set_data_func (UberHeatMap     *map,
                                        UberHeatMapFunc  func,
                                        gpointer         user_data,
                                        GDestroyNotify   destroy);

G_END_DECLS

#endif /* __UBER_HEAT_MAP_H__ */
