/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include "benchmark.h"

void fbench();	/* fbench.c */

static gpointer
parallel_raytrace(unsigned int start, unsigned int end, gpointer data, gint thread_number)
{
    unsigned int i;
    
    for (i = start; i <= end; i++) { 
        fbench();
    }
    
    return NULL;
}

void
benchmark_raytrace(void)
{
    gdouble elapsed = 0;
    
    shell_view_set_enabled(FALSE);
    shell_status_update("Performing John Walker's FBENCH...");
    
    elapsed = benchmark_parallel_for(0, 1000, parallel_raytrace, NULL);
    
    bench_results[BENCHMARK_RAYTRACE] = elapsed;
}

