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
    { "fpu",	N_("Onboard FPU (floating point support)") },
    { "vme",	N_("Virtual 8086 mode enhancements") },
    { "de", 	N_("Debugging Extensions (CR4.DE)") },
    { "pse",	N_("Page Size Extensions (4MB memory pages)") },
    { "tsc",	N_("Time Stamp Counter (RDTSC)") },
    { "msr",	N_("Model-Specific Registers (RDMSR, WRMSR)") },
    { "pae",	N_("Physical Address Extensions (support for more than 4GB of RAM)") },
    { "mce",	N_("Machine Check Exception") },
    { "cx8",	N_("CMPXCHG8 instruction (64-bit compare-and-swap)") },
    { "apic",	N_("Onboard APIC") },
    { "sep",	N_("SYSENTER/SYSEXIT") },
    { "mtrr",	N_("Memory Type Range Registers") },
    { "pge",	N_("Page Global Enable (global bit in PDEs and PTEs)") },
    { "mca",	N_("Machine Check Architecture") },
    { "cmov",	N_("CMOV instructions (conditional move) (also FCMOV)") },
    { "pat",	N_("Page Attribute Table") },
    { "pse36",	N_("36-bit PSEs (huge pages)") },
    { "pn", 	N_("Processor serial number") },
    { "clflush",	N_("Cache Line Flush instruction") },
    { "dts",	N_("Debug Store (buffer for debugging and profiling instructions)") },
    { "acpi",	N_("ACPI via MSR (temperature monitoring and clock speed modulation)") },
    { "mmx",	N_("Multimedia Extensions") },
    { "fxsr",	N_("FXSAVE/FXRSTOR, CR4.OSFXSR") },
    { "sse",	N_("Intel SSE vector instructions") },
    { "sse2",	N_("SSE2") },
    { "ss", 	N_("CPU self snoop") },
    { "ht", 	N_("Hyper-Threading") },
    { "tm", 	N_("Automatic clock control (Thermal Monitor)") },
    { "ia64",	N_("Intel Itanium Architecture 64-bit (not to be confused with Intel's 64-bit x86 architecture with flag x86-64 or \"AMD64\" bit indicated by flag lm)") },
    { "pbe",	N_("Pending Break Enable (PBE# pin) wakeup support") },
/* AMD-defined CPU features, CPUID level 0x80000001
 * See also Wikipedia and table 2-23 in Intel Advanced Vector Extensions Programming Reference */
    { "syscall",	N_("SYSCALL (Fast System Call) and SYSRET (Return From Fast System Call)") },
    { "mp", 	N_("Multiprocessing Capable.") },
    { "nx", 	N_("Execute Disable") },
    { "mmxext",	N_("AMD MMX extensions") },
    { "fxsr_opt",	N_("FXSAVE/FXRSTOR optimizations") },
    { "pdpe1gb",	N_("One GB pages (allows hugepagesz=1G)") },
    { "rdtscp",	N_("Read Time-Stamp Counter and Processor ID") },
    { "lm", 	N_("Long Mode (x86-64: amd64, also known as Intel 64, i.e. 64-bit capable)") },
    { "3dnow",	N_("3DNow! (AMD vector instructions, competing with Intel's SSE1)") },
    { "3dnowext",	N_("AMD 3DNow! extensions") },
/* Transmeta-defined CPU features, CPUID level 0x80860001 */
    { "recovery",	N_("CPU in recovery mode") },
    { "longrun",	N_("Longrun power control") },
    { "lrti",	N_("LongRun table interface") },
/* Other features, Linux-defined mapping */
    { "cxmmx",	N_("Cyrix MMX extensions") },
    { "k6_mtrr",	N_("AMD K6 nonstandard MTRRs") },
    { "cyrix_arr",	N_("Cyrix ARRs (= MTRRs)") },
    { "centaur_mcr",	N_("Centaur MCRs (= MTRRs)") },
    { "constant_tsc",	N_("TSC ticks at a constant rate") },
    { "up", 	N_("SMP kernel running on UP") },
    { "art",	N_("Always-Running Timer") },
    { "arch_perfmon",	N_("Intel Architectural PerfMon") },
    { "pebs",	N_("Precise-Event Based Sampling") },
    { "bts",	N_("Branch Trace Store") },
    { "rep_good",	N_("rep microcode works well") },
    { "acc_power",	N_("AMD accumulated power mechanism") },
    { "nopl",	N_("The NOPL (0F 1F) instructions") },
    { "xtopology",	N_("cpu topology enum extensions") },
    { "tsc_reliable",	N_("TSC is known to be reliable") },
    { "nonstop_tsc",	N_("TSC does not stop in C states") },
    { "extd_apicid",	N_("has extended APICID (8 bits)") },
    { "amd_dcm",	N_("multi-node processor") },
    { "aperfmperf",	N_("APERFMPERF") },
    { "eagerfpu",	N_("Non lazy FPU restore") },
    { "nonstop_tsc_s3",	N_("TSC doesn't stop in S3 state") },
    { "mce_recovery",	N_("CPU has recoverable machine checks") },
/* Intel-defined CPU features, CPUID level 0x00000001 (ecx)
 * See also Wikipedia and table 2-26 in Intel Advanced Vector Extensions Programming Reference */
    { "pni",	N_("SSE-3 (“Prescott New Instructions”)") },
    { "pclmulqdq",	N_("Perform a Carry-Less Multiplication of Quadword instruction — accelerator for GCM)") },
    { "dtes64",	N_("64-bit Debug Store") },
    { "monitor",	N_("Monitor/Mwait support (Intel SSE3 supplements)") },
    { "ds_cpl",	N_("CPL Qual. Debug Store") },
    { "vmx: Hardware virtualization",	N_("Intel VMX") },
    { "smx: Safer mode",	N_("TXT (TPM support)") },
    { "est",	N_("Enhanced SpeedStep") },
    { "tm2",	N_("Thermal Monitor 2") },
    { "ssse3",	N_("Supplemental SSE-3") },
    { "cid",	N_("Context ID") },
    { "sdbg",	N_("silicon debug") },
    { "fma",	N_("Fused multiply-add") },
    { "cx16",	N_("CMPXCHG16B") },
    { "xtpr",	N_("Send Task Priority Messages") },
    { "pdcm",	N_("Performance Capabilities") },
    { "pcid",	N_("Process Context Identifiers") },
    { "dca",	N_("Direct Cache Access") },
    { "sse4_1",	N_("SSE-4.1") },
    { "sse4_2",	N_("SSE-4.2") },
    { "x2apic",	N_("x2APIC") },
    { "movbe",	N_("Move Data After Swapping Bytes instruction") },
    { "popcnt",	N_("Return the Count of Number of Bits Set to 1 instruction (Hamming weight, i.e. bit count)") },
    { "tsc_deadline_timer",	N_("Tsc deadline timer") },
    { "aes/aes-ni",	N_("Advanced Encryption Standard (New Instructions)") },
    { "xsave",	N_("Save Processor Extended States: also provides XGETBY,XRSTOR,XSETBY") },
    { "avx",	N_("Advanced Vector Extensions") },
    { "f16c",	N_("16-bit fp conversions (CVT16)") },
    { "rdrand",	N_("Read Random Number from hardware random number generator instruction") },
    { "hypervisor",	N_("Running on a hypervisor") },
/* VIA/Cyrix/Centaur-defined CPU features, CPUID level 0xC0000001 */
    { "rng",	N_("Random Number Generator present (xstore)") },
    { "rng_en",	N_("Random Number Generator enabled") },
    { "ace",	N_("on-CPU crypto (xcrypt)") },
    { "ace_en",	N_("on-CPU crypto enabled") },
    { "ace2",	N_("Advanced Cryptography Engine v2") },
    { "ace2_en",	N_("ACE v2 enabled") },
    { "phe",	N_("PadLock Hash Engine") },
    { "phe_en",	N_("PHE enabled") },
    { "pmm",	N_("PadLock Montgomery Multiplier") },
    { "pmm_en",	N_("PMM enabled") },
/* More extended AMD flags: CPUID level 0x80000001, ecx */
    { "lahf_lm",	N_("Load AH from Flags (LAHF) and Store AH into Flags (SAHF) in long mode") },
    { "cmp_legacy",	N_("If yes HyperThreading not valid") },
    { "svm",	N_("\"Secure virtual machine\": AMD-V") },
    { "extapic",	N_("Extended APIC space") },
    { "cr8_legacy",	N_("CR8 in 32-bit mode") },
    { "abm",	N_("Advanced Bit Manipulation") },
    { "sse4a",	N_("SSE-4A") },
    { "misalignsse",	N_("indicates if a general-protection exception (#GP) is generated when some legacy SSE instructions operate on unaligned data. Also depends on CR0 and Alignment Checking bit") },
    { "3dnowprefetch",	N_("3DNow prefetch instructions") },
    { "osvw",	N_("indicates OS Visible Workaround, which allows the OS to work around processor errata.") },
    { "ibs",	N_("Instruction Based Sampling") },
    { "xop",	N_("extended AVX instructions") },
    { "skinit",	N_("SKINIT/STGI instructions") },
    { "wdt",	N_("Watchdog timer") },
    { "lwp",	N_("Light Weight Profiling") },
    { "fma4",	N_("4 operands MAC instructions") },
    { "tce",	N_("translation cache extension") },
    { "nodeid_msr",	N_("NodeId MSR") },
    { "tbm",	N_("Trailing Bit Manipulation") },
    { "topoext",	N_("Topology Extensions CPUID leafs") },
    { "perfctr_core",	N_("Core Performance Counter Extensions") },
    { "perfctr_nb",	N_("NB Performance Counter Extensions") },
    { "bpext",	N_("data breakpoint extension") },
    { "ptsc",	N_("performance time-stamp counter") },
    { "perfctr_l2",	N_("L2 Performance Counter Extensions") },
    { "mwaitx",	N_("MWAIT extension (MONITORX/MWAITX)") },
/* Auxiliary flags: Linux defined - For features scattered in various CPUID levels */
    { "cpb",	N_("AMD Core Performance Boost") },
    { "epb",	N_("IA32_ENERGY_PERF_BIAS support") },
    { "hw_pstate",	N_("AMD HW-PState") },
    { "proc_feedback",	N_("AMD ProcFeedbackInterface") },
    { "intel_pt",	N_("Intel Processor Tracing") },
/* Virtualization flags: Linux defined */
    { "tpr_shadow",	N_("Intel TPR Shadow") },
    { "vnmi",	N_("Intel Virtual NMI") },
    { "flexpriority",	N_("Intel FlexPriority") },
    { "ept",	N_("Intel Extended Page Table") },
    { "vpid",	N_("Intel Virtual Processor ID") },
    { "vmmcall",	N_("prefer VMMCALL to VMCALL") },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ebx) */
    { "fsgsbase",	N_("{RD/WR}{FS/GS}BASE instructions") },
    { "tsc_adjust",	N_("TSC adjustment MSR") },
    { "bmi1",	N_("1st group bit manipulation extensions") },
    { "hle",	N_("Hardware Lock Elision") },
    { "avx2",	N_("AVX2 instructions") },
    { "smep",	N_("Supervisor Mode Execution Protection") },
    { "bmi2",	N_("2nd group bit manipulation extensions") },
    { "erms",	N_("Enhanced REP MOVSB/STOSB") },
    { "invpcid",	N_("Invalidate Processor Context ID") },
    { "rtm",	N_("Restricted Transactional Memory") },
    { "cqm",	N_("Cache QoS Monitoring") },
    { "mpx",	N_("Memory Protection Extension") },
    { "avx512f",	N_("AVX-512 foundation") },
    { "avx512dq",	N_("AVX-512 Double/Quad instructions") },
    { "rdseed",	N_("The RDSEED instruction") },
    { "adx",	N_("The ADCX and ADOX instructions") },
    { "smap",	N_("Supervisor Mode Access Prevention") },
    { "clflushopt",	N_("CLFLUSHOPT instruction") },
    { "clwb",	N_("CLWB instruction") },
    { "avx512pf",	N_("AVX-512 Prefetch") },
    { "avx512er",	N_("AVX-512 Exponential and Reciprocal") },
    { "avx512cd",	N_("AVX-512 Conflict Detection") },
    { "sha_ni",	N_("SHA1/SHA256 Instruction Extensions") },
    { "avx512bw",	N_("AVX-512 Byte/Word instructions") },
    { "avx512vl",	N_("AVX-512 128/256 Vector Length extensions") },
/* Extended state features, CPUID level 0x0000000d:1 (eax) */
    { "xsaveopt",	N_("Optimized XSAVE") },
    { "xsavec",	N_("XSAVEC") },
    { "xgetbv1",	N_("XGETBV with ECX = 1") },
    { "xsaves",	N_("XSAVES/XRSTORS") },
/* Intel-defined CPU QoS sub-leaf, CPUID level 0x0000000F:0 (edx) */
    { "cqm_llc",	N_("LLC QoS") },
/* Intel-defined CPU QoS sub-leaf, CPUID level 0x0000000F:1 (edx) */
    { "cqm_occup_llc",	N_("LLC occupancy monitoring") },
    { "cqm_mbm_total",	N_("LLC total MBM monitoring") },
    { "cqm_mbm_local",	N_("LLC local MBM monitoring") },
/* AMD-defined CPU features, CPUID level 0x80000008 (ebx) */
    { "clzero",	N_("CLZERO instruction") },
    { "irperf",	N_("instructions retired performance counter") },
/* Thermal and Power Management leaf, CPUID level 0x00000006 (eax) */
    { "dtherm (formerly dts)",	N_("digital thermal sensor") },
    { "ida",	N_("Intel Dynamic Acceleration") },
    { "arat",	N_("Always Running APIC Timer") },
    { "pln",	N_("Intel Power Limit Notification") },
    { "pts",	N_("Intel Package Thermal Status") },
    { "hwp",	N_("Intel Hardware P-states") },
    { "hwp_notify",	N_("HWP notification") },
    { "hwp_act_window",	N_("HWP Activity Window") },
    { "hwp_epp",	N_("HWP Energy Performance Preference") },
    { "hwp_pkg_req",	N_("HWP package-level request") },
/* AMD SVM Feature Identification, CPUID level 0x8000000a (edx) */
    { "npt",	N_("AMD Nested Page Table support") },
    { "lbrv",	N_("AMD LBR Virtualization support") },
    { "svm_lock",	N_("AMD SVM locking MSR") },
    { "nrip_save",	N_("AMD SVM next_rip save") },
    { "tsc_scale",	N_("AMD TSC scaling support") },
    { "vmcb_clean",	N_("AMD VMCB clean bits support") },
    { "flushbyasid",	N_("AMD flush-by-ASID support") },
    { "decodeassists",	N_("AMD Decode Assists support") },
    { "pausefilter",	N_("AMD filtered pause intercept") },
    { "pfthreshold",	N_("AMD pause filter threshold") },
    { "avic",	N_("Virtual Interrupt Controller") },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ecx) */
    { "pku",	N_("Protection Keys for Userspace") },
    { "ospke",	N_("OS Protection Keys Enable") },
/* AMD-defined CPU features, CPUID level 0x80000007 (ebx) */
    { "overflow_recov",	N_("MCA overflow recovery support") },
    { "succor",	N_("uncorrectable error containment and recovery") },
    { "smca",	N_("Scalable MCA") },
    { NULL, NULL},
};

static struct {
    char *name, *meaning;
} bug_meaning[] = {
	{ "f00f",        N_("Intel F00F bug")    },
	{ "fdiv",        N_("FPU FDIV")          },
	{ "coma",        N_("Cyrix 6x86 coma")   },
	{ "tlb_mmatch",  N_("AMD Erratum 383")   },
	{ "apic_c1e",    N_("AMD Erratum 400")   },
	{ "11ap",        N_("Bad local APIC aka 11AP")  },
	{ "fxsave_leak", N_("FXSAVE leaks FOP/FIP/FOP") },
	{ "clflush_monitor",  N_("AAI65, CLFLUSH required before MONITOR") },
	{ "sysret_ss_attrs",  N_("SYSRET doesn't fix up SS attrs") },
	{ "espfix",      N_("IRET to 16-bit SS corrupts ESP/RSP high bits") },
	{ "null_seg",    N_("Nulling a selector preserves the base") },         /* see: detect_null_seg_behavior() */
	{ "swapgs_fence", N_("SWAPGS without input dep on GS") },
	{ "monitor",     N_("IPI required to wake up remote CPU") },
	{ "amd_e400",    N_("AMD Erratum 400") },
	{ NULL,		NULL },
};

/* from arch/x86/kernel/cpu/powerflags.h */
static struct {
    char *name, *meaning;
} pm_meaning[] = {
	{ "ts",            N_("temperature sensor")     },
	{ "fid",           N_("frequency id control")   },
	{ "vid",           N_("voltage id control")     },
	{ "ttp",           N_("thermal trip")           },
	{ "tm",            N_("hardware thermal control")   },
	{ "stc",           N_("software thermal control")   },
	{ "100mhzsteps",   N_("100 MHz multiplier control") },
	{ "hwpstate",      N_("hardware P-state control")   },
/*	{ "",              N_("tsc invariant mapped to constant_tsc") }, */
	{ "cpb",           N_("core performance boost")     },
	{ "eff_freq_ro",   N_("Readonly aperf/mperf")       },
	{ "proc_feedback", N_("processor feedback interface") },
	{ "acc_power",     N_("accumulated power mechanism")  },
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
                            _(flag_meaning[i].meaning) );
    }
    for (i = 0; bug_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, bug_meaning[i].name,
                            _(bug_meaning[i].meaning) );
    }
    for (i = 0; pm_meaning[i].name != NULL; i++) {
        g_hash_table_insert(cpu_flags, pm_meaning[i].name,
                            _(pm_meaning[i].meaning) );
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
