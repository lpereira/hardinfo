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

#include <nqueens.h>

static gpointer nqueens_for(unsigned int start, unsigned int end, void *data, GTimer *timer)
{
    unsigned int i;
    
    for (i = start; i <= end; i++) { 
        nqueens(0);
    }
    
    return NULL;
}

static void
benchmark_nqueens(void)
{
    gdouble elapsed = 0;
    
    shell_view_set_enabled(FALSE);
    shell_status_update("Running N-Queens benchmark...");
        
    elapsed = benchmark_parallel_for(0, 10, nqueens_for, NULL);
    
    bench_results[BENCHMARK_NQUEENS] = elapsed;
}


