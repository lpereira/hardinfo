#ifndef __PROCESSOR_PLATFORM_H__
#define __PROCESSOR_PLATFORM_H__

#include "hardinfo.h"

typedef struct _Processor Processor;
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

#endif				/* __PROCESSOR_PLATFORM_H__ */
