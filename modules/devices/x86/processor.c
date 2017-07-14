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
                                  cache->type,
                                  cache->ways_of_associativity,
                                  cache->number_of_sets,
                                  cache->size);
    }

    return result;
}

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
            get_int("cache size", processor->cache_size);
            get_float("cpu MHz", processor->cpu_mhz);
            get_float("bogomips", processor->bogomips);

            get_str("fpu", processor->has_fpu);

            get_str("fdiv_bug", processor->bug_fdiv);
            get_str("hlt_bug", processor->bug_hlt);
            get_str("f00f_bug", processor->bug_f00f);
            get_str("coma_bug", processor->bug_coma);

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

        get_processor_strfamily(processor);
        __cache_obtain_info(processor);

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

/*
 * Sources:
 * - https://unix.stackexchange.com/a/43540
 * - https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git/tree/arch/x86/include/asm/cpufeatures.h?id=refs/tags/v4.9
 * - Intel IA-32 Architecture Software Development Manual
 * - http://gentoo-wiki.com/Cpuinfo
 */
static struct {
    char *name, *meaning;
} flag_meaning[] = {
/* Intel-defined CPU features, CPUID level 0x00000001 (edx)
 * See also Wikipedia and table 2-27 in Intel Advanced Vector Extensions Programming Reference */
    { "fpu",     NC_("x86-flag", /*/flag:fpu*/  "Onboard FPU (floating point support)") },
    { "vme",     NC_("x86-flag", /*/flag:vme*/  "Virtual 8086 mode enhancements") },
    { "de",      NC_("x86-flag", /*/flag:de*/  "Debugging Extensions (CR4.DE)") },
    { "pse",     NC_("x86-flag", /*/flag:pse*/  "Page Size Extensions (4MB memory pages)") },
    { "tsc",     NC_("x86-flag", /*/flag:tsc*/  "Time Stamp Counter (RDTSC)") },
    { "msr",     NC_("x86-flag", /*/flag:msr*/  "Model-Specific Registers (RDMSR, WRMSR)") },
    { "pae",     NC_("x86-flag", /*/flag:pae*/  "Physical Address Extensions (support for more than 4GB of RAM)") },
    { "mce",     NC_("x86-flag", /*/flag:mce*/  "Machine Check Exception") },
    { "cx8",     NC_("x86-flag", /*/flag:cx8*/  "CMPXCHG8 instruction (64-bit compare-and-swap)") },
    { "apic",    NC_("x86-flag", /*/flag:apic*/  "Onboard APIC") },
    { "sep",     NC_("x86-flag", /*/flag:sep*/  "SYSENTER/SYSEXIT") },
    { "mtrr",    NC_("x86-flag", /*/flag:mtrr*/  "Memory Type Range Registers") },
    { "pge",     NC_("x86-flag", /*/flag:pge*/  "Page Global Enable (global bit in PDEs and PTEs)") },
    { "mca",     NC_("x86-flag", /*/flag:mca*/  "Machine Check Architecture") },
    { "cmov",    NC_("x86-flag", /*/flag:cmov*/  "CMOV instructions (conditional move) (also FCMOV)") },
    { "pat",     NC_("x86-flag", /*/flag:pat*/  "Page Attribute Table") },
    { "pse36",   NC_("x86-flag", /*/flag:pse36*/  "36-bit PSEs (huge pages)") },
    { "pn",      NC_("x86-flag", /*/flag:pn*/  "Processor serial number") },
    { "clflush", NC_("x86-flag", /*/flag:clflush*/  "Cache Line Flush instruction") },
    { "dts",     NC_("x86-flag", /*/flag:dts*/  "Debug Store (buffer for debugging and profiling instructions), or alternately: digital thermal sensor") },
    { "acpi",    NC_("x86-flag", /*/flag:acpi*/  "ACPI via MSR (temperature monitoring and clock speed modulation)") },
    { "mmx",     NC_("x86-flag", /*/flag:mmx*/  "Multimedia Extensions") },
    { "fxsr",    NC_("x86-flag", /*/flag:fxsr*/  "FXSAVE/FXRSTOR, CR4.OSFXSR") },
    { "sse",     NC_("x86-flag", /*/flag:sse*/  "Intel SSE vector instructions") },
    { "sse2",    NC_("x86-flag", /*/flag:sse2*/  "SSE2") },
    { "ss",      NC_("x86-flag", /*/flag:ss*/  "CPU self snoop") },
    { "ht",      NC_("x86-flag", /*/flag:ht*/  "Hyper-Threading") },
    { "tm",      NC_("x86-flag", /*/flag:tm*/  "Automatic clock control (Thermal Monitor)") },
    { "ia64",    NC_("x86-flag", /*/flag:ia64*/  "Intel Itanium Architecture 64-bit (not to be confused with Intel's 64-bit x86 architecture with flag x86-64 or \"AMD64\" bit indicated by flag lm)") },
    { "pbe",     NC_("x86-flag", /*/flag:pbe*/  "Pending Break Enable (PBE# pin) wakeup support") },
/* AMD-defined CPU features, CPUID level 0x80000001
 * See also Wikipedia and table 2-23 in Intel Advanced Vector Extensions Programming Reference */
    { "syscall",  NC_("x86-flag", /*/flag:syscall*/  "SYSCALL (Fast System Call) and SYSRET (Return From Fast System Call)") },
    { "mp",       NC_("x86-flag", /*/flag:mp*/  "Multiprocessing Capable.") },
    { "nx",       NC_("x86-flag", /*/flag:nx*/  "Execute Disable") },
    { "mmxext",   NC_("x86-flag", /*/flag:mmxext*/  "AMD MMX extensions") },
    { "fxsr_opt", NC_("x86-flag", /*/flag:fxsr_opt*/  "FXSAVE/FXRSTOR optimizations") },
    { "pdpe1gb",  NC_("x86-flag", /*/flag:pdpe1gb*/  "One GB pages (allows hugepagesz=1G)") },
    { "rdtscp",   NC_("x86-flag", /*/flag:rdtscp*/  "Read Time-Stamp Counter and Processor ID") },
    { "lm",       NC_("x86-flag", /*/flag:lm*/  "Long Mode (x86-64: amd64, also known as Intel 64, i.e. 64-bit capable)") },
    { "3dnow",    NC_("x86-flag", /*/flag:3dnow*/  "3DNow! (AMD vector instructions, competing with Intel's SSE1)") },
    { "3dnowext", NC_("x86-flag", /*/flag:3dnowext*/  "AMD 3DNow! extensions") },
/* Transmeta-defined CPU features, CPUID level 0x80860001 */
    { "recovery", NC_("x86-flag", /*/flag:recovery*/  "CPU in recovery mode") },
    { "longrun",  NC_("x86-flag", /*/flag:longrun*/  "Longrun power control") },
    { "lrti",     NC_("x86-flag", /*/flag:lrti*/  "LongRun table interface") },
/* Other features, Linux-defined mapping */
    { "cxmmx",        NC_("x86-flag", /*/flag:cxmmx*/  "Cyrix MMX extensions") },
    { "k6_mtrr",      NC_("x86-flag", /*/flag:k6_mtrr*/  "AMD K6 nonstandard MTRRs") },
    { "cyrix_arr",    NC_("x86-flag", /*/flag:cyrix_arr*/  "Cyrix ARRs (= MTRRs)") },
    { "centaur_mcr",  NC_("x86-flag", /*/flag:centaur_mcr*/  "Centaur MCRs (= MTRRs)") },
    { "constant_tsc", NC_("x86-flag", /*/flag:constant_tsc*/  "TSC ticks at a constant rate") },
    { "up",           NC_("x86-flag", /*/flag:up*/  "SMP kernel running on UP") },
    { "art",          NC_("x86-flag", /*/flag:art*/  "Always-Running Timer") },
    { "arch_perfmon", NC_("x86-flag", /*/flag:arch_perfmon*/  "Intel Architectural PerfMon") },
    { "pebs",         NC_("x86-flag", /*/flag:pebs*/  "Precise-Event Based Sampling") },
    { "bts",          NC_("x86-flag", /*/flag:bts*/  "Branch Trace Store") },
    { "rep_good",     NC_("x86-flag", /*/flag:rep_good*/  "rep microcode works well") },
    { "acc_power",    NC_("x86-flag", /*/flag:acc_power*/  "AMD accumulated power mechanism") },
    { "nopl",         NC_("x86-flag", /*/flag:nopl*/  "The NOPL (0F 1F) instructions") },
    { "xtopology",    NC_("x86-flag", /*/flag:xtopology*/  "cpu topology enum extensions") },
    { "tsc_reliable", NC_("x86-flag", /*/flag:tsc_reliable*/  "TSC is known to be reliable") },
    { "nonstop_tsc",  NC_("x86-flag", /*/flag:nonstop_tsc*/  "TSC does not stop in C states") },
    { "extd_apicid",  NC_("x86-flag", /*/flag:extd_apicid*/  "has extended APICID (8 bits)") },
    { "amd_dcm",      NC_("x86-flag", /*/flag:amd_dcm*/  "multi-node processor") },
    { "aperfmperf",   NC_("x86-flag", /*/flag:aperfmperf*/  "APERFMPERF") },
    { "eagerfpu",     NC_("x86-flag", /*/flag:eagerfpu*/  "Non lazy FPU restore") },
    { "nonstop_tsc_s3", NC_("x86-flag", /*/flag:nonstop_tsc_s3*/  "TSC doesn't stop in S3 state") },
    { "mce_recovery",   NC_("x86-flag", /*/flag:mce_recovery*/  "CPU has recoverable machine checks") },
/* Intel-defined CPU features, CPUID level 0x00000001 (ecx)
 * See also Wikipedia and table 2-26 in Intel Advanced Vector Extensions Programming Reference */
    { "pni",       NC_("x86-flag", /*/flag:pni*/  "SSE-3 (\"Prescott New Instructions\")") },
    { "pclmulqdq", NC_("x86-flag", /*/flag:pclmulqdq*/  "Perform a Carry-Less Multiplication of Quadword instruction - accelerator for GCM)") },
    { "dtes64",    NC_("x86-flag", /*/flag:dtes64*/  "64-bit Debug Store") },
    { "monitor",   NC_("x86-flag", /*/flag:monitor*/  "Monitor/Mwait support (Intel SSE3 supplements)") },
    { "ds_cpl",    NC_("x86-flag", /*/flag:ds_cpl*/  "CPL Qual. Debug Store") },
    { "vmx",       NC_("x86-flag", /*/flag:vmx*/  "Hardware virtualization, Intel VMX") },
    { "smx",       NC_("x86-flag", /*/flag:smx*/  "Safer mode TXT (TPM support)") },
    { "est",       NC_("x86-flag", /*/flag:est*/  "Enhanced SpeedStep") },
    { "tm2",       NC_("x86-flag", /*/flag:tm2*/  "Thermal Monitor 2") },
    { "ssse3",     NC_("x86-flag", /*/flag:ssse3*/  "Supplemental SSE-3") },
    { "cid",       NC_("x86-flag", /*/flag:cid*/  "Context ID") },
    { "sdbg",      NC_("x86-flag", /*/flag:sdbg*/  "silicon debug") },
    { "fma",       NC_("x86-flag", /*/flag:fma*/  "Fused multiply-add") },
    { "cx16",      NC_("x86-flag", /*/flag:cx16*/  "CMPXCHG16B") },
    { "xtpr",      NC_("x86-flag", /*/flag:xtpr*/  "Send Task Priority Messages") },
    { "pdcm",      NC_("x86-flag", /*/flag:pdcm*/  "Performance Capabilities") },
    { "pcid",      NC_("x86-flag", /*/flag:pcid*/  "Process Context Identifiers") },
    { "dca",       NC_("x86-flag", /*/flag:dca*/  "Direct Cache Access") },
    { "sse4_1",    NC_("x86-flag", /*/flag:sse4_1*/  "SSE-4.1") },
    { "sse4_2",    NC_("x86-flag", /*/flag:sse4_2*/  "SSE-4.2") },
    { "x2apic",    NC_("x86-flag", /*/flag:x2apic*/  "x2APIC") },
    { "movbe",     NC_("x86-flag", /*/flag:movbe*/  "Move Data After Swapping Bytes instruction") },
    { "popcnt",    NC_("x86-flag", /*/flag:popcnt*/  "Return the Count of Number of Bits Set to 1 instruction (Hamming weight, i.e. bit count)") },
    { "tsc_deadline_timer", NC_("x86-flag", /*/flag:tsc_deadline_timer*/  "Tsc deadline timer") },
    { "aes/aes-ni",  NC_("x86-flag", /*/flag:aes/aes-ni*/  "Advanced Encryption Standard (New Instructions)") },
    { "xsave",       NC_("x86-flag", /*/flag:xsave*/  "Save Processor Extended States: also provides XGETBY,XRSTOR,XSETBY") },
    { "avx",         NC_("x86-flag", /*/flag:avx*/  "Advanced Vector Extensions") },
    { "f16c",        NC_("x86-flag", /*/flag:f16c*/  "16-bit fp conversions (CVT16)") },
    { "rdrand",      NC_("x86-flag", /*/flag:rdrand*/  "Read Random Number from hardware random number generator instruction") },
    { "hypervisor",  NC_("x86-flag", /*/flag:hypervisor*/  "Running on a hypervisor") },
/* VIA/Cyrix/Centaur-defined CPU features, CPUID level 0xC0000001 */
    { "rng",     NC_("x86-flag", /*/flag:rng*/  "Random Number Generator present (xstore)") },
    { "rng_en",  NC_("x86-flag", /*/flag:rng_en*/  "Random Number Generator enabled") },
    { "ace",     NC_("x86-flag", /*/flag:ace*/  "on-CPU crypto (xcrypt)") },
    { "ace_en",  NC_("x86-flag", /*/flag:ace_en*/  "on-CPU crypto enabled") },
    { "ace2",    NC_("x86-flag", /*/flag:ace2*/  "Advanced Cryptography Engine v2") },
    { "ace2_en", NC_("x86-flag", /*/flag:ace2_en*/  "ACE v2 enabled") },
    { "phe",     NC_("x86-flag", /*/flag:phe*/  "PadLock Hash Engine") },
    { "phe_en",  NC_("x86-flag", /*/flag:phe_en*/  "PHE enabled") },
    { "pmm",     NC_("x86-flag", /*/flag:pmm*/  "PadLock Montgomery Multiplier") },
    { "pmm_en",  NC_("x86-flag", /*/flag:pmm_en*/  "PMM enabled") },
/* More extended AMD flags: CPUID level 0x80000001, ecx */
    { "lahf_lm",       NC_("x86-flag", /*/flag:lahf_lm*/  "Load AH from Flags (LAHF) and Store AH into Flags (SAHF) in long mode") },
    { "cmp_legacy",    NC_("x86-flag", /*/flag:cmp_legacy*/  "If yes HyperThreading not valid") },
    { "svm",           NC_("x86-flag", /*/flag:svm*/  "\"Secure virtual machine\": AMD-V") },
    { "extapic",       NC_("x86-flag", /*/flag:extapic*/  "Extended APIC space") },
    { "cr8_legacy",    NC_("x86-flag", /*/flag:cr8_legacy*/  "CR8 in 32-bit mode") },
    { "abm",           NC_("x86-flag", /*/flag:abm*/  "Advanced Bit Manipulation") },
    { "sse4a",         NC_("x86-flag", /*/flag:sse4a*/  "SSE-4A") },
    { "misalignsse",   NC_("x86-flag", /*/flag:misalignsse*/  "indicates if a general-protection exception (#GP) is generated when some legacy SSE instructions operate on unaligned data. Also depends on CR0 and Alignment Checking bit") },
    { "3dnowprefetch", NC_("x86-flag", /*/flag:3dnowprefetch*/  "3DNow prefetch instructions") },
    { "osvw",          NC_("x86-flag", /*/flag:osvw*/  "indicates OS Visible Workaround, which allows the OS to work around processor errata.") },
    { "ibs",           NC_("x86-flag", /*/flag:ibs*/  "Instruction Based Sampling") },
    { "xop",           NC_("x86-flag", /*/flag:xop*/  "extended AVX instructions") },
    { "skinit",        NC_("x86-flag", /*/flag:skinit*/  "SKINIT/STGI instructions") },
    { "wdt",           NC_("x86-flag", /*/flag:wdt*/  "Watchdog timer") },
    { "lwp",           NC_("x86-flag", /*/flag:lwp*/  "Light Weight Profiling") },
    { "fma4",          NC_("x86-flag", /*/flag:fma4*/  "4 operands MAC instructions") },
    { "tce",           NC_("x86-flag", /*/flag:tce*/  "translation cache extension") },
    { "nodeid_msr",    NC_("x86-flag", /*/flag:nodeid_msr*/  "NodeId MSR") },
    { "tbm",           NC_("x86-flag", /*/flag:tbm*/  "Trailing Bit Manipulation") },
    { "topoext",       NC_("x86-flag", /*/flag:topoext*/  "Topology Extensions CPUID leafs") },
    { "perfctr_core",  NC_("x86-flag", /*/flag:perfctr_core*/  "Core Performance Counter Extensions") },
    { "perfctr_nb",    NC_("x86-flag", /*/flag:perfctr_nb*/  "NB Performance Counter Extensions") },
    { "bpext",         NC_("x86-flag", /*/flag:bpext*/  "data breakpoint extension") },
    { "ptsc",          NC_("x86-flag", /*/flag:ptsc*/  "performance time-stamp counter") },
    { "perfctr_l2",    NC_("x86-flag", /*/flag:perfctr_l2*/  "L2 Performance Counter Extensions") },
    { "mwaitx",        NC_("x86-flag", /*/flag:mwaitx*/  "MWAIT extension (MONITORX/MWAITX)") },
/* Auxiliary flags: Linux defined - For features scattered in various CPUID levels */
    { "cpb",           NC_("x86-flag", /*/flag:cpb*/  "AMD Core Performance Boost") },
    { "epb",           NC_("x86-flag", /*/flag:epb*/  "IA32_ENERGY_PERF_BIAS support") },
    { "hw_pstate",     NC_("x86-flag", /*/flag:hw_pstate*/  "AMD HW-PState") },
    { "proc_feedback", NC_("x86-flag", /*/flag:proc_feedback*/  "AMD ProcFeedbackInterface") },
    { "intel_pt",      NC_("x86-flag", /*/flag:intel_pt*/  "Intel Processor Tracing") },
/* Virtualization flags: Linux defined */
    { "tpr_shadow",   NC_("x86-flag", /*/flag:tpr_shadow*/  "Intel TPR Shadow") },
    { "vnmi",         NC_("x86-flag", /*/flag:vnmi*/  "Intel Virtual NMI") },
    { "flexpriority", NC_("x86-flag", /*/flag:flexpriority*/  "Intel FlexPriority") },
    { "ept",          NC_("x86-flag", /*/flag:ept*/  "Intel Extended Page Table") },
    { "vpid",         NC_("x86-flag", /*/flag:vpid*/  "Intel Virtual Processor ID") },
    { "vmmcall",      NC_("x86-flag", /*/flag:vmmcall*/  "prefer VMMCALL to VMCALL") },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ebx) */
    { "fsgsbase",   NC_("x86-flag", /*/flag:fsgsbase*/  "{RD/WR}{FS/GS}BASE instructions") },
    { "tsc_adjust", NC_("x86-flag", /*/flag:tsc_adjust*/  "TSC adjustment MSR") },
    { "bmi1",       NC_("x86-flag", /*/flag:bmi1*/  "1st group bit manipulation extensions") },
    { "hle",        NC_("x86-flag", /*/flag:hle*/  "Hardware Lock Elision") },
    { "avx2",       NC_("x86-flag", /*/flag:avx2*/  "AVX2 instructions") },
    { "smep",       NC_("x86-flag", /*/flag:smep*/  "Supervisor Mode Execution Protection") },
    { "bmi2",       NC_("x86-flag", /*/flag:bmi2*/  "2nd group bit manipulation extensions") },
    { "erms",       NC_("x86-flag", /*/flag:erms*/  "Enhanced REP MOVSB/STOSB") },
    { "invpcid",    NC_("x86-flag", /*/flag:invpcid*/  "Invalidate Processor Context ID") },
    { "rtm",        NC_("x86-flag", /*/flag:rtm*/  "Restricted Transactional Memory") },
    { "cqm",        NC_("x86-flag", /*/flag:cqm*/  "Cache QoS Monitoring") },
    { "mpx",        NC_("x86-flag", /*/flag:mpx*/  "Memory Protection Extension") },
    { "avx512f",    NC_("x86-flag", /*/flag:avx512f*/  "AVX-512 foundation") },
    { "avx512dq",   NC_("x86-flag", /*/flag:avx512dq*/  "AVX-512 Double/Quad instructions") },
    { "rdseed",     NC_("x86-flag", /*/flag:rdseed*/  "The RDSEED instruction") },
    { "adx",        NC_("x86-flag", /*/flag:adx*/  "The ADCX and ADOX instructions") },
    { "smap",       NC_("x86-flag", /*/flag:smap*/  "Supervisor Mode Access Prevention") },
    { "clflushopt", NC_("x86-flag", /*/flag:clflushopt*/  "CLFLUSHOPT instruction") },
    { "clwb",       NC_("x86-flag", /*/flag:clwb*/  "CLWB instruction") },
    { "avx512pf",   NC_("x86-flag", /*/flag:avx512pf*/  "AVX-512 Prefetch") },
    { "avx512er",   NC_("x86-flag", /*/flag:avx512er*/  "AVX-512 Exponential and Reciprocal") },
    { "avx512cd",   NC_("x86-flag", /*/flag:avx512cd*/  "AVX-512 Conflict Detection") },
    { "sha_ni",     NC_("x86-flag", /*/flag:sha_ni*/  "SHA1/SHA256 Instruction Extensions") },
    { "avx512bw",   NC_("x86-flag", /*/flag:avx512bw*/  "AVX-512 Byte/Word instructions") },
    { "avx512vl",   NC_("x86-flag", /*/flag:avx512vl*/  "AVX-512 128/256 Vector Length extensions") },
/* Extended state features, CPUID level 0x0000000d:1 (eax) */
    { "xsaveopt",   NC_("x86-flag", /*/flag:xsaveopt*/  "Optimized XSAVE") },
    { "xsavec",     NC_("x86-flag", /*/flag:xsavec*/  "XSAVEC") },
    { "xgetbv1",    NC_("x86-flag", /*/flag:xgetbv1*/  "XGETBV with ECX = 1") },
    { "xsaves",     NC_("x86-flag", /*/flag:xsaves*/  "XSAVES/XRSTORS") },
/* Intel-defined CPU QoS sub-leaf, CPUID level 0x0000000F:0 (edx) */
    { "cqm_llc",    NC_("x86-flag", /*/flag:cqm_llc*/  "LLC QoS") },
/* Intel-defined CPU QoS sub-leaf, CPUID level 0x0000000F:1 (edx) */
    { "cqm_occup_llc",  NC_("x86-flag", /*/flag:cqm_occup_llc*/  "LLC occupancy monitoring") },
    { "cqm_mbm_total",  NC_("x86-flag", /*/flag:cqm_mbm_total*/  "LLC total MBM monitoring") },
    { "cqm_mbm_local",  NC_("x86-flag", /*/flag:cqm_mbm_local*/  "LLC local MBM monitoring") },
/* AMD-defined CPU features, CPUID level 0x80000008 (ebx) */
    { "clzero",         NC_("x86-flag", /*/flag:clzero*/  "CLZERO instruction") },
    { "irperf",         NC_("x86-flag", /*/flag:irperf*/  "instructions retired performance counter") },
/* Thermal and Power Management leaf, CPUID level 0x00000006 (eax) */
    { "dtherm",         NC_("x86-flag", /*/flag:dtherm*/  "digital thermal sensor") }, /* formerly dts */
    { "ida",            NC_("x86-flag", /*/flag:ida*/  "Intel Dynamic Acceleration") },
    { "arat",           NC_("x86-flag", /*/flag:arat*/  "Always Running APIC Timer") },
    { "pln",            NC_("x86-flag", /*/flag:pln*/  "Intel Power Limit Notification") },
    { "pts",            NC_("x86-flag", /*/flag:pts*/  "Intel Package Thermal Status") },
    { "hwp",            NC_("x86-flag", /*/flag:hwp*/  "Intel Hardware P-states") },
    { "hwp_notify",     NC_("x86-flag", /*/flag:hwp_notify*/  "HWP notification") },
    { "hwp_act_window", NC_("x86-flag", /*/flag:hwp_act_window*/  "HWP Activity Window") },
    { "hwp_epp",        NC_("x86-flag", /*/flag:hwp_epp*/  "HWP Energy Performance Preference") },
    { "hwp_pkg_req",    NC_("x86-flag", /*/flag:hwp_pkg_req*/  "HWP package-level request") },
/* AMD SVM Feature Identification, CPUID level 0x8000000a (edx) */
    { "npt",            NC_("x86-flag", /*/flag:npt*/  "AMD Nested Page Table support") },
    { "lbrv",           NC_("x86-flag", /*/flag:lbrv*/  "AMD LBR Virtualization support") },
    { "svm_lock",       NC_("x86-flag", /*/flag:svm_lock*/  "AMD SVM locking MSR") },
    { "nrip_save",      NC_("x86-flag", /*/flag:nrip_save*/  "AMD SVM next_rip save") },
    { "tsc_scale",      NC_("x86-flag", /*/flag:tsc_scale*/  "AMD TSC scaling support") },
    { "vmcb_clean",     NC_("x86-flag", /*/flag:vmcb_clean*/  "AMD VMCB clean bits support") },
    { "flushbyasid",    NC_("x86-flag", /*/flag:flushbyasid*/  "AMD flush-by-ASID support") },
    { "decodeassists",  NC_("x86-flag", /*/flag:decodeassists*/  "AMD Decode Assists support") },
    { "pausefilter",    NC_("x86-flag", /*/flag:pausefilter*/  "AMD filtered pause intercept") },
    { "pfthreshold",    NC_("x86-flag", /*/flag:pfthreshold*/  "AMD pause filter threshold") },
    { "avic",           NC_("x86-flag", /*/flag:avic*/  "Virtual Interrupt Controller") },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ecx) */
    { "pku",            NC_("x86-flag", /*/flag:pku*/  "Protection Keys for Userspace") },
    { "ospke",          NC_("x86-flag", /*/flag:ospke*/  "OS Protection Keys Enable") },
/* AMD-defined CPU features, CPUID level 0x80000007 (ebx) */
    { "overflow_recov", NC_("x86-flag", /*/flag:overflow_recov*/  "MCA overflow recovery support") },
    { "succor",         NC_("x86-flag", /*/flag:succor*/  "uncorrectable error containment and recovery") },
    { "smca",           NC_("x86-flag", /*/flag:smca*/  "Scalable MCA") },
    { NULL, NULL},
};

static struct {
    char *name, *meaning;
} bug_meaning[] = {
    { "f00f",       NC_("x86-flag", /*/bug:f00f*/  "Intel F00F bug")    },
    { "fdiv",       NC_("x86-flag", /*/bug:fdiv*/  "FPU FDIV")          },
    { "coma",       NC_("x86-flag", /*/bug:coma*/  "Cyrix 6x86 coma")   },
    { "tlb_mmatch", NC_("x86-flag", /*/bug:tlb_mmatch*/  "AMD Erratum 383")   },
    { "apic_c1e",   NC_("x86-flag", /*/bug:apic_c1e*/  "AMD Erratum 400")   },
    { "11ap",       NC_("x86-flag", /*/bug:11ap*/  "Bad local APIC aka 11AP")  },
    { "fxsave_leak",      NC_("x86-flag", /*/bug:fxsave_leak*/  "FXSAVE leaks FOP/FIP/FOP") },
    { "clflush_monitor",  NC_("x86-flag", /*/bug:clflush_monitor*/  "AAI65, CLFLUSH required before MONITOR") },
    { "sysret_ss_attrs",  NC_("x86-flag", /*/bug:sysret_ss_attrs*/  "SYSRET doesn't fix up SS attrs") },
    { "espfix",       NC_("x86-flag", /*/bug:espfix*/  "IRET to 16-bit SS corrupts ESP/RSP high bits") },
    { "null_seg",     NC_("x86-flag", /*/bug:null_seg*/  "Nulling a selector preserves the base") },         /* see: detect_null_seg_behavior() */
    { "swapgs_fence", NC_("x86-flag", /*/bug:swapgs_fence*/  "SWAPGS without input dep on GS") },
    { "monitor",      NC_("x86-flag", /*/bug:monitor*/  "IPI required to wake up remote CPU") },
    { "amd_e400",     NC_("x86-flag", /*/bug:amd_e400*/  "AMD Erratum 400") },
    { NULL, NULL },
};

/* from arch/x86/kernel/cpu/powerflags.h */
static struct {
    char *name, *meaning;
} pm_meaning[] = {
    { "ts",            NC_("x86-flag", /*/flag:pm:ts*/  "temperature sensor")     },
    { "fid",           NC_("x86-flag", /*/flag:pm:fid*/  "frequency id control")   },
    { "vid",           NC_("x86-flag", /*/flag:pm:vid*/  "voltage id control")     },
    { "ttp",           NC_("x86-flag", /*/flag:pm:ttp*/  "thermal trip")           },
    { "tm",            NC_("x86-flag", /*/flag:pm:tm*/  "hardware thermal control")   },
    { "stc",           NC_("x86-flag", /*/flag:pm:stc*/  "software thermal control")   },
    { "100mhzsteps",   NC_("x86-flag", /*/flag:pm:100mhzsteps*/  "100 MHz multiplier control") },
    { "hwpstate",      NC_("x86-flag", /*/flag:pm:hwpstate*/  "hardware P-state control")   },
    { "cpb",           NC_("x86-flag", /*/flag:pm:cpb*/  "core performance boost")     },
    { "eff_freq_ro",   NC_("x86-flag", /*/flag:pm:eff_freq_ro*/  "Readonly aperf/mperf")       },
    { "proc_feedback", NC_("x86-flag", /*/flag:pm:proc_feedback*/  "processor feedback interface") },
    { "acc_power",     NC_("x86-flag", /*/flag:pm:acc_power*/  "accumulated power mechanism")  },
    { NULL, NULL }
};

GHashTable *cpu_flags = NULL;

static void
populate_cpu_flags_list_internal()
{
    int i;

    DEBUG("using internal CPU flags database");

    for (i = 0; flag_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, flag_meaning[i].name,
                            C_("x86-flag", flag_meaning[i].meaning) );
    }
    for (i = 0; bug_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, bug_meaning[i].name,
                            C_("x86-flag", bug_meaning[i].meaning) );
    }
    for (i = 0; pm_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, pm_meaning[i].name,
                            C_("x86-flag", pm_meaning[i].meaning) );
    }
}

void cpu_flags_init(void)
{
    gint i;
    gchar *path;

    cpu_flags = g_hash_table_new(g_str_hash, g_str_equal);

    path = g_build_filename(g_get_home_dir(), ".hardinfo", "cpuflags.conf", NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        populate_cpu_flags_list_internal();
    } else {
        GKeyFile *flags_file;

        DEBUG("using %s as CPU flags database", path);

        flags_file = g_key_file_new();
        if (g_key_file_load_from_file(flags_file, path, 0, NULL)) {
            gchar **flag_keys;

            flag_keys = g_key_file_get_keys(flags_file, "flags",
                                            NULL, NULL);
            if (!flag_keys) {
                DEBUG("error while using %s as CPU flags database, falling back to internal",
                      path);
                populate_cpu_flags_list_internal();
            } else {
                for (i = 0; flag_keys[i]; i++) {
                    gchar *meaning;

                    meaning = g_key_file_get_string(flags_file, "flags",
                                                    flag_keys[i], NULL);

                    g_hash_table_insert(cpu_flags, g_strdup(flag_keys[i]), meaning);

                    /* can't free meaning */
                }

                g_strfreev(flag_keys);
            }
        }

        g_key_file_free(flags_file);
    }

    g_free(path);
}

gchar *processor_get_capabilities_from_flags(gchar * strflags)
{
    /* FIXME:
     * - Separate between processor capabilities, additional instructions and whatnot.
     */
    gchar **flags, **old;
    gchar *tmp = NULL;
    gint j = 0;

    if (!cpu_flags) {
        cpu_flags_init();
    }

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

gchar *processor_get_detailed_info(Processor * processor)
{
    gchar *tmp_flags, *tmp_bugs, *tmp_pm, *tmp_cpufreq, *tmp_topology, *ret, *cache_info;

    tmp_flags = processor_get_capabilities_from_flags(processor->flags);
    tmp_bugs = processor_get_capabilities_from_flags(processor->bugs);
    tmp_pm = processor_get_capabilities_from_flags(processor->pm);
    cache_info = __cache_get_info_as_string(processor);

    tmp_topology = cputopo_section_str(processor->cputopo);
    tmp_cpufreq = cpufreq_section_str(processor->cpufreq);

    ret = g_strdup_printf("[%s]\n"
                       "%s=%s\n"
                       "%s=%d, %d, %d (%s)\n" /* family, model, stepping (decoded name) */
                       "%s=%s\n"      /* vendor */
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
