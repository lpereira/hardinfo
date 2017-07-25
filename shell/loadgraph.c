/*
 * Simple Load Graph
 * Version 0.1 - Wed, Jan 11 2006
 *   - initial release
 * Version 0.1.1 - Fri, Jan 13 2006
 *   - fixes autoscaling
 *   - add color
 *
 * Copyright (C) 2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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

static void _draw(LoadGraph * lg);

LoadGraph *load_graph_new(gint size)
{
    LoadGraph *lg;

    lg = g_new0(LoadGraph, 1);

    size++;

    lg->suffix = g_strdup("");
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

    _draw(lg);
}

void load_graph_set_color(LoadGraph * lg, LoadGraphColor color)
{
    lg->color = color;
#if GTK_CHECK_VERSION(3, 0, 0)
#define UNPACK_COLOR(C) (((C)>>16) & 0xff), (((C)>>8) & 0xff), ((C) & 0xff)
    cairo_set_source_rgb(lg->trace, UNPACK_COLOR(lg->color) );
    cairo_set_source_rgb(lg->fill, UNPACK_COLOR(lg->color - 0x303030) );
    cairo_set_source_rgb(lg->grid, UNPACK_COLOR(lg->color - 0xcdcdcd) );
#else
    gdk_rgb_gc_set_foreground(lg->trace, lg->color);
    gdk_rgb_gc_set_foreground(lg->fill, lg->color - 0x303030);
    gdk_rgb_gc_set_foreground(lg->grid, lg->color - 0xcdcdcd);
#endif
}

void load_graph_destroy(LoadGraph * lg)
{
#if GTK_CHECK_VERSION(3, 0, 0)
    g_object_unref(lg->buf);
#else
    gdk_pixmap_unref(lg->buf);
#endif
    g_object_unref(lg->trace);
    g_object_unref(lg->grid);
    g_object_unref(lg->fill);
    g_object_unref(lg->layout);
    gtk_widget_destroy(lg->area);
    g_free(lg->data);
    g_free(lg);
}

static gboolean _expose(GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
    LoadGraph *lg = (LoadGraph *) user_data;
#if GTK_CHECK_VERSION(3, 0, 0)
    /* TODO:GTK3 copy from lg->buf or lg->area? to widget? */
#else
    GdkDrawable *draw = GDK_DRAWABLE(lg->buf);
    gdk_draw_drawable(lg->area->window,
            lg->area->style->black_gc,
            draw, 0, 0, 0, 0, lg->width, lg->height);
#endif
    return FALSE;
}

void load_graph_configure_expose(LoadGraph * lg)
{
    /* creates the backing store pixmap */
    gtk_widget_realize(lg->area);
#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_content_t content;
    GdkWindow *gdk_window = gtk_widget_get_window(lg->area);
    lg->buf = gdk_window_create_similar_surface(gdk_window, CAIRO_CONTENT_COLOR, lg->width, lg->height);
    lg->grid = cairo_create(lg->buf);
    lg->trace = cairo_create(lg->buf);
    lg->fill = cairo_create(lg->buf);
#else
    lg->buf = gdk_pixmap_new(lg->area->window, lg->width, lg->height, -1);
    /* create the graphic contexts */
    lg->grid = gdk_gc_new(GDK_DRAWABLE(lg->buf));
    lg->trace = gdk_gc_new(GDK_DRAWABLE(lg->buf));
    lg->fill = gdk_gc_new(GDK_DRAWABLE(lg->buf));
#endif

    /* the default color is green */
    load_graph_set_color(lg, LG_COLOR_GREEN);

    /* init graphic contexts */
#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_set_line_width(lg->grid, 1);
    cairo_set_line_cap(lg->grid, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(lg->grid, CAIRO_LINE_JOIN_MITER);
    cairo_set_dash(lg->grid, 0, (gint8*)"\2\2", 2);
#else
    gdk_gc_set_line_attributes(lg->grid,
               1, GDK_LINE_ON_OFF_DASH,
               GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);
    gdk_gc_set_dashes(lg->grid, 0, (gint8*)"\2\2", 2);
#endif

#if 0    /* old-style grid */
    gdk_rgb_gc_set_foreground(lg->grid, 0x707070);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_set_line_width(lg->trace, 1);
    cairo_set_line_cap(lg->trace, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(lg->trace, CAIRO_LINE_JOIN_MITER);
#else
    gdk_gc_set_line_attributes(lg->trace,
                   1, GDK_LINE_SOLID,
                   GDK_CAP_PROJECTING, GDK_JOIN_ROUND);
#endif

#if 0 /* old-style fill */
    gdk_gc_set_line_attributes(lg->fill,
                   1, GDK_LINE_SOLID,
                   GDK_CAP_BUTT, GDK_JOIN_BEVEL);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    /* configures the draw event */
    g_signal_connect(G_OBJECT(lg->area), "draw",
        (GCallback) _expose, lg);
#else
    /* configures the expose event */
    g_signal_connect(G_OBJECT(lg->area), "expose-event",
        (GCallback) _expose, lg);
#endif
}

#if GTK_CHECK_VERSION(3, 0, 0)
#define _draw_line(D, CR, X1, Y1, X2, Y2) \
    cairo_move_to(CR, X1, Y1); \
    cairo_line_to(CR, X2, Y2);
#else
#define _draw_line(D, GC, X1, Y1, X2, Y2) gdk_draw_line(D, GC, X1, Y1, X2, Y2)
#endif

static void _draw_label_and_line(LoadGraph * lg, gint position, gint value)
{
    gchar *tmp;

    /* draw lines */
    if (position > 0) {
        _draw_line(GDK_DRAWABLE(lg->buf), lg->grid, 0, position,
            lg->width, position);
    } else
        position = -1 * position;

    /* draw label */
    tmp =
        g_strdup_printf("<span size=\"x-small\">%d%s</span>", value,
            lg->suffix);

    pango_layout_set_markup(lg->layout, tmp, -1);
#if GTK_CHECK_VERSION(3, 0, 0)
    pango_layout_set_width(lg->layout,
                lg->width * PANGO_SCALE);
    gtk_widget_create_pango_layout(lg->area, NULL);
#else
    pango_layout_set_width(lg->layout,
                lg->area->allocation.width * PANGO_SCALE);
    gdk_draw_layout(GDK_DRAWABLE(lg->buf), lg->trace, 2, position,
                lg->layout);
#endif

    g_free(tmp);
}

static void _draw(LoadGraph * lg)
{
#if GTK_CHECK_VERSION(3, 0, 0)
    void *draw = NULL; /* not used by cairo */
#else
    GdkDrawable *draw = GDK_DRAWABLE(lg->buf);
#endif
    gint i, d;

    /* clears the drawing area */
#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_rectangle(lg->fill, 0, 0, lg->width, lg->height);
    cairo_fill (lg->fill);
#else
    gdk_draw_rectangle(draw, lg->area->style->black_gc,
                TRUE, 0, 0, lg->width, lg->height);
#endif


    /* the graph */
    GdkPoint *points = g_new0(GdkPoint, lg->size + 1);

    for (i = 0; i < lg->size; i++) {
        points[i].x = i * 4;
        points[i].y = lg->height - lg->data[i] * lg->scale;
    }

    points[0].x = points[1].x = 0;
    points[0].y = points[i].y = lg->height;
    points[i].x = points[i - 1].x = lg->width;

#if GTK_CHECK_VERSION(3, 0, 0)
    /* TODO:GTK3 draw using loop and _draw_line() */
#else
    gdk_draw_polygon(draw, lg->fill, TRUE, points, lg->size + 1);
    gdk_draw_polygon(draw, lg->trace, FALSE, points, lg->size + 1);
#endif

    g_free(points);

    /* vertical bars */
    for (i = lg->width, d = 0; i > 1; i--, d++)
        if ((d % 45) == 0 && d) {
            _draw_line(draw, lg->grid, i, 0, i, lg->height);
        }

    /* horizontal bars and labels; 25%, 50% and 75% */
    _draw_label_and_line(lg, -1, lg->max_value);
    _draw_label_and_line(lg, lg->height / 4, 3 * (lg->max_value / 4));
    _draw_label_and_line(lg, lg->height / 2, lg->max_value / 2);
    _draw_label_and_line(lg, 3 * (lg->height / 4), lg->max_value / 4);

#if 0  /* old-style drawing */
    for (i = 0; i < lg->size; i++) {
        gint this = lg->height - lg->data[i] * lg->scale;
        gint next = lg->height - lg->data[i + 1] * lg->scale;
        gint i4 = i * 4;

        _draw_line(draw, lg->fill, i4, this, i4, lg->height);
        _draw_line(draw, lg->fill, i4 + 2, this, i4 + 2, lg->height);
    }

    for (i = 0; i < lg->size; i++) {
        gint this = lg->height - lg->data[i] * lg->scale;
        gint next = lg->height - lg->data[i + 1] * lg->scale;
        gint i4 = i * 4;

        _draw_line(draw, lg->trace, i4, this, i4 + 2,
                  (this + next) / 2);
        _draw_line(draw, lg->trace, i4 + 2, (this + next) / 2,
                  i4 + 4, next);
    }
#endif

    gtk_widget_queue_draw(lg->area);
}

void load_graph_update(LoadGraph * lg, gint value)
{
    gint i;

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

#ifdef LOADGRAPH_UNIT_TEST
gboolean lg_update(gpointer d)
{
    LoadGraph *lg = (LoadGraph *) d;

    static int i = 0;
    static int j = 1;

    if (i > 150) {
        j = -1;
    } else if (i < 0) {
        j = 1;
    }

    i += j;
    if (rand() % 10 > 8)
        i *= 2;
    if (rand() % 10 < 2)
        i /= 2;
    load_graph_update(lg, i + rand() % 50);

    return TRUE;
}

int main(int argc, char **argv)
{
    LoadGraph *lg;
    GtkWidget *window;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_show(window);

    lg = load_graph_new(50);
    gtk_container_add(GTK_CONTAINER(window), load_graph_get_framed(lg));
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    load_graph_configure_expose(lg);

    lg_update(lg);

    g_timeout_add(100, lg_update, lg);

    gtk_main();

    return 0;
}
#endif
