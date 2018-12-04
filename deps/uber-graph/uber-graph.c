/* uber-graph.c
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

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <math.h>
#include <string.h>

#include "uber-graph.h"
#include "uber-scale.h"
#include "uber-frame-source.h"

#define WIDGET_CLASS (GTK_WIDGET_CLASS(uber_graph_parent_class))
#define RECT_RIGHT(r)  ((r).x + (r).width)
#define RECT_BOTTOM(r) ((r).y + (r).height)
#define UNSET_SURFACE(p)       \
    G_STMT_START {             \
        if (p) {               \
            cairo_surface_destroy (p); \
            p = NULL;          \
        }                      \
    } G_STMT_END
#define CLEAR_CAIRO(c, a)                             \
    G_STMT_START {                                    \
        cairo_save(c);                                \
        cairo_rectangle(c, 0, 0, a.width, a.height);  \
        cairo_set_operator(c, CAIRO_OPERATOR_CLEAR);  \
        cairo_fill(c);                                \
        cairo_restore(c);                             \
    } G_STMT_END

/**
 * SECTION:uber-graph.h
 * @title: UberGraph
 * @short_description: Graphing of realtime data.
 *
 * #UberGraph is an abstract base class for building realtime graphs.  It
 * handles scrolling the graph based on the required time intervals and tries
 * to render as little data as possible.
 *
 * Subclasses are responsible for acquiring data when #UberGraph notifies them
 * of their next data sample.  Additionally, there are two rendering methods.
 * UberGraph::render is a render of the full scene.  UberGraph::render_fast is
 * a rendering of just a new data sample.  Ideally, UberGraph::render_fast is
 * going to be called.
 *
 * #UberGraph uses a #cairo_surface_t as a ring buffer to store the contents of the
 * graph.  Upon destructive changes to the widget such as allocation changed
 * or a new #GtkStyle set, a full rendering of the graph will be required.
 */

G_DEFINE_ABSTRACT_TYPE(UberGraph, uber_graph, GTK_TYPE_DRAWING_AREA)

struct _UberGraphPrivate
{
	cairo_surface_t *fg_surface;
	cairo_surface_t *bg_surface;

	GdkRectangle     content_rect;  /* Content area rectangle. */
	GdkRectangle     nonvis_rect;   /* Non-visible drawing area larger than
	                                 * content rect. Used to draw over larger
	                                 * area so we can scroll and not fall out
	                                 * of view.
	                                 */
	UberGraphFormat  format;        /* Data type format. */
	gboolean         paused;        /* Is the graph paused. */
	gboolean         have_rgba;     /* Do we support 32-bit RGBA colormaps. */
	gint             x_slots;       /* Number of data points on x axis. */
	gint             fps;           /* Desired frames per second. */
	gint             fps_real;      /* Milleseconds between FPS callbacks. */
	gfloat           fps_each;      /* How far to move in each FPS tick. */
	guint            fps_handler;   /* Timeout for moving the content. */
	gfloat           dps;           /* Desired data points per second. */
	gint             dps_slot;      /* Which slot in the surface buffer. */
	gfloat           dps_each;      /* How many pixels between data points. */
	GTimeVal         dps_tv;        /* Timeval of last data point. */
	guint            dps_handler;   /* Timeout for getting new data. */
	guint            dps_downscale; /* Count since last downscale. */
	gboolean         fg_dirty;      /* Does the foreground need to be redrawn. */
	gboolean         bg_dirty;      /* Does the background need to be redrawn. */
	guint            tick_len;      /* How long should axis-ticks be. */
	gboolean         show_xlines;   /* Show X axis lines. */
	gboolean         show_xlabels;  /* Show X axis labels. */
	gboolean         show_ylines;   /* Show Y axis lines. */
	gboolean         full_draw;     /* Do we need to redraw all foreground content.
	                                 * If false, draws will try to only add new
	                                 * content to the back buffer.
	                                 */
	GtkWidget       *labels;        /* Container for graph labels. */
	GtkWidget       *align;         /* Alignment for labels. */
	gint             fps_count;     /* Track actual FPS. */
};

static gboolean show_fps = FALSE;

enum
{
	PROP_0,
	PROP_FORMAT,
};

/**
 * uber_graph_new:
 *
 * Creates a new instance of #UberGraph.
 *
 * Returns: the newly created instance of #UberGraph.
 * Side effects: None.
 */
GtkWidget*
uber_graph_new (void)
{
	UberGraph *graph;

	graph = g_object_new(UBER_TYPE_GRAPH, NULL);
	return GTK_WIDGET(graph);
}

/**
 * uber_graph_fps_timeout:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: %TRUE always.
 * Side effects: None.
 */
static gboolean
uber_graph_fps_timeout (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	priv = graph->priv;
	gtk_widget_queue_draw_area(GTK_WIDGET(graph),
	                           priv->content_rect.x,
	                           priv->content_rect.y,
	                           priv->content_rect.width,
	                           priv->content_rect.height);
	return TRUE;
}

/**
 * uber_graph_get_content_area:
 * @graph: A #UberGraph.
 *
 * Retrieves the content area of the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_get_content_area (UberGraph    *graph, /* IN */
                             GdkRectangle *rect)  /* OUT */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(rect != NULL);

	priv = graph->priv;
	*rect = priv->content_rect;
}

/**
 * uber_graph_get_show_xlines:
 * @graph: A #UberGraph.
 *
 * Retrieves if the X grid lines should be shown.
 *
 * Returns: None.
 * Side effects: None.
 */
gboolean
uber_graph_get_show_xlines (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	priv = graph->priv;
	return priv->show_xlines;
}

/**
 * uber_graph_set_show_xlines:
 * @graph: A #UberGraph.
 * @show_xlines: Show x lines.
 *
 * Sets if the x lines should be shown.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_show_xlines (UberGraph *graph,       /* IN */
                            gboolean   show_xlines) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->show_xlines = show_xlines;
	priv->bg_dirty = TRUE;
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}

/**
 * uber_graph_get_show_ylines:
 * @graph: A #UberGraph.
 *
 * Retrieves if the X grid lines should be shown.
 *
 * Returns: None.
 * Side effects: None.
 */
gboolean
uber_graph_get_show_ylines (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	priv = graph->priv;
	return priv->show_ylines;
}

/**
 * uber_graph_set_show_ylines:
 * @graph: A #UberGraph.
 * @show_ylines: Show x lines.
 *
 * Sets if the x lines should be shown.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_show_ylines (UberGraph *graph,       /* IN */
                            gboolean   show_ylines) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->show_ylines = show_ylines;
	priv->bg_dirty = TRUE;
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}

/**
 * uber_graph_get_labels:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
GtkWidget*
uber_graph_get_labels (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), NULL);

	priv = graph->priv;
	return priv->align;
}

/**
 * uber_graph_scale_changed:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_scale_changed (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GdkRectangle rect;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	if (!priv->paused) {
		priv->fg_dirty = TRUE;
		priv->bg_dirty = TRUE;
		priv->full_draw = TRUE;
		gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
		rect.x = 0;
		rect.y = 0;
		rect.width = alloc.width;
		rect.height = alloc.height;
		gdk_window_invalidate_rect(gtk_widget_get_window(GTK_WIDGET(graph)),
		                           &rect, TRUE);
	}
}

/**
 * uber_graph_get_next_data:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static inline gboolean
uber_graph_get_next_data (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	gboolean ret = TRUE;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	/*
	 * Get the current time for this data point.  This is used to calculate
	 * the proper offset in the FPS callback.
	 */
	priv = graph->priv;
	g_get_current_time(&priv->dps_tv);
	/*
	 * Notify the subclass to retrieve the data point.
	 */
	if (UBER_GRAPH_GET_CLASS(graph)->get_next_data) {
		ret = UBER_GRAPH_GET_CLASS(graph)->get_next_data(graph);
	}
	return ret;
}

/**
 * uber_graph_init_texture:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_init_texture (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GdkWindow *window;
	cairo_t *cr;
	gint width;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	window = gtk_widget_get_window(GTK_WIDGET(graph));
	/*
	 * Get drawable to base surface upon.
	 */
	if (!window) {
		g_critical("%s() called before GdkWindow is allocated.", G_STRFUNC);
		return;
	}
	/*
	 * Initialize foreground and background surface.
	 */
	width = MAX(priv->nonvis_rect.x + priv->nonvis_rect.width, alloc.width);
	priv->fg_surface = gdk_window_create_similar_surface(window, CAIRO_CONTENT_COLOR_ALPHA, width, alloc.height);
	/*
	 * Clear foreground contents.
	 */
	cr = cairo_create(priv->fg_surface);
	CLEAR_CAIRO(cr, alloc);
	cairo_destroy(cr);
}

/**
 * uber_graph_init_bg:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_init_bg (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GdkWindow *window;
	cairo_t *cr;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	window = gtk_widget_get_window(GTK_WIDGET(graph));
	/*
	 * Get drawable for surface.
	 */
	if (!window) {
		g_critical("%s() called before GdkWindow is allocated.", G_STRFUNC);
		return;
	}
	/*
	 * Create the server-side surface.
	 */
	priv->bg_surface = gdk_window_create_similar_surface(window, CAIRO_CONTENT_COLOR_ALPHA, alloc.width, alloc.height);
	/*
	 * Clear background contents.
	 */
	cr = cairo_create(priv->bg_surface);
	CLEAR_CAIRO(cr, alloc);
	cairo_destroy(cr);
}

/**
 * uber_graph_calculate_rects:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_calculate_rects (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	PangoLayout *layout;
	PangoFontDescription *font_desc;
	GdkWindow *window;
	gint pango_width;
	gint pango_height;
	cairo_t *cr;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	window = gtk_widget_get_window(GTK_WIDGET(graph));
	/*
	 * We can't calculate rectangles before we have a GdkWindow.
	 */
	if (!window) {
		return;
	}
	/*
	 * Determine the pixels required for labels.
	 */
	cr = gdk_cairo_create(window);
	layout = pango_cairo_create_layout(cr);
	font_desc = pango_font_description_new();
	pango_font_description_set_family_static(font_desc, "Monospace");
	pango_font_description_set_size(font_desc, 6 * PANGO_SCALE);
	pango_layout_set_font_description(layout, font_desc);
	pango_layout_set_text(layout, "XXXXXXXXXX", -1);
	pango_layout_get_pixel_size(layout, &pango_width, &pango_height);
	pango_font_description_free(font_desc);
	g_object_unref(layout);
	cairo_destroy(cr);
	/*
	 * Calculate content area rectangle.
	 */
	priv->content_rect.x = priv->tick_len + pango_width + 1.5;
	priv->content_rect.y = (pango_height / 2.) + 1.5;
	priv->content_rect.width = alloc.width - priv->content_rect.x - 3.0;
	priv->content_rect.height = alloc.height - priv->tick_len - pango_height
	                          - (pango_height / 2.) - 3.0;
	if (!priv->show_xlabels) {
		priv->content_rect.height += pango_height;
	}
	/*
	 * Adjust label offset.
	 */
	/*
	 * Calculate FPS/DPS adjustments.
	 */
	priv->dps_each = ceil((gfloat)priv->content_rect.width
	                      / (gfloat)(priv->x_slots - 1));
	priv->fps_each = priv->dps_each
	               / ((gfloat)priv->fps / (gfloat)priv->dps);
	/*
	 * XXX: Small hack to make things a bit smoother at small scales.
	 */
	if (priv->fps_each < .5) {
		priv->fps_each = 1;
		priv->fps_real = (1000. / priv->dps_each) / 2.;
	} else {
		priv->fps_real = 1000. / priv->fps;
	}
	/*
	 * Update FPS callback.
	 */
	if (priv->fps_handler) {
		g_source_remove(priv->fps_handler);
		priv->fps_handler = uber_frame_source_add(priv->fps,
		                                  (GSourceFunc)uber_graph_fps_timeout,
		                                  graph);
	}
	/*
	 * Calculate the non-visible area that drawing should happen within.
	 */
	priv->nonvis_rect = priv->content_rect;
	priv->nonvis_rect.width = priv->dps_each * priv->x_slots;
	/*
	 * Update positioning for label alignment.
	 */
	gtk_widget_set_margin_top(GTK_WIDGET(priv->align), 6);
	gtk_widget_set_margin_bottom(GTK_WIDGET(priv->align), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(priv->align), priv->content_rect.x);
	gtk_widget_set_margin_end(GTK_WIDGET(priv->align), 0);
}

/**
 * uber_graph_get_show_xlabels:
 * @graph: A #UberGraph.
 *
 * Retrieves if the X grid labels should be shown.
 *
 * Returns: None.
 * Side effects: None.
 */
gboolean
uber_graph_get_show_xlabels (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	priv = graph->priv;
	return priv->show_xlabels;
}

/**
 * uber_graph_set_show_xlabels:
 * @graph: A #UberGraph.
 * @show_xlabels: Show x labels.
 *
 * Sets if the x labels should be shown.
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_show_xlabels (UberGraph *graph,       /* IN */
                             gboolean   show_xlabels) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->show_xlabels = show_xlabels;
	priv->bg_dirty = TRUE;
	uber_graph_calculate_rects(graph);
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}

/**
 * uber_graph_dps_timeout:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: %TRUE always.
 * Side effects: None.
 */
static gboolean
uber_graph_dps_timeout (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), FALSE);

	priv = graph->priv;
	if (!uber_graph_get_next_data(graph)) {
		/*
		 * XXX: How should we handle failed data retrieval.
		 */
	}
	if (G_UNLIKELY(show_fps)) {
		g_print("UberGraph[%p] %02d FPS\n", graph, priv->fps_count);
		priv->fps_count = 0;
	}
	/*
	 * Make sure the content is re-rendered.
	 */
	if (!priv->paused) {
		priv->fg_dirty = TRUE;
		/*
		 * We do not queue a draw here since the next FPS callback will happen
		 * when it is the right time to show the frame.
		 */
	}
	/*
	 * Try to downscale the graph.  We do this whether or not we are paused
	 * as redrawing is deferred if we are in a paused state.
	 */
	priv->dps_downscale++;
	if (priv->dps_downscale >= 5) {
		if (UBER_GRAPH_GET_CLASS(graph)->downscale) {
			if (UBER_GRAPH_GET_CLASS(graph)->downscale(graph)) {
				if (!priv->paused) {
					uber_graph_redraw(graph);
				}
			}
		}
		priv->dps_downscale = 0;
	}
	return TRUE;
}

/**
 * uber_graph_register_dps_handler:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_register_dps_handler (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	guint dps_freq;
	gboolean do_now = TRUE;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	if (priv->dps_handler) {
		g_source_remove(priv->dps_handler);
		do_now = FALSE;
	}
	/*
	 * Calculate the update frequency.
	 */
	dps_freq = 1000 / priv->dps;
	/*
	 * Install the data handler.
	 */
	priv->dps_handler = g_timeout_add(dps_freq,
	                                  (GSourceFunc)uber_graph_dps_timeout,
	                                  graph);
	/*
	 * Call immediately.
	 */
	if (do_now) {
		uber_graph_dps_timeout(graph);
	}
}

/**
 * uber_graph_register_fps_handler:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_register_fps_handler (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	/*
	 * Remove any existing FPS handler.
	 */
	if (priv->fps_handler) {
		g_source_remove(priv->fps_handler);
	}
	/*
	 * Install the FPS timeout.
	 */
	priv->fps_handler = uber_frame_source_add(priv->fps,
	                                  (GSourceFunc)uber_graph_fps_timeout,
	                                  graph);
}

/**
 * uber_graph_set_dps:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_dps (UberGraph *graph, /* IN */
                    gfloat     dps)   /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->dps = dps;
	/*
	 * TODO: Does this belong somewhere else?
	 */
	if (UBER_GRAPH_GET_CLASS(graph)->set_stride) {
		UBER_GRAPH_GET_CLASS(graph)->set_stride(graph, priv->x_slots);
	}
	/*
	 * Recalculate frame rates and timeouts.
	 */
	uber_graph_calculate_rects(graph);
	uber_graph_register_dps_handler(graph);
	uber_graph_register_fps_handler(graph);
}

/**
 * uber_graph_set_fps:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_fps (UberGraph *graph, /* IN */
                    guint      fps)   /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->fps = fps;
	uber_graph_register_fps_handler(graph);
}

/**
 * uber_graph_realize:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_realize (GtkWidget *widget) /* IN */
{
	UberGraph *graph;
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	graph = UBER_GRAPH(widget);
	priv = graph->priv;
	WIDGET_CLASS->realize(widget);
	/*
	 * Calculate new layout based on allocation.
	 */
	uber_graph_calculate_rects(graph);
	/*
	 * Re-initialize textures for updated sizes.
	 */
	UNSET_SURFACE(priv->bg_surface);
	UNSET_SURFACE(priv->fg_surface);
	uber_graph_init_bg(graph);
	uber_graph_init_texture(graph);
	/*
	 * Notify subclass of current data stride (points per graph).
	 */
	if (UBER_GRAPH_GET_CLASS(widget)->set_stride) {
		UBER_GRAPH_GET_CLASS(widget)->set_stride(UBER_GRAPH(widget),
		                                         priv->x_slots);
	}
	/*
	 * Install the data collector.
	 */
	uber_graph_register_dps_handler(graph);
}

/**
 * uber_graph_unrealize:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_unrealize (GtkWidget *widget) /* IN */
{
	UberGraph *graph;
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	graph = UBER_GRAPH(widget);
	priv = graph->priv;
	/*
	 * Unregister any data acquisition handlers.
	 */
	if (priv->dps_handler) {
		g_source_remove(priv->dps_handler);
		priv->dps_handler = 0;
	}
	/*
	 * Destroy textures.
	 */
	UNSET_SURFACE(priv->bg_surface);
	UNSET_SURFACE(priv->fg_surface);
}

/**
 * uber_graph_screen_changed:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_screen_changed (GtkWidget *widget,     /* IN */
                           GdkScreen *old_screen) /* IN */
{
	UberGraphPrivate *priv;
	GdkScreen *screen;
	GdkVisual *visual;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	screen = gtk_widget_get_screen (GTK_WIDGET (widget));
	visual = gdk_screen_get_rgba_visual (screen);

	priv = UBER_GRAPH(widget)->priv;
	/*
	 * Check if we have RGBA colormaps available.
	 */
	priv->have_rgba = visual != NULL;
}

/**
 * uber_graph_show:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_show (GtkWidget *widget) /* IN */
{
	g_return_if_fail(UBER_IS_GRAPH(widget));

	WIDGET_CLASS->show(widget);
	/*
	 * Only run the FPS timeout when we are visible.
	 */
	uber_graph_register_fps_handler(UBER_GRAPH(widget));
}

/**
 * uber_graph_hide:
 * @widget: A #GtkWIdget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_hide (GtkWidget *widget) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	priv = UBER_GRAPH(widget)->priv;
	/*
	 * Disable the FPS timeout when we are not visible.
	 */
	if (priv->fps_handler) {
		g_source_remove(priv->fps_handler);
		priv->fps_handler = 0;
	}
}

static inline void
uber_graph_get_pixmap_rect (UberGraph    *graph, /* IN */
                            GdkRectangle *rect)  /* OUT */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	rect->x = 0;
	rect->y = 0;
	rect->width = MAX(alloc.width,
	                  priv->nonvis_rect.x + priv->nonvis_rect.width);
	rect->height = alloc.height;
}

/**
 * uber_graph_render_fg:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_render_fg (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GdkRectangle rect;
	cairo_t *cr;
	gfloat each;
	gfloat x_epoch;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	/*
	 * Acquire resources.
	 */
	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	uber_graph_get_pixmap_rect(graph, &rect);
	cr = cairo_create(priv->fg_surface);
	/*
	 * Render to texture if needed.
	 */
	if (priv->fg_dirty) {
		/*
		 * Caclulate relative positionings for use in renderers.
		 */
		each = priv->content_rect.width / (gfloat)(priv->x_slots - 1);
		x_epoch = RECT_RIGHT(priv->nonvis_rect);
		/*
		 * If we are in a fast draw, lets copy the content from the other
		 * buffer at the next offset.
		 */
		if (!priv->full_draw && UBER_GRAPH_GET_CLASS(graph)->render_fast) {
			/*
			 * Determine next rendering slot.
			 */
			rect.x = priv->content_rect.x
			       + (priv->dps_each * priv->dps_slot);
			rect.width = priv->dps_each;
			rect.y = priv->content_rect.y;
			rect.height = priv->content_rect.height;
			priv->dps_slot = (priv->dps_slot + 1) % priv->x_slots;
			x_epoch = RECT_RIGHT(rect);
			/*
			 * Clear content area.
			 */
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
			gdk_cairo_rectangle(cr, &rect);
			cairo_fill(cr);
			cairo_restore(cr);

#if 0
			/*
			 * XXX: Draw line helper for debugging.
			 */
			cairo_save(cr);
			cairo_set_source_rgb(cr, .3, .3, .3);
			cairo_rectangle(cr,
			                rect.x,
			                rect.y + (rect.height / (gfloat)priv->x_slots * priv->dps_slot),
			                rect.width,
			                rect.height / priv->x_slots);
			cairo_fill(cr);
			cairo_restore(cr);
#endif

			/*
			 * Render new content clipped.
			 */
			cairo_save(cr);
			cairo_reset_clip(cr);
			gdk_cairo_rectangle(cr, &rect);
			cairo_clip(cr);
			/*
			 * Determine area for this draw.
			 */
			UBER_GRAPH_GET_CLASS(graph)->render_fast(graph,
			                                         cr,
			                                         &rect,
			                                         x_epoch,
			                                         each + .5);
			cairo_restore(cr);
		} else {
			/*
			 * Clear content area.
			 */
			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
			gdk_cairo_rectangle(cr, &rect);
			cairo_fill(cr);
			cairo_restore(cr);
			/*
			 * Draw the entire foreground.
			 */
			if (UBER_GRAPH_GET_CLASS(graph)->render) {
				priv->dps_slot = 0;
				cairo_save(cr);
				gdk_cairo_rectangle(cr, &priv->nonvis_rect);
				cairo_clip(cr);
				UBER_GRAPH_GET_CLASS(graph)->render(graph,
				                                    cr,
				                                    &priv->nonvis_rect,
				                                    x_epoch,
				                                    each);
				cairo_restore(cr);
			}
		}
	}
	/*
	 * Foreground is no longer dirty.
	 */
	priv->fg_dirty = FALSE;
	priv->full_draw = FALSE;
	/*
	 * Cleanup.
	 */
	cairo_destroy(cr);
}

/**
 * uber_graph_redraw:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_redraw (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->fg_dirty = TRUE;
	priv->bg_dirty = TRUE;
	priv->full_draw = TRUE;
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}

/**
 * uber_graph_get_yrange:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
uber_graph_get_yrange (UberGraph *graph, /* IN */
                       UberRange *range) /* OUT */
{
	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(range != NULL);

	memset(range, 0, sizeof(*range));
	if (UBER_GRAPH_GET_CLASS(graph)->get_yrange) {
		UBER_GRAPH_GET_CLASS(graph)->get_yrange(graph, range);
	}
}

/**
 * uber_graph_set_format:
 * @graph: A UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_set_format (UberGraph       *graph, /* IN */
                       UberGraphFormat  format) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->format = format;
	priv->bg_dirty = TRUE;
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}

static void
uber_graph_render_x_axis (UberGraph *graph, /* IN */
                          cairo_t   *cr)    /* IN */
{
	UberGraphPrivate *priv;
	const gdouble dashes[] = { 1.0, 2.0 };
	PangoFontDescription *fd;
	PangoLayout *pl;
	GtkStyleContext *style;
    GdkRGBA fg_color;
	gfloat each;
	gfloat x;
	gfloat y;
	gfloat h;
	gchar text[16] = { 0 };
	gint count;
	gint wi;
	gint hi;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	style = gtk_widget_get_style_context(GTK_WIDGET(graph));
    gtk_style_context_get_color(style, GTK_STATE_FLAG_NORMAL, &fg_color);

	count = priv->x_slots / 10;
	each = priv->content_rect.width / (gfloat)count;
	/*
	 * Draw ticks.
	 */
	cairo_save(cr);
	pl = pango_cairo_create_layout(cr);
	fd = pango_font_description_new();
	pango_font_description_set_family_static(fd, "Monospace");
	pango_font_description_set_size(fd, 6 * PANGO_SCALE);
	pango_layout_set_font_description(pl, fd);
	gdk_cairo_set_source_rgba(cr, &fg_color);
	cairo_set_line_width(cr, 1.0);
	cairo_set_dash(cr, dashes, G_N_ELEMENTS(dashes), 0);
	for (i = 0; i <= count; i++) {
		x = RECT_RIGHT(priv->content_rect) - (gint)(i * each) + .5;
		if (priv->show_xlines && (i != 0 && i != count)) {
			y = priv->content_rect.y;
			h = priv->content_rect.height + priv->tick_len;
		} else {
			y = priv->content_rect.y + priv->content_rect.height;
			h = priv->tick_len;
		}
		if (i != 0 && i != count) {
			cairo_move_to(cr, x, y);
			cairo_line_to(cr, x, y + h);
			cairo_stroke(cr);
		}
		/*
		 * Render the label.
		 */
		if (priv->show_xlabels) {
			g_snprintf(text, sizeof(text), "%d", i * 10);
			pango_layout_set_text(pl, text, -1);
			pango_layout_get_pixel_size(pl, &wi, &hi);
			if (i != 0 && i != count) {
				cairo_move_to(cr, x - (wi / 2), y + h);
			} else if (i == 0) {
				cairo_move_to(cr,
				              RECT_RIGHT(priv->content_rect) - (wi / 2),
				              RECT_BOTTOM(priv->content_rect) + priv->tick_len);
			} else if (i == count) {
				cairo_move_to(cr,
				              priv->content_rect.x - (wi / 2),
				              RECT_BOTTOM(priv->content_rect) + priv->tick_len);
			}
			pango_cairo_show_layout(cr, pl);
		}
	}
	g_object_unref(pl);
	pango_font_description_free(fd);
	cairo_restore(cr);
}

static void G_GNUC_PRINTF(6, 7)
uber_graph_render_y_line (UberGraph   *graph,     /* IN */
                          cairo_t     *cr,        /* IN */
                          gint         y,         /* IN */
                          gboolean     tick_only, /* IN */
                          gboolean     no_label,  /* IN */
                          const gchar *format,    /* IN */
                          ...)                    /* IN */
{
	UberGraphPrivate *priv;
	const gdouble dashes[] = { 1.0, 2.0 };
	PangoFontDescription *fd;
	PangoLayout *pl;
	GtkStyleContext *style;
    GdkRGBA fg_color;
	va_list args;
	gchar *text;
	gint width;
	gint height;
	gfloat real_y = y + .5;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(cr != NULL);
	g_return_if_fail(format != NULL);

	priv = graph->priv;
	style = gtk_widget_get_style_context(GTK_WIDGET(graph));
    gtk_style_context_get_color(style, GTK_STATE_FLAG_NORMAL, &fg_color);
	/*
	 * Draw grid line.
	 */
	cairo_save(cr);
	cairo_set_dash(cr, dashes, G_N_ELEMENTS(dashes), 0);
	cairo_set_line_width(cr, 1.0);
	gdk_cairo_set_source_rgba(cr, &fg_color);
	cairo_move_to(cr, priv->content_rect.x - priv->tick_len, real_y);
	if (tick_only) {
		cairo_line_to(cr, priv->content_rect.x, real_y);
	} else {
		cairo_line_to(cr, RECT_RIGHT(priv->content_rect), real_y);
	}
	cairo_stroke(cr);
	cairo_restore(cr);
	/*
	 * Show label.
	 */
	if (!no_label) {
		cairo_save(cr);
		gdk_cairo_set_source_rgba(cr, &fg_color);
		/*
		 * Format text.
		 */
		va_start(args, format);
		text = g_strdup_vprintf(format, args);
		va_end(args);
		/*
		 * Render pango layout.
		 */
		pl = pango_cairo_create_layout(cr);
		fd = pango_font_description_new();
		pango_font_description_set_family_static(fd, "Monospace");
		pango_font_description_set_size(fd, 6 * PANGO_SCALE);
		pango_layout_set_font_description(pl, fd);
		pango_layout_set_text(pl, text, -1);
		pango_layout_get_pixel_size(pl, &width, &height);
		cairo_move_to(cr, priv->content_rect.x - priv->tick_len - width - 3,
		              real_y - height / 2);
		pango_cairo_show_layout(cr, pl);
		/*
		 * Cleanup resources.
		 */
		g_free(text);
		pango_font_description_free(fd);
		g_object_unref(pl);
		cairo_restore(cr);
	}
}

static void
uber_graph_render_y_axis_percent (UberGraph *graph,       /* IN */
                                  cairo_t   *cr,          /* IN */
                                  UberRange *range,       /* IN */
                                  UberRange *pixel_range, /* IN */
                                  gint       n_lines)     /* IN */
{
	static const gchar format[] = "%0.0f %%";
	UberGraphPrivate *priv;
	gdouble value;
	gdouble y;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(range != NULL);
	g_return_if_fail(pixel_range != NULL);
	g_return_if_fail(n_lines >= 0);
	g_return_if_fail(n_lines < 6);

	priv = graph->priv;
	/*
	 * Render top and bottom lines.
	 */
	uber_graph_render_y_line(graph, cr,
	                         priv->content_rect.y - 1,
	                         TRUE, FALSE, format, 100.);
	uber_graph_render_y_line(graph, cr,
	                         RECT_BOTTOM(priv->content_rect),
	                         TRUE, FALSE, format, 0.);
	/*
	 * Render lines between the edges.
	 */
	for (i = 1; i < n_lines; i++) {
		value = (n_lines - i) / (gfloat)n_lines;
		y = pixel_range->end - (pixel_range->range * value);
		value *= 100.;
		uber_graph_render_y_line(graph, cr, y,
		                         !priv->show_ylines, FALSE,
		                         format, value);
	}
}

/**
 * uber_graph_render_y_axis_direct:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_render_y_axis_direct (UberGraph *graph,       /* IN */
                                 cairo_t   *cr,          /* IN */
                                 UberRange *range,       /* IN */
                                 UberRange *pixel_range, /* IN */
                                 gint       n_lines,     /* IN */
                                 gboolean   kibi)        /* IN */
{
	static const gchar format[] = "%0.1f%s";
	const gchar *modifier = "";
	UberGraphPrivate *priv;
	gdouble value;
	gdouble y;
	gint i;

	g_return_if_fail(UBER_IS_GRAPH(graph));

#define CONDENSE(v) \
G_STMT_START { \
    if (kibi) { \
        if ((v) >= 1073741824.) { \
            (v) /= 1073741824.; \
            modifier = " Gi"; \
        } else if ((v) >= 1048576.) { \
            (v) /= 1048576.; \
            modifier = " Mi"; \
        } else if ((v) >= 1024.) { \
            (v) /= 1024.; \
            modifier = " Ki"; \
        } else { \
            modifier = ""; \
        } \
    } else {  \
        if ((v) >= 1000000000.) { \
            (v) /= 1000000000.; \
            modifier = " G"; \
        } else if ((v) >= 1000000.) { \
            (v) /= 1000000.; \
            modifier = " M"; \
        } else if ((v) >= 1000.) { \
            (v) /= 1000.; \
            modifier = " K"; \
        } else { \
            modifier = ""; \
        } \
    } \
} G_STMT_END

	priv = graph->priv;
	/*
	 * Render top and bottom lines.
	 */
	value = range->end;
	CONDENSE(value);
	uber_graph_render_y_line(graph, cr,
	                         priv->content_rect.y - 1,
	                         TRUE, FALSE, format, value, modifier);
	value = range->begin;
	CONDENSE(value);
	uber_graph_render_y_line(graph, cr,
	                         RECT_BOTTOM(priv->content_rect),
	                         TRUE, FALSE, format, value, modifier);
	/*
	 * Render lines between the edges.
	 */
	for (i = 1; i < n_lines; i++) {
		y = value = range->range / (gfloat)(n_lines) * (gfloat)i;
		/*
		 * TODO: Use proper scale.
		 */
		uber_scale_linear(range, pixel_range, &y, NULL);
		if (y == 0 || y == pixel_range->begin || y == pixel_range->end) {
			continue;
		}
		y = pixel_range->end - y;
		CONDENSE(value);
		uber_graph_render_y_line(graph, cr, y,
		                         !priv->show_ylines,
		                         (range->begin == range->end),
		                         format, value, modifier);
	}
}

/**
 * uber_graph_render_y_axis:
 * @graph: A #UberGraph.
 *
 * Render the Y axis grid lines and labels.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_render_y_axis (UberGraph *graph, /* IN */
                          cairo_t   *cr)    /* IN */
{
	UberGraphPrivate *priv;
	UberRange pixel_range;
	UberRange range;
	gint n_lines;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	/*
	 * Calculate ranges.
	 */
	uber_graph_get_yrange(graph, &range);
	pixel_range.begin = priv->content_rect.y;
	pixel_range.end = priv->content_rect.y + priv->content_rect.height;
	pixel_range.range = pixel_range.end - pixel_range.begin;
	/*
	 * Render grid lines for given format.
	 */
	n_lines = MIN(priv->content_rect.height / 25, 5);
	switch (priv->format) {
	case UBER_GRAPH_FORMAT_PERCENT:
		uber_graph_render_y_axis_percent(graph, cr, &range, &pixel_range,
		                                 n_lines);
		break;
	case UBER_GRAPH_FORMAT_DIRECT:
		uber_graph_render_y_axis_direct(graph, cr, &range, &pixel_range,
		                                n_lines, FALSE);
		break;
	case UBER_GRAPH_FORMAT_DIRECT1024:
		uber_graph_render_y_axis_direct(graph, cr, &range, &pixel_range,
		                                n_lines, TRUE);
		break;
	default:
		g_assert_not_reached();
	}
}

/**
 * uber_graph_render_bg:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_render_bg (UberGraph *graph) /* IN */
{
	const gdouble dashes[] = { 1.0, 2.0 };
	UberGraphPrivate *priv;
	GtkAllocation alloc;
	GtkStyleContext *style;
    GdkRGBA fg_color;
    GdkRGBA light_color;
	cairo_t *cr;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	/*
	 * Acquire resources.
	 */
	priv = graph->priv;
	gtk_widget_get_allocation(GTK_WIDGET(graph), &alloc);
	style = gtk_widget_get_style_context(GTK_WIDGET(graph));
    gtk_style_context_get_color(style, GTK_STATE_FLAG_NORMAL, &fg_color);
    gtk_style_context_get_color(style, GTK_STATE_FLAG_SELECTED, &light_color);
	cr = cairo_create(priv->bg_surface);
	/*
	 * Ensure valid resources.
	 */
	g_assert(style);
	g_assert(priv->bg_surface);
	/*
	 * Clear entire background.  Hopefully this looks okay for RGBA themes
	 * that are translucent.
	 */
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	cairo_restore(cr);
	/*
	 * Paint the content area background.
	 */
	cairo_save(cr);
	gdk_cairo_set_source_rgba(cr, &light_color);
	gdk_cairo_rectangle(cr, &priv->content_rect);
	cairo_fill(cr);
	cairo_restore(cr);
	/*
	 * Stroke the border around the content area.
	 */
	cairo_save(cr);
	gdk_cairo_set_source_rgba(cr, &fg_color);
	cairo_set_line_width(cr, 1.0);
	cairo_set_dash(cr, dashes, G_N_ELEMENTS(dashes), 0);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_rectangle(cr,
	                priv->content_rect.x - .5,
	                priv->content_rect.y - .5,
	                priv->content_rect.width + 1.0,
	                priv->content_rect.height + 1.0);
	cairo_stroke(cr);
	cairo_restore(cr);
	/*
	 * Render the axis ticks.
	 */
	uber_graph_render_y_axis(graph, cr);
	uber_graph_render_x_axis(graph, cr);
	/*
	 * Background is no longer dirty.
	 */
	priv->bg_dirty = FALSE;
	/*
	 * Cleanup.
	 */
	cairo_destroy(cr);
}

static inline void
g_time_val_subtract (GTimeVal *a, /* IN */
                     GTimeVal *b, /* IN */
                     GTimeVal *c) /* OUT */
{
	g_return_if_fail(a != NULL);
	g_return_if_fail(b != NULL);
	g_return_if_fail(c != NULL);

	c->tv_sec = a->tv_sec - b->tv_sec;
	c->tv_usec = a->tv_usec - b->tv_usec;
	if (c->tv_usec < 0) {
		c->tv_usec += G_USEC_PER_SEC;
		c->tv_sec -= 1;
	}
}

/**
 * uber_graph_get_fps_offset:
 * @graph: A #UberGraph.
 *
 * Calculates the number of pixels that the foreground should be rendered
 * from the origin.
 *
 * Returns: The pixel offset to render the foreground.
 * Side effects: None.
 */
static gfloat
uber_graph_get_fps_offset (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;
	GTimeVal rel = { 0 };
	GTimeVal tv;
	gfloat f;

	g_return_val_if_fail(UBER_IS_GRAPH(graph), 0.);

	priv = graph->priv;
	g_get_current_time(&tv);
	g_time_val_subtract(&tv, &priv->dps_tv, &rel);
	f = ((rel.tv_sec * 1000) + (rel.tv_usec / 1000))
	  / (1000. / priv->dps) /* MSec Per Data Point */
	  * priv->dps_each;     /* Pixels Per Data Point */
	return MIN(f, (priv->dps_each - priv->fps_each));
}

/**
 * uber_graph_draw:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: %FALSE always.
 * Side effects: None.
 */
static gboolean
uber_graph_draw 	(GtkWidget      *widget, /* IN */
                         cairo_t        *cr) /* IN */
{
	UberGraphPrivate *priv;
	GtkAllocation alloc;
//	cairo_t *cr;
	gfloat offset;
	gint x;

	g_return_val_if_fail(UBER_IS_GRAPH(widget), FALSE);

	priv = UBER_GRAPH(widget)->priv;
	gtk_widget_get_allocation(widget, &alloc);
	priv->fps_count++;
	/*
	 * Ensure that the texture is initialized.
	 */
	g_assert(priv->fg_surface);
	g_assert(priv->bg_surface);
	/*
	 * Clear window background.
	 */
#if 0
	gdk_window_clear_area(expose->window,
	                      expose->area.x,
	                      expose->area.y,
	                      expose->area.width,
	                      expose->area.height);
	/*
	 * Allocate resources.
	 */
	cr = gdk_cairo_create(expose->window);
	/*
	 * Clip to exposure area.
	 */
	gdk_cairo_rectangle(cr, &expose->area);
	cairo_clip(cr);
#endif
	/*
	 * Render background or foreground if needed.
	 */
	if (priv->bg_dirty) {
		uber_graph_render_bg(UBER_GRAPH(widget));
	}
	if (priv->fg_dirty) {
		uber_graph_render_fg(UBER_GRAPH(widget));
	}
	/*
	 * Paint the background to the exposure area.
	 */
	cairo_save(cr);
	cairo_set_source_surface(cr, priv->bg_surface, 0, 0);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	cairo_restore(cr);
	/*
	 * Draw the foreground.
	 */
	offset = uber_graph_get_fps_offset(UBER_GRAPH(widget));
	if (priv->have_rgba) {
		cairo_save(cr);
		/*
		 * Clip exposure to the content area.
		 */
		cairo_reset_clip(cr);
		gdk_cairo_rectangle(cr, &priv->content_rect);
		cairo_clip(cr);
		/*
		 * Data in the fg surface is a ring bufer. Render the first portion
		 * at its given offset.
		 */
		x = ((priv->x_slots - priv->dps_slot) * priv->dps_each) - offset;
		cairo_set_source_surface(cr, priv->fg_surface, (gint)x, 0);
		gdk_cairo_rectangle(cr, &priv->content_rect);
		cairo_fill(cr);
		/*
		 * Render the second part of the ring surface buffer.
		 */
		x = (priv->dps_each * -priv->dps_slot) - offset;
		cairo_set_source_surface(cr, priv->fg_surface, (gint)x, 0);
		gdk_cairo_rectangle(cr, &priv->content_rect);
		cairo_fill(cr);
		/*
		 * Cleanup.
		 */
		cairo_restore(cr);
	} else {
		/*
		 * TODO: Use XOR command for fallback.
		 */
		g_warn_if_reached();
	}
	/*
	 * Cleanup resources.
	 */
	//cairo_destroy(cr);
	return FALSE;
}

/**
 * uber_graph_style_set:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_style_set (GtkWidget *widget,    /* IN */
                      GtkStyle  *old_style) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	priv = UBER_GRAPH(widget)->priv;
	WIDGET_CLASS->style_set(widget, old_style);
	priv->fg_dirty = TRUE;
	priv->bg_dirty = TRUE;
	gtk_widget_queue_draw(widget);
}

/**
 * uber_graph_size_allocate:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_size_allocate (GtkWidget     *widget, /* IN */
                          GtkAllocation *alloc)  /* IN */
{
	UberGraph *graph;
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(widget));

	graph = UBER_GRAPH(widget);
	priv = graph->priv;
	WIDGET_CLASS->size_allocate(widget, alloc);
	/*
	 * If there is no window yet, we can defer setup.
	 */
	if (!gtk_widget_get_window(widget)) {
		return;
	}
	/*
	 * Recalculate rectangles.
	 */
	uber_graph_calculate_rects(graph);
	/*
	 * Recreate server side surface.
	 */
	UNSET_SURFACE(priv->bg_surface);
	UNSET_SURFACE(priv->fg_surface);
	uber_graph_init_bg(graph);
	uber_graph_init_texture(graph);
	/*
	 * Mark foreground and background as dirty.
	 */
	priv->fg_dirty = TRUE;
	priv->bg_dirty = TRUE;
	priv->full_draw = TRUE;
	gtk_widget_queue_draw(widget);
}

static void
uber_graph_size_request (GtkWidget      *widget, /* IN */
                         GtkRequisition *req)    /* OUT */
{
	g_return_if_fail(req != NULL);

	req->width = 150;
	req->height = 50;
}

static void
uber_graph_get_preferred_width (GtkWidget      *widget, /* IN */
                                gint           *minimal_width,
                                gint           *natural_width)
{
	GtkRequisition requisition;
	uber_graph_size_request(widget, &requisition);
	*minimal_width = * natural_width = requisition.width;
}

static void
uber_graph_get_preferred_height (GtkWidget      *widget, /* IN */
                                 gint           *minimal_height,
                                 gint           *natural_height)
{
	GtkRequisition requisition;
	uber_graph_size_request(widget, &requisition);
	*minimal_height = * natural_height = requisition.height;
}

/**
 * uber_graph_add_label:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
void
uber_graph_add_label (UberGraph *graph, /* IN */
                      UberLabel *label) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));
	g_return_if_fail(UBER_IS_LABEL(label));

	priv = graph->priv;
	gtk_box_pack_start(GTK_BOX(priv->labels), GTK_WIDGET(label),
	                   TRUE, TRUE, 0);
	gtk_widget_show(GTK_WIDGET(label));
}

/**
 * uber_graph_take_screenshot:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_take_screenshot (UberGraph *graph) /* IN */
{
	GtkWidget *widget;
	GtkWidget *dialog;
	GtkAllocation alloc;
	const gchar *filename;
	cairo_status_t status;
	cairo_surface_t *surface;
	cairo_t *cr;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	widget = GTK_WIDGET(graph);
	gtk_widget_get_allocation(widget, &alloc);

	/*
	 * Create save dialog and ask user for filename.
	 */
	dialog = gtk_file_chooser_dialog_new(_("Save As"),
	                                     GTK_WINDOW(gtk_widget_get_toplevel(widget)),
	                                     GTK_FILE_CHOOSER_ACTION_SAVE,
	                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                     _("_Save"), GTK_RESPONSE_ACCEPT,
	                                     NULL);
	if (GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog))) {
		/*
		 * Create surface and cairo context.
		 */
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		                                     alloc.width, alloc.height);
		cr = cairo_create(surface);
		cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
		cairo_clip(cr);

		/* Paint to the image surface instead of the screen */
		uber_graph_draw(widget, cr);

		/*
		 * Save surface to png.
		 */
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		status = cairo_surface_write_to_png(surface, filename);
		if (status != CAIRO_STATUS_SUCCESS) {
			g_critical("Failed to save pixmap to file.");
			goto cleanup;
		}
		/*
		 * Cleanup resources.
		 */
	  cleanup:
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
	}
	gtk_widget_destroy(dialog);
}

/**
 * uber_graph_toggle_paused:
 * @graph: A #UberGraph.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_toggle_paused (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	g_return_if_fail(UBER_IS_GRAPH(graph));

	priv = graph->priv;
	priv->paused = !priv->paused;
	if (priv->fps_handler) {
		g_source_remove(priv->fps_handler);
		priv->fps_handler = 0;
	} else {
		if (!priv->paused) {
			uber_graph_redraw(graph);
		}
		uber_graph_register_fps_handler(graph);
	}
}

/**
 * uber_graph_button_press:
 * @widget: A #GtkWidget.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
uber_graph_button_press_event (GtkWidget      *widget, /* IN */
                               GdkEventButton *button) /* IN */
{
	g_return_val_if_fail(UBER_IS_GRAPH(widget), FALSE);

	switch (button->button) {
	case 2: /* Middle Click */
		if (button->state & GDK_CONTROL_MASK) {
			uber_graph_take_screenshot(UBER_GRAPH(widget));
		} else {
			uber_graph_toggle_paused(UBER_GRAPH(widget));
		}
		break;
	default:
		break;
	}
	return FALSE;
}

/**
 * uber_graph_finalize:
 * @object: A #UberGraph.
 *
 * Finalizer for a #UberGraph instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(uber_graph_parent_class)->finalize(object);
}

/**
 * uber_graph_dispose:
 * @object: A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
uber_graph_dispose (GObject *object) /* IN */
{
	UberGraph *graph;
	UberGraphPrivate *priv;

	graph = UBER_GRAPH(object);
	priv = graph->priv;
	/*
	 * Stop any timeout handlers.
	 */
	if (priv->fps_handler) {
		g_source_remove(priv->fps_handler);
	}
	if (priv->dps_handler) {
		g_source_remove(priv->dps_handler);
	}
	/*
	 * Destroy textures.
	 */
	UNSET_SURFACE(priv->bg_surface);
	UNSET_SURFACE(priv->fg_surface);
	/*
	 * Call base class.
	 */
	G_OBJECT_CLASS(uber_graph_parent_class)->dispose(object);
}

/**
 * uber_graph_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
uber_graph_get_property (GObject    *object,  /* IN */
                         guint       prop_id, /* IN */
                         GValue     *value,   /* OUT */
                         GParamSpec *pspec)   /* IN */
{
	UberGraph *graph = UBER_GRAPH(object);

	switch (prop_id) {
	case PROP_FORMAT:
		g_value_set_uint(value, graph->priv->format);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * uber_graph_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
uber_graph_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
	UberGraph *graph = UBER_GRAPH(object);

	switch (prop_id) {
	case PROP_FORMAT:
		uber_graph_set_format(graph, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * uber_graph_class_init:
 * @klass: A #UberGraphClass.
 *
 * Initializes the #UberGraphClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_class_init (UberGraphClass *klass) /* IN */
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = uber_graph_dispose;
	object_class->finalize = uber_graph_finalize;
	object_class->get_property = uber_graph_get_property;
	object_class->set_property = uber_graph_set_property;
	g_type_class_add_private(object_class, sizeof(UberGraphPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->draw = uber_graph_draw;
	widget_class->hide = uber_graph_hide;
	widget_class->realize = uber_graph_realize;
	widget_class->screen_changed = uber_graph_screen_changed;
	widget_class->show = uber_graph_show;
	widget_class->size_allocate = uber_graph_size_allocate;
	widget_class->style_set = uber_graph_style_set;
	widget_class->unrealize = uber_graph_unrealize;
	widget_class->get_preferred_width = uber_graph_get_preferred_width;
	widget_class->get_preferred_height = uber_graph_get_preferred_height;
	widget_class->button_press_event = uber_graph_button_press_event;

	show_fps = !!g_getenv("UBER_SHOW_FPS");

	/*
	 * FIXME: Use enum.
	 */
	g_object_class_install_property(object_class,
	                                PROP_FORMAT,
	                                g_param_spec_uint("format",
	                                                  "format",
	                                                  "format",
	                                                  0,
	                                                  UBER_GRAPH_FORMAT_PERCENT,
	                                                  0,
	                                                  G_PARAM_READWRITE));
}

/**
 * uber_graph_init:
 * @graph: A #UberGraph.
 *
 * Initializes the newly created #UberGraph instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
uber_graph_init (UberGraph *graph) /* IN */
{
	UberGraphPrivate *priv;

	/*
	 * Store pointer to private data allocation.
	 */
	graph->priv = G_TYPE_INSTANCE_GET_PRIVATE(graph,
	                                          UBER_TYPE_GRAPH,
	                                          UberGraphPrivate);
	priv = graph->priv;
	/*
	 * Enable required events.
	 */
	gtk_widget_set_events(GTK_WIDGET(graph), GDK_BUTTON_PRESS_MASK);
	/*
	 * Prepare default values.
	 */
	priv->tick_len = 10;
	priv->fps = 20;
	priv->fps_real = 1000. / priv->fps;
	priv->dps = 1.;
	priv->x_slots = 60;
	priv->fg_dirty = TRUE;
	priv->bg_dirty = TRUE;
	priv->full_draw = TRUE;
	priv->show_xlines = TRUE;
	priv->show_ylines = TRUE;
	/*
	 * TODO: Support labels in a grid.
	 */
	priv->labels = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_box_set_homogeneous (GTK_BOX(priv->labels), TRUE);
	priv->align = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(priv->align), priv->labels);
	gtk_widget_show(priv->labels);
}
