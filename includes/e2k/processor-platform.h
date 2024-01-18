/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2020 EntityFX <artem.solopiy@gmail.com> and MCST Elbrus Team 
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

/*

E2K /proc/cpuinfo:


processor	: 0
vendor_id	: E8C-SWTX
cpu family	: 4
model		: 7
model name	: E8C
revision	: 2
cpu MHz		: 1299.912740
cache0          : level=1 type=Instruction scope=Private size=128K line_size=256 associativity=4
cache1          : level=1 type=Data scope=Private size=64K line_size=32 associativity=4
cache2          : level=2 type=Unified scope=Private size=512K line_size=64 associativity=4
cache3          : level=3 type=Unified scope=Private size=16384K line_size=64 associativity=16
bogomips	: 2600.68

*/


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

//e2k processor structure
struct _Processor {
    gint id;
    gfloat cpu_mhz;
    gchar *model_name;
    gchar *vendor_id;
    gfloat bogomips;

    gint model, family, revision;

    GSList *cache;
};

#endif	/* __PROCESSOR_PLATFORM_H__ */
