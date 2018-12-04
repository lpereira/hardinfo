/* uber-scatter.c
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

#include <math.h>
#include <string.h>

#include "uber-scatter.h"
#include "uber-scale.h"
#include "uber-range.h"
#include "g-ring.h"

#define RADIUS 3

/**
 * SECTION:uber-scatter.h
 * @title: UberScatter
 * @short_description:
 *
 * Section overview.
 */

G_DEFINE_TYPE(UberScatter, uber_scatter, UBER_TYPE_GRAPH)

struct _UberScatterPrivate
{
	GRing           *raw_data;
	UberRange        range;
	gint             stride;
	GdkRGBA          fg_color;
	gboolean         fg_color_set;
	UberScatterFunc  func;
	gpointer         func_user_data;
	GDestroyNotify   func_destroy;
};

/**
 * uber_scatter_new:
 *
 * Creates a new instance of #UberScatter.
 *
 * Returns: the newly created instance of #UberScatter.
 * Side effects: None.
 */
GtkWidget*
uber_scatter_new (void)
{
	UberScatter *scatter;

	scatter = g_object_new(UBER_TYPE_SCATTER, NULL);
	return GTK_WIDGET(scatter);
}

/**
 * uber_scatter_set_data_func:
 * @scatter: A #UberScatter.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_scatter_set_data_func (UberScatter *scatter,       /* IN */
                            UberScatterFunc  func,      /* IN */
                            gpointer         user_data, /* IN */
                            GDestroyNotify   destroy)   /* IN */
{
	UberScatterPrivate *priv;

	g_return_if_fail(UBER_IS_SCATTER(scatter));
	g_return_if_fail(func != NULL);

	priv = scatter->priv;
	/*
	 * Cleanup previous data func if necessary.
	 */
	if (priv->func_destroy) {
		priv->func_destroy(priv->func_user_data);
	}
	priv->func = func;
	priv->func_destroy = destroy;
	priv->func_user_data = user_data;
}

/**
 * uber_scatter_destroy_array:
 * @array: A #GArray.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_scatter_destroy_array (gpointer data) /* IN */
{
	GArray **ar = data;

	if (ar) {
		g_array_unref(*ar);
	}
}

/**
 * uber_scatter_set_stride:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_scatter_set_stride (UberGraph *graph,  /* IN */
                          guint      stride) /* IN */
{
	UberScatterPrivate *priv;

	g_return_if_fail(UBER_IS_SCATTER(graph));

	priv = UBER_SCATTER(graph)->priv;
	if (priv->stride == stride) {
		return;
	}
	priv->stride = stride;
	if (priv->raw_data) {
		g_ring_unref(priv->raw_data);
	}
	priv->raw_data = g_ring_sized_new(sizeof(GArray*), stride,
	                                  uber_scatter_destroy_array);
}

/**
 * uber_scatter_render:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_scatter_render (UberGraph     *graph, /* IN */
                      cairo_t      *cr,    /* IN */
                      GdkRectangle *area,  /* IN */
                      guint         epoch, /* IN */
                      gfloat        each)  /* IN */
{
	UberScatterPrivate *priv;
	UberRange pixel_range;
	GtkStyleContext *style;
	GdkRGBA color;
	GArray *ar;
	gdouble x;
	gdouble y;
	gint i;
	gint j;

	g_return_if_fail(UBER_IS_SCATTER(graph));

	priv = UBER_SCATTER(graph)->priv;
	color = priv->fg_color;
	if (!priv->fg_color_set) {
		style = gtk_widget_get_style_context(GTK_WIDGET(graph));
		gtk_style_context_get_color(style, GTK_STATE_FLAG_SELECTED, &color);
	}
	/*
	 * Calculate ranges.
	 */
	pixel_range.begin = area->y + (RADIUS / 2.);
	pixel_range.end = area->y + area->height - RADIUS;
	pixel_range.range = pixel_range.end - pixel_range.begin;
	/*
	 * Retrieve the current data set.
	 */
	for (i = 0; i < priv->raw_data->len; i++) {
		if (!(ar = g_ring_get_index(priv->raw_data, GArray*, i))) {
			continue;
		}
		x = epoch - (i * each) - (each / 2.);
		for (j = 0; j < ar->len; j++) {
			y = g_array_index(ar, gdouble, j);
//			g_debug("Raw ==> %f", y);
			uber_scale_linear(&priv->range, &pixel_range, &y, NULL);
			/*
			 * Shadow.
			 */
			cairo_arc(cr, x + .5, y + .5, RADIUS, 0, 2 * M_PI);
			cairo_set_source_rgb(cr, .1, .1, .1);
			cairo_fill(cr);
			/*
			 * Foreground.
			 */
			cairo_arc(cr, x, y, RADIUS, 0, 2 * M_PI);
			cairo_set_source_rgb(cr,
			                     color.red,
			                     color.green,
			                     color.blue);
			cairo_fill(cr);
		}
	}
}

/**
 * uber_scatter_render_fast:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_scatter_render_fast (UberGraph    *graph, /* IN */
                          cairo_t      *cr,    /* IN */
                          GdkRectangle *area,  /* IN */
                          guint         epoch, /* IN */
                          gfloat        each)  /* IN */
{
	UberScatterPrivate *priv;
	UberRange pixel_range;
	GtkStyleContext *style;
	GdkRGBA color;
	GArray *ar;
	gdouble x;
	gdouble y;
	gint i;

	g_return_if_fail(UBER_IS_SCATTER(graph));

	priv = UBER_SCATTER(graph)->priv;
	color = priv->fg_color;
	if (!priv->fg_color_set) {
		style = gtk_widget_get_style_context(GTK_WIDGET(graph));
		gtk_style_context_get_color(style, GTK_STATE_FLAG_SELECTED, &color);
	}
	/*
	 * Calculate ranges.
	 */
	pixel_range.begin = area->y + (RADIUS / 2.);
	pixel_range.end = area->y + area->height - RADIUS;
	pixel_range.range = pixel_range.end - pixel_range.begin;
	/*
	 * Retrieve the current data set.
	 */
	ar = g_ring_get_index(priv->raw_data, GArray*, 0);
	if (!ar) {
		return;
	}
	/*
	 * Calculate X position (Center of this chunk).
	 */
	x = epoch - (each / 2.);
	/*
	 * Draw scatter dots.
	 */
	for (i = 0; i < ar->len; i++) {
		/*
		 * Scale the value to our graph coordinates.
		 */
		y = g_array_index(ar, gdouble, i);
		/*
		 * XXX: Support multiple scales.
		 */
		uber_scale_linear(&priv->range, &pixel_range, &y, NULL);
		/*
		 * Shadow.
		 */
		cairo_arc(cr, x + .5, y + .5, RADIUS, 0, 2 * M_PI);
		cairo_set_source_rgb(cr, .1, .1, .1);
		cairo_fill(cr);
		/*
		 * Foreground.
		 */
		cairo_arc(cr, x, y, RADIUS, 0, 2 * M_PI);
		cairo_set_source_rgb(cr,
		                     color.red,
		                     color.green,
		                     color.blue);
		cairo_fill(cr);
	}
}

/**
 * uber_scatter_get_next_data:
 * @graph: A #UberGraph.
 *
 * Retrieve the next data point for the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
uber_scatter_get_next_data (UberGraph *graph) /* IN */
{
	UberScatterPrivate *priv;
	GArray *array = NULL;

	g_return_val_if_fail(UBER_IS_SCATTER(graph), FALSE);

	priv = UBER_SCATTER(graph)->priv;
	if (priv->func) {
		if (!priv->func(UBER_SCATTER(graph), &array, priv->func_user_data)) {
			array = NULL;
		}
		g_ring_append_val(priv->raw_data, array);
		return TRUE;
	}
	return FALSE;
}

/**
 * uber_scatter_set_fg_color:
 * @scatter: A #UberScatter.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_scatter_set_fg_color (UberScatter    *scatter, /* IN */
                           const GdkRGBA *color)   /* IN */
{
	UberScatterPrivate *priv;

	g_return_if_fail(UBER_IS_SCATTER(scatter));

	priv = scatter->priv;
	if (color) {
		priv->fg_color = *color;
		priv->fg_color_set = TRUE;
	} else {
		memset(&priv->fg_color, 0, sizeof(priv->fg_color));
		priv->fg_color_set = FALSE;
	}
}

/**
 * uber_scatter_finalize:
 * @object: A #UberScatter.
 *
 * Finalizer for a #UberScatter instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_scatter_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(uber_scatter_parent_class)->finalize(object);
}

/**
 * uber_scatter_class_init:
 * @klass: A #UberScatterClass.
 *
 * Initializes the #UberScatterClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_scatter_class_init (UberScatterClass *klass) /* IN */
{
	GObjectClass *object_class;
	UberGraphClass *graph_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = uber_scatter_finalize;
	g_type_class_add_private(object_class, sizeof(UberScatterPrivate));

	graph_class = UBER_GRAPH_CLASS(klass);
	graph_class->render = uber_scatter_render;
	graph_class->render_fast = uber_scatter_render_fast;
	graph_class->set_stride = uber_scatter_set_stride;
	graph_class->get_next_data = uber_scatter_get_next_data;
}

/**
 * uber_scatter_init:
 * @scatter: A #UberScatter.
 *
 * Initializes the newly created #UberScatter instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_scatter_init (UberScatter *scatter) /* IN */
{
	UberScatterPrivate *priv;

	scatter->priv = G_TYPE_INSTANCE_GET_PRIVATE(scatter,
	                                            UBER_TYPE_SCATTER,
	                                            UberScatterPrivate);
	priv = scatter->priv;

	priv->range.begin = 0.;
	priv->range.end = 15000.;
	priv->range.range = priv->range.end - priv->range.begin;
}
