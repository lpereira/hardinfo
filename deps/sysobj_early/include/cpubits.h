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

#ifndef _CPUBITS_H_
#define _CPUBITS_H_

#include <stdint.h>

typedef uint32_t cpubits;
uint32_t cpubits_count(cpubits *b);
int cpubits_min(cpubits *b);
int cpubits_max(cpubits *b);
int cpubits_next(cpubits *b, int start, int end);
cpubits *cpubits_from_str(char *str);
char *cpubits_to_str(cpubits *bits, char *str, int max_len);

#define CPUBITS_SIZE 4096 /* bytes, multiple of sizeof(uint32_t) */
#define CPUBIT_SET(BITS, BIT) ((BITS)[(BIT)/32] |= (1 << (BIT)%32))
#define CPUBIT_GET(BITS, BIT) (((BITS)[(BIT)/32] & (1 << (BIT)%32)) >> (BIT)%32)
#define CPUBITS_CLEAR(BITS) memset((BITS), 0, CPUBITS_SIZE)

#endif
