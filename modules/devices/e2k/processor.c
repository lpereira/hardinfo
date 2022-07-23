/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2020 EntityFX <artem.solopiy@gmail.com> and MCST Elbrus Team 
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
    gint cur_count = 0, i = 0;

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

GSList *processor_scan(void)
{
    GSList *procs = NULL, *l = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[1024];

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
        return NULL;

    while (fgets(buffer, 1024, cpuinfo)) {
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
            get_int("cpu family", processor->family);
            get_int("model", processor->model);
            get_int("revision", processor->revision);

            get_float("cpu MHz", processor->cpu_mhz);
            get_float("bogomips", processor->bogomips);
        }

        //populate processor structure
        g_strfreev(tmp);
    }

    //appent to the list
    if (processor)
        procs = g_slist_append(procs, processor);

    for (l = procs; l; l = l->next) {
        processor = (Processor *) l->data;
        __cache_obtain_info(processor);
    }

    fclose(cpuinfo);

    return procs;
}

gchar *processor_name(GSList * processors) {
    return processor_name_default(processors);
}

gchar *processor_describe(GSList * processors) {
    return processor_describe_default(processors);
}

gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *ret;
    gchar *cache_info;
    cache_info = __cache_get_info_as_string(processor);

    ret = g_strdup_printf("[%s]\n"
                    "$^$%s=%s\n"   /* name */
                    "$^$%s=%s\n"   /* vendor */
                    "%s=%d\n"      /* family */
                    "%s=%d\n"      /* model */
                    "%s=%d\n"      /* revision */
                    "%s=%.2f %s\n" /* frequency */
                    "%s=%.2f\n"    /* bogomips */
                    "%s=%s\n"      /* byte order */
                    "[%s]\n"       /* cache */
                    "%s\n",
                    _("Processor"),
                    _("Name"), processor->model_name,
                    _("Vendor"), processor->vendor_id,
                    _("Family"), processor->family,
                    _("Model"), processor->model,
                    _("Revision"), processor->revision,
                    _("Frequency"), processor->cpu_mhz, _("MHz"),
                    _("BogoMips"), processor->bogomips,
                    _("Byte Order"), byte_order_str(),
                    _("Cache"), cache_info
                );
    g_free(cache_info);
    return ret;
}

//prepare processor info for all cpus
gchar *processor_get_info(GSList * processors)
{
    Processor *processor;

    gchar *ret, *tmp, *hashkey;
    GSList *l;
    gchar *icons=g_strdup("");

    tmp = g_strdup("");

    for (l = processors; l; l = l->next) {
        processor = (Processor *) l->data;

        gchar *model_name = g_strdup_printf("MCST Elbrus %s", processor->model_name);
        const Vendor *v = vendor_match(processor->vendor_id, NULL);
        if (v)
            tag_vendor(&model_name, 0, v->name_short ? v->name_short : v->name, v->ansi_color, params.fmt_opts);

        tmp = g_strdup_printf("%s$CPU%d$cpu%d=%.2f %s|%s\n",
                tmp, processor->id,
                processor->id,
                processor->cpu_mhz, _("MHz"),
                model_name);

        hashkey = g_strdup_printf("CPU%d", processor->id);
        moreinfo_add_with_prefix("DEV", hashkey,
                processor_get_detailed_info(processor));
        g_free(hashkey);
    }

    ret = g_strdup_printf("[$ShellParam$]\n"
                "ViewType=1\n"
                "ColumnTitle$TextValue=%s\n"
                "ColumnTitle$Value=%s\n"
                "ColumnTitle$Extra1=%s\n"
                "ShowColumnHeaders=true\n"
                "%s"
                "[Processors]\n"
                "%s", _("Device"), _("Frequency"), _("Model"), icons, tmp);
    g_free(tmp);
    g_free(icons);

    return ret;
}
