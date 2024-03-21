/*
 *    hardinfo2 - System Information and Benchmark
 *    Copyright (C) 2024-2024 hwspeedy - hardinfo2 project
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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
#include <cairo.h>

#include "iconcache.h"
#include "config.h"

#define CRUNCH_TIME 3

static int count=0;
static int testnumber=0;
static GTimer *timer,*frametimer;
static gdouble score = 0.0f;
static GdkPixbuf *pixbufs[3];
static GRand *r;
double *frametime;
int *framecount;

gboolean on_draw (GtkWidget *widget, GdkEventExpose *event, gpointer data) {
#if GTK_CHECK_VERSION(3,0,0)
   const int divfactor[5]={2231,2122,2113,2334,2332};
#else //Note: OLD GTK does not do the same amount of work
   const int divfactor[5]={12231,12122,12113,12334,12332};
#endif
   const int iterations[5]={100,300,100,300,100};
   int i;
   cairo_t * cr;
   GdkWindow* window = gtk_widget_get_window(widget);


#if GTK_CHECK_VERSION(3,22,0)
   cairo_region_t * cairoRegion = cairo_region_create();
   GdkDrawingContext * drawingContext;
    
   drawingContext = gdk_window_begin_draw_frame (window,cairoRegion);
   cr = gdk_drawing_context_get_cairo_context (drawingContext);
#else
   cr = gdk_cairo_create(window);
#endif

   g_timer_continue(frametimer);
   for (i = iterations[testnumber]; i >= 0; i--) {
       switch(testnumber) {
	  case 0 : //Line Drawing
                cairo_move_to(cr, g_rand_int_range(r,0,1024), g_rand_int_range(r,0,800));
		cairo_set_source_rgb(cr,g_rand_double_range(r,0,1),g_rand_double_range(r,0,1),g_rand_double_range(r,0,1));
		cairo_line_to(cr, g_rand_int_range(r,0,1024), g_rand_int_range(r,0,800));
		cairo_stroke(cr);
		break;
	  case 1 : //Shape Drawing
	        cairo_rectangle(cr,g_rand_int_range(r,0,1024-200),g_rand_int_range(r,0,800-200),g_rand_int_range(r,0,400),g_rand_int_range(r,0,300));
		cairo_set_source_rgb(cr,g_rand_double_range(r,0,1),g_rand_double_range(r,0,1),g_rand_double_range(r,0,1));
		cairo_stroke(cr);
		break;
	  case 2 : //Filled Shape Drawing
	        cairo_rectangle(cr,g_rand_int_range(r,0,1024-200),g_rand_int_range(r,0,800-200),g_rand_int_range(r,0,400),g_rand_int_range(r,0,300));
		cairo_set_source_rgb(cr,g_rand_double_range(r,0,1),g_rand_double_range(r,0,1),g_rand_double_range(r,0,1));
		cairo_fill(cr);
		break;
	  case 3 : //Text Drawing
                cairo_move_to(cr,g_rand_int_range(r,0,1024-100),g_rand_int_range(r,0,800));
                cairo_set_font_size(cr,25);
		cairo_set_source_rgb(cr,g_rand_double_range(r,0,1),g_rand_double_range(r,0,1),g_rand_double_range(r,0,1));
                cairo_show_text(cr, "I \342\231\245 hardinfo2");
		break;
		//
	  case 4 : //Icon Blitting
                gdk_cairo_set_source_pixbuf (cr, pixbufs[g_rand_int_range(r,0,3)],g_rand_int_range(r,0,1024-64), g_rand_int_range(r,0,800-64));
		cairo_paint(cr);
	        break;
	  }
     }
     g_timer_stop(frametimer);
#if GTK_CHECK_VERSION(3,22,0)
     gdk_window_end_draw_frame(window,drawingContext);
#endif
     count++;
     if(g_timer_elapsed(timer, NULL)<CRUNCH_TIME) {
         gtk_widget_queue_draw_area(widget,0,0,1024,800);
     } else {
         score += ((double)iterations[testnumber]*count/g_timer_elapsed(frametimer,NULL)) / divfactor[testnumber];
	 frametime[testnumber]=g_timer_elapsed(frametimer,NULL);
	 framecount[testnumber]=count;
         DEBUG("GPU Test %d => %d =>score:%f (frametime=%f)",testnumber,count,score,g_timer_elapsed(frametimer,NULL));
         count=0;
	 testnumber++;
	 //Done
         if(testnumber>=5){
	     gtk_main_quit();
         } else {
	     g_timer_start(frametimer);
	     g_timer_stop(frametimer);
             g_timer_start(timer);
             gtk_widget_queue_draw_area(widget,0,0,1024,800);
	 }
     }

     // cleanup
#if GTK_CHECK_VERSION(3,22,0)
     cairo_region_destroy(cairoRegion);
#endif

     return FALSE;
}


double guibench(double *frameTime, int *frameCount)
{
    GtkWindow * window;
    cairo_t *cr;

    frametime=frameTime;
    framecount=frameCount;
    
    DEBUG("GUIBENCH");
    pixbufs[0] = gdk_pixbuf_scale_simple(icon_cache_get_pixbuf("hardinfo2.png"),64,64,GDK_INTERP_BILINEAR);
    pixbufs[1] = gdk_pixbuf_scale_simple(icon_cache_get_pixbuf("syncmanager.png"),64,64,GDK_INTERP_BILINEAR);
    pixbufs[2] = gdk_pixbuf_scale_simple(icon_cache_get_pixbuf("report-large.png"),64,64,GDK_INTERP_BILINEAR);

    r = g_rand_new();

    // window setup
    window = (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (window, 1024, 800);
    gtk_window_set_position     (window, GTK_WIN_POS_CENTER);
    gtk_window_set_title        (window, "GPU Benchmarking...");
    g_signal_connect(window, "destroy", gtk_main_quit, NULL);

    // create the are we can draw in
    GtkDrawingArea* drawingArea;
    drawingArea = (GtkDrawingArea*) gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), (GtkWidget*)drawingArea);
#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect((GtkWidget*)drawingArea, "draw", G_CALLBACK(on_draw), NULL);
#else
    g_signal_connect((GtkWidget*)drawingArea, "expose-event", G_CALLBACK(on_draw), NULL);
#endif
    frametimer = g_timer_new();
    g_timer_stop(frametimer);
    timer = g_timer_new();
    gtk_widget_show_all ((GtkWidget*)window);

    gtk_main();

    g_timer_destroy(timer);
    g_timer_destroy(frametimer);
    g_rand_free(r);
    g_object_unref(pixbufs[0]);
    g_object_unref(pixbufs[1]);
    g_object_unref(pixbufs[2]);

    return score;
}
