/* uber-label.h
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

#ifndef __UBER_LABEL_H__
#define __UBER_LABEL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UBER_TYPE_LABEL            (uber_label_get_type())
#define UBER_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_LABEL, UberLabel))
#define UBER_LABEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), UBER_TYPE_LABEL, UberLabel const))
#define UBER_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  UBER_TYPE_LABEL, UberLabelClass))
#define UBER_IS_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UBER_TYPE_LABEL))
#define UBER_IS_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  UBER_TYPE_LABEL))
#define UBER_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  UBER_TYPE_LABEL, UberLabelClass))

typedef struct _UberLabel        UberLabel;
typedef struct _UberLabelClass   UberLabelClass;
typedef struct _UberLabelPrivate UberLabelPrivate;

struct _UberLabel
{
	GtkAlignment parent;

	/*< private >*/
	UberLabelPrivate *priv;
};

struct _UberLabelClass
{
	GtkAlignmentClass parent_class;
};

GType      uber_label_get_type   (void) G_GNUC_CONST;
GtkWidget* uber_label_new        (void);
void       uber_label_set_color  (UberLabel      *label,
                                  const GdkRGBA  *color);
void       uber_label_set_text   (UberLabel      *label,
                                  const gchar    *text);

G_END_DECLS

#endif /* __UBER_LABEL_H__ */
