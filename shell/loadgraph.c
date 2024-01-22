/*
 * Simple Load Graph
 * Version 0.1 - Wed, Jan 11 2006
 *   - initial release
 * Version 0.1.1 - Fri, Jan 13 2006
 *   - fixes autoscaling
 *   - add color
 *
 * Copyright (C) 2006 L. A. F. Pereira <l@tia.mat.br>
 *
 * The Simple Load Graph is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License, version 2.1, as published by the Free Software Foundation.
 *
 * The Simple Load Graph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Simple Load Graph; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.
 */

#include "loadgraph.h"

struct _LoadGraph {
    GdkPixmap     *buf;
    GdkGC         *grid;
    GdkGC         *trace;
    GdkGC         *fill;
    GtkWidget     *area;

    gint          *data;
    gfloat         scale;

    gint       size;
    gint       width, height;
    LoadGraphColor color;

    gint       max_value, remax_count;

    PangoLayout   *layout;
    gchar     *suffix;
    gchar *title;
};

static void _draw(LoadGraph * lg);

LoadGraph *load_graph_new(gint size)
{
    LoadGraph *lg;

    lg = g_new0(LoadGraph, 1);

    size++;

    lg->suffix = g_strdup("");
    lg->title = g_strdup("");
    lg->area = gtk_drawing_area_new();
    lg->size = (size * 3) / 2;
    lg->data = g_new0(gint, lg->size);

    lg->scale = 1.0;

    lg->width = size * 6;
    lg->height = size * 2;

    lg->max_value = 1;
    lg->remax_count = 0;

    lg->layout = pango_layout_new(gtk_widget_get_pango_context(lg->area));

    gtk_widget_set_size_request(lg->area, lg->width, lg->height);
    gtk_widget_show(lg->area);

    return lg;
}

void load_graph_set_data_suffix(LoadGraph * lg, gchar * suffix)
{
    g_free(lg->suffix);
    lg->suffix = g_strdup(suffix);
}

gchar *load_graph_get_data_suffix(LoadGraph * lg)
{
    return lg->suffix;
}

void load_graph_set_title(LoadGraph * lg, const gchar * title)
{
    g_free(lg->title);
    lg->title = g_strdup(title);
}

const gchar *load_graph_get_title(LoadGraph *lg)
{
    if (lg != NULL) return lg->title;
    return NULL;
}

GtkWidget *load_graph_get_framed(LoadGraph * lg)
{
    GtkWidget *align, *frame;

    align = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_show(align);

    frame = gtk_frame_new(NULL);
    gtk_widget_show(frame);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    gtk_container_add(GTK_CONTAINER(align), frame);
    gtk_container_add(GTK_CONTAINER(frame), lg->area);

    return align;
}

void load_graph_clear(LoadGraph * lg)
{
    gint i;

    for (i = 0; i < lg->size; i++)
        lg->data[i] = 0;

    lg->scale = 1.0;
    lg->max_value = 1;
    lg->remax_count = 0;

    load_graph_set_title(lg, "");

    _draw(lg);
}

void load_graph_set_color(LoadGraph * lg, LoadGraphColor color)
{
    lg->color = color;
    gdk_rgb_gc_set_foreground(lg->trace, lg->color);
    gdk_rgb_gc_set_foreground(lg->fill, lg->color - 0x303030);
    gdk_rgb_gc_set_foreground(lg->grid, lg->color - 0xcdcdcd);
}

void load_graph_destroy(LoadGraph * lg)
{
    g_free(lg->data);
    gtk_widget_destroy(lg->area);
    gdk_pixmap_unref(lg->buf);
    g_object_unref(lg->trace);
    g_object_unref(lg->grid);
    g_object_unref(lg->fill);
    g_object_unref(lg->layout);
    g_free(lg);
}

static gboolean _expose(GtkWidget * widget, GdkEventExpose * event,
            gpointer user_data)
{
    LoadGraph *lg = (LoadGraph *) user_data;
    GdkDrawable *draw = GDK_DRAWABLE(lg->buf);

    gdk_draw_drawable(lg->area->window,
              lg->area->style->black_gc,
              draw, 0, 0, 0, 0, lg->width, lg->height);
    return FALSE;
}

void load_graph_configure_expose(LoadGraph * lg)
{
    /* creates the backing store pixmap */
    gtk_widget_realize(lg->area);
    lg->buf = gdk_pixmap_new(lg->area->window, lg->width, lg->height, -1);

    /* create the graphic contexts */
    lg->grid = gdk_gc_new(GDK_DRAWABLE(lg->buf));
    lg->trace = gdk_gc_new(GDK_DRAWABLE(lg->buf));
    lg->fill = gdk_gc_new(GDK_DRAWABLE(lg->buf));

    /* the default color is green */
    load_graph_set_color(lg, LG_COLOR_GREEN);

    /* init graphic contexts */
    gdk_gc_set_line_attributes(lg->grid,
                   1, GDK_LINE_ON_OFF_DASH,
                   GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);
    gdk_gc_set_dashes(lg->grid, 0, (gint8*)"\2\2", 2);

    gdk_gc_set_line_attributes(lg->trace,
                   1, GDK_LINE_SOLID,
                   GDK_CAP_PROJECTING, GDK_JOIN_ROUND);

    /* configures the expose event */
    g_signal_connect(G_OBJECT(lg->area), "expose-event",
             (GCallback) _expose, lg);
}

static void _draw_title(LoadGraph * lg, const char* title) {
    gchar *tmp = g_strdup_printf("<span size=\"x-small\">%s</span>", title);
    pango_layout_set_markup(lg->layout, tmp, -1);
    int width = 0;
    int height = 0;
    pango_layout_get_pixel_size(lg->layout, &width, &height);
    gint position = (lg->width / 2) - (width / 2);
    gdk_draw_layout(GDK_DRAWABLE(lg->buf), lg->trace, position, 2,
                    lg->layout);
    g_free(tmp);
}

static void _draw_label_and_line(LoadGraph * lg, gint position, gint value)
{
    gchar *tmp;

    /* draw lines */
    if (position > 0)
        gdk_draw_line(GDK_DRAWABLE(lg->buf), lg->grid, 0, position,
                  lg->width, position);
    else
        position = -1 * position;

    /* draw label */
    tmp =
    g_strdup_printf("<span size=\"x-small\">%d%s</span>", value,
            lg->suffix);

    pango_layout_set_markup(lg->layout, tmp, -1);
    pango_layout_set_width(lg->layout,
               lg->area->allocation.width * PANGO_SCALE);
    gdk_draw_layout(GDK_DRAWABLE(lg->buf), lg->trace, 2, position,
            lg->layout);

    g_free(tmp);
}

static void _draw(LoadGraph * lg)
{
    GdkDrawable *draw = GDK_DRAWABLE(lg->buf);
    gint i, d;

    /* clears the drawing area */
    gdk_draw_rectangle(draw, lg->area->style->black_gc,
               TRUE, 0, 0, lg->width, lg->height);


    /* the graph */
    GdkPoint *points = g_new0(GdkPoint, lg->size + 1);

    for (i = 0; i < lg->size; i++) {
        points[i].x = i * 4;
        points[i].y = lg->height - lg->data[i] * lg->scale;
    }

    points[0].x = points[1].x = 0;
    points[0].y = points[i].y = lg->height;
    points[i].x = points[i - 1].x = lg->width;

    gdk_draw_polygon(draw, lg->fill, TRUE, points, lg->size + 1);
    gdk_draw_polygon(draw, lg->trace, FALSE, points, lg->size + 1);

    g_free(points);

    /* vertical bars */
    for (i = lg->width, d = 0; i > 1; i--, d++)
        if ((d % 45) == 0 && d)
            gdk_draw_line(draw, lg->grid, i, 0, i, lg->height);

    /* horizontal bars and labels; 25%, 50% and 75% */
    _draw_label_and_line(lg, -1, lg->max_value);
    _draw_label_and_line(lg, lg->height / 4, 3 * (lg->max_value / 4));
    _draw_label_and_line(lg, lg->height / 2, lg->max_value / 2);
    _draw_label_and_line(lg, 3 * (lg->height / 4), lg->max_value / 4);

    /* graph title */
    _draw_title(lg, lg->title);

    gtk_widget_queue_draw(lg->area);
}

void load_graph_update_ex(LoadGraph *lg, guint line, gdouble value)
{
    /* not implemented */
    if (line == 0)
        load_graph_update(lg, value);
}

void load_graph_update(LoadGraph * lg, gdouble v)
{
    gint i;
    gint value = (gint)v;

    if (value < 0)
        return;

    /* shift-right our data */
    for (i = 0; i < lg->size - 1; i++) {
        lg->data[i] = lg->data[i + 1];
    }

    /* insert the updated value */
    lg->data[i] = value;

    /* calculates the maximum value */
    if (lg->remax_count++ > 20) {
        /* only finds the maximum amongst the data every 20 times */
        lg->remax_count = 0;

        gint max = lg->data[0];
        for (i = 1; i < lg->size; i++) {
            if (lg->data[i] > max)
            max = lg->data[i];
        }

        lg->max_value = max;
    } else {
        /* otherwise, select the maximum between the current maximum
           and the supplied value */
        lg->max_value = MAX(value, lg->max_value);
    }

    /* recalculates the scale; always use 90% of it */
    lg->scale = 0.90 * ((gfloat) lg->height / (gfloat) lg->max_value);

    /* redraw */
    _draw(lg);
}

gint load_graph_get_height(LoadGraph *lg) {
    if (lg != NULL)
        return lg->height;
    return 0;
}
