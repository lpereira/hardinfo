/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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

#include "benchmark.h"
#include "guibench.h"

#define BENCH_REVISION 5

void
benchmark_gui(void)
{
    bench_value r = EMPTY_BENCH_VALUE;
    static double frametime[5];
    static int framecount[5];

    shell_view_set_enabled(FALSE);
    shell_status_update("Running GPU Drawing...");

    r.result = guibench(frametime,framecount);
    r.revision = BENCH_REVISION;
#if GTK_CHECK_VERSION(3,0,0)
    snprintf(r.extra, 255, "g:3 f:%0.4f/%0.4f/%0.4f/%0.4f/%0.4f c:%d/%d/%d/%d/%d",frametime[0],frametime[1],frametime[2],frametime[3],frametime[4],framecount[0],framecount[1],framecount[2],framecount[3],framecount[4]);
#else
    snprintf(r.extra, 255, "g:2 f:%0.4f/%0.4f/%0.4f/%0.4f/%0.4f c:%d/%d/%d/%d/%d",frametime[0],frametime[1],frametime[2],frametime[3],frametime[4],framecount[0],framecount[1],framecount[2],framecount[3],framecount[4]);
#endif

    bench_results[BENCHMARK_GUI] = r;
}
