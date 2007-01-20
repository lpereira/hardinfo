/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

void fbench();	/* fbench.c */

static void
benchmark_raytrace(void)
{
    int i;
    GTimer *timer = g_timer_new();
    gdouble elapsed = 0;
    
    shell_view_set_enabled(FALSE);
    shell_status_update("Performing John Walker's FBENCH...");
    
    for (i = 0; i <= 1000; i++) { 
        g_timer_start(timer);
        
        fbench();
        
        g_timer_stop(timer);
        elapsed += g_timer_elapsed(timer, NULL);
        
        shell_status_set_percentage(i/10);
    }
    
    g_timer_destroy(timer);
    
    bench_results[BENCHMARK_RAYTRACE] = elapsed;
}

