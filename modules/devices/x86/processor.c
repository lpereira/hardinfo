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

        result = h_strdup_cprintf("Level %d (%s)=%d-way set-associative, %d sets, %dKB size\n",
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

static gchar* get_cpu_str(const gchar* file, gint cpuid) {
    gchar *tmp0 = NULL;
    gchar *tmp1 = NULL;
    tmp0 = g_strdup_printf("/sys/devices/system/cpu/cpu%d/%s", cpuid, file);
    g_file_get_contents(tmp0, &tmp1, NULL, NULL);
    g_free(tmp0);
    return tmp1;
}

static gint get_cpu_int(const char* item, int cpuid) {
    gchar *fc = NULL;
    int ret = 0;
    fc = get_cpu_str(item, cpuid);
    if (fc) {
        ret = atol(fc);
        g_free(fc);
    }
    return ret;
}

GSList *processor_scan(void)
{
    GSList *procs = NULL, *l = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[512];

    cpuinfo = fopen("/proc/cpuinfo", "r");
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

#define STRIFNULL(f,cs) if (processor->f == NULL) processor->f = g_strdup(cs);
#define UNKIFNULL(f) STRIFNULL(f, _("(Unknown)") )
#define EMPIFNULL(f) STRIFNULL(f, "")

        /* topo */
        processor->package_id = get_cpu_str("topology/physical_package_id", processor->id);
        processor->core_id = get_cpu_str("topology/core_id", processor->id);
        UNKIFNULL(package_id);
        UNKIFNULL(core_id);

        /* freq */
        processor->scaling_driver = get_cpu_str("cpufreq/scaling_driver", processor->id);
        processor->scaling_governor = get_cpu_str("cpufreq/scaling_governor", processor->id);
        UNKIFNULL(scaling_driver);
        UNKIFNULL(scaling_governor);
        processor->transition_latency = get_cpu_int("cpufreq/cpuinfo_transition_latency", processor->id);
        processor->cpukhz_cur = get_cpu_int("cpufreq/scaling_cur_freq", processor->id);
        processor->cpukhz_min = get_cpu_int("cpufreq/scaling_min_freq", processor->id);
        processor->cpukhz_max = get_cpu_int("cpufreq/scaling_max_freq", processor->id);
        if (processor->cpukhz_max)
            processor->cpu_mhz = processor->cpukhz_max / 1000;
    }

    fclose(cpuinfo);

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
    { "fpu",	 "Onboard FPU (floating point support)" },
    { "vme",	 "Virtual 8086 mode enhancements" },
    { "de", 	 "Debugging Extensions (CR4.DE)" },
    { "pse",	 "Page Size Extensions (4MB memory pages)" },
    { "tsc",	 "Time Stamp Counter (RDTSC)" },
    { "msr",	 "Model-Specific Registers (RDMSR, WRMSR)" },
    { "pae",	 "Physical Address Extensions (support for more than 4GB of RAM)" },
    { "mce",	 "Machine Check Exception" },
    { "cx8",	 "CMPXCHG8 instruction (64-bit compare-and-swap)" },
    { "apic",	 "Onboard APIC" },
    { "sep",	 "SYSENTER/SYSEXIT" },
    { "mtrr",	 "Memory Type Range Registers" },
    { "pge",	 "Page Global Enable (global bit in PDEs and PTEs)" },
    { "mca",	 "Machine Check Architecture" },
    { "cmov",	 "CMOV instructions (conditional move) (also FCMOV)" },
    { "pat",	 "Page Attribute Table" },
    { "pse36",	 "36-bit PSEs (huge pages)" },
    { "pn", 	 "Processor serial number" },
    { "clflush",	 "Cache Line Flush instruction" },
    { "dts",	 "Debug Store (buffer for debugging and profiling instructions)" },
    { "acpi",	 "ACPI via MSR (temperature monitoring and clock speed modulation)" },
    { "mmx",	 "Multimedia Extensions" },
    { "fxsr",	 "FXSAVE/FXRSTOR, CR4.OSFXSR" },
    { "sse",	 "Intel SSE vector instructions" },
    { "sse2",	 "SSE2" },
    { "ss", 	 "CPU self snoop" },
    { "ht", 	 "Hyper-Threading" },
    { "tm", 	 "Automatic clock control (Thermal Monitor)" },
    { "ia64",	 "Intel Itanium Architecture 64-bit (not to be confused with Intel's 64-bit x86 architecture with flag x86-64 or \"AMD64\" bit indicated by flag lm)" },
    { "pbe",	 "Pending Break Enable (PBE# pin) wakeup support" },
/* AMD-defined CPU features, CPUID level 0x80000001
 * See also Wikipedia and table 2-23 in Intel Advanced Vector Extensions Programming Reference */
    { "syscall",	 "SYSCALL (Fast System Call) and SYSRET (Return From Fast System Call)" },
    { "mp", 	 "Multiprocessing Capable." },
    { "nx", 	 "Execute Disable" },
    { "mmxext",	 "AMD MMX extensions" },
    { "fxsr_opt",	 "FXSAVE/FXRSTOR optimizations" },
    { "pdpe1gb",	 "One GB pages (allows hugepagesz=1G)" },
    { "rdtscp",	 "Read Time-Stamp Counter and Processor ID" },
    { "lm", 	 "Long Mode (x86-64: amd64, also known as Intel 64, i.e. 64-bit capable)" },
    { "3dnow",	 "3DNow! (AMD vector instructions, competing with Intel's SSE1)" },
    { "3dnowext",	 "AMD 3DNow! extensions" },
/* Transmeta-defined CPU features, CPUID level 0x80860001 */
    { "recovery",	 "CPU in recovery mode" },
    { "longrun",	 "Longrun power control" },
    { "lrti",	 "LongRun table interface" },
/* Other features, Linux-defined mapping */
    { "cxmmx",	 "Cyrix MMX extensions" },
    { "k6_mtrr",	 "AMD K6 nonstandard MTRRs" },
    { "cyrix_arr",	 "Cyrix ARRs (= MTRRs)" },
    { "centaur_mcr",	 "Centaur MCRs (= MTRRs)" },
    { "constant_tsc",	 "TSC ticks at a constant rate" },
    { "up", 	 "SMP kernel running on UP" },
    { "art",	 "Always-Running Timer" },
    { "arch_perfmon",	 "Intel Architectural PerfMon" },
    { "pebs",	 "Precise-Event Based Sampling" },
    { "bts",	 "Branch Trace Store" },
    { "rep_good",	 "rep microcode works well" },
    { "acc_power",	 "AMD accumulated power mechanism" },
    { "nopl",	 "The NOPL (0F 1F) instructions" },
    { "xtopology",	 "cpu topology enum extensions" },
    { "tsc_reliable",	 "TSC is known to be reliable" },
    { "nonstop_tsc",	 "TSC does not stop in C states" },
    { "extd_apicid",	 "has extended APICID (8 bits)" },
    { "amd_dcm",	 "multi-node processor" },
    { "aperfmperf",	 "APERFMPERF" },
    { "eagerfpu",	 "Non lazy FPU restore" },
    { "nonstop_tsc_s3",	 "TSC doesn't stop in S3 state" },
    { "mce_recovery",	 "CPU has recoverable machine checks" },
/* Intel-defined CPU features, CPUID level 0x00000001 (ecx)
 * See also Wikipedia and table 2-26 in Intel Advanced Vector Extensions Programming Reference */
    { "pni",	 "SSE-3 (“Prescott New Instructions”)" },
    { "pclmulqdq",	 "Perform a Carry-Less Multiplication of Quadword instruction — accelerator for GCM)" },
    { "dtes64",	 "64-bit Debug Store" },
    { "monitor",	 "Monitor/Mwait support (Intel SSE3 supplements)" },
    { "ds_cpl",	 "CPL Qual. Debug Store" },
    { "vmx: Hardware virtualization",	 "Intel VMX" },
    { "smx: Safer mode",	 "TXT (TPM support)" },
    { "est",	 "Enhanced SpeedStep" },
    { "tm2",	 "Thermal Monitor 2" },
    { "ssse3",	 "Supplemental SSE-3" },
    { "cid",	 "Context ID" },
    { "sdbg",	 "silicon debug" },
    { "fma",	 "Fused multiply-add" },
    { "cx16",	 "CMPXCHG16B" },
    { "xtpr",	 "Send Task Priority Messages" },
    { "pdcm",	 "Performance Capabilities" },
    { "pcid",	 "Process Context Identifiers" },
    { "dca",	 "Direct Cache Access" },
    { "sse4_1",	 "SSE-4.1" },
    { "sse4_2",	 "SSE-4.2" },
    { "x2apic",	 "x2APIC" },
    { "movbe",	 "Move Data After Swapping Bytes instruction" },
    { "popcnt",	 "Return the Count of Number of Bits Set to 1 instruction (Hamming weight, i.e. bit count)" },
    { "tsc_deadline_timer",	 "Tsc deadline timer" },
    { "aes/aes-ni",	 "Advanced Encryption Standard (New Instructions)" },
    { "xsave",	 "Save Processor Extended States: also provides XGETBY,XRSTOR,XSETBY" },
    { "avx",	 "Advanced Vector Extensions" },
    { "f16c",	 "16-bit fp conversions (CVT16)" },
    { "rdrand",	 "Read Random Number from hardware random number generator instruction" },
    { "hypervisor",	 "Running on a hypervisor" },
/* VIA/Cyrix/Centaur-defined CPU features, CPUID level 0xC0000001 */
    { "rng",	 "Random Number Generator present (xstore)" },
    { "rng_en",	 "Random Number Generator enabled" },
    { "ace",	 "on-CPU crypto (xcrypt)" },
    { "ace_en",	 "on-CPU crypto enabled" },
    { "ace2",	 "Advanced Cryptography Engine v2" },
    { "ace2_en",	 "ACE v2 enabled" },
    { "phe",	 "PadLock Hash Engine" },
    { "phe_en",	 "PHE enabled" },
    { "pmm",	 "PadLock Montgomery Multiplier" },
    { "pmm_en",	 "PMM enabled" },
/* More extended AMD flags: CPUID level 0x80000001, ecx */
    { "lahf_lm",	 "Load AH from Flags (LAHF) and Store AH into Flags (SAHF) in long mode" },
    { "cmp_legacy",	 "If yes HyperThreading not valid" },
    { "svm",	 "\"Secure virtual machine\": AMD-V" },
    { "extapic",	 "Extended APIC space" },
    { "cr8_legacy",	 "CR8 in 32-bit mode" },
    { "abm",	 "Advanced Bit Manipulation" },
    { "sse4a",	 "SSE-4A" },
    { "misalignsse",	 "indicates if a general-protection exception (#GP) is generated when some legacy SSE instructions operate on unaligned data. Also depends on CR0 and Alignment Checking bit" },
    { "3dnowprefetch",	 "3DNow prefetch instructions" },
    { "osvw",	 "indicates OS Visible Workaround, which allows the OS to work around processor errata." },
    { "ibs",	 "Instruction Based Sampling" },
    { "xop",	 "extended AVX instructions" },
    { "skinit",	 "SKINIT/STGI instructions" },
    { "wdt",	 "Watchdog timer" },
    { "lwp",	 "Light Weight Profiling" },
    { "fma4",	 "4 operands MAC instructions" },
    { "tce",	 "translation cache extension" },
    { "nodeid_msr",	 "NodeId MSR" },
    { "tbm",	 "Trailing Bit Manipulation" },
    { "topoext",	 "Topology Extensions CPUID leafs" },
    { "perfctr_core",	 "Core Performance Counter Extensions" },
    { "perfctr_nb",	 "NB Performance Counter Extensions" },
    { "bpext",	 "data breakpoint extension" },
    { "ptsc",	 "performance time-stamp counter" },
    { "perfctr_l2",	 "L2 Performance Counter Extensions" },
    { "mwaitx",	 "MWAIT extension (MONITORX/MWAITX)" },
/* Auxiliary flags: Linux defined - For features scattered in various CPUID levels */
    { "cpb",	 "AMD Core Performance Boost" },
    { "epb",	 "IA32_ENERGY_PERF_BIAS support" },
    { "hw_pstate",	 "AMD HW-PState" },
    { "proc_feedback",	 "AMD ProcFeedbackInterface" },
    { "intel_pt",	 "Intel Processor Tracing" },
/* Virtualization flags: Linux defined */
    { "tpr_shadow",	 "Intel TPR Shadow" },
    { "vnmi",	 "Intel Virtual NMI" },
    { "flexpriority",	 "Intel FlexPriority" },
    { "ept",	 "Intel Extended Page Table" },
    { "vpid",	 "Intel Virtual Processor ID" },
    { "vmmcall",	 "prefer VMMCALL to VMCALL" },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ebx) */
    { "fsgsbase",	 "{RD/WR}{FS/GS}BASE instructions" },
    { "tsc_adjust",	 "TSC adjustment MSR" },
    { "bmi1",	 "1st group bit manipulation extensions" },
    { "hle",	 "Hardware Lock Elision" },
    { "avx2",	 "AVX2 instructions" },
    { "smep",	 "Supervisor Mode Execution Protection" },
    { "bmi2",	 "2nd group bit manipulation extensions" },
    { "erms",	 "Enhanced REP MOVSB/STOSB" },
    { "invpcid",	 "Invalidate Processor Context ID" },
    { "rtm",	 "Restricted Transactional Memory" },
    { "cqm",	 "Cache QoS Monitoring" },
    { "mpx",	 "Memory Protection Extension" },
    { "avx512f",	 "AVX-512 foundation" },
    { "avx512dq",	 "AVX-512 Double/Quad instructions" },
    { "rdseed",	 "The RDSEED instruction" },
    { "adx",	 "The ADCX and ADOX instructions" },
    { "smap",	 "Supervisor Mode Access Prevention" },
    { "clflushopt",	 "CLFLUSHOPT instruction" },
    { "clwb",	 "CLWB instruction" },
    { "avx512pf",	 "AVX-512 Prefetch" },
    { "avx512er",	 "AVX-512 Exponential and Reciprocal" },
    { "avx512cd",	 "AVX-512 Conflict Detection" },
    { "sha_ni",	 "SHA1/SHA256 Instruction Extensions" },
    { "avx512bw",	 "AVX-512 Byte/Word instructions" },
    { "avx512vl",	 "AVX-512 128/256 Vector Length extensions" },
/* Extended state features, CPUID level 0x0000000d:1 (eax) */
    { "xsaveopt",	 "Optimized XSAVE" },
    { "xsavec",	 "XSAVEC" },
    { "xgetbv1",	 "XGETBV with ECX = 1" },
    { "xsaves",	 "XSAVES/XRSTORS" },
/* Intel-defined CPU QoS sub-leaf, CPUID level 0x0000000F:0 (edx) */
    { "cqm_llc",	 "LLC QoS" },
/* Intel-defined CPU QoS sub-leaf, CPUID level 0x0000000F:1 (edx) */
    { "cqm_occup_llc",	 "LLC occupancy monitoring" },
    { "cqm_mbm_total",	 "LLC total MBM monitoring" },
    { "cqm_mbm_local",	 "LLC local MBM monitoring" },
/* AMD-defined CPU features, CPUID level 0x80000008 (ebx) */
    { "clzero",	 "CLZERO instruction" },
    { "irperf",	 "instructions retired performance counter" },
/* Thermal and Power Management leaf, CPUID level 0x00000006 (eax) */
    { "dtherm (formerly dts)",	 "digital thermal sensor" },
    { "ida",	 "Intel Dynamic Acceleration" },
    { "arat",	 "Always Running APIC Timer" },
    { "pln",	 "Intel Power Limit Notification" },
    { "pts",	 "Intel Package Thermal Status" },
    { "hwp",	 "Intel Hardware P-states" },
    { "hwp_notify",	 "HWP notification" },
    { "hwp_act_window",	 "HWP Activity Window" },
    { "hwp_epp",	 "HWP Energy Performance Preference" },
    { "hwp_pkg_req",	 "HWP package-level request" },
/* AMD SVM Feature Identification, CPUID level 0x8000000a (edx) */
    { "npt",	 "AMD Nested Page Table support" },
    { "lbrv",	 "AMD LBR Virtualization support" },
    { "svm_lock",	 "AMD SVM locking MSR" },
    { "nrip_save",	 "AMD SVM next_rip save" },
    { "tsc_scale",	 "AMD TSC scaling support" },
    { "vmcb_clean",	 "AMD VMCB clean bits support" },
    { "flushbyasid",	 "AMD flush-by-ASID support" },
    { "decodeassists",	 "AMD Decode Assists support" },
    { "pausefilter",	 "AMD filtered pause intercept" },
    { "pfthreshold",	 "AMD pause filter threshold" },
    { "avic",	 "Virtual Interrupt Controller" },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ecx) */
    { "pku",	 "Protection Keys for Userspace" },
    { "ospke",	 "OS Protection Keys Enable" },
/* AMD-defined CPU features, CPUID level 0x80000007 (ebx) */
    { "overflow_recov",	 "MCA overflow recovery support" },
    { "succor",	 "uncorrectable error containment and recovery" },
    { "smca",	 "Scalable MCA" },
    { NULL, NULL},
};

static struct {
    char *name, *meaning;
} bug_meaning[] = {
	{ "f00f",        "Intel F00F bug"    },
	{ "fdiv",        "FPU FDIV"          },
	{ "coma",        "Cyrix 6x86 coma"   },
	{ "tlb_mmatch",  "AMD Erratum 383"   },
	{ "apic_c1e",    "AMD Erratum 400"   },
	{ "11ap",        "Bad local APIC aka 11AP"  },
	{ "fxsave_leak", "FXSAVE leaks FOP/FIP/FOP" },
	{ "clflush_monitor",  "AAI65, CLFLUSH required before MONITOR" },
	{ "sysret_ss_attrs",  "SYSRET doesn't fix up SS attrs" },
	{ "espfix",      "IRET to 16-bit SS corrupts ESP/RSP high bits" },
	{ "null_seg",    "Nulling a selector preserves the base" },         /* see: detect_null_seg_behavior() */
	{ "swapgs_fence","SWAPGS without input dep on GS" },
	{ "monitor",     "IPI required to wake up remote CPU" },
	{ "amd_e400",    "AMD Erratum 400" },
	{ NULL,		NULL						},
};

/* from arch/x86/kernel/cpu/powerflags.h */
static struct {
    char *name, *meaning;
} pm_meaning[] = {
	{ "ts",            "temperature sensor"     },
	{ "fid",           "frequency id control"   },
	{ "vid",           "voltage id control"     },
	{ "ttp",           "thermal trip"           },
	{ "tm",            "hardware thermal control"   },
	{ "stc",           "software thermal control"   },
	{ "100mhzsteps",   "100 MHz multiplier control" },
	{ "hwpstate",      "hardware P-state control"   },
/*	{ "",              "tsc invariant mapped to constant_tsc" }, */
	{ "cpb",           "core performance boost"     },
	{ "eff_freq_ro",   "Readonly aperf/mperf"       },
	{ "proc_feedback", "processor feedback interface" },
	{ "acc_power",     "accumulated power mechanism"  },
	{ NULL,		NULL						},
};

GHashTable *cpu_flags = NULL;

static void
populate_cpu_flags_list_internal()
{
    int i;

    DEBUG("using internal CPU flags database");

    for (i = 0; flag_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, flag_meaning[i].name,
                            flag_meaning[i].meaning);
    }
    for (i = 0; bug_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, bug_meaning[i].name,
                            bug_meaning[i].meaning);
    }
    for (i = 0; pm_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, pm_meaning[i].name,
                            pm_meaning[i].meaning);
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

    tmp_topology = g_strdup_printf(
                    "[%s]\n"
                    "%s=%d\n"
                    "%s=%s\n"
                    "%s=%s\n",
                   _("Topology"),
                   _("ID"), processor->id,
                   _("Socket"), processor->package_id,
                   _("Core"), processor->core_id);

    if (processor->cpukhz_min || processor->cpukhz_max || processor->cpukhz_cur) {
        tmp_cpufreq = g_strdup_printf(
                    "[%s]\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%s\n"
                    "%s=%s\n",
                   _("Frequency Scaling"),
                   _("Minimum"), processor->cpukhz_min, _("kHz"),
                   _("Maximum"), processor->cpukhz_max, _("kHz"),
                   _("Current"), processor->cpukhz_cur, _("kHz"),
                   _("Transition Latency"), processor->transition_latency, _("ns"),
                   _("Governor"), processor->scaling_governor,
                   _("Driver"), processor->scaling_driver);
    } else {
        tmp_cpufreq = g_strdup_printf(
                    "[%s]\n"
                    "%s=%s\n",
                   _("Frequency Scaling"),
                   _("Driver"), processor->scaling_driver);
    }

    ret = g_strdup_printf("[%s]\n"
                       "Name=%s\n"
                       "Family, model, stepping=%d, %d, %d (%s)\n"
                       "Vendor=%s\n"
                       "[Configuration]\n"
                       "Cache Size=%dkb\n"
                       "%s=%.2f %s\n" /* frequency */
                       "%s=%.2f\n"    /* bogomips */
                       "%s=%s\n"      /* byte order */
                       "%s" /* topology */
                       "%s" /* frequency scaling */
                       "[Cache]\n"
                       "%s\n"
                       "[%s]\n" /* pm */
                       "%s"
                       "[%s]\n" /* bugs */
                       "%s"
                       "[%s]\n" /* flags */
                       "%s",
                   _("Processor"),
                   processor->model_name,
                   processor->family,
                   processor->model,
                   processor->stepping,
                   processor->strmodel,
                   vendor_get_name(processor->vendor_id),
                   processor->cache_size,
                   _("Frequency"), processor->cpu_mhz, _("MHz"),
                   _("BogoMips"), processor->bogomips,
                   _("Byte Order"),
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                   _("Little Endian"),
#else
                   _("Big Endian"),
#endif
                   tmp_topology,
                   tmp_cpufreq,
                   cache_info,
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
