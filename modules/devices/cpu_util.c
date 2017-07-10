/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    This file by Burt P. <pburt0@gmail.com>
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

#include <string.h>
#include "hardinfo.h"
#include "cpu_util.h"

gchar *byte_order_str() {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    return _("Little Endian");
#else
    return _("Big Endian");
#endif
}

int processor_has_flag(gchar * strflags, gchar * strflag)
{
    gchar **flags;
    gint ret = 0;
    if (strflags == NULL || strflag == NULL)
        return 0;
    flags = g_strsplit(strflags, " ", 0);
    ret = g_strv_contains((const gchar * const *)flags, strflag);
    g_strfreev(flags);
    return ret;
}

gchar* get_cpu_str(const gchar* file, gint cpuid) {
    gchar *tmp0 = NULL;
    gchar *tmp1 = NULL;
    tmp0 = g_strdup_printf("/sys/devices/system/cpu/cpu%d/%s", cpuid, file);
    g_file_get_contents(tmp0, &tmp1, NULL, NULL);
    g_free(tmp0);
    return tmp1;
}

gint get_cpu_int(const char* item, int cpuid) {
    gchar *fc = NULL;
    int ret = 0;
    fc = get_cpu_str(item, cpuid);
    if (fc) {
        ret = atol(fc);
        g_free(fc);
    }
    return ret;
}

cpufreq_data *cpufreq_new(gint id)
{
    cpufreq_data *cpufd;
    cpufd = malloc(sizeof(cpufreq_data));
    if (cpufd) {
        memset(cpufd, 0, sizeof(cpufreq_data));
        cpufd->id = id;
        cpufreq_update(cpufd, 0);
    }
    return cpufd;
}

void cpufreq_update(cpufreq_data *cpufd, int cur_only)
{
    if (cpufd) {
        cpufd->cpukhz_cur = get_cpu_int("cpufreq/scaling_cur_freq", cpufd->id);
        if (cur_only) return;
        cpufd->scaling_driver = get_cpu_str("cpufreq/scaling_driver", cpufd->id);
        cpufd->scaling_governor = get_cpu_str("cpufreq/scaling_governor", cpufd->id);
        cpufd->transition_latency = get_cpu_int("cpufreq/cpuinfo_transition_latency", cpufd->id);
        cpufd->cpukhz_min = get_cpu_int("cpufreq/scaling_min_freq", cpufd->id);
        cpufd->cpukhz_max = get_cpu_int("cpufreq/scaling_max_freq", cpufd->id);
    }
}

void cpufreq_free(cpufreq_data *cpufd)
{
    if (cpufd) {
        g_free(cpufd->scaling_driver);
        g_free(cpufd->scaling_governor);
    }
    g_free(cpufd);
}

cpu_topology_data *cputopo_new(gint id)
{
    cpu_topology_data *cputd;
    cputd = malloc(sizeof(cpu_topology_data));
    if (cputd) {
        memset(cputd, 0, sizeof(cpu_topology_data));
        cputd->id = id;
        cputd->socket_id = get_cpu_int("topology/physical_package_id", id);
        cputd->core_id = get_cpu_int("topology/core_id", id);
    }
    return cputd;

}

void cputopo_free(cpu_topology_data *cputd)
{
    g_free(cputd);
}


gchar *cpufreq_section_str(cpufreq_data *cpufd)
{
    if (cpufd == NULL)
        return g_strdup("");

    if (cpufd->cpukhz_min || cpufd->cpukhz_max || cpufd->cpukhz_cur) {
        return g_strdup_printf(
                    "[%s]\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%s\n"
                    "%s=%s\n",
                   _("Frequency Scaling"),
                   _("Minimum"), cpufd->cpukhz_min, _("kHz"),
                   _("Maximum"), cpufd->cpukhz_max, _("kHz"),
                   _("Current"), cpufd->cpukhz_cur, _("kHz"),
                   _("Transition Latency"), cpufd->transition_latency, _("ns"),
                   _("Governor"), cpufd->scaling_governor,
                   _("Driver"), cpufd->scaling_driver);
    } else {
        return g_strdup_printf(
                    "[%s]\n"
                    "%s=%s\n",
                   _("Frequency Scaling"),
                   _("Driver"), cpufd->scaling_driver);
    }
}

gchar *cputopo_section_str(cpu_topology_data *cputd)
{
    if (cputd == NULL)
        return g_strdup("");

    return g_strdup_printf(
                    "[%s]\n"
                    "%s=%d\n"
                    "%s=%d\n"
                    "%s=%d\n",
                   _("Topology"),
                   _("ID"), cputd->id,
                   _("Socket"), cputd->socket_id,
                   _("Core"), cputd->core_id);
}
