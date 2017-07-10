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
    { NULL, NULL }
};

const char *riscv_ext_meaning(const char *ext) {
    int i = 0;
    if (ext)
    while(tab_ext_meaning[i].name != NULL) {
        if (strcmp(tab_ext_meaning[i].name, ext) == 0)
            return tab_ext_meaning[i].meaning;
        i++;
    }
    return NULL;
}

#define FSTR_SIZE 1024
#define ADD_EXT_FLAG(ext) l = strlen(ext); strncpy(pd, ext " ", l + 1); pd += l + 1;
char *riscv_isa_to_flags(const char *isa) {
    char *flags = NULL, *ps = (char*)isa, *pd = NULL;
    int l = 0;
    if (isa) {
        flags = malloc(FSTR_SIZE);
        if (flags) {
            memset(flags, 0, FSTR_SIZE);
            pd = flags;
            if ( strncmp(ps, "RV32", 4) == 0 ) {
                ADD_EXT_FLAG("rv32");
                ps += 4;
            } else if ( strncmp(ps, "RV64", 4) == 0 ) {
                ADD_EXT_FLAG("rv64");
                ps += 4;
            } else if ( strncmp(ps, "RV128", 5) == 0) {
                ADD_EXT_FLAG("rv128");
                ps += 5;
            }
            while (*ps != 0) {
                switch(*ps) {
                    case 'G': /* G = IMAFD */
                        ADD_EXT_FLAG("rv_I");
                        ADD_EXT_FLAG("rv_M");
                        ADD_EXT_FLAG("rv_A");
                        ADD_EXT_FLAG("rv_F");
                        ADD_EXT_FLAG("rv_D");
                        break;
                    case 'E': ADD_EXT_FLAG("rv_E"); break;
                    case 'I': ADD_EXT_FLAG("rv_I"); break;
                    case 'M': ADD_EXT_FLAG("rv_M"); break;
                    case 'A': ADD_EXT_FLAG("rv_A"); break;
                    case 'C': ADD_EXT_FLAG("rv_C"); break;
                    case 'F': ADD_EXT_FLAG("rv_F"); break;
                    case 'D': ADD_EXT_FLAG("rv_D"); break;
                    case 'Q': ADD_EXT_FLAG("rv_Q"); break;
                    case 'B': ADD_EXT_FLAG("rv_B"); break;
                    case 'V': ADD_EXT_FLAG("rv_V"); break;
                    default: break;
                }
                ps++;
            }
        }
    }
    return flags;
}
