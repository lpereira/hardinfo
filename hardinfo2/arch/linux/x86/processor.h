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

typedef struct _ProcessorCache	ProcessorCache;

struct _ProcessorCache {
  gint level;
  gint number_of_sets;
  gint physical_line_partition;
  gint size;
  gchar *type;
  gint ways_of_associativity;
};

struct _Processor {
    gchar *model_name;
    gchar *vendor_id;
    gchar *flags;
    gint cache_size;
    gfloat bogomips, cpu_mhz;

    gchar *has_fpu;
    gchar *bug_fdiv, *bug_hlt, *bug_f00f, *bug_coma;
    
    gint model, family, stepping;
    gchar *strmodel;
    
    gint id;
    
    GSList *cache;
};

/*
 * This function is partly based on x86cpucaps
 * by Osamu Kayasono <jacobi@jcom.home.ne.jp>
 */
static void get_processor_strfamily(Processor * processor)
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
        return g_strdup("Cache information not available=\n");
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

static void __cache_obtain_info(Processor *processor, gint processor_number)
{
    ProcessorCache *cache;
    gchar *endpoint, *entry, *index;
    gint i;
    
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

static GSList *__scan_processors(void)
{
    GSList *procs = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[256];
    gint processor_number = 0;

    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo)
	return NULL;

    while (fgets(buffer, 256, cpuinfo)) {
	gchar **tmp = g_strsplit(buffer, ":", 2);

	if (g_str_has_prefix(tmp[0], "processor")) {
	    if (processor) {
		get_processor_strfamily(processor);
		procs = g_slist_append(procs, processor);
	    }

	    processor = g_new0(Processor, 1);
	    
	    __cache_obtain_info(processor, processor_number++);
	}

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

	    get_int("processor", processor->id);
	}
	g_strfreev(tmp);
    }

    if (processor) {
	get_processor_strfamily(processor);
	procs = g_slist_append(procs, processor);
    }

    fclose(cpuinfo);

    return procs;
}

/*
 * Sources: 
 * - Linux' cpufeature.h
 * - http://gentoo-wiki.com/Cpuinfo
 * - Intel IA-32 Architecture Software Development Manual
 */
static struct {
    char *name, *meaning;
} flag_meaning[] = {
	{ "3dnow",	"3DNow! Technology"				},
	{ "3dnowext",	"Extended 3DNow! Technology"			},
	{ "fpu",	"Floating Point Unit"				},
	{ "vme",	"Virtual 86 Mode Extension"			},
	{ "de",		"Debug Extensions - I/O breakpoints"		},
	{ "pse",	"Page Size Extensions (4MB pages)"		},
	{ "tsc",	"Time Stamp Counter and RDTSC instruction"	},
	{ "msr",	"Model Specific Registers"			},
	{ "pae",	"Physical Address Extensions"			},
	{ "mce",	"Machine Check Architeture"			},
	{ "cx8",	"CMPXCHG8 instruction"				},
	{ "apic",	"Advanced Programmable Interrupt Controller"	},
	{ "sep",	"Fast System Call (SYSENTER/SYSEXIT)"		},
	{ "mtrr",	"Memory Type Range Registers"			},
	{ "pge",	"Page Global Enable"				},
	{ "mca",	"Machine Check Architecture"			},
	{ "cmov",	"Conditional Move instruction"			},
	{ "pat",	"Page Attribute Table"				},
	{ "pse36",	"36bit Page Size Extensions"			},
	{ "psn",	"96 bit Processor Serial Number"		},
	{ "mmx",	"MMX technology"				},
	{ "mmxext",	"Extended MMX Technology"			},
	{ "cflush",	"Cache Flush"					},
	{ "dtes",	"Debug Trace Store"				},
	{ "fxsr",	"FXSAVE and FXRSTOR instructions"		},
	{ "kni",	"Streaming SIMD instructions"			},
	{ "xmm",	"Streaming SIMD instructions"			},
	{ "ht",		"HyperThreading"				},
	{ "mp",		"Multiprocessing Capable"			},
	{ "sse",	"SSE instructions"				},
	{ "sse2",	"SSE2 (WNI) instructions"			},
	{ "acc",	"Automatic Clock Control"			},
	{ "ia64",	"IA64 Instructions"				},
	{ "syscall",	"SYSCALL and SYSEXIT instructions"		},
	{ "nx",		"No-execute Page Protection"			},
	{ "xd",		"Execute Disable"				},
	{ "clflush",	"Cache Line Flush instruction"			},
	{ "acpi",	"Thermal Monitor and Software Controlled Clock"	},
	{ "dts",	"Debug Store"					},
	{ "ss",		"Self Snoop"					},
	{ "tm",		"Thermal Monitor"				},
	{ "pbe",	"Pending Break Enable"				},
	{ "pb",		"Pending Break Enable"				},
	{ "pn",		"Processor serial number"			},
	{ "ds",		"Debug Store"					},
	{ "xmm2",	"Streaming SIMD Extensions-2"			},
	{ "xmm3",	"Streaming SIMD Extensions-3"			},
	{ "selfsnoop",	"CPU self snoop"				},
	{ "rdtscp",	"RDTSCP"					},
	{ "recovery",	"CPU in recovery mode"				},
	{ "longrun",	"Longrun power control"				},
	{ "lrti",	"LongRun table interface"			},
	{ "cxmmx",	"Cyrix MMX extensions"				},
	{ "k6_mtrr",	"AMD K6 nonstandard MTRRs"			},
	{ "cyrix_arr",	"Cyrix ARRs (= MTRRs)"				},
	{ "centaur_mcr","Centaur MCRs (= MTRRs)"			},
	{ "constant_tsc","TSC ticks at a constant rate"			},
	{ "up",		"smp kernel running on up"			},
	{ "fxsave_leak","FXSAVE leaks FOP/FIP/FOP"			},
	{ "arch_perfmon","Intel Architectural PerfMon"			},
	{ "pebs",	"Precise-Event Based Sampling"			},
	{ "bts",	"Branch Trace Store"				},
	{ "sync_rdtsc",	"RDTSC synchronizes the CPU"			},
	{ "rep_good",	"rep microcode works well on this CPU"		},
	{ "mwait",	"Monitor/Mwait support"				},
	{ "ds_cpl",	"CPL Qualified Debug Store"			},
	{ "est",	"Enhanced SpeedStep"				},
	{ "tm2",	"Thermal Monitor 2"				},
	{ "cid",	"Context ID"					},
	{ "xtpr",	"Send Task Priority Messages"			},
	{ "xstore",	"on-CPU RNG present (xstore insn)"		},
	{ "xstore_en",	"on-CPU RNG enabled"				},
	{ "xcrypt",	"on-CPU crypto (xcrypt insn)"			},
	{ "xcrypt_en",	"on-CPU crypto enabled"				},
	{ "ace2",	"Advanced Cryptography Engine v2"		},
	{ "ace2_en",	"ACE v2 enabled"				},
	{ "phe",	"PadLock Hash Engine"				},
	{ "phe_en",	"PHE enabled"					},
	{ "pmm",	"PadLock Montgomery Multiplier"			},
	{ "pmm_en",	"PMM enabled"					},
	{ "lahf_lm",	"LAHF/SAHF in long mode"			},
	{ "cmp_legacy",	"HyperThreading not valid"			},
	{ "lm",		"LAHF/SAHF in long mode"			},
	{ "ds_cpl",	"CPL Qualified Debug Store"			},
	{ "vmx",	"Virtualization support (Intel)"		},
	{ "svm",	"Virtualization support (AMD)"			},
	{ "est",	"Enhanced SpeedStep"				},
	{ "tm2",	"Thermal Monitor 2"				},
	{ "ssse3",	"Supplemental Streaming SIMD Extension 3"	},
	{ "cx16",	"CMPXCHG16B instruction"			},
	{ "xptr",	"Send Task Priority Messages"			},
	{ "pebs",	"Precise Event Based Sampling"			},
	{ "bts",	"Branch Trace Store"				},
	{ "ida",	"Intel Dynamic Acceleration"			},
	{ "arch_perfmon","Intel Architectural PerfMon"			},
	{ "pni",	"Streaming SIMD Extension 3 (Prescott New Instruction)"	},
	{ "rep_good",	"rep microcode works well on this CPU"		},
	{ "ts",		"Thermal Sensor"				},
	{ "sse3",	"Streaming SIMD Extension 3"			},
	{ "sse4",	"Streaming SIMD Extension 4"			},
	{ "tni",	"Tejas New Instruction"				},
	{ "nni",	"Nehalem New Instruction"			},
	{ "tpr",	"Task Priority Register"			},
	{ "vid",	"Voltage Identifier"				},
	{ "fid", 	"Frequency Identifier"				},
	{ "dtes64", 	"64-bit Debug Store"				},
	{ "monitor", 	"Monitor/Mwait support"				},
	{ NULL,		NULL						},
};

GHashTable *cpu_flags = NULL;

static void cpu_flags_init(void)
{
    gint i;
    gchar *path;
    
    cpu_flags = g_hash_table_new(g_str_hash, g_str_equal);
    
    path = g_build_filename(g_get_home_dir(), ".hardinfo", "cpuflags.conf", NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        DEBUG("using internal CPU flags database");

        for (i = 0; flag_meaning[i].name != NULL; i++) {
            g_hash_table_insert(cpu_flags, flag_meaning[i].name,
                                flag_meaning[i].meaning);
        }
    } else {
        GKeyFile *flags_file;
        
        DEBUG("using %s as CPU flags database", path);
        
        flags_file = g_key_file_new();
        if (g_key_file_load_from_file(flags_file, path, 0, NULL)) {
            gchar **flag_keys;
            
            flag_keys = g_key_file_get_keys(flags_file, "flags",
                                            NULL, NULL);
            for (i = 0; flag_keys[i]; i++) {
                gchar *meaning;
                
                meaning = g_key_file_get_string(flags_file, "flags",
                                                flag_keys[i], NULL);
                                                
                g_hash_table_insert(cpu_flags, g_strdup(flag_keys[i]), meaning);
                
                /* can't free meaning */
            }
            
            g_strfreev(flag_keys);
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

    g_strfreev(old);
    return tmp;
}

static gchar *processor_get_detailed_info(Processor * processor)
{
    gchar *tmp, *ret, *cache_info;

    tmp = processor_get_capabilities_from_flags(processor->flags);
    cache_info = __cache_get_info_as_string(processor);
    
    ret = g_strdup_printf("[Processor]\n"
			  "Name=%s\n"
			  "Family, model, stepping=%d, %d, %d (%s)\n"
			  "Vendor=%s\n"
			  "[Configuration]\n"
			  "Cache Size=%dkb\n"
			  "Frequency=%.2fMHz\n"
			  "BogoMIPS=%.2f\n"
			  "Byte Order=%s\n"
			  "[Features]\n"
			  "FDIV Bug=%s\n"
			  "HLT Bug=%s\n"
			  "F00F Bug=%s\n"
			  "Coma Bug=%s\n"
			  "Has FPU=%s\n"
			  "[Cache]\n"
			  "%s\n"
			  "[Capabilities]\n"
			  "%s",
			  processor->model_name,
			  processor->family,
			  processor->model,
			  processor->stepping,
			  processor->strmodel,
			  vendor_get_name(processor->vendor_id),
			  processor->cache_size,
			  processor->cpu_mhz, processor->bogomips,
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
			  "Little Endian",
#else
			  "Big Endian",
#endif
			  processor->bug_fdiv ? processor->bug_fdiv : "no",
			  processor->bug_hlt  ? processor->bug_hlt  : "no",
			  processor->bug_f00f ? processor->bug_f00f : "no",
			  processor->bug_coma ? processor->bug_coma : "no",
			  processor->has_fpu  ? processor->has_fpu  : "no",
			  cache_info,
			  tmp);
    g_free(tmp);
    g_free(cache_info);
    
    return ret;
}

static gchar *processor_get_info(GSList * processors)
{
    Processor *processor;

    if (g_slist_length(processors) > 1) {
	gchar *ret, *tmp, *hashkey;
	GSList *l;

	tmp = g_strdup("");

	for (l = processors; l; l = l->next) {
	    processor = (Processor *) l->data;

	    tmp = g_strdup_printf("%s$CPU%d$%s=%.2fMHz\n",
				  tmp, processor->id,
				  processor->model_name,
				  processor->cpu_mhz);

	    hashkey = g_strdup_printf("CPU%d", processor->id);
	    g_hash_table_insert(moreinfo, hashkey,
				processor_get_detailed_info(processor));
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
