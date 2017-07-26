/*
 * Christian Hergert's uber-graph (GPL3)
 * wrapped in an interface compatible with
 * Leandro A. F. Pereira's loadgraph (GPL2.1).
 */

#include <string.h>
#include "loadgraph.h"
#include "uber.h"

struct _LoadGraph {
    GtkWidget *uber_widget;
    gdouble cur_value;
    gint height;
};

gdouble
_sample_func (UberLineGraph *graph,
                 guint      line,
                 gpointer   user_data)
{
    LoadGraph *lg = (LoadGraph *)user_data;
        return lg->cur_value;
}

LoadGraph *load_graph_new(gint size)
{
    LoadGraph *lg;
    GdkRGBA color;

    lg = g_new0(LoadGraph, 1);

    lg->height = (size+1) * 2; /* idk */
    lg->cur_value = 0.0;
    lg->uber_widget = uber_line_graph_new();

    uber_line_graph_set_autoscale(UBER_LINE_GRAPH(lg->uber_widget), TRUE);
    GtkWidget *label = uber_label_new();
    uber_label_set_text(UBER_LABEL(label), "BLAH!");
    gdk_rgba_parse(&color, "#729fcf");
    uber_line_graph_add_line(UBER_LINE_GRAPH(lg->uber_widget), &color, UBER_LABEL(label));
    uber_line_graph_set_data_func(UBER_LINE_GRAPH(lg->uber_widget),
                (UberLineGraphFunc)_sample_func, (gpointer *)lg, NULL);

    return lg;
}

void load_graph_set_data_suffix(LoadGraph * lg, gchar * suffix)
{

}

gchar *load_graph_get_data_suffix(LoadGraph * lg)
{
    return strdup("");
}

GtkWidget *load_graph_get_framed(LoadGraph * lg)
{
    if (lg != NULL)
        return lg->uber_widget;
    return NULL;
}

void load_graph_clear(LoadGraph * lg)
{

}

void load_graph_set_color(LoadGraph * lg, LoadGraphColor color)
{

}

void load_graph_destroy(LoadGraph * lg)
{
    if (lg != NULL) {
        g_object_unref(lg->uber_widget);
        g_free(lg);
    }
}

static gboolean _expose(GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
    return TRUE;
}

void load_graph_configure_expose(LoadGraph * lg)
{

}

void load_graph_update(LoadGraph * lg, gdouble value)
{
    if (lg != NULL)
        lg->cur_value = value;
}

gint load_graph_get_height(LoadGraph *lg) {
    if (lg != NULL)
        return lg->height;
    return 0;
}
