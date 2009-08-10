/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <gtk/gtk.h>

#define N_ITERATIONS 100000

typedef double (*BenchCallback)(GtkWindow *window);

double test_lines(GtkWindow *window);
double test_shapes(GtkWindow *window);
double test_filled_shapes(GtkWindow *window);
double test_text(GtkWindow *window);

/*
Results on a AMD Athlon 3200+ (Barton), 1GB RAM,
nVidia Geforce 6200 with nvidia Xorg driver.

Test                Time           Iter/Sec       
Line Drawing        3.9570     25271.7663 
Shape Drawing       22.2499    4494.4065  
Filled Shape Drawing4.0377     24766.2806 
Text Drawing        59.1565    1690.4309  

Results are normalized according to these values.
So a value around 1.0 will be similar to this 
computer.
*/

static struct {
  BenchCallback callback;
  gchar *title;
  gdouble weight;
} tests[] = {
  { test_lines, "Line Drawing", 25271.77 },
  { test_shapes, "Shape Drawing", 4494.49 },
  { test_filled_shapes, "Filled Shape Drawing", 24766.28 },
  { test_text, "Text Drawing", 1690.43  },
  { NULL, NULL }
};

double test_text(GtkWindow *window)
{
  GRand *rand;
  GTimer *timer;
  GdkGC *gc;
  double time;
  PangoLayout *layout;
  PangoFontDescription *font;
  GdkWindow *gdk_window = GTK_WIDGET(window)->window;
  int strings;
  gchar *tmp;
  
  gdk_window_clear(gdk_window);
  
  rand = g_rand_new();
  gc = gdk_gc_new(GDK_DRAWABLE(gdk_window));
  timer = g_timer_new();
  
  font = pango_font_description_new();
  layout = pango_layout_new(gtk_widget_get_pango_context(GTK_WIDGET(window)));
  pango_layout_set_text(layout, "I \342\231\245 HardInfo", -1);
  
  g_timer_start(timer);
  for (strings = N_ITERATIONS; strings >= 0; strings--) {
    int x, y, size;

    x = g_rand_int_range(rand, 0, 800);
    y = g_rand_int_range(rand, 0, 600);
    size = g_rand_int_range(rand, 1, 96) * PANGO_SCALE;
    
    pango_font_description_set_size(font, size);
    pango_layout_set_font_description(layout, font);
    gdk_draw_layout(GDK_DRAWABLE(gdk_window), gc, x, y, layout);
    
    gdk_rgb_gc_set_foreground(gc, strings << 8);

    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
    
  }
  g_timer_stop(timer);
  
  time = g_timer_elapsed(timer, NULL);
  
  g_rand_free(rand);
  gdk_gc_destroy(gc);
  g_timer_destroy(timer);
  g_object_unref(layout);
  pango_font_description_free(font);
  
  return time;
}

double test_filled_shapes(GtkWindow *window)
{
  GRand *rand;
  GTimer *timer;
  GdkGC *gc;
  double time;
  GdkWindow *gdk_window = GTK_WIDGET(window)->window;
  int lines;
  
  gdk_window_clear(gdk_window);
  
  rand = g_rand_new();
  gc = gdk_gc_new(GDK_DRAWABLE(gdk_window));
  timer = g_timer_new();
  
  g_timer_start(timer);
  for (lines = N_ITERATIONS; lines >= 0; lines--) {
    int x1, y1;
    
    x1 = g_rand_int_range(rand, 0, 800);
    y1 = g_rand_int_range(rand, 0, 600);
    
    gdk_rgb_gc_set_foreground(gc, lines << 8);

    gdk_draw_rectangle(GDK_DRAWABLE(gdk_window), gc, TRUE,
                       x1, y1,
                       g_rand_int_range(rand, 0, 400),
                       g_rand_int_range(rand, 0, 300));

    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }
  g_timer_stop(timer);
  
  time = g_timer_elapsed(timer, NULL);
  
  g_rand_free(rand);
  gdk_gc_destroy(gc);
  g_timer_destroy(timer);
  
  return time;
}

double test_shapes(GtkWindow *window)
{
  GRand *rand;
  GTimer *timer;
  GdkGC *gc;
  double time;
  GdkWindow *gdk_window = GTK_WIDGET(window)->window;
  int lines;
  
  gdk_window_clear(gdk_window);
  
  rand = g_rand_new();
  gc = gdk_gc_new(GDK_DRAWABLE(gdk_window));
  timer = g_timer_new();
  
  g_timer_start(timer);
  for (lines = N_ITERATIONS; lines >= 0; lines--) {
    int x1, y1;
    
    x1 = g_rand_int_range(rand, 0, 800);
    y1 = g_rand_int_range(rand, 0, 600);
    
    gdk_rgb_gc_set_foreground(gc, lines << 8);

    gdk_draw_rectangle(GDK_DRAWABLE(gdk_window), gc, FALSE,
                       x1, y1,
                       g_rand_int_range(rand, 0, 400),
                       g_rand_int_range(rand, 0, 300));
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }
  g_timer_stop(timer);
  
  time = g_timer_elapsed(timer, NULL);
  
  g_rand_free(rand);
  gdk_gc_destroy(gc);
  g_timer_destroy(timer);
  
  return time;
}

double test_lines(GtkWindow *window)
{
  GRand *rand;
  GTimer *timer;
  GdkGC *gc;
  double time;
  GdkWindow *gdk_window = GTK_WIDGET(window)->window;
  int lines;
  
  gdk_window_clear(gdk_window);
  
  rand = g_rand_new();
  gc = gdk_gc_new(GDK_DRAWABLE(gdk_window));
  timer = g_timer_new();
  
  g_timer_start(timer);
  for (lines = N_ITERATIONS; lines >= 0; lines--) {
    int x1, y1, x2, y2;
    
    x1 = g_rand_int_range(rand, 0, 800);
    y1 = g_rand_int_range(rand, 0, 600);
    x2 = g_rand_int_range(rand, 0, 800);
    y2 = g_rand_int_range(rand, 0, 600);
    
    gdk_draw_line(GDK_DRAWABLE(gdk_window), gc, x1, y1, x2, y2);
    gdk_rgb_gc_set_foreground(gc, lines << 8);
    
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }
  g_timer_stop(timer);
  
  time = g_timer_elapsed(timer, NULL);
  
  g_rand_free(rand);
  gdk_gc_destroy(gc);
  g_timer_destroy(timer);
  
  return time;
}

double guibench(void)
{
  GtkWidget *window;
  gdouble score = 0.0f;
  gint i;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(window, 800, 600);
  gtk_window_set_title(GTK_WINDOW(window), "guibench");
  
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show(window);

  for (i = 0; tests[i].title; i++) {
    double time;
    
    gtk_window_set_title(GTK_WINDOW(window), tests[i].title); 
    time = tests[i].callback(GTK_WINDOW(window));
    score += (N_ITERATIONS / time) / tests[i].weight;
  }
  
  gtk_widget_destroy(window);
  
  return score / i;
}
