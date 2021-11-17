/*
 * Christian Hergert's uber-graph (GPL3)
 * wrapped in an interface compatible with
 * L. A. F. Pereira's loadgraph (GPL2.1).
 */

#include <string.h>
#include "loadgraph.h"
#include "uber.h"

#define LG_MAX_LINES 9

static const gchar *default_colors[] = { "#73d216",
                                         "#f57900",
     /*colors from simple.c sample */    "#3465a4",
                                         "#ef2929",
                                         "#75507b",
                                         "#ce5c00",
                                         "#c17d11",
                                         "#ce5c00",
                                         "#729fcf",
                                         NULL };

struct _LoadGraph {
    GtkWidget *uber_widget;
    gdouble cur_value[LG_MAX_LINES];
    gint height;
};

gdouble
_sample_func (UberLineGraph *graph,
                 guint      line,
                 gpointer   user_data)
{
    LoadGraph *lg = (LoadGraph *)user_data;
    return lg->cur_value[line-1];
}

LoadGraph *load_graph_new(gint size)
{
    LoadGraph *lg;
    GdkRGBA color;
    int i = 0;

    lg = g_new0(LoadGraph, 1);
    lg->uber_widget = uber_line_graph_new();
    lg->height = (size+1) * 2; /* idk */
    for (i = 0; i < LG_MAX_LINES; i++) {
        lg->cur_value[i] = UBER_LINE_GRAPH_NO_VALUE;
        //GtkWidget *label = uber_label_new();
        //uber_label_set_text(UBER_LABEL(label), "BLAH!");
        gdk_rgba_parse(&color, default_colors[i]);
        uber_line_graph_add_line(UBER_LINE_GRAPH(lg->uber_widget), &color, NULL); /* UBER_LABEL(label) */
    }
    uber_line_graph_set_autoscale(UBER_LINE_GRAPH(lg->uber_widget), TRUE);
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

void load_graph_set_title(LoadGraph * lg, const gchar *title)
{
}

void load_graph_clear(LoadGraph * lg)
{
    int i;
    if (lg != NULL) {
        for (i = 0; i < LG_MAX_LINES; i++) {
            lg->cur_value[i] = UBER_LINE_GRAPH_NO_VALUE;
        }
        uber_graph_scale_changed(UBER_GRAPH(lg->uber_widget));
    }
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

void load_graph_update_ex(LoadGraph *lg, guint line, gdouble value)
{
    if (lg != NULL && line < LG_MAX_LINES)
        lg->cur_value[line] = value;
}

void load_graph_update(LoadGraph * lg, gdouble value)
{
    load_graph_update_ex(lg, 0, value);
}

gint load_graph_get_height(LoadGraph *lg) {
    if (lg != NULL)
        return lg->height;
    return 0;
}
