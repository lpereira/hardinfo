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

/* source: https://github.com/riscv/riscv-linux/blob/riscv-next/arch/riscv/kernel/cpu.c */

#include "hardinfo.h"
#include "devices.h"
#include "cpu_util.h"
#include "dt_util.h"

#include "riscv_data.h"
#include "riscv_data.c"

static gchar *__cache_get_info_as_string(Processor *processor)
{
    gchar *result = g_strdup("");
    GSList *cache_list;
    ProcessorCache *cache;

    if (!processor->cache) {
        return g_strdup(_("Cache information not available=\n"));
    }

    for (cache_list = processor->cache; cache_list; cache_list = cache_list->next) {
        cache = (ProcessorCache *)cache_list->data;

        result = h_strdup_cprintf(_("Level %d (%s)=%d-way set-associative, %d sets, %dKB size\n"),
                                  result,
                                  cache->level,
                                  C_("cache-type", cache->type),
                                  cache->ways_of_associativity,
                                  cache->number_of_sets,
                                  cache->size);
    }

    return result;
}

/* This is not used directly, but creates translatable strings for
 * the type string returned from /sys/.../cache */
//static const char* cache_types[] = {
//    NC_("cache-type", /*/cache type, as appears in: Level 1 (Data)*/ "Data"),
//    NC_("cache-type", /*/cache type, as appears in: Level 1 (Instruction)*/ "Instruction"),
//    NC_("cache-type", /*/cache type, as appears in: Level 2 (Unified)*/ "Unified")
//};

static void __cache_obtain_info(Processor *processor)
{
    ProcessorCache *cache;
    gchar *endpoint, *entry, *index;
    gchar *uref = NULL;
    gint i;
    gint processor_number = processor->id;

    endpoint = g_strdup_printf("/sys/devices/system/cpu/cpu%d/cache", processor_number);

    for (i = 0; ; i++) {
      cache = g_new0(ProcessorCache, 1);

      index = g_strdup_printf("index%d/", i);

      entry = g_strconcat(index, "type", NULL);
      cache->type = h_sysfs_read_string(endpoint, entry);
      g_free(entry);

      if (!cache->type) {
        g_free(cache);
        g_free(index);
        goto fail;
      }

      entry = g_strconcat(index, "level", NULL);
      cache->level = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "number_of_sets", NULL);
      cache->number_of_sets = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "physical_line_partition", NULL);
      cache->physical_line_partition = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "size", NULL);
      cache->size = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "ways_of_associativity", NULL);
      cache->ways_of_associativity = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      /* unique cache references: id is nice, but share_cpu_list can be
       * used if it is not available. */
      entry = g_strconcat(index, "id", NULL);
      uref = h_sysfs_read_string(endpoint, entry);
      g_free(entry);
      if (uref != NULL && *uref != 0 )
        cache->uid = atoi(uref);
      else
        cache->uid = -1;
      g_free(uref);
      entry = g_strconcat(index, "shared_cpu_list", NULL);
      cache->shared_cpu_list = h_sysfs_read_string(endpoint, entry);
      g_free(entry);

      /* reacharound */
      entry = g_strconcat(index, "../../topology/physical_package_id", NULL);
      cache->phy_sock = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      g_free(index);

      processor->cache = g_slist_append(processor->cache, cache);
    }

fail:
    g_free(endpoint);
}


#define cmp_cache_test(f) if (a->f < b->f) return -1; if (a->f > b->f) return 1;

static gint cmp_cache(ProcessorCache *a, ProcessorCache *b) {
        gint i = 0;
        cmp_cache_test(phy_sock);
        i = g_strcmp0(a->type, b->type); if (i!=0) return i;
        cmp_cache_test(level);
        cmp_cache_test(size);
        cmp_cache_test(uid); /* uid is unique among caches with the same (type, level) */
        if (a->uid == -1) {
            /* if id wasn't available, use shared_cpu_list as a unique ref */
            i = g_strcmp0(a->shared_cpu_list, b->shared_cpu_list); if (i!=0)
            return i;
        }
        return 0;
}

static gint cmp_cache_ignore_id(ProcessorCache *a, ProcessorCache *b) {
        gint i = 0;
        cmp_cache_test(phy_sock);
        i = g_strcmp0(a->type, b->type); if (i!=0) return i;
        cmp_cache_test(level);
        cmp_cache_test(size);
        return 0;
}

gchar *caches_summary(GSList * processors)
{
    gchar *ret = g_strdup_printf("[%s]\n", _("Caches"));
    GSList *all_cache = NULL, *uniq_cache = NULL;
    GSList *tmp, *l;
    Processor *p;
    ProcessorCache *c, *cur = NULL;
    gint cur_count = 0;

    /* create list of all cache references */
    for (l = processors; l; l = l->next) {
        p = (Processor*)l->data;
        if (p->cache) {
            tmp = g_slist_copy(p->cache);
            if (all_cache) {
                all_cache = g_slist_concat(all_cache, tmp);
            } else {
                all_cache = tmp;
            }
        }
    }

    if (g_slist_length(all_cache) == 0) {
        ret = h_strdup_cprintf("%s=\n", ret, _("(Not Available)") );
        g_slist_free(all_cache);
        return ret;
    }

    /* ignore duplicate references */
    all_cache = g_slist_sort(all_cache, (GCompareFunc)cmp_cache);
    for (l = all_cache; l; l = l->next) {
        c = (ProcessorCache*)l->data;
        if (!cur) {
            cur = c;
        } else {
            if (cmp_cache(cur, c) != 0) {
                uniq_cache = g_slist_prepend(uniq_cache, cur);
                cur = c;
            }
        }
    }
    uniq_cache = g_slist_prepend(uniq_cache, cur);
    uniq_cache = g_slist_reverse(uniq_cache);
    cur = 0, cur_count = 0;

    /* count and list caches */
    for (l = uniq_cache; l; l = l->next) {
        c = (ProcessorCache*)l->data;
        if (!cur) {
            cur = c;
            cur_count = 1;
        } else {
            if (cmp_cache_ignore_id(cur, c) != 0) {
                ret = h_strdup_cprintf(_("Level %d (%s)#%d=%dx %dKB (%dKB), %d-way set-associative, %d sets\n"),
                                      ret,
                                      cur->level,
                                      C_("cache-type", cur->type),
                                      cur->phy_sock,
                                      cur_count,
                                      cur->size,
                                      cur->size * cur_count,
                                      cur->ways_of_associativity,
                                      cur->number_of_sets);
                cur = c;
                cur_count = 1;
            } else {
                cur_count++;
            }
        }
    }
    ret = h_strdup_cprintf(_("Level %d (%s)#%d=%dx %dKB (%dKB), %d-way set-associative, %d sets\n"),
                          ret,
                          cur->level,
                          C_("cache-type", cur->type),
                          cur->phy_sock,
                          cur_count,
                          cur->size,
                          cur->size * cur_count,
                          cur->ways_of_associativity,
                          cur->number_of_sets);

    g_slist_free(all_cache);
    g_slist_free(uniq_cache);
    return ret;
}


gchar *processor_name(GSList * processors) {
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
        { "allwinner,sun201-d1", "Allwinner", "SUN20I-D1" },
        { "sifive,fu540", "SiFive", "FU540" },
        { "sifive,fu740", "SiFive", "FU740" },
        { "sophgo,sg2042", "Sophgo", "SG2042" },
        { "sophgo,cv1800b", "Sophgo", "CV1800B" },
        { "sophgo,cv1812h", "Sophgo", "CV1812H" },
        { "starfive,jh7100", "StarFive", "JH7100" },
        { "starfive,jh7110", "StarFive", "JH7110" },
        { "starfive,visionfive-2", "StarFive", "JH7110" },
        { "thead,c910", "T-Head", "C910" },
        { "thead,light-lpi4a", "T-Head", "TH1520" },
	//
        { "allwinner,", "Allwinner", UNKSOC },
        { "canaan,", "Canaan", UNKSOC },
        { "microchip,", "MicroChip", UNKSOC },
        { "renesas,", "Renesas", UNKSOC },
        { "sifive,", "SiFive", UNKSOC },
        { "sophgo,", "Sophgo", UNKSOC },
        { "starfive,", "StarFive", UNKSOC },
        { "thead,", "T-Head", UNKSOC },
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
	        if(strstr(dt_compat_searches[i].soc,"Unknown"))
		    ret = g_strdup_printf("RISC-V %s %s (%s)", dt_compat_searches[i].vendor, dt_compat_searches[i].soc, compat);
		else
                    ret = g_strdup_printf("RISC-V %s %s", dt_compat_searches[i].vendor, dt_compat_searches[i].soc);
                break;
            }
            i++;
        }
        if(!ret) ret = g_strdup_printf("RISC-V Processor (%s)", compat);
        g_free(compat);
    }

    if(!ret) ret = g_strdup("RISC-V Processor (NoDT)");
    return ret;
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
        cmp_clocks_test(cpukhz_max);
        cmp_clocks_test(cpukhz_min);
        return 0;
}

gchar *clocks_summary(GSList * processors)
{
    gchar *ret = g_strdup_printf("[%s]\n", _("Clocks"));
    GSList *all_clocks = NULL, *uniq_clocks = NULL;
    GSList *l;
    Processor *p;
    cpufreq_data *c, *cur = NULL;
    gint cur_count = 0;

    /* create list of all clock references */
    for (l = processors; l; l = l->next) {
        p = (Processor*)l->data;
        if (p->cpufreq && p->cpufreq->cpukhz_max > 0) {
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
        if (tmp[0] && tmp[1] && !strstr(tmp[0],"isa-") ) {//just drop empty isa-ext
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);
        } else {
            g_strfreev(tmp);
            continue;
        }

        //get_str("Processor", rep_pname);

        if ( CHECK_FOR("processor") ) {
            /* finish previous */
            if (processor) {
                procs = g_slist_append(procs, processor);
            }

            /* start next */
            processor = g_new0(Processor, 1);
            processor->id = atol(tmp[1]);

            if (rep_pname)
                processor->model_name = g_strdup(rep_pname);

            g_strfreev(tmp);
            continue;
        }

        if (!processor &&
            (  CHECK_FOR("mmu")
            || CHECK_FOR("isa")
            || CHECK_FOR("uarch") ) ) {

            /* single proc/core may not have "hart : n" */
            processor = g_new0(Processor, 1);
            processor->id = 0;

            if (rep_pname)
                processor->model_name = g_strdup(rep_pname);
        }

        if (processor) {
            get_str("mmu", processor->mmu);
            get_str("isa", processor->isa);
            get_str("uarch", processor->uarch);
        }
        g_strfreev(tmp);
    }

    if (processor)
        procs = g_slist_append(procs, processor);

    g_free(rep_pname);
    fclose(cpuinfo);

    /* TODO: redup */

    /* data not from /proc/cpuinfo */
    for (pi = procs; pi; pi = pi->next) {
        processor = (Processor *) pi->data;

	__cache_obtain_info(processor);

        /* strings can't be null or segfault later */
        STRIFNULL(processor->model_name, "RISC-V Processor" );
        UNKIFNULL(processor->mmu);
        UNKIFNULL(processor->isa);
        UNKIFNULL(processor->uarch);

        processor->flags = riscv_isa_to_flags(processor->isa);

        /* topo & freq */
        processor->cpufreq = cpufreq_new(processor->id);
        processor->cputopo = cputopo_new(processor->id);

        if (processor->cpufreq->cpukhz_max)
            processor->cpu_mhz = processor->cpufreq->cpukhz_max / 1000;
        else
            processor->cpu_mhz = 0.0f;
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
        const gchar *meaning = riscv_ext_meaning( flags[j] );

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

gchar *processor_describe(GSList * processors) {
    return processor_describe_by_counting_names(processors);
}

gchar *processor_meta(GSList * processors) {
    gchar *meta_soc = processor_name(processors);
    gchar *meta_cpu_desc = processor_describe(processors);
    gchar *meta_cpu_topo = processor_describe_default(processors);
    gchar *meta_freq_desc = processor_frequency_desc(processors);
    gchar *meta_clocks = clocks_summary(processors);
    gchar *meta_caches = caches_summary(processors);
    gchar *ret = NULL;
    UNKIFNULL(meta_cpu_desc);
    ret = g_strdup_printf("[%s]\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s"
                            "%s",
                            _("SOC/Package"),
                            _("Name"), meta_soc,
                            _("Description"), meta_cpu_desc,
                            _("Topology"), meta_cpu_topo,
                            _("Logical CPU Config"), meta_freq_desc,
			    meta_clocks, meta_caches );
    g_free(meta_soc);
    g_free(meta_cpu_desc);
    g_free(meta_cpu_topo);
    g_free(meta_freq_desc);
    g_free(meta_clocks);
    g_free(meta_caches);
    return ret;
}

gchar *processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_flags, *tmp_cpufreq, *tmp_topology, *ret;
    tmp_flags = processor_get_capabilities_from_flags(processor->flags);
    tmp_topology = cputopo_section_str(processor->cputopo);
    tmp_cpufreq = cpufreq_section_str(processor->cpufreq);

    ret = g_strdup_printf("[%s]\n"
                   "%s=%s\n"  /* model */
                   "%s=%s\n"  /* isa */
                   "%s=%s\n"  /* uarch */
                   "%s=%s\n"  /* mmu */
                   "%s=%.2f %s\n" /* frequency */
                   "%s=%s\n"      /* byte order */
                   "%s" /* topology */
                   "%s" /* frequency scaling */
                   "[%s]\n" /* extensions */
                   "%s"
                   "%s",/* empty */
                   _("Processor"),
                   _("Model"), processor->model_name,
                   _("Architecture"), processor->isa,
                   _("uarch"), processor->uarch,
                   _("MMU"), processor->mmu,
                   _("Frequency"), processor->cpu_mhz, _("MHz"),
                   _("Byte Order"), byte_order_str(),
                   tmp_topology,
                   tmp_cpufreq,
                   _("Capabilities"), tmp_flags,
                    "");
    g_free(tmp_flags);
    g_free(tmp_cpufreq);
    g_free(tmp_topology);
    return ret;
}

gchar *processor_get_info(GSList * processors)
{
    Processor *processor;

    if (g_slist_length(processors) > 1) {
    gchar *ret, *tmp, *hashkey;
    gchar *meta;
    GSList *l;

    tmp = g_strdup_printf("$!CPU_META$%s=\n", _("SOC/Package Information") );

    meta = processor_meta(processors);
    moreinfo_add_with_prefix("DEV", "CPU_META", meta);

    for (l = processors; l; l = l->next) {
        processor = (Processor *) l->data;

        processor->model_name=processor_name(processors);

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

    processor = (Processor *) processors->data;
    return processor_get_detailed_info(processor);
}
