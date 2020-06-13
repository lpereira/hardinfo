/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#ifndef __GPU_UTIL_H__
#define __GPU_UTIL_H__

#include <stdint.h>
#include "pci_util.h"
#include "dt_util.h"

typedef struct nvgpu {
    char *model;
    char *bios_version;
    char *uuid;
} nvgpu;

typedef struct gpud {
    char *id; /* ours */
    char *nice_name;
    char *vendor_str;
    char *device_str;
    char *location;
    uint32_t khz_min, khz_max; /* core */
    uint32_t mem_khz_min, mem_khz_max; /* memory */

    char *drm_dev;
    char *sysfs_drm_path;
    pcid *pci_dev;

    char *dt_compat, *dt_status, *dt_name, *dt_path;
    const char *dt_vendor, *dt_device;
    dt_opp_range *dt_opp;

    nvgpu *nv_info;
    /* ... */

    struct gpud *next; /* this is a linked list */
} gpud;

gpud *gpu_get_device_list();
int gpud_list_count(gpud *);
void gpud_list_free(gpud *);

void gpud_free(gpud *);

void scan_gpu_do(void);

#endif
