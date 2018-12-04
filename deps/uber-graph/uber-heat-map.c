/* uber-heat-map.c
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

#include <string.h>

#include "uber-heat-map.h"
#include "g-ring.h"

/**
 * SECTION:uber-heat-map.h
 * @title: UberHeatMap
 * @short_description:
 *
 * Section overview.
 */

G_DEFINE_TYPE(UberHeatMap, uber_heat_map, UBER_TYPE_GRAPH)

struct _UberHeatMapPrivate
{
	GRing           *raw_data;
	gboolean         fg_color_set;
	GdkRGBA         fg_color;
	UberHeatMapFunc  func;
	GDestroyNotify   func_destroy;
	gpointer         func_user_data;
};

/**
 * uber_heat_map_new:
 *
 * Creates a new instance of #UberHeatMap.
 *
 * Returns: the newly created instance of #UberHeatMap.
 * Side effects: None.
 */
GtkWidget*
uber_heat_map_new (void)
{
	UberHeatMap *map;

	map = g_object_new(UBER_TYPE_HEAT_MAP, NULL);
	return GTK_WIDGET(map);
}

/**
 * uber_heat_map_destroy_array:
 * @array: A #GArray.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_heat_map_destroy_array (gpointer data) /* IN */
{
	GArray **ar = data;

	if (ar) {
		g_array_unref(*ar);
	}
}

/**
 * uber_heat_map_set_stride:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_heat_map_set_stride (UberGraph *graph,  /* IN */
                          guint      stride) /* IN */
{
	UberHeatMapPrivate *priv;

	g_return_if_fail(UBER_IS_HEAT_MAP(graph));

	priv = UBER_HEAT_MAP(graph)->priv;
	if (priv->raw_data) {
		g_ring_unref(priv->raw_data);
	}
	priv->raw_data = g_ring_sized_new(sizeof(GArray*), stride,
	                                  uber_heat_map_destroy_array);
}

/**
 * uber_heat_map_set_data_func:
 * @map: A #UberHeatMap.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_heat_map_set_data_func (UberHeatMap     *map,       /* IN */
                             UberHeatMapFunc  func,      /* IN */
                             gpointer         user_data, /* IN */
                             GDestroyNotify   destroy)   /* IN */
{
	UberHeatMapPrivate *priv;

	g_return_if_fail(UBER_IS_HEAT_MAP(map));

	priv = map->priv;
	if (priv->func_destroy) {
		priv->func_destroy(priv->func_user_data);
	}
	priv->func = func;
	priv->func_destroy = destroy;
	priv->func_user_data = user_data;
}

/**
 * uber_heat_map_render:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_heat_map_render (UberGraph    *graph, /* IN */
                      cairo_t      *cr,    /* IN */
                      GdkRectangle *area,  /* IN */
                      guint         epoch, /* IN */
                      gfloat        each)  /* IN */
{
#if 0
	UberGraphPrivate *priv;
	cairo_pattern_t *cp;

	g_return_if_fail(UBER_IS_HEAT_MAP(graph));

	priv = graph->priv;
	/*
	 * XXX: Temporarily draw a nice little gradient to test sliding.
	 */
	cp = cairo_pattern_create_linear(0, 0, area->width, 0);
	cairo_pattern_add_color_stop_rgb(cp, 0, .1, .1, .1);
	cairo_pattern_add_color_stop_rgb(cp, .2, .3, .3, .5);
	cairo_pattern_add_color_stop_rgb(cp, .4, .2, .7, .4);
	cairo_pattern_add_color_stop_rgb(cp, .7, .6, .2, .1);
	cairo_pattern_add_color_stop_rgb(cp, .8, .6, .8, .1);
	cairo_pattern_add_color_stop_rgb(cp, 1., .3, .8, .5);
	gdk_cairo_rectangle(cr, area);
	cairo_set_source(cr, cp);
	cairo_fill(cr);
	cairo_pattern_destroy(cp);
#endif
}

/**
 * uber_heat_map_render_fast:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_heat_map_render_fast (UberGraph    *graph, /* IN */
                           cairo_t      *cr,    /* IN */
                           GdkRectangle *area,  /* IN */
                           guint         epoch, /* IN */
                           gfloat        each)  /* IN */
{
	UberHeatMapPrivate *priv;
	GtkStyleContext *style;
	GdkRGBA color;
	gfloat height;
	gint i;

	g_return_if_fail(UBER_IS_HEAT_MAP(graph));

	priv = UBER_HEAT_MAP(graph)->priv;
	color = priv->fg_color;
	if (!priv->fg_color_set) {
		style = gtk_widget_get_style_context(GTK_WIDGET(graph));
		gtk_style_context_get_color(style, GTK_STATE_FLAG_SELECTED, &color);
	}
	/*
	 * XXX: Temporarily draw nice little squares.
	 */
	#define COUNT 10
	height = area->height / (gfloat)COUNT;
	for (i = 0; i < COUNT; i++) {
		cairo_rectangle(cr,
		                area->x + area->width - each,
		                area->y + (i * height),
		                each,
		                height);
		cairo_set_source_rgba(cr,
		                      color.red,
		                      color.green,
		                      color.blue,
		                      g_random_double_range(0., 1.));
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_fill(cr);
	}
}

/**
 * uber_heat_map_get_next_data:
 * @graph: A #UberGraph.
 *
 * Retrieve the next data point for the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
uber_heat_map_get_next_data (UberGraph *graph) /* IN */
{
	UberHeatMapPrivate *priv;
	GArray *array = NULL;

	g_return_val_if_fail(UBER_IS_HEAT_MAP(graph), FALSE);

	priv = UBER_HEAT_MAP(graph)->priv;
	if (!priv->func) {
		return FALSE;
	}
	/*
	 * Retrieve the next data point.
	 */
	if (!priv->func(UBER_HEAT_MAP(graph), &array, priv->func_user_data)) {
		return FALSE;
	}
	/*
	 * Store data points.
	 */
	g_ring_append_val(priv->raw_data, array);
//	for (int i = 0; i < array->len; i++) {
//		g_print("==> %f\n", g_array_index(array, gdouble, i));
//	}
	return TRUE;
}

/**
 * uber_heat_map_set_fg_color:
 * @map: A #UberHeatMap.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_heat_map_set_fg_color (UberHeatMap    *map,   /* IN */
                            const GdkRGBA  *color) /* IN */
{
	UberHeatMapPrivate *priv;

	g_return_if_fail(UBER_IS_HEAT_MAP(map));

	priv = map->priv;
	if (!color) {
		priv->fg_color_set = FALSE;
		memset(&priv->fg_color, 0, sizeof(priv->fg_color));
	} else {
		priv->fg_color = *color;
		priv->fg_color_set = TRUE;
	}
}

/**
 * uber_heat_map_finalize:
 * @object: A #UberHeatMap.
 *
 * Finalizer for a #UberHeatMap instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_heat_map_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(uber_heat_map_parent_class)->finalize(object);
}

/**
 * uber_heat_map_class_init:
 * @klass: A #UberHeatMapClass.
 *
 * Initializes the #UberHeatMapClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_heat_map_class_init (UberHeatMapClass *klass) /* IN */
{
	GObjectClass *object_class;
	UberGraphClass *graph_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = uber_heat_map_finalize;
	g_type_class_add_private(object_class, sizeof(UberHeatMapPrivate));

	graph_class = UBER_GRAPH_CLASS(klass);
	graph_class->render = uber_heat_map_render;
	graph_class->render_fast = uber_heat_map_render_fast;
	graph_class->set_stride = uber_heat_map_set_stride;
	graph_class->get_next_data = uber_heat_map_get_next_data;
}

/**
 * uber_heat_map_init:
 * @map: A #UberHeatMap.
 *
 * Initializes the newly created #UberHeatMap instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_heat_map_init (UberHeatMap *map) /* IN */
{
	map->priv = G_TYPE_INSTANCE_GET_PRIVATE(map,
	                                        UBER_TYPE_HEAT_MAP,
	                                        UberHeatMapPrivate);
}
