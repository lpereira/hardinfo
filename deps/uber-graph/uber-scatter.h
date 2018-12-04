/* uber-scatter.h
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

#ifndef __UBER_SCATTER_H__
#define __UBER_SCATTER_H__

#include "uber-graph.h"

G_BEGIN_DECLS

#define UBER_TYPE_SCATTER            (uber_scatter_get_type())
#define UBER_SCATTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_SCATTER, UberScatter))
#define UBER_SCATTER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_SCATTER, UberScatter const))
#define UBER_SCATTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  UBER_TYPE_SCATTER, UberScatterClass))
#define UBER_IS_SCATTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UBER_TYPE_SCATTER))
#define UBER_IS_SCATTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  UBER_TYPE_SCATTER))
#define UBER_SCATTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  UBER_TYPE_SCATTER, UberScatterClass))

typedef struct _UberScatter        UberScatter;
typedef struct _UberScatterClass   UberScatterClass;
typedef struct _UberScatterPrivate UberScatterPrivate;

typedef gboolean (*UberScatterFunc) (UberScatter  *scatter,
                                     GArray      **values,
                                     gpointer      user_data);

struct _UberScatter
{
	UberGraph parent;

	/*< private >*/
	UberScatterPrivate *priv;
};

struct _UberScatterClass
{
	UberGraphClass parent_class;
};

GType      uber_scatter_get_type      (void) G_GNUC_CONST;
GtkWidget* uber_scatter_new           (void);
void       uber_scatter_set_fg_color  (UberScatter     *scatter,
                                       const GdkRGBA   *color);
void       uber_scatter_set_data_func (UberScatter     *scatter,
                                       UberScatterFunc  func,
                                       gpointer         user_data,
                                       GDestroyNotify   destroy);

G_END_DECLS

#endif /* __UBER_SCATTER_H__ */
