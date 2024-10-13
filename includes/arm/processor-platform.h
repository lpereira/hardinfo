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

typedef struct _ProcessorCache ProcessorCache;

struct _ProcessorCache {
    gint level;
    gint number_of_sets;
    gint physical_line_partition;
    gint size;
    gchar *type;
    gint ways_of_associativity;
    gint uid; /* uid is unique among caches with the same (type, level) */
    gchar *shared_cpu_list; /* some kernel's don't give a uid, so try shared_cpu_list */
    gint phy_sock;
};

struct _Processor {
    gchar *model_name;
    gchar *linux_name;
    gchar *flags;
    gfloat bogomips;

    gint id;
    gfloat cpu_mhz; /* for devices.c, identical to cpukhz_max/1000 */
    cpu_topology_data *cputopo;
    cpufreq_data *cpufreq;

    gchar *cpu_implementer;
    gchar *cpu_architecture;
    gchar *cpu_variant;
    gchar *cpu_part;
    gchar *cpu_revision;

    gint mode;

    GSList *cache;

};

#endif	/* __PROCESSOR_PLATFORM_H__ */
