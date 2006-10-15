/*
 * Simple Load Graph
 * Version 0.1 - Wed, Jan 11 2006
 *   - initial release
 * Version 0.1.1 - Fri, Jan 13 2006
 *   - fixes autoscaling
 *   - add color
 *
 * Copyright (C) 2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

static void _draw(LoadGraph *lg);

LoadGraph *load_graph_new(gint size)
{
    LoadGraph *lg;
    
    lg = g_new0(LoadGraph, 1);
    
    lg->area = gtk_drawing_area_new();
    lg->size = size;
    lg->data = g_new0(gint, size);

    lg->scale = 1.0;
    
    lg->width  = size * 4;
    lg->height = size * 2;
    
    lg->max_value = -1;

    gtk_widget_set_size_request(lg->area, lg->width, lg->height);
    gtk_widget_show(lg->area);
    
    return lg;
}

int load_graph_get_max(LoadGraph *lg)
{
    return lg->max_value;
}

void load_graph_set_max(LoadGraph *lg, gint value)
{
    lg->max_value = value;
}

GtkWidget *load_graph_get_framed(LoadGraph *lg)
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

void load_graph_clear(LoadGraph *lg)
{
    gint i;
    
    for (i = 0; i < lg->size; i++)
        lg->data[i] = 0;

    lg->scale = 1.0;
//    lg->max_value = -1;
    _draw(lg);
}

void load_graph_set_color(LoadGraph *lg, LoadGraphColor color)
{
    lg->color = color;
    gdk_rgb_gc_set_foreground(lg->trace, lg->color);
}

void load_graph_destroy(LoadGraph *lg)
{
    g_free(lg->data);
    gtk_widget_destroy(lg->area);
    gdk_pixmap_unref(lg->buf);
    g_object_unref(lg->trace);
    g_object_unref(lg->grid);
    g_free(lg);
}

static gboolean _expose(GtkWidget *widget, GdkEventExpose *event,
                    gpointer user_data)
{
    LoadGraph *lg = (LoadGraph *)user_data;
    GdkDrawable *draw = GDK_DRAWABLE(lg->buf);

    gdk_draw_drawable(lg->area->window,
                      lg->area->style->white_gc,
                      draw,
                      0, 0,
                      0, 0,
                      lg->width, lg->height);
    return FALSE;
}                    

void load_graph_configure_expose(LoadGraph *lg)
{
    /* creates the backing store pixmap */
    gtk_widget_realize(lg->area);
    lg->buf = gdk_pixmap_new(lg->area->window, lg->width, lg->height, -1);

    /* create the graphic contexts */
    lg->grid = gdk_gc_new(GDK_DRAWABLE(lg->buf));
    lg->trace = gdk_gc_new(GDK_DRAWABLE(lg->buf));

    /* the default color is green */
    load_graph_set_color(lg, LG_COLOR_GREEN);

    /* init graphic contexts */
    gdk_gc_set_line_attributes(lg->grid, 
                               1, GDK_LINE_ON_OFF_DASH,
                               GDK_CAP_NOT_LAST,
                               GDK_JOIN_ROUND);
    gdk_rgb_gc_set_foreground(lg->grid, 0x707070);

    gdk_gc_set_line_attributes(lg->trace, 
                               2, GDK_LINE_SOLID,
                               GDK_CAP_NOT_LAST,
                               GDK_JOIN_ROUND);

    /* configures the expose event */
    g_signal_connect(G_OBJECT(lg->area), "expose-event", 
                     (GCallback) _expose, lg);
}

static void
_draw(LoadGraph *lg)
{
    GdkDrawable *draw = GDK_DRAWABLE(lg->buf);
    gint i, d;
    
    /* clears the drawing area */
    gdk_draw_rectangle(draw, lg->area->style->black_gc,
                       TRUE, 0, 0, lg->width, lg->height);
                      
    /* horizontal bars; 25%, 50% and 75% */ 
    d = lg->height / 4;
    gdk_draw_line(draw, lg->grid, 0, d, lg->width, d);
    d = lg->height / 2;
    gdk_draw_line(draw, lg->grid, 0, d, lg->width, d);
    d = 3 * (lg->height / 4);
    gdk_draw_line(draw, lg->grid, 0, d, lg->width, d);

    /* vertical bars */
    for (i = lg->width, d = 0; i > 1; i--, d++)
        if ((d % 45) == 0 && d)
            gdk_draw_line(draw, lg->grid, i, 0, i, lg->height);

    /* the graph */
    for (i = 0; i < lg->size; i++) {    
          gint this = lg->height - lg->data[i] * lg->scale;
          gint next = lg->height - lg->data[i+1] * lg->scale;
    
          gdk_draw_line(draw, lg->trace, i * 4, this, i * 4 + 2,
                        (this + next) / 2);
          gdk_draw_line(draw, lg->trace, i * 4 + 2, (this + next) / 2,
                        i * 4 + 4, next); 
    }
    
    gtk_widget_queue_draw(lg->area);
}

static inline int
_max(LoadGraph *lg)
{
    gint i;
    gint max = 1.0;
    
    for (i = 0; i < lg->size; i++) {
        if (lg->data[i] > max)
            max = lg->data[i];
    }
    
    return max;
}

void
load_graph_update(LoadGraph *lg, gint value)
{
    gint i;
    
    if (value < 0)
        return;
    
    if (lg->max_value > 0) {
      lg->scale = (gfloat)lg->height / (gfloat)_max(lg);
    } else {
      lg->scale = (gfloat)lg->height / (gfloat)lg->max_value;
      
      g_print("using max value %d; scale is %f\n", lg->max_value, lg->scale);
    }

    /* shift-right our data */
    for (i = 0; i < lg->size; i++) {
        lg->data[i] = lg->data[i+1];
    }
    
    /* insert the updated value */
    lg->data[i] = value;
    
    /* redraw */
    _draw(lg);
}

#ifdef LOADGRAPH_UNIT_TEST
gboolean lg_update(gpointer d)
{
    LoadGraph *lg = (LoadGraph *)d;
    
    static int i = 0;
    static int j = 1;
    
    if (i > 150) {
        j = -1;
    } else if (i < 0) {
        j = 1;
    }

    i += j;
    if (rand() % 10 > 8) i*= 2;
    if (rand() % 10 < 2) i/= 2;
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
    
    lg = load_graph_new(200);
    gtk_container_add(GTK_CONTAINER(window), load_graph_get_framed(lg));
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    load_graph_configure_expose(lg);
    
    lg_update(lg);
    
    g_timeout_add(100, lg_update, lg);
    
    gtk_main();
    
    return 0;
}
#endif
