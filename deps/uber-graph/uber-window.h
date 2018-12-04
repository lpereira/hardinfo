/* uber-window.h
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

#ifndef __UBER_WINDOW_H__
#define __UBER_WINDOW_H__

#include <gtk/gtk.h>

#include "uber-graph.h"

G_BEGIN_DECLS

#define UBER_TYPE_WINDOW            (uber_window_get_type())
#define UBER_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_WINDOW, UberWindow))
#define UBER_WINDOW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_WINDOW, UberWindow const))
#define UBER_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  UBER_TYPE_WINDOW, UberWindowClass))
#define UBER_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UBER_TYPE_WINDOW))
#define UBER_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  UBER_TYPE_WINDOW))
#define UBER_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  UBER_TYPE_WINDOW, UberWindowClass))

typedef struct _UberWindow        UberWindow;
typedef struct _UberWindowClass   UberWindowClass;
typedef struct _UberWindowPrivate UberWindowPrivate;

struct _UberWindow
{
	GtkWindow parent;

	/*< private >*/
	UberWindowPrivate *priv;
};

struct _UberWindowClass
{
	GtkWindowClass parent_class;
};

GType      uber_window_get_type    (void) G_GNUC_CONST;
GtkWidget* uber_window_new         (void);
void       uber_window_add_graph   (UberWindow  *window,
                                    UberGraph   *graph,
                                    const gchar *title);
void       uber_window_show_labels (UberWindow  *window,
                                    UberGraph   *graph);
void       uber_window_hide_labels (UberWindow  *window,
                                    UberGraph   *graph);

G_END_DECLS

#endif /* __UBER_WINDOW_H__ */
