/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    This file
 *    Copyright (C) 2018 Burt P. <pburt0@gmail.com>
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

#include <string.h>

#include "hardinfo.h"
#include "devices.h"
#include "gpu_util.h"

void scan_gpu_do(void);

gchar *gpu_list = NULL;
gchar *gpu_summary = NULL;

void gpu_summary_add(const char *gpu_name) {
    if (strlen(gpu_summary) == 0) {
        /* first one */
        gpu_summary = h_strdup_cprintf("%s", gpu_summary, gpu_name);
    } else {
        /* additional */
        gpu_summary = h_strdup_cprintf(" + %s", gpu_summary, gpu_name);
    }
}

#define UNKIFNULL_AC(f) (f != NULL) ? f : _("(Unknown)");

static void _gpu_pci_dev(gpud* gpu) {
    pcid *p = gpu->pci_dev;
    gchar *str;
    gchar *vendor, *svendor, *v_str, *sv_str, *product, *sproduct;
    gchar *name, *key;
    gchar *drm_path = NULL;

    vendor = UNKIFNULL_AC(p->vendor_id_str);
    svendor = UNKIFNULL_AC(p->sub_vendor_id_str);
    product = UNKIFNULL_AC(p->device_id_str);
    sproduct = UNKIFNULL_AC(p->sub_device_id_str);
    if (gpu->drm_dev)
        drm_path = g_strdup_printf("/dev/dri/%s", gpu->drm_dev);
    else
        drm_path = g_strdup(_("(Unknown)"));

#define USE_HARDINFO_VENDOR_THING 1
    if (USE_HARDINFO_VENDOR_THING) {
        const gchar *v_url = vendor_get_url(vendor);
        const gchar *v_name = vendor_get_name(vendor);
        if (v_url != NULL) {
            v_str = g_strdup_printf("%s (%s)", v_name, v_url);
        } else {
            v_str = g_strdup(vendor);
        }
        v_url = vendor_get_url(svendor);
        v_name = vendor_get_name(svendor);
        if (v_url != NULL) {
            sv_str = g_strdup_printf("%s (%s)", v_name, v_url);
        } else {
            sv_str = g_strdup(svendor);
        }
    } else {
            v_str = g_strdup(vendor);
            sv_str = g_strdup(svendor);
    }

    name = g_strdup_printf("%s %s", vendor, product);
    key = g_strdup_printf("GPU%s", gpu->id);

    gpu_summary_add((gpu->nice_name) ? gpu->nice_name : name);
    gpu_list = h_strdup_cprintf("$%s$%s=%s\n", gpu_list, key, gpu->id, name);

    gchar *vendor_device_str;
    if (p->vendor_id == p->sub_vendor_id && p->device_id == p->sub_device_id) {
        vendor_device_str = g_strdup_printf(
                     /* Vendor */     "%s=[%04x] %s\n"
                     /* Device */     "%s=[%04x] %s\n",
                    _("Vendor"), p->vendor_id, v_str,
                    _("Device"), p->device_id, product);
    } else {
        vendor_device_str = g_strdup_printf(
                     /* Vendor */     "%s=[%04x] %s\n"
                     /* Device */     "%s=[%04x] %s\n"
                     /* Sub-device vendor */     "%s=[%04x] %s\n"
                     /* Sub-device */     "%s=[%04x] %s\n",
                    _("Vendor"), p->vendor_id, v_str,
                    _("Device"), p->device_id, product,
                    _("SVendor"), p->sub_vendor_id, sv_str,
                    _("SDevice"), p->sub_device_id, sproduct);
    }

    gchar *pcie_str;
    if (p->pcie_width_curr) {
        pcie_str = g_strdup_printf("[%s]\n"
                     /* Width (max) */  "%s=x%u\n"
                     /* Speed (max) */  "%s=%0.1f %s\n",
                    _("PCI Express"),
                    _("Maximum Link Width"), p->pcie_width_max,
                    _("Maximum Link Speed"), p->pcie_speed_max, _("GT/s") );
    } else
        pcie_str = strdup("");

    gchar *nv_str;
    if (gpu->nv_info) {
        nv_str = g_strdup_printf("[%s]\n"
                     /* model */  "%s=%s\n"
                     /* bios */   "%s=%s\n"
                     /* uuid */   "%s=%s\n",
                    _("NVIDIA"),
                    _("Model"), gpu->nv_info->model,
                    _("BIOS Version"), gpu->nv_info->bios_version,
                    _("UUID"), gpu->nv_info->uuid );
    } else
        nv_str = strdup("");

    str = g_strdup_printf("[%s]\n"
             /* Location */  "%s=%s\n"
             /* DRM Dev */   "%s=%s\n"
             /* Class */     "%s=[%04x] %s\n"
                             "%s"
             /* Revision */  "%s=%02x\n"
             /* NV */        "%s"
             /* PCIe */      "%s"
                             "[%s]\n"
             /* Driver */    "%s=%s\n"
             /* Modules */   "%s=%s\n",
                _("Device Information"),
                _("Location"), gpu->location,
                _("DRM Device"), drm_path,
                _("Class"), p->class, p->class_str,
                vendor_device_str,
                _("Revision"), p->revision,
                nv_str,
                pcie_str,
                _("Driver"),
                _("In Use"), (p->driver) ? p->driver : _("(Unknown)"),
                _("Kernel Modules"), (p->driver_list) ? p->driver_list : _("(Unknown)")
                );

    moreinfo_add_with_prefix("DEV", key, str); /* str now owned by morinfo */

    g_free(drm_path);
    g_free(pcie_str);
    g_free(nv_str);
    g_free(vendor_device_str);
    g_free(v_str);
    g_free(sv_str);
    g_free(name);
    g_free(key);
}

int _dt_soc_gpu(gpud *gpu) {
    static char UNKSOC[] = "(Unknown)"; /* don't translate this */
    gchar *vendor = gpu->vendor_str;
    gchar *device = gpu->device_str;
    if (vendor == NULL) vendor = UNKSOC;
    if (device == NULL) device = UNKSOC;
    gchar *key = g_strdup(gpu->id);
    gchar *name = (vendor == UNKSOC && device == UNKSOC)
            ? g_strdup(_("Unknown integrated GPU"))
            : g_strdup_printf("%s %s", vendor, device);
    gpu_summary_add((gpu->nice_name) ? gpu->nice_name : name);
    gpu_list = h_strdup_cprintf("$%s$%s=%s\n", gpu_list, key, key, name);
    gchar *str = g_strdup_printf("[%s]\n"
             /* Location */  "%s=%s\n"
             /* dt compat */  "%s=%s\n"
             /* Vendor */  "%s=%s\n"
             /* Device */  "%s=%s\n",
                _("Device Information"),
                _("Location"), gpu->location,
                _("DT Compatibility"), gpu->dt_compat,
                _("Vendor"), vendor,
                _("Device"), device
                );
    moreinfo_add_with_prefix("DEV", key, str); /* str now owned by morinfo */
    return 1;
}

void scan_gpu_do(void) {
    if (gpu_summary)
        g_free(gpu_summary);
    if (gpu_list) {
        moreinfo_del_with_prefix("DEV:GPU");
        g_free(gpu_list);
    }
    gpu_summary = strdup("");
    gpu_list = g_strdup_printf("[%s]\n", _("GPUs"));

    gpud *gpus = gpu_get_device_list();
    gpud *curr = gpus;

    int c = gpud_list_count(gpus);

    if (c > 0) {
        while(curr) {
            if (curr->pci_dev) {
                _gpu_pci_dev(curr);
            }
            if (curr->dt_compat) {
                _dt_soc_gpu(curr);
            }
            curr=curr->next;
        }
    }
    gpud_list_free(gpus);

    if (c)
        gpu_list = g_strconcat(gpu_list, "[$ShellParam$]\n", "ViewType=1\n", NULL);
    else  {
        /* NO GPU? */
        gpu_list = g_strconcat(gpu_list, _("No GPU devices found"), "=\n", NULL);
    }
}
