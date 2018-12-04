/* uber-line-graph.c
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

#include "uber-line-graph.h"
#include "uber-range.h"
#include "uber-scale.h"
#include "g-ring.h"

#define RECT_BOTTOM(r) ((r).y + (r).height)
#define RECT_RIGHT(r)  ((r).x + (r).width)
#define SCALE_FACTOR   (0.2)

/**
 * SECTION:uber-line-graph.h
 * @title: UberLineGraph
 * @short_description:
 *
 * Section overview.
 */

G_DEFINE_TYPE(UberLineGraph, uber_line_graph, UBER_TYPE_GRAPH)

typedef struct
{
	GRing     *raw_data;
	GdkRGBA    color;
	gdouble    width;
	gdouble   *dashes;
	gint       num_dashes;
	gdouble    dash_offset;
	UberLabel *label;
	guint      label_id;
} LineInfo;

struct _UberLineGraphPrivate
{
	GArray            *lines;
	cairo_antialias_t  antialias;
	guint              stride;
	gboolean           autoscale;
	UberRange          range;
	UberScale          scale;
	gpointer           scale_data;
	GDestroyNotify     scale_notify;
	UberLineGraphFunc  func;
	gpointer           func_data;
	GDestroyNotify     func_notify;
};

enum
{
	PROP_0,
	PROP_AUTOSCALE,
	PROP_RANGE,
};

/**
 * uber_line_graph_init_ring:
 * @ring: A #GRing.
 *
 * Initialize the #GRing to default values (UBER_LINE_GRAPH_NO_VALUE).
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_line_graph_init_ring (GRing *ring) /* IN */
{
	gdouble val = UBER_LINE_GRAPH_NO_VALUE;
	gint i;

	g_return_if_fail(ring != NULL);

	for (i = 0; i < ring->len; i++) {
		g_ring_append_val(ring, val);
	}
}

/**
 * uber_line_graph_new:
 *
 * Creates a new instance of #UberLineGraph.
 *
 * Returns: the newly created instance of #UberLineGraph.
 * Side effects: None.
 */
GtkWidget*
uber_line_graph_new (void)
{
	UberLineGraph *graph;

	graph = g_object_new(UBER_TYPE_LINE_GRAPH, NULL);
	return GTK_WIDGET(graph);
}

/**
 * uber_line_graph_color:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_color_changed (UberLabel     *label, /* IN */
                               GdkRGBA       *color, /* IN */
                               UberLineGraph *graph) /* IN */
{
	UberLineGraphPrivate *priv;
	LineInfo *info;
	gint i;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(color != NULL);

	priv = graph->priv;
	for (i = 0; i < priv->lines->len; i++) {
		info = &g_array_index(priv->lines, LineInfo, i);
		if (info->label == label) {
			info->color = *color;
		}
	}
	uber_graph_redraw(UBER_GRAPH(graph));
}

void
uber_line_graph_bind_label (UberLineGraph *graph, /* IN */
                            guint          line,  /* IN */
                            UberLabel     *label) /* IN */
{
	UberLineGraphPrivate *priv;
	LineInfo *info;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(UBER_IS_LABEL(label));
	g_return_if_fail(line > 0);
	g_return_if_fail(line <= graph->priv->lines->len);

	priv = graph->priv;
	info = &g_array_index(priv->lines, LineInfo, line - 1);
	if (info->label_id) {
		g_signal_handler_disconnect(info->label, info->label_id);
	}
	info->label = label;
	info->label_id = g_signal_connect(label,
	                                  "color-changed",
	                                  G_CALLBACK(uber_line_graph_color_changed),
	                                  graph);
}

/**
 * uber_line_graph_set_autoscale:
 * @graph: A #UberLineGraph.
 * @autoscale: Should we autoscale.
 *
 * Sets if we should autoscale the range of the graph when a new input
 * value is outside the visible range.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_line_graph_set_autoscale (UberLineGraph *graph,     /* IN */
                               gboolean       autoscale) /* IN */
{
	UberLineGraphPrivate *priv;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));

	priv = graph->priv;
	priv->autoscale = autoscale;
}

/**
 * uber_line_graph_get_autoscale:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
gboolean
uber_line_graph_get_autoscale (UberLineGraph *graph) /* IN */
{
	g_return_val_if_fail(UBER_IS_LINE_GRAPH(graph), FALSE);
	return graph->priv->autoscale;
}

/**
 * uber_line_graph_add_line:
 * @graph: A #UberLineGraph.
 * @color: A #GdkRGBA for the line or %NULL.
 *
 * Adds a new line to the graph.  If color is %NULL, the next value
 * in the default color list will be used.
 *
 * See uber_line_graph_remove_line().
 *
 * Returns: The line identifier.
 * Side effects: None.
 */
gint
uber_line_graph_add_line (UberLineGraph  *graph, /* IN */
                          const GdkRGBA  *color, /* IN */
                          UberLabel      *label) /* IN */
{
	UberLineGraphPrivate *priv;
	LineInfo info = { 0 };

	g_return_val_if_fail(UBER_IS_LINE_GRAPH(graph), 0);

	priv = graph->priv;
	info.width = 1.0;
	/*
	 * Retrieve the lines color.
	 */
	if (color) {
		info.color = *color;
	} else {
		gdk_rgba_parse(&info.color, "#729fcf");
	}
	/*
	 * Allocate buffers for data points.
	 */
	info.raw_data = g_ring_sized_new(sizeof(gdouble), priv->stride, NULL);
	uber_line_graph_init_ring(info.raw_data);
	/*
	 * Store the newly crated line.
	 */
	g_array_append_val(priv->lines, info);
	/*
	 * Mark the graph for full redraw.
	 */
	uber_graph_redraw(UBER_GRAPH(graph));
	/*
	 * Attach label.
	 */
	if (label) {
		uber_line_graph_bind_label(graph, priv->lines->len, label);
		uber_graph_add_label(UBER_GRAPH(graph), label);
		uber_label_set_color(label, &info.color);
	}
	/*
	 * Line indexes start from 1.
	 */
	return priv->lines->len;
}

/**
 * uber_line_graph_set_antialias:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_line_graph_set_antialias (UberLineGraph     *graph,     /* IN */
                               cairo_antialias_t  antialias) /* IN */
{
	UberLineGraphPrivate *priv;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));

	priv = graph->priv;
	priv->antialias = antialias;
	uber_graph_redraw(UBER_GRAPH(graph));
}

/**
 * uber_line_graph_get_antialias:
 * @graph: A #UberLineGraph.
 *
 * Retrieves the antialias mode for the graph.
 *
 * Returns: A cairo_antialias_t.
 * Side effects: None.
 */
cairo_antialias_t
uber_line_graph_get_antialias (UberLineGraph *graph) /* IN */
{
	g_return_val_if_fail(UBER_IS_LINE_GRAPH(graph), 0);

	return graph->priv->antialias;
}

/**
 * uber_line_graph_get_next_data:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
uber_line_graph_get_next_data (UberGraph *graph) /* IN */
{
	UberLineGraphPrivate *priv;
	gboolean scale_changed = FALSE;
	gboolean ret = FALSE;
	LineInfo *line;
	gdouble val;
	gint i;

	g_return_val_if_fail(UBER_IS_LINE_GRAPH(graph), FALSE);

	priv = UBER_LINE_GRAPH(graph)->priv;
	/*
	 * Retrieve the next data point.
	 */
	if (priv->func) {
		for (i = 0; i < priv->lines->len; i++) {
			line = &g_array_index(priv->lines, LineInfo, i);
			val = priv->func(UBER_LINE_GRAPH(graph), i + 1, priv->func_data);
			g_ring_append_val(line->raw_data, val);
			if (priv->autoscale) {
				if (val < priv->range.begin) {
					priv->range.begin = val - (val * SCALE_FACTOR);
					priv->range.range = priv->range.end - priv->range.begin;
					scale_changed = TRUE;
				} else if (val > priv->range.end) {
					priv->range.end = val + (val * SCALE_FACTOR);
					priv->range.range = priv->range.end - priv->range.begin;
					scale_changed = TRUE;
				}
			}
		}
	}
	if (scale_changed) {
		uber_graph_scale_changed(graph);
	}
	return ret;
}

/**
 * uber_line_graph_set_data_func:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_line_graph_set_data_func (UberLineGraph     *graph,     /* IN */
                               UberLineGraphFunc  func,      /* IN */
                               gpointer           user_data, /* IN */
                               GDestroyNotify     notify)    /* IN */
{
	UberLineGraphPrivate *priv;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));

	priv = graph->priv;
	/*
	 * Free existing data func if neccessary.
	 */
	if (priv->func_notify) {
		priv->func_notify(priv->func_data);
	}
	/*
	 * Store data func.
	 */
	priv->func = func;
	priv->func_data = user_data;
	priv->func_notify = notify;
}

/**
 * uber_line_graph_stylize_line:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_stylize_line (UberLineGraph *graph, /* IN */
                              LineInfo      *info,  /* IN */
                              cairo_t       *cr)    /* IN */
{
	UberLineGraphPrivate *priv;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(info != NULL);

	priv = graph->priv;
	if (info->dashes) {
		cairo_set_dash(cr, info->dashes, info->num_dashes, info->dash_offset);
	} else {
		cairo_set_dash(cr, NULL, 0, 0);
	}
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_width(cr, info->width);
	cairo_set_antialias(cr, priv->antialias);
	cairo_set_source_rgba(cr,
                          info->color.red,
                          info->color.green,
                          info->color.blue,
                          info->color.alpha);
}

/**
 * uber_line_graph_render:
 * @graph: A #UberGraph.
 * @cr: A #cairo_t context.
 * @area: Full area to render contents within.
 * @line: The line to render.
 *
 * Render a particular line to the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_render_line (UberLineGraph *graph, /* IN */
                             cairo_t       *cr,    /* IN */
                             GdkRectangle  *area,  /* IN */
                             LineInfo      *line,  /* IN */
                             guint          epoch, /* IN */
                             gfloat         each)  /* IN */
{
	UberLineGraphPrivate *priv;
	UberRange pixel_range;
	GdkRectangle vis;
	guint x;
	guint last_x;
	gdouble y;
	gdouble last_y;
	gdouble val;
	gint i;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));

	priv = graph->priv;
	uber_graph_get_content_area(UBER_GRAPH(graph), &vis);
	pixel_range.begin = area->y + 1;
	pixel_range.end = area->y + area->height;
	pixel_range.range = pixel_range.end - pixel_range.begin;
	/*
	 * Prepare cairo settings.
	 */
	uber_line_graph_stylize_line(graph, line, cr);
	/*
	 * Force a new path.
	 */
	cairo_new_path(cr);
	/*
	 * Draw the line contents as bezier curves.
	 */
	for (i = 0; i < line->raw_data->len; i++) {
		/*
		 * Retrieve data point.
		 */
		val = g_ring_get_index(line->raw_data, gdouble, i);
		/*
		 * Once we get to UBER_LINE_GRAPH_NO_VALUE, we must be at the end of the data
		 * sequence.  This may not always be true in the future.
		 */
		if (val == UBER_LINE_GRAPH_NO_VALUE) {
			break;
		}
		/*
		 * Translate value to coordinate system.
		 */
		if (!priv->scale(&priv->range, &pixel_range, &val, priv->scale_data)) {
			break;
		}
		/*
		 * Calculate X/Y coordinate.
		 */
		y = (gint)(RECT_BOTTOM(*area) - val) - .5;
		x = epoch - (each * i);
		if (i == 0) {
			/*
			 * Just move to the right position on first entry.
			 */
			cairo_move_to(cr, x, y);
			goto next;
		} else {
			/*
			 * Draw curve to data point using the last X/Y positions as
			 * control points.
			 */
			cairo_curve_to(cr,
			               last_x - (each / 2.),
			               last_y,
			               last_x - (each / 2.),
			               y, x, y);
		}
	  next:
		last_y = y;
		last_x = x;
	}
	/*
	 * Stroke the line content.
	 */
	cairo_stroke(cr);
}

/**
 * uber_line_graph_render:
 * @graph: A #UberGraph.
 *
 * Render the entire contents of the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_render (UberGraph    *graph, /* IN */
                        cairo_t      *cr,    /* IN */
                        GdkRectangle *rect,  /* IN */
                        guint         epoch, /* IN */
                        gfloat        each)  /* IN */
{
	UberLineGraphPrivate *priv;
	LineInfo *line;
	gint i;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));

	priv = UBER_LINE_GRAPH(graph)->priv;
	/*
	 * Render each line to the graph.
	 */
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		uber_line_graph_render_line(UBER_LINE_GRAPH(graph), cr, rect,
		                            line, epoch, each);
	}
}

/**
 * uber_line_graph_render_fast:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_render_fast (UberGraph    *graph, /* IN */
                             cairo_t      *cr,    /* IN */
                             GdkRectangle *rect,  /* IN */
                             guint         epoch, /* IN */
                             gfloat        each)  /* IN */
{
	UberLineGraphPrivate *priv;
	UberRange pixel_range;
	LineInfo *line;
	gdouble last_y;
	gdouble y;
	gint i;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(cr != NULL);
	g_return_if_fail(rect != NULL);

	priv = UBER_LINE_GRAPH(graph)->priv;
	pixel_range.begin = rect->y + 1;
	pixel_range.end = rect->y + rect->height;
	pixel_range.range = pixel_range.end - pixel_range.begin;
	/*
	 * Render most recent data point for each line.
	 */
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		uber_line_graph_stylize_line(UBER_LINE_GRAPH(graph), line, cr);
		/*
		 * Calculate positions.
		 */
		y = g_ring_get_index(line->raw_data, gdouble, 0);
		last_y = g_ring_get_index(line->raw_data, gdouble, 1);
		/*
		 * Don't try to draw before we have real values.
		 */
		if ((isnan(y) || isinf(y)) || (isnan(last_y) || isinf(last_y))) {
			continue;
		}
		/*
		 * Translate to coordinate scale.
		 */
		if (!priv->scale(&priv->range, &pixel_range, &y, priv->scale_data) ||
		    !priv->scale(&priv->range, &pixel_range, &last_y, priv->scale_data)) {
			continue;
		}
		/*
		 * Translate position from bottom right corner.
		 */
		y = (gint)(RECT_BOTTOM(*rect) - y) - .5;
		last_y = (gint)(RECT_BOTTOM(*rect) - last_y) - .5;
		/*
		 * Convert relative position to fixed from bottom pixel.
		 */
		cairo_new_path(cr);
		cairo_move_to(cr, epoch, y);
		cairo_curve_to(cr,
		               epoch - (each / 2.),
		               y,
		               epoch - (each / 2.),
		               last_y,
		               epoch - each,
		               last_y);
		cairo_stroke(cr);
	}
}

/**
 * uber_line_graph_set_stride:
 * @graph: A #UberGraph.
 * @stride: The number of data points within the graph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_set_stride (UberGraph *graph,  /* IN */
                            guint      stride) /* IN */
{
	UberLineGraphPrivate *priv;
	LineInfo *line;
	gint i;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));

	priv = UBER_LINE_GRAPH(graph)->priv;
	priv->stride = stride;
	/*
	 * TODO: Support changing stride after lines have been added.
	 */
	if (priv->lines->len) {
		for (i = 0; i < priv->lines->len; i++) {
			line = &g_array_index(priv->lines, LineInfo, i);
			g_ring_unref(line->raw_data);
			line->raw_data = g_ring_sized_new(sizeof(gdouble),
			                                  priv->stride, NULL);
			uber_line_graph_init_ring(line->raw_data);
		}
		return;
	}
}

/**
 * uber_line_graph_get_range:
 * @graph: (in): A #UberLineGraph.
 *
 * XXX
 *
 * Returns: An #UberRange which should not be modified or freed.
 * Side effects: None.
 */
const UberRange*
uber_line_graph_get_range (UberLineGraph *graph) /* IN */
{
	g_return_val_if_fail(UBER_IS_LINE_GRAPH(graph), NULL);
	return &graph->priv->range;
}

/**
 * uber_line_graph_set_range:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_line_graph_set_range (UberLineGraph   *graph, /* IN */
                           const UberRange *range) /* IN */
{
	UberLineGraphPrivate *priv;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(range != NULL);

	priv = graph->priv;
	priv->range = *range;
}

/**
 * uber_line_graph_get_yrange:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_get_yrange (UberGraph *graph, /* IN */
                            UberRange *range) /* OUT */
{
	UberLineGraphPrivate *priv;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(range != NULL);

	priv = UBER_LINE_GRAPH(graph)->priv;
	*range = priv->range;
}

/**
 * uber_line_graph_set_line_dash:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_line_graph_set_line_dash (UberLineGraph *graph,      /* IN */
                               guint          line,       /* IN */
                               const gdouble *dashes,     /* IN */
                               gint           num_dashes, /* IN */
                               gdouble        offset)     /* IN */
{
	UberLineGraphPrivate *priv;
	LineInfo *info;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(dashes != NULL);
	g_return_if_fail(num_dashes > 0);
	g_return_if_fail(line > 0);
	g_return_if_fail(line <= graph->priv->lines->len);

	priv = graph->priv;
	info = &g_array_index(priv->lines, LineInfo, line - 1);
	if (info->dashes) {
		g_free(info->dashes);
		info->dashes = NULL;
		info->num_dashes = 0;
		info->dash_offset = 0;
	}
	if (dashes) {
		info->dashes = g_new0(gdouble, num_dashes);
		memcpy(info->dashes, dashes, sizeof(gdouble) * num_dashes);
		info->num_dashes = num_dashes;
		info->dash_offset = offset;
	}
}

/**
 * uber_line_graph_set_line_width:
 * @graph: A #UberLineGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_line_graph_set_line_width (UberLineGraph *graph, /* IN */
                                gint           line,  /* IN */
                                gdouble        width) /* IN */
{
	LineInfo *info;

	g_return_if_fail(UBER_IS_LINE_GRAPH(graph));
	g_return_if_fail(line > 0);
	g_return_if_fail(line <= graph->priv->lines->len);

	info = &g_array_index(graph->priv->lines, LineInfo, line - 1);
	info->width = width;
	uber_graph_redraw(UBER_GRAPH(graph));
}

/**
 * uber_line_graph_downscale:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
uber_line_graph_downscale (UberGraph *graph) /* IN */
{
	UberLineGraphPrivate *priv;
	gboolean ret = FALSE;
	gdouble val = 0;
	gdouble cur;
	LineInfo *line;
	gint i;
	gint j;

	g_return_val_if_fail(UBER_IS_LINE_GRAPH(graph), FALSE);

	priv = UBER_LINE_GRAPH(graph)->priv;
	/*
	 * If we are set to autoscale, ignore request.
	 */
	if (!priv->autoscale) {
		return FALSE;
	}
	/*
	 * Determine the largest value available.
	 */
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		for (j = 0; j < line->raw_data->len; j++) {
			cur = g_ring_get_index(line->raw_data, gdouble, j);
			val = (cur > val) ? cur : val;
		}
	}
	/*
	 * Downscale if we can.
	 */
	if (val != priv->range.begin) {
		if ((val * (1. + SCALE_FACTOR)) < priv->range.end) {
			priv->range.end = val * (1. + SCALE_FACTOR);
			priv->range.range = priv->range.end - priv->range.begin;
			ret = TRUE;
		}
	}
	return ret;
}

/**
 * uber_line_graph_finalize:
 * @object: A #UberLineGraph.
 *
 * Finalizer for a #UberLineGraph instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_finalize (GObject *object) /* IN */
{
	UberLineGraphPrivate *priv;
	LineInfo *line;
	gint i;

	priv = UBER_LINE_GRAPH(object)->priv;
	/*
	 * Clean up after cached values.
	 */
	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		g_ring_unref(line->raw_data);
		g_free(line->dashes);
	}
	G_OBJECT_CLASS(uber_line_graph_parent_class)->finalize(object);
}

/**
 * uber_line_graph_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
uber_line_graph_get_property (GObject    *object,  /* IN */
                              guint       prop_id, /* IN */
                              GValue     *value,   /* OUT */
                              GParamSpec *pspec)   /* IN */
{
	UberLineGraph *graph = UBER_LINE_GRAPH(object);

	switch (prop_id) {
	case PROP_AUTOSCALE:
		g_value_set_boolean(value, uber_line_graph_get_autoscale(graph));
		break;
	case PROP_RANGE:
		g_value_set_boxed(value, uber_line_graph_get_range(graph));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * uber_line_graph_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
uber_line_graph_set_property (GObject      *object,  /* IN */
                              guint         prop_id, /* IN */
                              const GValue *value,   /* IN */
                              GParamSpec   *pspec)   /* IN */
{
	UberLineGraph *graph = UBER_LINE_GRAPH(object);

	switch (prop_id) {
	case PROP_AUTOSCALE:
		uber_line_graph_set_autoscale(graph, g_value_get_boolean(value));
		break;
	case PROP_RANGE:
		uber_line_graph_set_range(graph, g_value_get_boxed(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * uber_line_graph_class_init:
 * @klass: A #UberLineGraphClass.
 *
 * Initializes the #UberLineGraphClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_class_init (UberLineGraphClass *klass) /* IN */
{
	GObjectClass *object_class;
	UberGraphClass *graph_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = uber_line_graph_finalize;
	object_class->get_property = uber_line_graph_get_property;
	object_class->set_property = uber_line_graph_set_property;
	g_type_class_add_private(object_class, sizeof(UberLineGraphPrivate));

	graph_class = UBER_GRAPH_CLASS(klass);
	graph_class->downscale = uber_line_graph_downscale;
	graph_class->get_next_data = uber_line_graph_get_next_data;
	graph_class->get_yrange = uber_line_graph_get_yrange;
	graph_class->render = uber_line_graph_render;
	graph_class->render_fast = uber_line_graph_render_fast;
	graph_class->set_stride = uber_line_graph_set_stride;

	g_object_class_install_property(object_class,
	                                PROP_AUTOSCALE,
	                                g_param_spec_boolean("autoscale",
	                                                     "autoscale",
	                                                     "autoscale",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_RANGE,
	                                g_param_spec_boxed("range",
	                                                   "range",
	                                                   "range",
	                                                   UBER_TYPE_RANGE,
	                                                   G_PARAM_READWRITE));
}

/**
 * uber_line_graph_init:
 * @graph: A #UberLineGraph.
 *
 * Initializes the newly created #UberLineGraph instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_line_graph_init (UberLineGraph *graph) /* IN */
{
	UberLineGraphPrivate *priv;

	/*
	 * Keep pointer to private data.
	 */
	graph->priv = G_TYPE_INSTANCE_GET_PRIVATE(graph,
	                                          UBER_TYPE_LINE_GRAPH,
	                                          UberLineGraphPrivate);
	priv = graph->priv;
	/*
	 * Initialize defaults.
	 */
	priv->stride = 60;
	priv->antialias = CAIRO_ANTIALIAS_DEFAULT;
	priv->lines = g_array_sized_new(FALSE, FALSE, sizeof(LineInfo), 2);
	priv->scale = uber_scale_linear;
	priv->autoscale = TRUE;
}

void
uber_line_graph_clear (UberLineGraph *graph) /* IN */
{
	UberLineGraphPrivate *priv = graph->priv;
	LineInfo *line;
	gint i;

	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, LineInfo, i);
		uber_line_graph_init_ring(line->raw_data);
	}
}
