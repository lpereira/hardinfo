/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
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

#include "hardinfo.h"
#include "gpu_util.h"
#include "nice_name.h"
#include "cpu_util.h" /* for EMPIFNULL() */

nvgpu *nvgpu_new() {
    return g_new0(nvgpu, 1);
}

void nvgpu_free(nvgpu *s) {
    if (s) {
        free(s->model);
        free(s->bios_version);
        free(s->uuid);
    }
}

static char *_line_value(char *line, const char *prefix) {
    if (g_str_has_prefix(g_strstrip(line), prefix)) {
        line += strlen(prefix) + 1;
        return g_strstrip(line);
    } else
        return NULL;
}

static gboolean nv_fill_procfs_info(gpud *s) {
    gchar *data, *p, *l, *next_nl;
    gchar *pci_loc = pci_address_str(s->pci_dev->domain, s->pci_dev->bus, s->pci_dev->device, s->pci_dev->function);
    gchar *nvi_file = g_strdup_printf("/proc/driver/nvidia/gpus/%s/information", pci_loc);

    g_file_get_contents(nvi_file, &data, NULL, NULL);
    g_free(pci_loc);
    g_free(nvi_file);

    if (data) {
        s->nv_info = nvgpu_new();
        p = data;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            g_strstrip(p);
            if (l = _line_value(p, "Model")) {
                s->nv_info->model = g_strdup(l);
                goto nv_details_next;
            }
            if (l = _line_value(p, "GPU UUID")) {
                s->nv_info->uuid = g_strdup(l);
                goto nv_details_next;
            }
            if (l = _line_value(p, "Video BIOS")) {
                s->nv_info->bios_version = g_strdup(l);
                goto nv_details_next;
            }

            /* TODO: more details */

            nv_details_next:
                p = next_nl + 1;
        }
        g_free(data);
        return TRUE;
    }
    return FALSE;
}

static void intel_fill_freq(gpud *s) {
    gchar path[256] = "";
    gchar *min_mhz = NULL, *max_mhz = NULL;
    if (s->sysfs_drm_path) {
        snprintf(path, 255, "%s/%s/gt_min_freq_mhz", s->sysfs_drm_path, s->id);
        g_file_get_contents(path, &min_mhz, NULL, NULL);
        snprintf(path, 255, "%s/%s/gt_max_freq_mhz", s->sysfs_drm_path, s->id);
        g_file_get_contents(path, &max_mhz, NULL, NULL);

        if (min_mhz)
            s->khz_min = atoi(min_mhz) * 1000;
        if (max_mhz)
            s->khz_max = atoi(max_mhz) * 1000;

        g_free(min_mhz);
        g_free(max_mhz);
    }
}

static void amdgpu_parse_dpmclk(gchar *path, int *min, int *max) {
    gchar *data = NULL, *p, *next_nl;
    int sp, i, clk;

    *min = -1;
    *max = -1;

    g_file_get_contents(path, &data, NULL, NULL);
    if (data) {
        p = data;

        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            sp = sscanf(p, "%d: %dMhz", &i, &clk);

            if (sp == 2) {
                if (clk > 0) {
                    if (*min < 0 || clk < *min)
                        *min = clk;
                    if (clk > *max)
                        *max = clk;
                }
            }

            p = next_nl + 1;
        }
    }
    g_free(data);
}

static void amdgpu_fill_freq(gpud *s) {
    gchar path[256] = "";
    int clk_min = -1, clk_max = -1, mem_clk_min = -1, mem_clk_max = -1;

    if (s->sysfs_drm_path) {
        /* core */
        snprintf(path, 255, "%s/%s/device/pp_dpm_sclk", s->sysfs_drm_path, s->id);
        amdgpu_parse_dpmclk(path, &clk_min, &clk_max);

        if (clk_max > 0)
            s->khz_max = clk_max * 1000;
        if (clk_min > 0)
            s->khz_min = clk_min * 1000;

        /* memory */
        snprintf(path, 255, "%s/%s/device/pp_dpm_mclk", s->sysfs_drm_path, s->id);
        amdgpu_parse_dpmclk(path, &mem_clk_min, &mem_clk_max);

        if (mem_clk_max > 0)
            s->mem_khz_max = mem_clk_max * 1000;
        if (mem_clk_min > 0)
            s->mem_khz_min = mem_clk_min * 1000;
    }
}

gpud *gpud_new() {
    return g_new0(gpud, 1);
}

void gpud_free(gpud *s) {
    if (s) {
        free(s->id);
        free(s->nice_name);
        free(s->vendor_str);
        free(s->device_str);
        free(s->location);
        free(s->drm_dev);
        free(s->sysfs_drm_path);
        free(s->dt_compat);
        g_free(s->dt_opp);
        pcid_free(s->pci_dev);
        nvgpu_free(s->nv_info);
        g_free(s);
    }
}

void gpud_list_free(gpud *s) {
    gpud *n;
    while(s != NULL) {
        n = s->next;
        gpud_free(s);
        s = n;
    }
}

/* returns number of items after append */
static int gpud_list_append(gpud *l, gpud *n) {
    int c = 0;
    while(l != NULL) {
        c++;
        if (l->next == NULL) {
            if (n != NULL) {
                l->next = n;
                c++;
            }
            break;
        }
        l = l->next;
    }
    return c;
}

int gpud_list_count(gpud *s) {
    return gpud_list_append(s, NULL);
}

/* TODO: In the future, when there is more vendor specific information available in
 * the gpu struct, then more precise names can be given to each gpu */
static void make_nice_name(gpud *s) {

    /* NV information available */
    if (s->nv_info && s->nv_info->model) {
        s->nice_name = g_strdup_printf("%s %s", "NVIDIA", s->nv_info->model);
        return;
    }

    static const char unk_v[] = "Unknown"; /* do not...    */
    static const char unk_d[] = "Device";  /* ...translate */
    const char *vendor_str = s->vendor_str;
    const char *device_str = s->device_str;
    if (!vendor_str)
        vendor_str = unk_v;
    if (!device_str)
        device_str = unk_d;

    /* try and a get a "short name" for the vendor */
    vendor_str = vendor_get_shortest_name(vendor_str);

    if (strstr(vendor_str, "Intel")) {
        gchar *device_str_clean = strdup(device_str);
        nice_name_intel_gpu_device(device_str_clean);
        s->nice_name = g_strdup_printf("%s %s", vendor_str, device_str_clean);
        g_free(device_str_clean);
    } else if (strstr(vendor_str, "AMD")) {
        /* AMD PCI strings are crazy stupid because they use the exact same
         * chip and device id for a zillion "different products" */
        char *full_name = strdup(device_str);
        /* Try and shorten it to the chip code name only, at least */
        char *b = strchr(full_name, '[');
        if (b) *b = '\0';
        s->nice_name = g_strdup_printf("%s %s", "AMD/ATI", g_strstrip(full_name));
        free(full_name);
    } else {
        /* nothing nicer */
        s->nice_name = g_strdup_printf("%s %s", vendor_str, device_str);
    }

}

/*  Look for this kind of thing:
 *     * /soc/gpu
 *     * /gpu@ff300000
 *
 *  Usually a gpu dt node will have ./name = "gpu"
 */
static gchar *dt_find_gpu(dtr *dt, char *np) {
    gchar *dir_path, *dt_path, *ret;
    gchar *ftmp, *ntmp;
    const gchar *fn;
    GDir *dir;
    dtr_obj *obj;

    /* consider self */
    obj = dtr_obj_read(dt, np);
    dt_path = dtr_obj_path(obj);
    ntmp = strstr(dt_path, "/gpu");
    if (ntmp) {
        /* was found in node name */
        if ( strchr(ntmp+1, '/') == NULL) {
            ftmp = ntmp + 4;
            /* should either be NULL or @ */
            if (*ftmp == '\0' || *ftmp == '@')
                return g_strdup(dt_path);
        }
    }

    /* search children ... */
    dir_path = g_strdup_printf("%s/%s", dtr_base_path(dt), np);
    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            ftmp = g_strdup_printf("%s/%s", dir_path, fn);
            if ( g_file_test(ftmp, G_FILE_TEST_IS_DIR) ) {
                if (strcmp(np, "/") == 0)
                    ntmp = g_strdup_printf("/%s", fn);
                else
                    ntmp = g_strdup_printf("%s/%s", np, fn);
                ret = dt_find_gpu(dt, ntmp);
                g_free(ntmp);
                if (ret != NULL) {
                    g_free(ftmp);
                    g_dir_close(dir);
                    return ret;
                }
            }
            g_free(ftmp);
        }
        g_dir_close(dir);
    }

    return NULL;
}

gpud *dt_soc_gpu() {
    static const char std_soc_gpu_drm_path[] = "/sys/devices/platform/soc/soc:gpu/drm";

    /* compatible contains a list of compatible hardware, so be careful
     * with matching order.
     * ex: "ti,omap3-beagleboard-xm", "ti,omap3450", "ti,omap3";
     * matches "omap3 family" first.
     * ex: "brcm,bcm2837", "brcm,bcm2836";
     * would match 2836 when it is a 2837.
     */
    const struct {
        char *search_str;
        char *vendor;
        char *soc;
    } dt_compat_searches[] = {
        { "brcm,bcm2837-vc4", "Broadcom", "VideoCore IV" },
        { "brcm,bcm2836-vc4", "Broadcom", "VideoCore IV" },
        { "brcm,bcm2835-vc4", "Broadcom", "VideoCore IV" },
        { "arm,mali-450", "ARM", "Mali 450" },
        { "arm,mali", "ARM", "Mali family" },
        { NULL, NULL, NULL }
    };
    char tmp_path[256] = "";
    char *dt_gpu_path = NULL;
    char *compat = NULL;
    char *vendor = NULL, *device = NULL;
    int i;

    gpud *gpu = NULL;

    dtr *dt = dtr_new(NULL);
    if (!dtr_was_found(dt))
        goto dt_gpu_end;

    dt_gpu_path = dt_find_gpu(dt, "/");

    if (dt_gpu_path == NULL)
        goto dt_gpu_end;

    snprintf(tmp_path, 255, "%s/compatible", dt_gpu_path);
    compat = dtr_get_string(tmp_path, 1);

    if (compat == NULL)
        goto dt_gpu_end;

    gpu = gpud_new();

    i = 0;
    while(dt_compat_searches[i].search_str != NULL) {
        if (strstr(compat, dt_compat_searches[i].search_str) != NULL) {
            vendor = dt_compat_searches[i].vendor;
            device = dt_compat_searches[i].soc;
            break;
        }
        i++;
    }

    gpu->dt_compat = compat;
    gpu->dt_vendor = vendor;
    gpu->dt_device = device;
    gpu->dt_path = dt_gpu_path;
    snprintf(tmp_path, 255, "%s/status", dt_gpu_path);
    gpu->dt_status = dtr_get_string(tmp_path, 1);
    snprintf(tmp_path, 255, "%s/name", dt_gpu_path);
    gpu->dt_name = dtr_get_string(tmp_path, 1);
    gpu->dt_opp = dtr_get_opp_range(dt, dt_gpu_path);
    if (gpu->dt_opp) {
        gpu->khz_max = gpu->dt_opp->khz_max;
        gpu->khz_min = gpu->dt_opp->khz_min;
    }
    EMPIFNULL(gpu->dt_name);
    EMPIFNULL(gpu->dt_status);

    gpu->id = strdup("dt-soc-gpu");
    gpu->location = strdup("SOC");

    if (access(std_soc_gpu_drm_path, F_OK) != -1)
        gpu->sysfs_drm_path = strdup(std_soc_gpu_drm_path);
    if (vendor) gpu->vendor_str = strdup(vendor);
    if (device) gpu->device_str = strdup(device);
    make_nice_name(gpu);


dt_gpu_end:
    dtr_free(dt);
    return gpu;
}

gpud *gpu_get_device_list() {
    int cn = 0;
    gpud *list = NULL;

/* Can we just ask DRM someway? ... */
    /* TODO: yes. /sys/class/drm/card* */

/* Try PCI ... */
    pcid_list pci_list = pci_get_device_list(0x300,0x3ff);
    GSList *l = pci_list;

    if (l) {
        while(l) {
            pcid *curr = (pcid*)l->data;
            char *pci_loc = NULL;
            gpud *new_gpu = gpud_new();
            new_gpu->pci_dev = curr;

            pci_loc = pci_address_str(curr->domain, curr->bus, curr->device, curr->function);

            int len;
            char drm_id[512] = "", card_id[64] = "";
            char *drm_dev = NULL;
            gchar *drm_path =
                g_strdup_printf("/dev/dri/by-path/pci-%s-card", pci_loc);
            memset(drm_id, 0, 512);
            if ((len = readlink(drm_path, drm_id, sizeof(drm_id)-1)) != -1)
                drm_id[len] = '\0';
            g_free(drm_path);

            if (strlen(drm_id) != 0) {
                /* drm has the card */
                drm_dev = strstr(drm_id, "card");
                if (drm_dev)
                    snprintf(card_id, 64, "%s", drm_dev);
            }

            if (strlen(card_id) == 0) {
                /* fallback to our own counter */
                snprintf(card_id, 64, "pci-dc%d", cn);
                cn++;
            }

            if (drm_dev)
                new_gpu->drm_dev = strdup(drm_dev);

            char *sysfs_path_candidate = g_strdup_printf("%s/%s/drm", SYSFS_PCI_ROOT, pci_loc);
            if (access(sysfs_path_candidate, F_OK) != -1) {
                new_gpu->sysfs_drm_path = sysfs_path_candidate;
            } else
                free(sysfs_path_candidate);
            new_gpu->location = g_strdup_printf("PCI/%s", pci_loc);
            new_gpu->id = strdup(card_id);
            if (curr->vendor_id_str) new_gpu->vendor_str = strdup(curr->vendor_id_str);
            if (curr->device_id_str) new_gpu->device_str = strdup(curr->device_id_str);
            nv_fill_procfs_info(new_gpu);
            intel_fill_freq(new_gpu);
            amdgpu_fill_freq(new_gpu);
            make_nice_name(new_gpu);
            if (list == NULL)
                list = new_gpu;
            else
                gpud_list_append(list, new_gpu);

            free(pci_loc);
            l=l->next;
        }

        /* don't pcid_list_free(pci_list); They will be freed by gpud_free() */
        g_slist_free(pci_list); /* just the linking data */
        return list;
    }

/* Try Device Tree ... */
    list = dt_soc_gpu();
    if (list) return list;

/* Try other things ... */

    return list;
}


