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
static const char* cache_types[] = {
    NC_("cache-type", /*/cache type, as appears in: Level 1 (Data)*/ "Data"),
    NC_("cache-type", /*/cache type, as appears in: Level 1 (Instruction)*/ "Instruction"),
    NC_("cache-type", /*/cache type, as appears in: Level 2 (Unified)*/ "Unified")
};

static void __cache_obtain_info(Processor *processor)
{
    ProcessorCache *cache;
    gchar *endpoint, *entry, *index;
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

      g_free(index);

      processor->cache = g_slist_append(processor->cache, cache);
    }

fail:
    g_free(endpoint);
}

GSList *processor_scan(void)
{
    GSList *procs = NULL, *l = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[512];

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
        return NULL;

    while (fgets(buffer, 512, cpuinfo)) {
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
    }

    return procs;
}

gchar *processor_get_capabilities_from_flags(gchar *strflags, gchar *lookup_prefix)
{
    gchar **flags, **old;
    gchar tmp_flag[64] = "";
    const gchar *meaning;
    gchar *tmp = NULL;
    gint j = 0;

    flags = g_strsplit(strflags, " ", 0);
    old = flags;

    while (flags[j]) {
        sprintf(tmp_flag, "%s%s", lookup_prefix, flags[j]);
        meaning = x86_flag_meaning(tmp_flag);

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
                       "%s=%s\n"      /* vendor */
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
                   _("Vendor"), vendor_get_name(processor->vendor_id),
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

gchar *processor_meta(GSList * processors) {
    gchar *meta_cpu_name = processor_name(processors);
    gchar *meta_cpu_desc = processor_describe(processors);
    gchar *ret = NULL;
    UNKIFNULL(meta_cpu_desc);
    ret = g_strdup_printf("[%s]\n"
                        "%s=%s\n"
                        "%s=%s\n",
                        _("Package Information"),
                        _("Name"), meta_cpu_name,
                        _("Description"), meta_cpu_desc);
    g_free(meta_cpu_desc);
    return ret;
}

gchar *processor_get_info(GSList * processors)
{
    Processor *processor;
    gchar *ret, *tmp, *hashkey;
    gchar *meta; /* becomes owned by more_info? no need to free? */
    GSList *l;

    tmp = g_strdup_printf("$CPU_META$%s=\n", _("Package Information") );

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

