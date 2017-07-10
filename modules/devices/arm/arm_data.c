/*
 * rpiz - https://github.com/bp0/rpiz
 * Copyright (C) 2017  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "arm_data.h"

#ifndef _
#define _(String) String
#endif
#ifndef N_
#define N_(String) String
#endif

/* sources:
 *   https://unix.stackexchange.com/a/43563
 *   git:linux/arch/arm/kernel/setup.c
 *   git:linux/arch/arm64/kernel/cpuinfo.c
 */
static struct {
    char *name, *meaning;
} tab_flag_meaning[] = {
    /* arm/hw_cap */
    { "swp",	N_("SWP instruction (atomic read-modify-write)") },
    { "half",	N_("Half-word loads and stores") },
    { "thumb",	N_("Thumb (16-bit instruction set)") },
    { "26bit",	N_("26-Bit Model (Processor status register folded into program counter)") },
    { "fastmult",	N_("32x32->64-bit multiplication") },
    { "fpa",	N_("Floating point accelerator") },
    { "vfp",	N_("VFP (early SIMD vector floating point instructions)") },
    { "edsp",	N_("DSP extensions (the 'e' variant of the ARM9 CPUs, and all others above)") },
    { "java",	N_("Jazelle (Java bytecode accelerator)") },
    { "iwmmxt",	N_("SIMD instructions similar to Intel MMX") },
    { "crunch",	N_("MaverickCrunch coprocessor (if kernel support enabled)") },
    { "thumbee",	N_("ThumbEE") },
    { "neon",	N_("Advanced SIMD/NEON on AArch32") },
    { "evtstrm",	N_("kernel event stream using generic architected timer") },
    { "vfpv3",	N_("VFP version 3") },
    { "vfpv3d16",	N_("VFP version 3 with 16 D-registers") },
    { "vfpv4",	N_("VFP version 4 with fast context switching") },
    { "vfpd32",	N_("VFP with 32 D-registers") },
    { "tls",	N_("TLS register") },
    { "idiva",	N_("SDIV and UDIV hardware division in ARM mode") },
    { "idivt",	N_("SDIV and UDIV hardware division in Thumb mode") },
    { "lpae",	N_("40-bit Large Physical Address Extension") },
    /* arm/hw_cap2 */
    { "pmull",	N_("64x64->128-bit F2m multiplication (arch>8)") },
    { "aes",	N_("Crypto:AES (arch>8)") },
    { "sha1",	N_("Crypto:SHA1 (arch>8)") },
    { "sha2",	N_("Crypto:SHA2 (arch>8)") },
    { "crc32",	N_("CRC32 checksum instructions (arch>8)") },
    /* arm64/hw_cap */
    { "fp",	N_("") },
    { "asimd",	N_("Advanced SIMD/NEON on AArch64 (arch>8)") },
    { "atomics",	N_("") },
    { "fphp",	N_("") },
    { "asimdhp",	N_("") },
    { "cpuid",	N_("") },
    { "asimdrdm",	N_("") },
    { "jscvt",	N_("") },
    { "fcma",	N_("") },
    { "lrcpc",	N_("") },

    { NULL, NULL},
};

static struct {
    int code; char *name;
} tab_arm_implementer[] = {
    { 0x41,	"ARM" },
    { 0x44,	"Intel (formerly DEC) StrongARM" },
    { 0x4e,	"nVidia" },
    { 0x54,	"Texas Instruments" },
    { 0x56,	"Marvell" },
    { 0x69,	"Intel XScale" },
    { 0, NULL},
};

static struct {
    /* source: t = tested, d = official docs, f = web */
    int code; char *part_desc;
} tab_arm_arm_part[] = { /* only valid for implementer 0x41 ARM */
    /*d */ { 0x920,	"ARM920" },
    /*d */ { 0x926,	"ARM926" },
    /*d */ { 0x946,	"ARM946" },
    /*d */ { 0x966,	"ARM966" },
    /*d */ { 0xb02,	"ARM11 MPCore" },
    /*d */ { 0xb36,	"ARM1136" },
    /*d */ { 0xb56,	"ARM1156" },
    /*dt*/ { 0xb76,	"ARM1176" },
    /*dt*/ { 0xc05,	"Cortex-A5" },
    /*d */ { 0xc07,	"Cortex-A7 MPCore" },
    /*dt*/ { 0xc08,	"Cortex-A8" },
    /*dt*/ { 0xc09,	"Cortex-A9" },
    /*d */ { 0xc0e,	"Cortex-A17 MPCore" },
    /*d */ { 0xc0f,	"Cortex-A15" },
    /*d */ { 0xd01,	"Cortex-A32" },
    /*dt*/ { 0xd03,	"Cortex-A53" },
    /*d */ { 0xd04,	"Cortex-A35" },
    /*d */ { 0xd07,	"Cortex-A57 MPCore" },
    /*d */ { 0xd08,	"Cortex-A72" },
    /*d */ { 0xd09,	"Cortex-A73" },
           { 0, NULL},
};

static struct {
    char *code; char *name; char *more;
} tab_arm_arch[] = {
    { "7",	"AArch32",	"AArch32 (ARMv7)" },
    { "8",	"AArch64",	"AArch64 (ARMv8)" },
    { "AArch32",	"AArch32",	"AArch32 (ARMv7)" },
    { "AArch64",	"AArch64",	"AArch64 (ARMv8)" },
    { NULL, NULL, NULL },
};

static char all_flags[1024] = "";

#define APPEND_FLAG(f) strcat(all_flags, f); strcat(all_flags, " ");
const char *arm_flag_list() {
    int i = 0, built = 0;
    built = strlen(all_flags);
    if (!built) {
        while(tab_flag_meaning[i].name != NULL) {
            APPEND_FLAG(tab_flag_meaning[i].name);
            i++;
        }
    }
    return all_flags;
}

const char *arm_flag_meaning(const char *flag) {
    int i = 0;
    if (flag)
    while(tab_flag_meaning[i].name != NULL) {
        if (strcmp(tab_flag_meaning[i].name, flag) == 0)
            return _(tab_flag_meaning[i].meaning);
        i++;
    }
    return NULL;
}

static int code_match(int c0, const char* code1) {
    int c1;
    if (code1 == NULL) return 0;
    c1 = strtol(code1, NULL, 0);
    return (c0 == c1) ? 1 : 0;
}

const char *arm_implementer(const char *code) {
    int i = 0;
    if (code)
    while(tab_arm_implementer[i].code) {
        if (code_match(tab_arm_implementer[i].code, code))
            return tab_arm_implementer[i].name;
        i++;
    }
    return NULL;
}

const char *arm_part(const char *imp_code, const char *part_code) {
    int i = 0;
    if (imp_code && part_code) {
        if (code_match(0x41, imp_code)) {
            /* 0x41=ARM parts */
            while(tab_arm_arm_part[i].code) {
                if (code_match(tab_arm_arm_part[i].code, part_code))
                    return tab_arm_arm_part[i].part_desc;
                i++;
            }
        }
    }
    return NULL;
}

const char *arm_arch(const char *cpuinfo_arch_str) {
    int i = 0;
    if (cpuinfo_arch_str)
    while(tab_arm_arch[i].code) {
        if (strcmp(tab_arm_arch[i].code, cpuinfo_arch_str) == 0)
            return tab_arm_arch[i].name;
        i++;
    }
    return cpuinfo_arch_str;
}

const char *arm_arch_more(const char *cpuinfo_arch_str) {
    int i = 0;
    if (cpuinfo_arch_str)
    while(tab_arm_arch[i].code) {
        if (strcmp(tab_arm_arch[i].code, cpuinfo_arch_str) == 0)
            return tab_arm_arch[i].more;
        i++;
    }
    return cpuinfo_arch_str;
}

char *arm_decoded_name(const char *imp, const char *part, const char *var, const char *rev, const char *arch, const char *model_name) {
    char *dnbuff;
    char *imp_name = NULL, *part_desc = NULL, *arch_name = NULL;
    int r = 0, p = 0;
    dnbuff = malloc(256);
    if (dnbuff) {
        memset(dnbuff, 0, 256);

        if (imp && arch && part && rev) {
            /* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0395b/CIHCAGHH.html
             * variant and revision can be rendered r{variant}p{revision} */
            r = strtol(var, NULL, 0);
            p = strtol(rev, NULL, 0);
            imp_name = (char*) arm_implementer(imp);
            part_desc = (char*) arm_part(imp, part);
            arch_name = (char *) arm_arch(arch);
            if (imp_name || part_desc) {
                if (arch_name != arch)
                    sprintf(dnbuff, "%s %s r%dp%d (%s)",
                        (imp_name) ? imp_name : imp,
                        (part_desc) ? part_desc : part,
                        r, p, arch_name);
                else
                    sprintf(dnbuff, "%s %s r%dp%d (arch:%s)",
                        (imp_name) ? imp_name : imp,
                        (part_desc) ? part_desc : part,
                        r, p, arch);
            } else {
                /* fallback for now */
                sprintf(dnbuff, "%s [imp:%s part:%s r%dp%d arch:%s]",
                        model_name,
                        (imp_name) ? imp_name : imp,
                        (part_desc) ? part_desc : part,
                        r, p, arch);
            }
        } else {
            /* prolly not ARM arch at all */
            if (model_name)
                sprintf(dnbuff, "%s", model_name);
            else {
                free(dnbuff);
                return NULL;
            }
        }
    }
    return dnbuff;
}
