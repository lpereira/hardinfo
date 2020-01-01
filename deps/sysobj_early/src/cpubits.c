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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpubits.h"

uint32_t cpubits_count(cpubits *b) {
    static const uint32_t max = CPUBITS_SIZE * 8;
    uint32_t count = 0, i = 0;
    while (i < max) {
        count += CPUBIT_GET(b, i);
        i++;
    }
    return count;
}

int cpubits_min(cpubits *b) {
    int i = 0;
    while (i < CPUBITS_SIZE * 8) {
        if (CPUBIT_GET(b, i))
            return i;
        i++;
    }
    return -1;
}

int cpubits_max(cpubits *b) {
    int i = CPUBITS_SIZE * 8 - 1;
    while (i >= 0) {
        if (CPUBIT_GET(b, i))
            return i;
        i--;
    }
    return i;
}

int cpubits_next(cpubits *b, int start, int end) {
    start++; /* not including the start bit */
    if (start >= 0) {
        int i = start;
        if (end == -1)
            end = CPUBITS_SIZE * 8;
        while (i < end) {
            if (CPUBIT_GET(b, i))
                return i;
            i++;
        }
    }
    return -1;
}

cpubits *cpubits_from_str(char *str) {
    char *v, *nv, *hy;
    int r0, r1;
    cpubits *newbits = malloc(CPUBITS_SIZE);
    if (newbits) {
        memset(newbits, 0, CPUBITS_SIZE);
        if (str != NULL) {
            v = (char*)str;
            while ( *v != 0 ) {
                nv = strchr(v, ',');                /* strchrnul() */
                if (nv == NULL) nv = strchr(v, 0);  /* equivalent  */
                hy = strchr(v, '-');
                if (hy && hy < nv) {
                    r0 = strtol(v, NULL, 0);
                    r1 = strtol(hy + 1, NULL, 0);
                } else {
                    r0 = r1 = strtol(v, NULL, 0);
                }
                for (; r0 <= r1; r0++) {
                    CPUBIT_SET(newbits, r0);
                }
                v = (*nv == ',') ? nv + 1 : nv;
            }
        }
    }
    return newbits;
}

char *cpubits_to_str(cpubits *bits, char *str, int max_len) {
    static const uint32_t max = CPUBITS_SIZE * 8;
    uint32_t i = 1, seq_start = 0, seq_last = 0, seq = 0, l = 0;
    char buffer[65536] = "";
    if (CPUBIT_GET(bits, 0)) {
        seq = 1;
        strcpy(buffer, "0");
    }
    while (i < max) {
        if (CPUBIT_GET(bits, i) ) {
            seq_last = i;
            if (!seq) {
                seq = 1;
                seq_start = i;
                l = strlen(buffer);
                sprintf(buffer + l, "%s%d", l ? "," : "", i);
            }
        } else {
            if (seq && seq_last != seq_start) {
                l = strlen(buffer);
                sprintf(buffer + l, "-%d", seq_last);
            }
            seq = 0;
        }
        i++;
    }
    if (str == NULL)
        return strdup(buffer);
    else {
        strncpy(str, buffer, max_len);
        return str;
    }
}
