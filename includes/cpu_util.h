#ifndef __CPU_UTIL_H__
#define __CPU_UTIL_H__

#include "hardinfo.h"

#ifndef PROC_CPUINFO
#define PROC_CPUINFO "/proc/cpuinfo"
#endif

#define STRIFNULL(f,cs) if (f == NULL) f = g_strdup(cs);
#define UNKIFNULL(f) STRIFNULL(f, _("(Unknown)") )
#define EMPIFNULL(f) STRIFNULL(f, "")

const gchar *byte_order_str(void);

/* from /sys/devices/system/cpu/cpu%d/%s */
gchar* get_cpu_str(const gchar* file, gint cpuid);
gint get_cpu_int(const char* item, int cpuid, int null_val);

/* space delimted list of flags, finds flag */
int processor_has_flag(gchar * strflags, gchar * strflag);

typedef struct {
    gint id;
    gint cpukhz_max, cpukhz_min, cpukhz_cur;
    gchar *scaling_driver, *scaling_governor;
    gint transition_latency;
    gchar *shared_list;
} cpufreq_data;

typedef struct {
    gint id; /* thread */
    gint socket_id;
    gint core_id;
    gint book_id;
    gint drawer_id;
} cpu_topology_data;

int cpu_procs_cores_threads(int *p, int *c, int *t);

cpufreq_data *cpufreq_new(gint id);
void cpufreq_update(cpufreq_data *cpufd, int cur_only);
void cpufreq_free(cpufreq_data *cpufd);

gchar *cpufreq_section_str(cpufreq_data *cpufd);

cpu_topology_data *cputopo_new(gint id);
void cputopo_free(cpu_topology_data *cputd);

gchar *cputopo_section_str(cpu_topology_data *cputd);

#endif
