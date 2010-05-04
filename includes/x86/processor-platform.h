/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#ifndef __PROCESSOR_PLATFORM_H__
#define __PROCESSOR_PLATFORM_H__

typedef struct _ProcessorCache ProcessorCache;

struct _ProcessorCache {
    gint level;
    gint number_of_sets;
    gint physical_line_partition;
    gint size;
    gchar *type;
    gint ways_of_associativity;
};

struct _Processor {
    gchar *model_name;
    gchar *vendor_id;
    gchar *flags;
    gint cache_size;
    gfloat bogomips, cpu_mhz;

    gchar *has_fpu;
    gchar *bug_fdiv, *bug_hlt, *bug_f00f, *bug_coma;

    gint model, family, stepping;
    gchar *strmodel;

    gint id;

    GSList *cache;
};

#endif	/* __PROCESSOR_PLATFORM_H__ */
