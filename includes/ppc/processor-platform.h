/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 L. A. F. Pereira <l@tia.mat.br>
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

#ifndef __PROCESSOR_PLATFORM_H__
#define __PROCESSOR_PLATFORM_H__

#include "cpu_util.h"

struct _Processor {
    gint id;
    gfloat cpu_mhz; /* for devices.c, identical to cpukhz_max/1000 */
    cpu_topology_data *cputopo;
    cpufreq_data *cpufreq;

    gchar *model_name;
    gchar *revision;
    /* gint cache_size; */
    gfloat bogomips;
};

#endif	/* __PROCESSOR_PLATFORM_H__ */
