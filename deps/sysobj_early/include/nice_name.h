/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
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

#ifndef _NICE_NAME_H_
#define _NICE_NAME_H_

/* cleaned in-place */
void nice_name_x86_cpuid_model_string(char *cpuid_model_string);

/* Intel Graphics may have very long names,
 * like "Intel Corporation Seventh Generation Something Core Something Something Integrated Graphics Processor Revision Ninety-four"
 * cleaned in-place */
void nice_name_intel_gpu_device(char *pci_ids_device_string);

#endif
