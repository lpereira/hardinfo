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

/* sources:
 *   https://unix.stackexchange.com/a/43563
 *   git:linux/arch/arm/kernel/setup.c
 *   git:linux/arch/arm64/kernel/cpuinfo.c
 */
static struct {
    char *name, *meaning;
} flag_meaning[] = {
    /* arm/hw_cap */
    { "swp",	"SWP instruction (atomic read-modify-write)" },
    { "half",	"Half-word loads and stores" },
    { "thumb",	"Thumb (16-bit instruction set)" },
    { "26bit",	"26-Bit Model (Processor status register folded into program counter)" },
    { "fastmult",	"32x32->64-bit multiplication" },
    { "fpa",	"Floating point accelerator" },
    { "vfp",	"VFP (early SIMD vector floating point instructions)" },
    { "edsp",	"DSP extensions (the 'e' variant of the ARM9 CPUs, and all others above)" },
    { "java",	"Jazelle (Java bytecode accelerator)" },
    { "iwmmxt",	"SIMD instructions similar to Intel MMX" },
    { "crunch",	"MaverickCrunch coprocessor (if kernel support enabled)" },
    { "thumbee",	"ThumbEE" },
    { "neon",	"Advanced SIMD/NEON on AArch32" },
    { "evtstrm",	"kernel event stream using generic architected timer" },
    { "vfpv3",	"VFP version 3" },
    { "vfpv3d16",	"VFP version 3 with 16 D-registers" },
    { "vfpv4",	"VFP version 4 with fast context switching" },
    { "vfpd32",	"VFP with 32 D-registers" },
    { "tls",	"TLS register" },
    { "idiva",	"SDIV and UDIV hardware division in ARM mode" },
    { "idivt",	"SDIV and UDIV hardware division in Thumb mode" },
    { "lpae",	"40-bit Large Physical Address Extension" },
    /* arm/hw_cap2 */
    { "pmull",	"64x64->128-bit F2m multiplication (arch>8)" },
    { "aes",	"Crypto:AES (arch>8)" },
    { "sha1",	"Crypto:SHA1 (arch>8)" },
    { "sha2",	"Crypto:SHA2 (arch>8)" },
    { "crc32",	"CRC32 checksum instructions (arch>8)" },
    /* arm64/hw_cap */
    { "fp",	"" },
    { "asimd",	"Advanced SIMD/NEON on AArch64 (arch>8)" },
    { "atomics",	"" },
    { "fphp",	"" },
    { "asimdhp",	"" },
    { "cpuid",	"" },
    { "asimdrdm",	"" },
    { "jscvt",	"" },
    { "fcma",	"" },
    { "lrcpc",	"" },

    { NULL, NULL},
};

GHashTable *cpu_flags = NULL; /* FIXME: when is it freed? */

static void
populate_cpu_flags_list_internal()
{
    int i;
    cpu_flags = g_hash_table_new(g_str_hash, g_str_equal);
    for (i = 0; flag_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, flag_meaning[i].name,
                            flag_meaning[i].meaning);
    }
}

static gint get_cpu_int(const gchar* file, gint cpuid) {
    gchar *tmp0 = NULL;
    gchar *tmp1 = NULL;
    gint ret = 0;

    tmp0 = g_strdup_printf("/sys/devices/system/cpu/cpu%d/%s", cpuid, file);
    g_file_get_contents(tmp0, &tmp1, NULL, NULL);
    ret = atol(tmp1);

    g_free(tmp0);
    g_free(tmp1);
    return ret;
}

static gboolean _g_strv_contains(const gchar * const * strv, const gchar *str) {
    /* g_strv_contains() requires glib>2.44 */
    //return g_strv_contains(strv, str);
    gint i = 0;
    while(strv[i] != NULL) {
        if (g_strcmp0(strv[i], str) == 0)
            return 1;
        i++;
    }
    return 0;
}

int processor_has_flag(gchar * strflags, gchar * strflag)
{
    gchar **flags;
    gint ret = 0;
    if (strflags == NULL || strflag == NULL)
        return 0;
    flags = g_strsplit(strflags, " ", 0);
    ret = _g_strv_contains((const gchar * const *)flags, strflag);
    g_strfreev(flags);
    return ret;
}

GSList *
processor_scan(void)
{
    GSList *procs = NULL;
    Processor *processor;
    gint processor_number = 0;
    FILE *cpuinfo;
    gchar buffer[128];
    gchar *tmpfreq_str = NULL;

    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo)
    return NULL;

    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);

        if (g_str_has_prefix(tmp[0], "processor")) {
            if (processor) {
                procs = g_slist_append(procs, processor);
            }

            processor = g_new0(Processor, 1);
            //get_int("processor", processor->id);
            processor->id = processor_number;
            processor_number++;

            /* freq */
            processor->cpukhz_cur = get_cpu_int("cpufreq/scaling_cur_freq", processor->id);
            processor->cpukhz_min = get_cpu_int("cpufreq/scaling_min_freq", processor->id);
            processor->cpukhz_max = get_cpu_int("cpufreq/scaling_max_freq", processor->id);
            if (processor->cpukhz_max)
                processor->cpu_mhz = processor->cpukhz_max / 1000;
            else
                processor->cpu_mhz = 0.0f;
        }

        if (processor && tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);

            get_str("Processor", processor->model_name);
            if (!processor->model_name)
                get_str("model name", processor->model_name);
            get_str("Features", processor->flags);
            get_float("BogoMIPS", processor->bogomips);

            get_str("CPU implementer", processor->cpu_implementer);
            get_str("CPU architecture", processor->cpu_architecture);
            get_str("CPU variant", processor->cpu_variant);
            get_str("CPU part", processor->cpu_part);
            get_str("CPU revision", processor->cpu_revision);

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
        g_strfreev(tmp);
    }

    if (processor)
        procs = g_slist_append(procs, processor);

    fclose(cpuinfo);

    return procs;
}

gchar *processor_get_capabilities_from_flags(gchar * strflags)
{
    gchar **flags, **old;
    gchar *tmp = NULL;
    gint j = 0;

    if (!cpu_flags)
        populate_cpu_flags_list_internal();

    flags = g_strsplit(strflags, " ", 0);
    old = flags;

    while (flags[j]) {
        gchar *meaning = g_hash_table_lookup(cpu_flags, flags[j]);

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

gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_flags, *ret;
    tmp_flags = processor_get_capabilities_from_flags(processor->flags);

    ret = g_strdup_printf("[Processor]\n"
                           "Name=%s\n"
                           "Mode=%s\n"
                   "BogoMips=%.2f\n"
                   "Endianesss="
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                       "Little Endian"
#else
                       "Big Endian"
#endif
                       "\n"
                       "[Freq]\n"
                       "Min=%d kHz\n"
                       "Max=%d kHz\n"
                       "Cur=%d kHz\n"
                       "[ARM]\n"
                       "Implementer=%s\n"
                       "Architecture=%s\n"
                       "Variant=%s\n"
                       "Part=%s\n"
                       "Revision=%s\n"
                       "[Capabilities]\n"
                       "%s"
                       "%s",
                   processor->model_name,
                   arm_mode_str[processor->mode],
                   processor->bogomips,
                   processor->cpukhz_min,
                   processor->cpukhz_max,
                   processor->cpukhz_cur,
                   processor->cpu_implementer,
                   processor->cpu_architecture,
                   processor->cpu_variant,
                   processor->cpu_part,
                   processor->cpu_revision,
                   tmp_flags,
                    "");
    g_free(tmp_flags);
    return ret;
}

gchar *processor_get_info(GSList * processors)
{
    Processor *processor;

    if (g_slist_length(processors) > 1) {
    gchar *ret, *tmp, *hashkey;
    GSList *l;

    tmp = g_strdup("");

    for (l = processors; l; l = l->next) {
        processor = (Processor *) l->data;

        tmp = g_strdup_printf(_("%s$CPU%d$%s=%.2fMHz\n"),
                  tmp, processor->id,
                  processor->model_name,
                  processor->cpu_mhz);

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
