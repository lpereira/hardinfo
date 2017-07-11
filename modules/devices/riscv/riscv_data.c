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
#include <ctype.h>
#include "riscv_data.h"

#ifndef _
#define _(String) String
#endif
#ifndef N_
#define N_(String) String
#endif

static struct {
    char *name, *meaning;
} tab_ext_meaning[] = {
    { "RV32",  N_("RISC-V 32-bit") },
    { "RV64",  N_("RISC-V 64-bit") },
    { "RV128", N_("RISC-V 128-bit") },
    { "E",     N_("Base embedded integer instructions (15 registers)") },
    { "I",     N_("Base integer instructions (31 registers)") },
    { "M",     N_("Hardware integer multiply and divide") },
    { "A",     N_("Atomic memory operations") },
    { "C",     N_("Compressed 16-bit instructions") },
    { "F",     N_("Floating-point instructions, single-precision") },
    { "D",     N_("Floating-point instructions, double-precision") },
    { "Q",     N_("Floating-point instructions, quad-precision") },
    { "B",     N_("Bit manipulation instructions") },
    { "V",     N_("Vector operations") },
    { "T",     N_("Transactional memory") },
    { "P",     N_("Packed SIMD instructions") },
    { "L",     N_("Decimal floating-point instructions") },
    { "J",     N_("Dynamically translated languages") },
    { "N",     N_("User-level interrupts") },
    { NULL, NULL }
};

static char all_extensions[1024] = "";

#define APPEND_EXT(f) strcat(all_extensions, f); strcat(all_extensions, " ");
const char *riscv_ext_list() {
    int i = 0, built = 0;
    built = strlen(all_extensions);
    if (!built) {
        while(tab_ext_meaning[i].name != NULL) {
            APPEND_EXT(tab_ext_meaning[i].name);
            i++;
        }
    }
    return all_extensions;
}

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
            if (strncasecmp(tab_ext_meaning[i].name, ext, l) == 0) {
                if (tab_ext_meaning[i].meaning != NULL)
                    return _(tab_ext_meaning[i].meaning);
                else return NULL;
            }
            i++;
        }
    }
    return NULL;
}

/* see RISC-V spec 2.2: Chapter 22: ISA Subset Naming Conventions */

/* Spec says case-insensitve, but I prefer single-letter extensions
 * capped and version string (like "2p1") with a lowercase p. */
#define RV_FIX_CASE(fstr,vo) \
    p = fstr; while (*p != 0 && *p != ':') { if (!vo) *p = toupper(*p); p++; } \
    if (*p == ':') while (*p != 0) { if (*p == 'P') *p = 'p'; p++; }

static int riscv_isa_next(const char *isap, char *flag) {
    char *p = NULL, *start = NULL;
    char *next_sep = NULL, *next_digit = NULL;
    int skip_len = 0, tag_len = 0, ver_len = 0;
    char ext_str[32], ver_str[32];

    if (isap == NULL)
        return 0;

    /* find start by skipping any '_' */
    start = (char*)isap;
    while (*start != 0 && *start == '_') { start++; skip_len++; };
    if (*start == 0)
        return 0;

    /* find next '_' or \0 */
    p = start; while (*p != 0 && *p != '_') { p++; }; next_sep = p;

    /* find next digit that may be a version, find length of version */
    p = start; while (*p != 0 && !isdigit(*p)) { p++; };
    if (isdigit(*p)) next_digit = p;
    if (next_digit) {
        while (*p != 0 && (isdigit(*p) || *p == 'p' || *p == 'P') ) {
            if ((*p == 'p' || *p == 'P') && !isdigit(*(p+1)) )
                break;
            ver_len++;
            p++;
        }
    }

    /* is next version nearer than next separator */
    p = start;
    if (next_digit && next_digit < next_sep)
        tag_len = next_digit - p;
    else {
        tag_len = next_sep - p;
        ver_len = 0;
    }

    switch(*p) {
        case 'S': case 's': /* supervisor extension */
        case 'X': case 'x': /* custom extension */
            /* custom supervisor extension (SX..) handled by S */
            break;
        default: /* single character (standard) extension */
            tag_len = 1;
            if (next_digit != p+1) ver_len = 0;
            break;
    }

    memset(ext_str, 0, 32);
    memset(ver_str, 0, 32);
    if (ver_len) {
        strncpy(ext_str, p, tag_len);
        strncpy(ver_str, next_digit, ver_len);
        sprintf(flag, "%s:%s", ext_str, ver_str);
        if (tag_len == 1) {
            RV_FIX_CASE(flag, 0);
        } else {
            RV_FIX_CASE(flag, 1);
        }
        return skip_len + tag_len + ver_len;
    } else {
        strncpy(ext_str, p, tag_len);
        sprintf(flag, "%s", ext_str);
        if (tag_len == 1) { RV_FIX_CASE(flag, 0); }
        return skip_len + tag_len;
    }
}

#define FSTR_SIZE 1024
#define RV_CHECK_FOR(e) ( strncasecmp(ps, e, 2) == 0 )
#define ADD_EXT_FLAG(ext) el = strlen(ext); strncpy(pd, ext, el); strncpy(pd + el, " ", 1); pd += el + 1;
char *riscv_isa_to_flags(const char *isa) {
    char *flags = NULL, *ps = (char*)isa, *pd = NULL;
    char flag_buf[64] = "";
    int isa_len = 0, tl = 0, el = 0; /* el used in macro */

    if (isa) {
        isa_len = strlen(isa);
        flags = malloc(FSTR_SIZE);
        if (flags) {
            memset(flags, 0, FSTR_SIZE);
            ps = (char*)isa;
            pd = flags;
            if ( RV_CHECK_FOR("RV") )
            { ps += 2; }
            if ( RV_CHECK_FOR("32") )
            { ADD_EXT_FLAG("RV32"); ps += 2; }
            else if ( RV_CHECK_FOR("64") )
            { ADD_EXT_FLAG("RV64"); ps += 2; }
            else if ( RV_CHECK_FOR("128") )
            { ADD_EXT_FLAG("RV128"); ps += 3; }

            while( (tl = riscv_isa_next(ps, flag_buf)) ) {
                if (flag_buf[0] == 'G') { /* G = IMAFD */
                    flag_buf[0] = 'I'; ADD_EXT_FLAG(flag_buf);
                    flag_buf[0] = 'M'; ADD_EXT_FLAG(flag_buf);
                    flag_buf[0] = 'A'; ADD_EXT_FLAG(flag_buf);
                    flag_buf[0] = 'F'; ADD_EXT_FLAG(flag_buf);
                    flag_buf[0] = 'D'; ADD_EXT_FLAG(flag_buf);
                } else {
                    ADD_EXT_FLAG(flag_buf);
                }
                ps += tl;
                if (ps - isa >= isa_len) break; /* just in case */
            }
        }
    }
    return flags;
}
