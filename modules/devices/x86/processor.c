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

#include "hardinfo.h"
#include "devices.h"
#include "cpu_util.h"
#include "nice_name.h"
#include "x86_data.h"
#include "x86_data.c"

/*
 * This function is partly based on x86cpucaps
 * by Osamu Kayasono <jacobi@jcom.home.ne.jp>
 */
void get_processor_strfamily(Processor * processor)
{
    gint family = processor->family;
    gint model = processor->model;

    if (g_str_equal(processor->vendor_id, "GenuineIntel")) {
	if (family == 4) {
	    processor->strmodel = g_strdup("i486 series");
	} else if (family == 5) {
	    if (model < 4) {
		processor->strmodel = g_strdup("Pentium Classic");
	    } else {
		processor->strmodel = g_strdup("Pentium MMX");
	    }
	} else if (family == 6) {
	    if (model <= 1) {
		processor->strmodel = g_strdup("Pentium Pro");
	    } else if (model < 7) {
		processor->strmodel = g_strdup("Pentium II/Pentium II Xeon/Celeron");
	    } else if (model == 9) {
		processor->strmodel = g_strdup("Pentium M");
	    } else {
		processor->strmodel = g_strdup("Pentium III/Pentium III Xeon/Celeron/Core Duo/Core Duo 2");
	    }
	} else if (family > 6) {
	    processor->strmodel = g_strdup("Pentium 4");
	} else {
	    processor->strmodel = g_strdup("i386 class");
	}
    } else if (g_str_equal(processor->vendor_id, "AuthenticAMD")) {
	if (family == 4) {
	    if (model <= 9) {
		processor->strmodel = g_strdup("AMD i80486 series");
	    } else {
		processor->strmodel = g_strdup("AMD 5x86");
	    }
	} else if (family == 5) {
	    if (model <= 3) {
		processor->strmodel = g_strdup("AMD K5");
	    } else if (model <= 7) {
		processor->strmodel = g_strdup("AMD K6");
	    } else if (model == 8) {
		processor->strmodel = g_strdup("AMD K6-2");
	    } else if (model == 9) {
		processor->strmodel = g_strdup("AMD K6-III");
	    } else {
		processor->strmodel = g_strdup("AMD K6-2+/III+");
	    }
	} else if (family == 6) {
	    if (model == 1) {
		processor->strmodel = g_strdup("AMD Athlon (K7)");
	    } else if (model == 2) {
		processor->strmodel = g_strdup("AMD Athlon (K75)");
	    } else if (model == 3) {
		processor->strmodel = g_strdup("AMD Duron (Spitfire)");
	    } else if (model == 4) {
		processor->strmodel = g_strdup("AMD Athlon (Thunderbird)");
	    } else if (model == 6) {
		processor->strmodel = g_strdup("AMD Athlon XP/MP/4 (Palomino)");
	    } else if (model == 7) {
		processor->strmodel = g_strdup("AMD Duron (Morgan)");
	    } else if (model == 8) {
		processor->strmodel = g_strdup("AMD Athlon XP/MP (Thoroughbred)");
	    } else if (model == 10) {
		processor->strmodel = g_strdup("AMD Athlon XP/MP (Barton)");
	    } else {
		processor->strmodel = g_strdup("AMD Athlon (unknown)");
	    }
	} else if (family > 6) {
	    processor->strmodel = g_strdup("AMD Opteron/Athlon64/FX");
	} else {
	    processor->strmodel = g_strdup("AMD i386 class");
	}
    } else if (g_str_equal(processor->vendor_id, "CyrixInstead")) {
	if (family == 4) {
	    processor->strmodel = g_strdup("Cyrix 5x86");
	} else if (family == 5) {
	    processor->strmodel = g_strdup("Cyrix M1 (6x86)");
	} else if (family == 6) {
	    if (model == 0) {
		processor->strmodel = g_strdup("Cyrix M2 (6x86MX)");
	    } else if (model <= 5) {
		processor->strmodel = g_strdup("VIA Cyrix III (M2 core)");
	    } else if (model == 6) {
		processor->strmodel = g_strdup("VIA Cyrix III (WinChip C5A)");
	    } else if (model == 7) {
		processor->strmodel = g_strdup("VIA Cyrix III (WinChip C5B/C)");
	    } else {
		processor->strmodel = g_strdup("VIA Cyrix III (WinChip C5C-T)");
	    }
	} else {
	    processor->strmodel = g_strdup("Cyrix i386 class");
	}
    } else if (g_str_equal(processor->vendor_id, "CentaurHauls")) {
	if (family == 5) {
	    if (model <= 4) {
		processor->strmodel = g_strdup("Centaur WinChip C6");
	    } else if (model <= 8) {
		processor->strmodel = g_strdup("Centaur WinChip 2");
	    } else {
		processor->strmodel = g_strdup("Centaur WinChip 2A");
	    }
	} else {
	    processor->strmodel = g_strdup("Centaur i386 class");
	}
    } else if (g_str_equal(processor->vendor_id, "GenuineTMx86")) {
	processor->strmodel = g_strdup("Transmeta Crusoe TM3x00/5x00");
    } else {
	processor->strmodel = g_strdup("Unknown");
    }
}

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

#define PROC_SCAN_READ_BUFFER_SIZE 2048
GSList *processor_scan(void)
{
    GSList *procs = NULL, *l = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar *buffer;

    buffer = g_malloc(PROC_SCAN_READ_BUFFER_SIZE);
    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
        return NULL;

    while (fgets(buffer, PROC_SCAN_READ_BUFFER_SIZE, cpuinfo)) {
        int rlen = strlen(buffer);
        if (rlen >= PROC_SCAN_READ_BUFFER_SIZE - 1) {
            fprintf(stderr, "Warning: truncated a line (probably flags list) longer than %d bytes while reading %s.\n", PROC_SCAN_READ_BUFFER_SIZE, PROC_CPUINFO);
        }
        gchar **tmp = g_strsplit(buffer, ":", 2);
        if (!tmp[1] || !tmp[0]) {
            g_strfreev(tmp);
            continue;
        }

        tmp[0] = g_strstrip(tmp[0]);
        tmp[1] = g_strstrip(tmp[1]);

        if (g_str_has_prefix(tmp[0], "processor")) {
            /* finish previous */
            if (processor)
                procs = g_slist_append(procs, processor);

            /* start next */
            processor = g_new0(Processor, 1);
            processor->id = atol(tmp[1]);
            g_strfreev(tmp);
            continue;
        }

        if (processor) {
            get_str("model name", processor->model_name);
            get_str("vendor_id", processor->vendor_id);
            get_str("flags", processor->flags);
            get_str("bugs", processor->bugs);
            get_str("power management", processor->pm);
            get_str("microcode", processor->microcode);
            get_int("cache size", processor->cache_size);
            get_float("cpu MHz", processor->cpu_mhz);
            get_float("bogomips", processor->bogomips);

            get_str("fpu", processor->has_fpu);

            get_str("fdiv_bug", processor->bug_fdiv);
            get_str("hlt_bug", processor->bug_hlt);
            get_str("f00f_bug", processor->bug_f00f);
            get_str("coma_bug", processor->bug_coma);
            /* sep_bug? */

            get_int("model", processor->model);
            get_int("cpu family", processor->family);
            get_int("stepping", processor->stepping);
        }
        g_strfreev(tmp);
    }

    fclose(cpuinfo);
    g_free(buffer);

    /* finish last */
    if (processor)
        procs = g_slist_append(procs, processor);

    for (l = procs; l; l = l->next) {
        processor = (Processor *) l->data;

        STRIFNULL(processor->microcode, _("(Not Available)") );

        get_processor_strfamily(processor);
        __cache_obtain_info(processor);

#define NULLIFNOTYES(f) if (processor->f) if (strcmp(processor->f, "yes") != 0) { g_free(processor->f); processor->f = NULL; }
        NULLIFNOTYES(bug_fdiv);
        NULLIFNOTYES(bug_hlt);
        NULLIFNOTYES(bug_f00f);
        NULLIFNOTYES(bug_coma);

        if (processor->bugs == NULL || g_strcmp0(processor->bugs, "") == 0) {
            g_free(processor->bugs);
            /* make bugs list on old kernels that don't offer one */
            processor->bugs = g_strdup_printf("%s%s%s%s%s%s%s%s%s%s",
                    /* the oldest bug workarounds indicated in /proc/cpuinfo */
                    processor->bug_fdiv ? " fdiv" : "",
                    processor->bug_hlt  ? " _hlt" : "",
                    processor->bug_f00f ? " f00f" : "",
                    processor->bug_coma ? " coma" : "",
                    /* these bug workarounds were reported as "features" in older kernels */
                    processor_has_flag(processor->flags, "fxsave_leak")     ? " fxsave_leak" : "",
                    processor_has_flag(processor->flags, "clflush_monitor") ? " clflush_monitor" : "",
                    processor_has_flag(processor->flags, "11ap")            ? " 11ap" : "",
                    processor_has_flag(processor->flags, "tlb_mmatch")      ? " tlb_mmatch" : "",
                    processor_has_flag(processor->flags, "apic_c1e")        ? " apic_c1e" : "",
                    ""); /* just to make adding lines easier */
            g_strchug(processor->bugs);
        }

        if (processor->pm == NULL || g_strcmp0(processor->pm, "") == 0) {
            g_free(processor->pm);
            /* make power management list on old kernels that don't offer one */
            processor->pm = g_strdup_printf("%s%s",
                    /* "hw_pstate" -> "hwpstate" */
                    processor_has_flag(processor->flags, "hw_pstate") ? " hwpstate" : "",
                    ""); /* just to make adding lines easier */
            g_strchug(processor->pm);
        }

        /* topo & freq */
        processor->cpufreq = cpufreq_new(processor->id);
        processor->cputopo = cputopo_new(processor->id);

        if (processor->cpufreq->cpukhz_max)
            processor->cpu_mhz = processor->cpufreq->cpukhz_max / 1000;

        nice_name_x86_cpuid_model_string(processor->model_name);
    }

    return procs;
}

gchar *processor_get_capabilities_from_flags(gchar *strflags, gchar *lookup_prefix)
{
    gchar **flags, **old;
    gchar tmp_flag[64] = "";
    const gchar *meaning;
    gchar *tmp = NULL;
    gint j = 0, i = 0;

    flags = g_strsplit(strflags, " ", 0);
    old = flags;

    while (flags[j]) {
        if ( sscanf(flags[j], "[%d]", &i)==1 ) {
            /* Some flags are indexes, like [13], and that looks like
             * a new section to hardinfo shell */
            tmp = h_strdup_cprintf("(%s%d)=\n", tmp,
                (lookup_prefix) ? lookup_prefix : "",
                i );
        } else {
            sprintf(tmp_flag, "%s%s", lookup_prefix, flags[j]);
            meaning = x86_flag_meaning(tmp_flag);

            if (meaning) {
                tmp = h_strdup_cprintf("%s=%s\n", tmp, flags[j], meaning);
            } else {
                tmp = h_strdup_cprintf("%s=\n", tmp, flags[j]);
            }
        }
        j++;
    }
    if (tmp == NULL || g_strcmp0(tmp, "") == 0)
        tmp = g_strdup_printf("%s=%s\n", "empty", _("Empty List"));

    g_strfreev(old);
    return tmp;
}

gchar *processor_get_detailed_info(Processor * processor)
{
    gchar *tmp_flags, *tmp_bugs, *tmp_pm, *tmp_cpufreq, *tmp_topology, *ret, *cache_info;

    tmp_flags = processor_get_capabilities_from_flags(processor->flags, "");
    tmp_bugs = processor_get_capabilities_from_flags(processor->bugs, "bug:");
    tmp_pm = processor_get_capabilities_from_flags(processor->pm, "pm:");
    cache_info = __cache_get_info_as_string(processor);

    tmp_topology = cputopo_section_str(processor->cputopo);
    tmp_cpufreq = cpufreq_section_str(processor->cpufreq);

    ret = g_strdup_printf("[%s]\n"
                       "%s=%s\n"
                       "%s=%d, %d, %d (%s)\n" /* family, model, stepping (decoded name) */
                       "$^$%s=%s\n"      /* vendor */
                       "%s=%s\n"      /* microcode */
                       "[%s]\n"       /* configuration */
                       "%s=%d %s\n"   /* cache size (from cpuinfo) */
                       "%s=%.2f %s\n" /* frequency */
                       "%s=%.2f\n"    /* bogomips */
                       "%s=%s\n"      /* byte order */
                       "%s"     /* topology */
                       "%s"     /* frequency scaling */
                       "[%s]\n" /* cache */
                       "%s\n"
                       "[%s]\n" /* pm */
                       "%s"
                       "[%s]\n" /* bugs */
                       "%s"
                       "[%s]\n" /* flags */
                       "%s",
                   _("Processor"),
                   _("Model Name"), processor->model_name,
                   _("Family, model, stepping"),
                   processor->family,
                   processor->model,
                   processor->stepping,
                   processor->strmodel,
                   _("Vendor"), processor->vendor_id,
                   _("Microcode Version"), processor->microcode,
                   _("Configuration"),
                   _("Cache Size"), processor->cache_size, _("kb"),
                   _("Frequency"), processor->cpu_mhz, _("MHz"),
                   _("BogoMips"), processor->bogomips,
                   _("Byte Order"), byte_order_str(),
                   tmp_topology,
                   tmp_cpufreq,
                   _("Cache"), cache_info,
                   _("Power Management"), tmp_pm,
                   _("Bug Workarounds"), tmp_bugs,
                   _("Capabilities"), tmp_flags );
    g_free(tmp_flags);
    g_free(tmp_bugs);
    g_free(tmp_pm);
    g_free(cache_info);
    g_free(tmp_cpufreq);
    g_free(tmp_topology);
    return ret;
}

gchar *processor_name(GSList * processors) {
    return processor_name_default(processors);
}

gchar *processor_describe(GSList * processors) {
    return processor_describe_default(processors);
}

gchar *dmi_socket_info() {
    gchar *ret;
    dmi_type dt = 4;
    guint i;
    dmi_handle_list *hl = dmidecode_handles(&dt);

    if (!hl) {
        ret = g_strdup_printf("[%s]\n%s=%s\n",
                _("Socket Information"), _("Result"),
                (getuid() == 0)
                ? _("(Not available)")
                : _("(Not available; Perhaps try running hardinfo2 as root.)") );
    } else {
        ret = g_strdup("");
        for(i = 0; i < hl->count; i++) {
            dmi_handle h = hl->handles[i];
            gchar *upgrade = dmidecode_match("Upgrade", &dt, &h);
            gchar *socket = dmidecode_match("Socket Designation", &dt, &h);
            gchar *bus_clock_str = dmidecode_match("External Clock", &dt, &h);
            gchar *voltage_str = dmidecode_match("Voltage", &dt, &h);
            gchar *max_speed_str = dmidecode_match("Max Speed", &dt, &h);

            ret = h_strdup_cprintf("[%s (%d) %s]\n"
                            "%s=0x%x\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n",
                            ret,
                            _("CPU Socket"), i, socket,
                            _("DMI Handle"), h,
                            _("Type"), upgrade,
                            _("Voltage"), voltage_str,
                            _("External Clock"), bus_clock_str,
                            _("Max Frequency"), max_speed_str
                            );
            g_free(upgrade);
            g_free(socket);
            g_free(bus_clock_str);
            g_free(voltage_str);
            g_free(max_speed_str);
        }

        dmi_handle_list_free(hl);
    }

    return ret;
}

gchar *processor_meta(GSList * processors) {
    gchar *meta_cpu_name = processor_name(processors);
    gchar *meta_cpu_desc = processor_describe(processors);
    gchar *meta_freq_desc = processor_frequency_desc(processors);
    gchar *meta_clocks = clocks_summary(processors);
    gchar *meta_caches = caches_summary(processors);
    gchar *meta_dmi = dmi_socket_info();
    gchar *ret = NULL;
    UNKIFNULL(meta_cpu_desc);
    ret = g_strdup_printf("[%s]\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s"
                        "%s"
                        "%s",
                        _("Package Information"),
                        _("Name"), meta_cpu_name,
                        _("Topology"), meta_cpu_desc,
                        _("Logical CPU Config"), meta_freq_desc,
                        meta_clocks,
                        meta_caches,
                        meta_dmi);
    g_free(meta_cpu_desc);
    g_free(meta_freq_desc);
    g_free(meta_clocks);
    g_free(meta_caches);
    return ret;
}

gchar *processor_get_info(GSList * processors)
{
    Processor *processor;
    gchar *ret, *tmp, *hashkey;
    gchar *meta; /* becomes owned by more_info? no need to free? */
    GSList *l;
    gchar *icons=g_strdup("");

    tmp = g_strdup_printf("$!CPU_META$%s=|Summary\n", "all");

    meta = processor_meta(processors);
    moreinfo_add_with_prefix("DEV", "CPU_META", meta);

    for (l = processors; l; l = l->next) {
        processor = (Processor *) l->data;

        gchar *model_name = g_strdup(processor->model_name);
        const Vendor *v = vendor_match(processor->vendor_id, NULL);
        if (v)
            tag_vendor(&model_name, 0, v->name_short ? v->name_short : v->name, v->ansi_color, params.fmt_opts);

        // bp: not convinced it looks good, but here's how it would be done...
        //icons = h_strdup_cprintf("Icon$CPU%d$cpu%d=processor.png\n", icons, processor->id, processor->id);

        tmp = g_strdup_printf("%s$CPU%d$cpu%d=%.2f %s|%s|%d:%d\n",
                  tmp, processor->id,
                  processor->id,
                  processor->cpu_mhz, _("MHz"),
                  model_name,
                  processor->cputopo->socket_id,
                  processor->cputopo->core_id);

        hashkey = g_strdup_printf("CPU%d", processor->id);
        moreinfo_add_with_prefix("DEV", hashkey,
                processor_get_detailed_info(processor));
        g_free(hashkey);
        g_free(model_name);
    }

    ret = g_strdup_printf("[$ShellParam$]\n"
                  "ViewType=1\n"
                  "ColumnTitle$TextValue=%s\n"
                  "ColumnTitle$Value=%s\n"
                  "ColumnTitle$Extra1=%s\n"
                  "ColumnTitle$Extra2=%s\n"
                  "ShowColumnHeaders=true\n"
                  "%s"
                  "[Processors]\n"
                  "%s", _("Device"), _("Frequency"), _("Model"), _("Socket:Core"), icons, tmp);
    g_free(tmp);
    g_free(icons);

    // now here's something fun...
    struct Info *i = info_unflatten(ret);
    g_free(ret);
    ret = info_flatten(i);

    return ret;
}

