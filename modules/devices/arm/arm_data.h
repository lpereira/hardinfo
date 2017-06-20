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

#ifndef _ARMDATA_H_
#define _ARMDATA_H_

/* table lookups */
const char *arm_implementer(const char *code);
const char *arm_part(const char *imp_code, const char *part_code);
const char *arm_arch(const char *cpuinfo_arch_str);
const char *arm_arch_more(const char *cpuinfo_arch_str);

/* cpu_implementer, cpu_part, cpu_variant, cpu_revision, cpu_architecture from /proc/cpuinfo
 * model_name is returned as a fallback if not enough data is known */
char *arm_decoded_name(
    const char *imp, const char *part, const char *var, const char *rev,
    const char *arch, const char *model_name);

/* cpu flags from /proc/cpuinfo */
const char *arm_flag_list(void);                  /* list of all known flags */
const char *arm_flag_meaning(const char *flag);  /* lookup flag meaning */

#endif
