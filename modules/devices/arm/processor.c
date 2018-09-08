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

#include "hardinfo.h"
#include "devices.h"
#include "cpu_util.h"
#include "dt_util.h"

#include "arm_data.h"
#include "arm_data.c"

enum {
    ARM_A32 = 0,
    ARM_A64 = 1,
    ARM_A32_ON_A64 = 2,
};

static const gchar *arm_mode_str[] = {
    "A32",
    "A64",
    "A32 on A64",
};

GSList *
processor_scan(void)
{
    GSList *procs = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[128];
    gchar *rep_pname = NULL;
    GSList *pi = NULL;

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
    return NULL;

#define CHECK_FOR(k) (g_str_has_prefix(tmp[0], k))
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);
        if (tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);
        } else {
            g_strfreev(tmp);
            continue;
        }

        get_str("Processor", rep_pname);

        if ( CHECK_FOR("processor") ) {
            /* finish previous */
            if (processor) {
                procs = g_slist_append(procs, processor);
            }

            /* start next */
            processor = g_new0(Processor, 1);
            processor->id = atol(tmp[1]);

            if (rep_pname)
                processor->linux_name = g_strdup(rep_pname);

            g_strfreev(tmp);
            continue;
        }

        if (!processor &&
            (  CHECK_FOR("model name")
            || CHECK_FOR("Features")
            || CHECK_FOR("BogoMIPS") ) ) {

            /* single proc/core may not have "processor : n" */
            processor = g_new0(Processor, 1);
            processor->id = 0;

            if (rep_pname)
                processor->linux_name = g_strdup(rep_pname);
        }

        if (processor) {
            get_str("model name", processor->linux_name);
            get_str("Features", processor->flags);
            get_float("BogoMIPS", processor->bogomips);

            get_str("CPU implementer", processor->cpu_implementer);
            get_str("CPU architecture", processor->cpu_architecture);
            get_str("CPU variant", processor->cpu_variant);
            get_str("CPU part", processor->cpu_part);
            get_str("CPU revision", processor->cpu_revision);
        }
        g_strfreev(tmp);
    }

    if (processor)
        procs = g_slist_append(procs, processor);

    g_free(rep_pname);
    fclose(cpuinfo);

    /* re-duplicate missing data for /proc/cpuinfo variant that de-duplicated it */
#define REDUP(f) if (dproc->f && !processor->f) processor->f = g_strdup(dproc->f);
    Processor *dproc;
    GSList *l;
    l = procs = g_slist_reverse(procs);
    while (l) {
        processor = l->data;
        if (processor->flags) {
            dproc = processor;
        } else if (dproc) {
            REDUP(flags);
            REDUP(cpu_implementer);
            REDUP(cpu_architecture);
            REDUP(cpu_variant);
            REDUP(cpu_part);
            REDUP(cpu_revision);
        }
        l = g_slist_next(l);
    }
    procs = g_slist_reverse(procs);

    /* data not from /proc/cpuinfo */
    for (pi = procs; pi; pi = pi->next) {
        processor = (Processor *) pi->data;

        /* strings can't be null or segfault later */
        STRIFNULL(processor->linux_name, _("ARM Processor") );
        EMPIFNULL(processor->flags);
        UNKIFNULL(processor->cpu_implementer);
        UNKIFNULL(processor->cpu_architecture);
        UNKIFNULL(processor->cpu_variant);
        UNKIFNULL(processor->cpu_part);
        UNKIFNULL(processor->cpu_revision);

        processor->model_name = arm_decoded_name(
            processor->cpu_implementer, processor->cpu_part,
            processor->cpu_variant, processor->cpu_revision,
            processor->cpu_architecture, processor->linux_name);
        UNKIFNULL(processor->model_name);

        /* topo & freq */
        processor->cpufreq = cpufreq_new(processor->id);
        processor->cputopo = cputopo_new(processor->id);

        if (processor->cpufreq->cpukhz_max)
            processor->cpu_mhz = processor->cpufreq->cpukhz_max / 1000;
        else
            processor->cpu_mhz = 0.0f;

        /* mode */
        processor->mode = ARM_A32;
        if ( processor_has_flag(processor->flags, "pmull")
             || processor_has_flag(processor->flags, "crc32") ) {
#ifdef __aarch64__
                processor->mode = ARM_A64;
#else
                processor->mode = ARM_A32_ON_A64;
#endif
        }
    }

    return procs;
}

gchar *processor_get_capabilities_from_flags(gchar * strflags)
{
    gchar **flags, **old;
    gchar *tmp = NULL;
    gint j = 0;

    flags = g_strsplit(strflags, " ", 0);
    old = flags;

    while (flags[j]) {
        const gchar *meaning = arm_flag_meaning( flags[j] );

        if (meaning) {
            tmp = h_strdup_cprintf("%s=%s\n", tmp, flags[j], meaning);
        } else {
            tmp = h_strdup_cprintf("%s=\n", tmp, flags[j]);
        }
        j++;
    }
    if (tmp == NULL || g_strcmp0(tmp, "") == 0)
        tmp = g_strdup_printf("%s=%s\n", "empty", _("Empty List"));

    g_strfreev(old);
    return tmp;
}

#define khzint_to_mhzdouble(k) (((double)k)/1000)
#define cmp_clocks_test(f) if (a->f < b->f) return -1; if (a->f > b->f) return 1;

static gint cmp_cpufreq_data(cpufreq_data *a, cpufreq_data *b) {
        gint i = 0;
        i = g_strcmp0(a->shared_list, b->shared_list); if (i!=0) return i;
        cmp_clocks_test(cpukhz_max);
        cmp_clocks_test(cpukhz_min);
        return 0;
}

static gint cmp_cpufreq_data_ignore_affected(cpufreq_data *a, cpufreq_data *b) {
        gint i = 0;
        cmp_clocks_test(cpukhz_max);
        cmp_clocks_test(cpukhz_min);
        return 0;
}

gchar *clocks_summary(GSList * processors)
{
    gchar *ret = g_strdup_printf("[%s]\n", _("Clocks"));
    GSList *all_clocks = NULL, *uniq_clocks = NULL;
    GSList *tmp, *l;
    Processor *p;
    cpufreq_data *c, *cur = NULL;
    gint cur_count = 0, i = 0;

    /* create list of all clock references */
    for (l = processors; l; l = l->next) {
        p = (Processor*)l->data;
        if (p->cpufreq) {
            all_clocks = g_slist_prepend(all_clocks, p->cpufreq);
        }
    }

    if (g_slist_length(all_clocks) == 0) {
        ret = h_strdup_cprintf("%s=\n", ret, _("(Not Available)") );
        g_slist_free(all_clocks);
        return ret;
    }

    /* ignore duplicate references */
    all_clocks = g_slist_sort(all_clocks, (GCompareFunc)cmp_cpufreq_data);
    for (l = all_clocks; l; l = l->next) {
        c = (cpufreq_data*)l->data;
        if (!cur) {
            cur = c;
        } else {
            if (cmp_cpufreq_data(cur, c) != 0) {
                uniq_clocks = g_slist_prepend(uniq_clocks, cur);
                cur = c;
            }
        }
    }
    uniq_clocks = g_slist_prepend(uniq_clocks, cur);
    uniq_clocks = g_slist_reverse(uniq_clocks);
    cur = 0, cur_count = 0;

    /* count and list clocks */
    for (l = uniq_clocks; l; l = l->next) {
        c = (cpufreq_data*)l->data;
        if (!cur) {
            cur = c;
            cur_count = 1;
        } else {
            if (cmp_cpufreq_data_ignore_affected(cur, c) != 0) {
                ret = h_strdup_cprintf(_("%.2f-%.2f %s=%dx\n"),
                                ret,
                                khzint_to_mhzdouble(cur->cpukhz_min),
                                khzint_to_mhzdouble(cur->cpukhz_max),
                                _("MHz"),
                                cur_count);
                cur = c;
                cur_count = 1;
            } else {
                cur_count++;
            }
        }
    }
    ret = h_strdup_cprintf(_("%.2f-%.2f %s=%dx\n"),
                    ret,
                    khzint_to_mhzdouble(cur->cpukhz_min),
                    khzint_to_mhzdouble(cur->cpukhz_max),
                    _("MHz"),
                    cur_count);

    g_slist_free(all_clocks);
    g_slist_free(uniq_clocks);
    return ret;
}

gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_flags, *tmp_imp, *tmp_part, *tmp_arch, *tmp_cpufreq, *tmp_topology, *ret;
    tmp_flags = processor_get_capabilities_from_flags(processor->flags);
    tmp_imp = (char*)arm_implementer(processor->cpu_implementer);
    tmp_part = (char*)arm_part(processor->cpu_implementer, processor->cpu_part);
    tmp_arch = (char*)arm_arch_more(processor->cpu_architecture);

    tmp_topology = cputopo_section_str(processor->cputopo);
    tmp_cpufreq = cpufreq_section_str(processor->cpufreq);

    ret = g_strdup_printf("[%s]\n"
                       "%s=%s\n"       /* linux name */
                       "%s=%s\n"       /* decoded name */
                       "%s=%s\n"       /* mode */
                       "%s=%.2f %s\n"  /* frequency */
                       "%s=%.2f\n"     /* bogomips */
                       "%s=%s\n"       /* byte order */
                       "%s" /* topology */
                       "%s" /* frequency scaling */
                       "[%s]\n"    /* ARM */
                       "%s=[%s] %s\n"  /* implementer */
                       "%s=[%s] %s\n"  /* part */
                       "%s=[%s] %s\n"  /* architecture */
                       "%s=%s\n"       /* variant */
                       "%s=%s\n"       /* revision */
                       "[%s]\n" /* flags */
                       "%s"
                       "%s",    /* empty */
                   _("Processor"),
                   _("Linux Name"), processor->linux_name,
                   _("Decoded Name"), processor->model_name,
                   _("Mode"), arm_mode_str[processor->mode],
                   _("Frequency"), processor->cpu_mhz, _("MHz"),
                   _("BogoMips"), processor->bogomips,
                   _("Byte Order"), byte_order_str(),
                   tmp_topology,
                   tmp_cpufreq,
                   _("ARM"),
                   _("Implementer"), processor->cpu_implementer, (tmp_imp) ? tmp_imp : "",
                   _("Part"), processor->cpu_part, (tmp_part) ? tmp_part : "",
                   _("Architecture"), processor->cpu_architecture, (tmp_arch) ? tmp_arch : "",
                   _("Variant"), processor->cpu_variant,
                   _("Revision"), processor->cpu_revision,
                   _("Capabilities"), tmp_flags,
                    "");
    g_free(tmp_flags);
    g_free(tmp_cpufreq);
    g_free(tmp_topology);
    return ret;
}

gchar *processor_name(GSList *processors) {
    /* compatible contains a list of compatible hardware, so be careful
     * with matching order.
     * ex: "ti,omap3-beagleboard-xm", "ti,omap3450", "ti,omap3";
     * matches "omap3 family" first.
     * ex: "brcm,bcm2837", "brcm,bcm2836";
     * would match 2836 when it is a 2837.
     */
#define UNKSOC "(Unknown)" /* don't translate this */
    const struct {
        char *search_str;
        char *vendor;
        char *soc;
    } dt_compat_searches[] = {
        { "brcm,bcm2837", "Broadcom", "BCM2837" },
        { "brcm,bcm2836", "Broadcom", "BCM2836" },
        { "brcm,bcm2835", "Broadcom", "BCM2835" },
        { "ti,omap5432", "Texas Instruments", "OMAP5432" },
        { "ti,omap5430", "Texas Instruments", "OMAP5430" },
        { "ti,omap4470", "Texas Instruments", "OMAP4470" },
        { "ti,omap4460", "Texas Instruments", "OMAP4460" },
        { "ti,omap4430", "Texas Instruments", "OMAP4430" },
        { "ti,omap3620", "Texas Instruments", "OMAP3620" },
        { "ti,omap3450", "Texas Instruments", "OMAP3450" },
        { "ti,omap5", "Texas Instruments", "OMAP5-family" },
        { "ti,omap4", "Texas Instruments", "OMAP4-family" },
        { "ti,omap3", "Texas Instruments", "OMAP3-family" },
        { "ti,omap2", "Texas Instruments", "OMAP2-family" },
        { "ti,omap1", "Texas Instruments", "OMAP1-family" },
        { "mediatek,mt6799", "MediaTek", "MT6799 Helio X30" },
        { "mediatek,mt6799", "MediaTek", "MT6799 Helio X30" },
        { "mediatek,mt6797x", "MediaTek", "MT6797X Helio X27" },
        { "mediatek,mt6797t", "MediaTek", "MT6797T Helio X25" },
        { "mediatek,mt6797", "MediaTek", "MT6797 Helio X20" },
        { "mediatek,mt6757T", "MediaTek", "MT6757T Helio P25" },
        { "mediatek,mt6757", "MediaTek", "MT6757 Helio P20" },
        { "mediatek,mt6795", "MediaTek", "MT6795 Helio X10" },
        { "mediatek,mt6755", "MediaTek", "MT6755 Helio P10" },
        { "mediatek,mt6750t", "MediaTek", "MT6750T" },
        { "mediatek,mt6750", "MediaTek", "MT6750" },
        { "mediatek,mt6753", "MediaTek", "MT6753" },
        { "mediatek,mt6752", "MediaTek", "MT6752" },
        { "mediatek,mt6738", "MediaTek", "MT6738" },
        { "mediatek,mt6737t", "MediaTek", "MT6737T" },
        { "mediatek,mt6735", "MediaTek", "MT6735" },
        { "mediatek,mt6732", "MediaTek", "MT6732" },
        { "qcom,msm8939", "Qualcomm", "Snapdragon 615"},
        { "qcom,msm", "Qualcomm", "Snapdragon-family"},
        { "nvidia,tegra", "nVidia", "Tegra-family" },
        { "bcm,", "Broadcom", UNKSOC },
        { "nvidia,", "nVidia", UNKSOC },
        { "rockchip,", "Rockchip", UNKSOC },
        { "ti,", "Texas Instruments", UNKSOC },
        { "qcom,", "Qualcom", UNKSOC },
        { "mediatek,", "MediaTek", UNKSOC },
        { NULL, NULL }
    };
    gchar *ret = NULL;
    gchar *compat = NULL;
    int i;

    compat = dtr_get_string("/compatible", 1);

    if (compat != NULL) {
        i = 0;
        while(dt_compat_searches[i].search_str != NULL) {
            if (strstr(compat, dt_compat_searches[i].search_str) != NULL) {
                ret = g_strdup_printf("%s %s", dt_compat_searches[i].vendor, dt_compat_searches[i].soc);
                break;
            }
            i++;
        }
    }
    g_free(compat);
    UNKIFNULL(ret);
    return ret;
}

gchar *processor_describe(GSList * processors) {
    return processor_describe_by_counting_names(processors);
}

gchar *processor_meta(GSList * processors) {
    gchar *meta_soc = processor_name(processors);
    gchar *meta_cpu_desc = processor_describe(processors);
    gchar *meta_cpu_topo = processor_describe_default(processors);
    gchar *meta_freq_desc = processor_frequency_desc(processors);
    gchar *meta_clocks = clocks_summary(processors);
    gchar *ret = NULL;
    UNKIFNULL(meta_cpu_desc);
    ret = g_strdup_printf("[%s]\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s",
                            _("SOC/Package"),
                            _("Name"), meta_soc,
                            _("Description"), meta_cpu_desc,
                            _("Topology"), meta_cpu_topo,
                            _("Logical CPU Config"), meta_freq_desc,
                            meta_clocks );
    g_free(meta_soc);
    g_free(meta_cpu_desc);
    g_free(meta_cpu_topo);
    g_free(meta_freq_desc);
    g_free(meta_clocks);
    return ret;
}

gchar *processor_get_info(GSList * processors)
{
    Processor *processor;
    gchar *ret, *tmp, *hashkey;
    gchar *meta; /* becomes owned by more_info? no need to free? */
    GSList *l;

    tmp = g_strdup_printf("$CPU_META$%s=\n", _("SOC/Package Information") );

    meta = processor_meta(processors);
    moreinfo_add_with_prefix("DEV", "CPU_META", meta);

    for (l = processors; l; l = l->next) {
        processor = (Processor *) l->data;

        tmp = g_strdup_printf("%s$CPU%d$%s=%.2f %s\n",
                  tmp, processor->id,
                  processor->model_name,
                  processor->cpu_mhz, _("MHz"));

        hashkey = g_strdup_printf("CPU%d", processor->id);
        moreinfo_add_with_prefix("DEV", hashkey,
                processor_get_detailed_info(processor));
        g_free(hashkey);
    }

    ret = g_strdup_printf("[$ShellParam$]\n"
                  "ViewType=1\n"
                  "[Processors]\n"
                  "%s", tmp);
    g_free(tmp);

    return ret;
}
