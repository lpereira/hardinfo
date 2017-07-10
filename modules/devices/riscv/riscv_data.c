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
#include "riscv_data.h"

static struct {
    char *name, *meaning;
} tab_ext_meaning[] = {
    { "rv32",	"32-bit" },
    { "rv64",	"64-bit" },
    { "rv128",	"128-bit" },
    { "rv_E",	"Base embedded integer instructions (15 registers)" },
    { "rv_I",	"Base integer instructions (31 registers)" },
    { "rv_M",	"Hardware integer multiply and divide" },
    { "rv_A",	"Atomic memory operations" },
    { "rv_C",	"Compressed 16-bit instructions" },
    { "rv_F",	"Floating-point instructions, single-precision" },
    { "rv_D",	"Floating-point instructions, double-precision" },
    { "rv_Q",	"Floating-point instructions, quad-precision" },
    { "rv_B",	"Bit manipulation instructions" },
    { "rv_V",	"Vector operations" },
    { "rv_T",	"Transactional memory" },
    { "rv_P",	"Packed SIMD instructions" },
    { "rv_L",	"Decimal floating-point instructions" },
    { "rv_J",	"Dynamically translated languages" },
    { "rv_N",	"User-level interrupts" },
    { NULL, NULL }
};

const char *riscv_ext_meaning(const char *ext) {
    int i = 0, l = 0;
    char *c = NULL;
    if (ext) {
        c = strchr(ext, ':'); /* allow extension:version, ignore version */
        if (c != NULL)
            l = c - ext;
        else
            l = strlen(ext);
        while(tab_ext_meaning[i].name != NULL) {
            if (strncmp(tab_ext_meaning[i].name, ext, l) == 0)
                return tab_ext_meaning[i].meaning;
            i++;
        }
    }
    return NULL;
}

/* see RISC-V spec 2.2: Chapter 22: ISA Subset Naming Conventions */

#define FSTR_SIZE 1024
#define EAT_VERSION() if (isdigit(*ps)) while(isdigit(*ps) || *ps == 'p' ) { ps++; };
#define CP_EXT_FLAG() while(*ps != 0 && *ps != '_' && *ps != ' ') { if (isdigit(*ps)) { EAT_VERSION(); break; } *pd = *ps; ps++; pd++; }; *pd = ' '; pd++;
#define ADD_EXT_FLAG(ext) l = strlen(ext); strncpy(pd, ext " ", l + 1); pd += l + 1;
char *riscv_isa_to_flags(const char *isa) {
    char *flags = NULL, *ps = (char*)isa, *pd = NULL;
    char ext[64] = "", ver[64] = "";
    int l = 0;
    if (isa) {
        flags = malloc(FSTR_SIZE);
        if (flags) {
            memset(flags, 0, FSTR_SIZE);
            pd = flags;
            if ( strncmp(ps, "rv", 2) == 0
                || strncmp(ps, "RV", 2) == 0 ) {
                ps += 2;
            }
            if ( strncmp(ps, "32", 2) == 0 ) {
                ADD_EXT_FLAG("rv32");
                ps += 2;
            } else if ( strncmp(ps, "64", 2) == 0 ) {
                ADD_EXT_FLAG("rv64");
                ps += 2;
            } else if ( strncmp(ps, "128", 3) == 0) {
                ADD_EXT_FLAG("rv128");
                ps += 3;
            }
            while (*ps != 0) {
                switch(*ps) {
                    case 'G': case 'g': /* G = IMAFD */
                        ADD_EXT_FLAG("rv_I");
                        ADD_EXT_FLAG("rv_M");
                        ADD_EXT_FLAG("rv_A");
                        ADD_EXT_FLAG("rv_F");
                        ADD_EXT_FLAG("rv_D");
                        break;
                    case 'E': case 'e': ADD_EXT_FLAG("rv_E"); break;
                    case 'I': case 'i': ADD_EXT_FLAG("rv_I"); break;
                    case 'M': case 'm': ADD_EXT_FLAG("rv_M"); break;
                    case 'A': case 'a': ADD_EXT_FLAG("rv_A"); break;
                    case 'C': case 'c': ADD_EXT_FLAG("rv_C"); break;
                    case 'F': case 'f': ADD_EXT_FLAG("rv_F"); break;
                    case 'D': case 'd': ADD_EXT_FLAG("rv_D"); break;
                    case 'Q': case 'q': ADD_EXT_FLAG("rv_Q"); break;
                    case 'P': case 'p': ADD_EXT_FLAG("rv_P"); break;
                    case 'B': case 'b': ADD_EXT_FLAG("rv_B"); break;
                    case 'V': case 'v': ADD_EXT_FLAG("rv_V"); break;
                    case 'J': case 'j': ADD_EXT_FLAG("rv_J"); break;
                    case 'N': case 'n': ADD_EXT_FLAG("rv_N"); break;
                    case 'S': case 's': CP_EXT_FLAG(); break; /* supervisor extension */
                    case 'X': case 'x': CP_EXT_FLAG(); break; /* custom extension */
                    /* custom supervisor extension (SX..) handled by S */
                    default: break;
                }
                ps++;
                EAT_VERSION();
            }
        }
    }
    return flags;
}
