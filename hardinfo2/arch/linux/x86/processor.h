/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

/*
 * This function is partly based on x86cpucaps
 * by Osamu Kayasono <jacobi@jcom.home.ne.jp>
 */   
static void
get_processor_strfamily(Processor *processor)
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
                processor->strmodel = g_strdup("Pentium III/Pentium III Xeon/Celeron");
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
                processor->strmodel = g_strdup("AMD Athlon (K7");
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

static Processor *
computer_get_processor(void)
{
    Processor *processor;
    FILE *cpuinfo;
    gchar buffer[128];

    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo)
	return NULL;

    processor = g_new0(Processor, 1);
    while (fgets(buffer, 128, cpuinfo)) {
	gchar **tmp = g_strsplit(buffer, ":", 2);

	if (tmp[0] && tmp[1]) {
	    tmp[0] = g_strstrip(tmp[0]);
	    tmp[1] = g_strstrip(tmp[1]);

	    get_str("model name", processor->model_name);
	    get_str("vendor_id", processor->vendor_id);
	    get_str("flags", processor->flags);
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

    get_processor_strfamily(processor);

    fclose(cpuinfo);

    return processor;
}

static struct {
    char *name, *meaning;
} flag_meaning[] = {
    { "3dnow",		"3DNow! Technology" },
    { "3dnowext",	"Extended 3DNow! Technology" },
    { "fpu",		"Floating Point Unit" },
    { "vme",		"Virtual 86 Mode Extension" },
    { "de",		"Debug Extensions - I/O breakpoints" },
    { "pse",		"Page Size Extensions (4MB pages)" },
    { "tsc",		"Time Stamp Counter and RDTSC instruction" },
    { "msr",		"Model Specific Registers" },
    { "pae",		"Physical Address Extensions (36-bit address, 2MB pages)" },
    { "mce",		"Machine Check Architeture" },
    { "cx8",		"CMPXCHG8 instruction" },
    { "apic",		"Advanced Programmable Interrupt Controller" },
    { "sep",		"Fast System Call (SYSENTER/SYSEXIT instructions)" },
    { "mtrr",		"Memory Type Range Registers" },
    { "pge",		"Page Global Enable" },
    { "mca",		"Machine Check Architecture" },
    { "cmov",		"Conditional Move instruction" },
    { "pat",		"Page Attribute Table" },
    { "pse36",		"36bit Page Size Extensions" },
    { "psn",		"96 bit Processor Serial Number" },
    { "mmx",		"MMX technology" },
    { "mmxext",		"Extended MMX Technology" },
    { "cflush",		"Cache Flush" },
    { "dtes",		"Debug Trace Store" },
    { "fxsr",		"FXSAVE and FXRSTOR instructions" },
    { "kni",		"Streaming SIMD instructions" },
    { "xmm",		"Streaming SIMD instructions" },
    { "ht",		"HyperThreading" },
    { "mp",		"Multiprocessing Capable" },
    { "sse",		"SSE instructions" },
    { "sse2",		"SSE2 (WNI) instructions" },
    { "acc",		"Automatic Clock Control" },
    { "ia64",		"IA64 Instructions" },
    { "syscall",	"SYSCALL and SYSEXIT instructions" },
    { "nx",		"No-execute Page Protection" },
    { "xd",		"Execute Disable" },
    { "clflush",	"Cache Line Flush instruction" },
    { "acpi",		"Thermal Monitor and Software Controlled Clock Facilities" },
    { "dts",		"Debug Store" },
    { "ss",		"Self Snoop" },
    { "tm",		"Thermal Monitor" },
    { "pbe",		"Pending Break Enable" },
    { "pb",		"Pending Break Enable" },
    { NULL, NULL}
};

gchar *
processor_get_capabilities_from_flags(gchar * strflags)
{
    /* FIXME: * Separate between processor capabilities, additional instructions and whatnot.  */
    gchar **flags, **old;
    gchar *tmp = "";
    gint i;

    flags = g_strsplit(strflags, " ", 0);
    old = flags;

    while (*flags) {
	gchar *meaning = "";
	for (i = 0; flag_meaning[i].name != NULL; i++) {
	    if (!strcmp(*flags, flag_meaning[i].name)) {
		meaning = flag_meaning[i].meaning;
		break;
	    }
	}

	tmp = g_strdup_printf("%s%s=%s\n", tmp, *flags, meaning);
	*flags++;
    }

    g_strfreev(old);
    return tmp;
}

static gchar *
processor_get_info(Processor *processor)
{
	gchar *tmp = processor_get_capabilities_from_flags(processor->
						  flags);
	gchar *ret = g_strdup_printf("[Processor]\n"
	                       "Name=%s\n"
	                       "Specification=%s\n"
                               "Family, model, stepping=%d, %d, %d\n"
			       "Vendor=%s\n"
			       "Cache Size=%dkb\n"
			       "Frequency=%.2fMHz\n"
			       "BogoMips=%.2f\n"
			       "Byte Order=%s\n"
			       "[Features]\n"
			       "FDIV Bug=%s\n"
			       "HLT Bug=%s\n"
			       "F00F Bug=%s\n"
			       "Coma Bug=%s\n"
			       "Has FPU=%s\n"
			       "[Capabilities]\n" "%s",
			       processor->strmodel,
			       processor->model_name,
			       processor->family,
			       processor->model,
			       processor->stepping,
			       processor->vendor_id,
			       processor->cache_size,
			       processor->cpu_mhz,
			       processor->bogomips,
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                               "Little Endian",
#else
                               "Big Endian",
#endif
			       processor->bug_fdiv,
			       processor->bug_hlt,
			       processor->bug_f00f,
			       processor->bug_coma,
			       processor->has_fpu,
			       tmp);
      g_free(tmp);
      return ret;
}
