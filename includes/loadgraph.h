/*
 * Simple Load Graph
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __LOADGRAPH_H__
#define __LOADGRAPH_H__

#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>

typedef struct _LoadGraph LoadGraph;

typedef enum {
    LG_COLOR_GREEN = 0x4FB05A,
    LG_COLOR_BLUE  = 0x4F58B0,
    LG_COLOR_RED   = 0xB04F4F
} LoadGraphColor;

LoadGraph   *load_graph_new(gint size);
void         load_graph_destroy(LoadGraph *lg);
void         load_graph_configure_expose(LoadGraph *lg);
GtkWidget   *load_graph_get_framed(LoadGraph *lg);

void         load_graph_update(LoadGraph *lg, gdouble value);
void         load_graph_update_ex(LoadGraph *lg, guint line, gdouble value);

void         load_graph_set_color(LoadGraph *lg, LoadGraphColor color);
void         load_graph_clear(LoadGraph *lg);

void         load_graph_set_data_suffix(LoadGraph *lg, gchar *suffix);
gchar       *load_graph_get_data_suffix(LoadGraph *lg);

void         load_graph_set_title(LoadGraph *lg, const gchar *title);
const gchar *load_graph_get_title(LoadGraph *lg);

gint         load_graph_get_height(LoadGraph *lg);

#endif  /* __LOADGRAPH_H__ */
