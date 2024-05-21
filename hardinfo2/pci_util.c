/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
 *    This file
 *    Copyright (C) 2018 Burt P. <pburt0@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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
#include "pci_util.h"
#include "util_ids.h"

gchar *pci_ids_file = NULL;
GTimer *pci_ids_timer = NULL;
const gboolean nolspci = FALSE; /* true for testing */

/* Two pieces of info still only from lspci:
 * - kernel driver in use
 * - kernel modules list
 *
 * TODO: could use readlink() and basename() to get kernel driver from sysfs
 * - /sys/bus/pci/devices/<addy>/driver is a symlink
 */

const gchar *find_pci_ids_file() {
    if (pci_ids_file) {
        if (!strstr(pci_ids_file, ".min"))
            return pci_ids_file;
        if (g_timer_elapsed(pci_ids_timer, NULL) > 2.0) {
            /* try again for the full version */
            DEBUG("find_pci_ids_file() found only a \".min\" version, trying again...");
            g_free(pci_ids_file);
            pci_ids_file = NULL;
        }
    }
    char *file_search_order[] = {
        g_build_filename(g_get_user_config_dir(), "hardinfo2", "pci.ids", NULL),
        g_build_filename(params.path_data, "pci.ids", NULL),
        NULL
    };
    int n;
    for(n = 0; file_search_order[n]; n++) {
        if (!pci_ids_file && !access(file_search_order[n], R_OK))
            pci_ids_file = file_search_order[n];
        else
            g_free(file_search_order[n]);
    }
    DEBUG("find_pci_ids_file() result: %s", pci_ids_file);
    if (pci_ids_file) {
        if (!pci_ids_timer)
            pci_ids_timer = g_timer_new();
        else
            g_timer_reset(pci_ids_timer);
    }
    return pci_ids_file;
}

char *pci_lookup_ids_vendor_str(uint32_t id) {
    gchar *ret = NULL;

    ids_query_result result;// = {};
    gchar *qpath;
    memset(&result,0,sizeof(ids_query_result));
    if (!find_pci_ids_file())
        return FALSE;

    qpath = g_strdup_printf("%04x", id);
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        ret = g_strdup(result.results[0]);
    }
    g_free(qpath);

    return ret;
}

static gboolean pci_lookup_ids(pcid *d) {
    gboolean ret = FALSE;
    ids_query_result result;// = {};
    gchar *qpath;
    memset(&result,0,sizeof(ids_query_result));
    if (!find_pci_ids_file())
        return FALSE;

    /* lookup vendor, device, sub device */
    qpath = g_strdup_printf("%04x/%04x/%04x %04x",
        d->vendor_id, d->device_id, d->sub_vendor_id, d->sub_device_id);
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        if (d->vendor_id_str) g_free(d->vendor_id_str);
        d->vendor_id_str = g_strdup(result.results[0]);
        ret = TRUE;
    }
    if (result.results[1]) {
        if (d->device_id_str) g_free(d->device_id_str);
        d->device_id_str = g_strdup(result.results[1]);
        ret = TRUE;
    }
    if (result.results[2]) {
        if (d->sub_device_id_str) g_free(d->sub_device_id_str);
        d->sub_device_id_str = g_strdup(result.results[2]);
        ret = TRUE;
    }
    g_free(qpath);

    /* lookup sub vendor by itself */
    qpath = g_strdup_printf("%04x", d->sub_vendor_id);
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        if (d->sub_vendor_id_str) g_free(d->sub_vendor_id_str);
        d->sub_vendor_id_str = g_strdup(result.results[0]);
        ret = TRUE;
    };
    g_free(qpath);

    /* lookup class */
    qpath = g_strdup_printf("C %02x/%02x", (d->class >> 8) & 0xff, (d->class & 0xff));
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        if (d->class_str) g_free(d->class_str);
        d->class_str = g_strdup(result.results[0]);
        if (result.results[1]
            && !SEQ(result.results[0], result.results[1]) ) {
                /* options 1: results[0] + " :: " + results[1] */
                //d->class_str = appf(d->class_str, " :: ", "%s", result.results[1]);

                /* option 2: results[1] or results[0] */
                g_free(d->class_str);
                d->class_str = g_strdup(result.results[1]);
        }
        ret = TRUE;
    }
    g_free(qpath);

    return ret;
}

gint pcid_cmp_by_addy(gconstpointer a, gconstpointer b)
{
    const struct pcid *dev_a = a;
    const struct pcid *dev_b = b;

    if (!dev_a)
        return !!dev_b;
    if (!dev_b)
        return !!dev_a;

    return g_strcmp0(dev_a->slot_str, dev_b->slot_str);
}

void pcid_free(pcid *s) {
    if (s) {
        g_free(s->slot_str);
        g_free(s->class_str);
        g_free(s->vendor_id_str);
        g_free(s->device_id_str);
        g_free(s->sub_vendor_id_str);
        g_free(s->sub_device_id_str);
        g_free(s->driver);
        g_free(s->driver_list);
        g_free(s);
    }
}

static char *lspci_line_value(char *line, const char *prefix) {
    if (g_str_has_prefix(g_strstrip(line), prefix)) {
        line += strlen(prefix) + 1;
        return g_strstrip(line);
    } else
        return NULL;
}

/* read output line of lspci -vmmnn */
static int lspci_line_string_and_code(char *line, char *prefix, char **str, uint32_t *code) {
    char *l = lspci_line_value(line, prefix);
    char *e;

    if (l) {
        e = strchr(l, 0);
        while (e > l && *e != '[') e--;
        sscanf(e, "[%x]", code);
        *e = 0; /* terminate at start of code */
        if (*str) free(*str); /* free if replacing */
        *str = strdup(g_strstrip(l));
    }
    return 0;
}

static gboolean pci_fill_details(pcid *s) {
    if (nolspci) return FALSE;
    gboolean spawned;
    gchar *out, *err, *p, *l, *next_nl;
    gchar *pci_loc = pci_address_str(s->domain, s->bus, s->device, s->function);
    gchar *lspci_cmd = g_strdup_printf("lspci -D -s %s -vvv", pci_loc);

    spawned = hardinfo_spawn_command_line_sync(lspci_cmd,
            &out, &err, NULL, NULL);
    g_free(lspci_cmd);
    g_free(pci_loc);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            g_strstrip(p);
            if (l = lspci_line_value(p, "Kernel driver in use")) {
                s->driver = g_strdup(l);
                goto pci_details_next;
            }
            if (l = lspci_line_value(p, "Kernel modules")) {
                s->driver_list = g_strdup(l);
                goto pci_details_next;
            }
            /* TODO: more details */

            pci_details_next:
                p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
        return TRUE;
    }
    return FALSE;
}

char *pci_address_str(uint32_t dom, uint32_t bus, uint32_t  dev, uint32_t func) {
    return g_strdup_printf("%04x:%02x:%02x.%01x", dom, bus, dev, func);
}

/* /sys/bus/pci/devices/0000:01:00.0/ */
char *_sysfs_bus_pci(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, const char *item) {
    char *ret = NULL, *pci_loc, *sysfs_path;
    pci_loc = pci_address_str(dom, bus, dev, func);
    sysfs_path = g_strdup_printf("%s/%s/%s", SYSFS_PCI_ROOT, pci_loc, item);
    g_file_get_contents(sysfs_path, &ret, NULL, NULL);
    free(pci_loc);
    free(sysfs_path);
    return ret;
}

gboolean _sysfs_bus_pci_read_hex(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, const char *item, uint32_t *val) {
    char *tmp = _sysfs_bus_pci(dom, bus, dev, func, item);
    uint32_t tval;
    if (tmp && val) {
        int ec = sscanf(tmp, "%x", &tval);
        free(tmp);
        if (ec==1) {
            *val = tval;
            return TRUE;
        }
    }
    return FALSE;
}

/* https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-pci */
static gboolean pci_get_device_sysfs(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, pcid *s) {
    char *tmp = NULL;
    int ec = 0;
    float tf;
    s->domain = dom;
    s->bus = bus;
    s->device = dev;
    s->function = func;
    s->slot_str = s->slot_str ? s->slot_str : pci_address_str(dom, bus, dev, func);
    if (! _sysfs_bus_pci_read_hex(dom, bus, dev, func, "class", &s->class) )
        return FALSE;
    s->class >>= 8; /* TODO: find out why */
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "device", &s->device_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "vendor", &s->vendor_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "subsystem_device", &s->sub_device_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "subsystem_vendor", &s->sub_vendor_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "revision", &s->revision);

    tmp = _sysfs_bus_pci(dom, bus, dev, func, "max_link_speed");
    if (tmp) {
        ec = sscanf(tmp, "%f GT/s", &tf);
        if (ec==1) s->pcie_speed_max = tf;
        free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "current_link_speed");
    if (tmp) {
        ec = sscanf(tmp, "%f GT/s", &tf);
        if (ec==1) s->pcie_speed_curr = tf;
        free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "max_link_width");
    if (tmp) {
        s->pcie_width_max = strtoul(tmp, NULL, 0);
        free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "current_link_width");
    if (tmp) {
        s->pcie_width_curr = strtoul(tmp, NULL, 0);
        free(tmp);
    }
    return TRUE;
}

static gboolean pci_get_device_lspci(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, pcid *s) {
    if (nolspci) return FALSE;
    gboolean spawned;
    gchar *out, *err, *p, *l, *next_nl;
    gchar *pci_loc = pci_address_str(dom, bus, dev, func);
    gchar *lspci_cmd = g_strdup_printf("lspci -D -s %s -vmmnn", pci_loc);

    s->domain = dom;
    s->bus = bus;
    s->device = dev;
    s->function = func;

    spawned = hardinfo_spawn_command_line_sync(lspci_cmd,
            &out, &err, NULL, NULL);
    g_free(lspci_cmd);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            g_strstrip(p);
            if (l = lspci_line_value(p, "Slot")) {
                s->slot_str = g_strdup(l);
                if (strcmp(s->slot_str, pci_loc) != 0) {
                   printf("PCI: %s != %s\n", s->slot_str, pci_loc);
                }
            }
            if (l = lspci_line_value(p, "Rev")) {
                s->revision = strtol(l, NULL, 16);
            }
            lspci_line_string_and_code(p, "Class", &s->class_str, &s->class);
            lspci_line_string_and_code(p, "Vendor", &s->vendor_id_str, &s->vendor_id);
            lspci_line_string_and_code(p, "Device", &s->device_id_str, &s->device_id);
            lspci_line_string_and_code(p, "SVendor", &s->sub_vendor_id_str, &s->sub_vendor_id);
            lspci_line_string_and_code(p, "SDevice", &s->sub_device_id_str, &s->sub_device_id);

            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
        g_free(pci_loc);
        return TRUE;
    }
    g_free(pci_loc);
    return FALSE;
}

pcid *pci_get_device_str(const char *addy) {
    uint32_t dom, bus, dev, func;
    int ec;
    if (addy) {
        ec = sscanf(addy, "%x:%x:%x.%x", &dom, &bus, &dev, &func);
        if (ec == 4) {
            return pci_get_device(dom, bus, dev, func);
        }
    }
    return NULL;
}

pcid *pci_get_device(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func) {
    pcid *s = pcid_new();
    gboolean ok = FALSE;
    if (s) {
        ok = pci_get_device_sysfs(dom, bus, dev, func, s);
        if (ok) {
            ok |= pci_lookup_ids(s);
            if (!ok)
                ok |= pci_get_device_lspci(dom, bus, dev, func, s);
        }
        if (!ok) {
            pcid_free(s);
            s = NULL;
        }
    }
    return s;
}

static pcid_list pci_get_device_list_lspci(uint32_t class_min, uint32_t class_max) {
    if (nolspci) return NULL;
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;
    pcid_list dl = NULL;
    pcid *nd;
    uint32_t dom, bus, dev, func, cls;
    int ec;

    if (class_max == 0) class_max = 0xffff;

    spawned = hardinfo_spawn_command_line_sync("lspci -D -mn",
            &out, &err, NULL, NULL);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            ec = sscanf(p, "%x:%x:%x.%x \"%x\"", &dom, &bus, &dev, &func, &cls);
            if (ec == 5) {
                if (cls >= class_min && cls <= class_max) {
                    nd = pci_get_device(dom, bus, dev, func);
                    pci_fill_details(nd);
                    dl = g_slist_append(dl, nd);
                }
            }
            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }
    return dl;
}

static pcid_list pci_get_device_list_sysfs(uint32_t class_min, uint32_t class_max) {
    pcid_list dl = NULL;
    pcid *nd;
    uint32_t dom, bus, dev, func, cls;
    int ec;

    if (class_max == 0) class_max = 0xffff;

    const gchar *f = NULL;
    GDir *d = g_dir_open("/sys/bus/pci/devices", 0, NULL);
    if (!d) return 0;

    while((f = g_dir_read_name(d))) {
        ec = sscanf(f, "%x:%x:%x.%x", &dom, &bus, &dev, &func);
        if (ec == 4) {
            gchar *cf = g_strdup_printf("/sys/bus/pci/devices/%s/class", f);
            gchar *cstr = NULL;
            if (g_file_get_contents(cf, &cstr, NULL, NULL) ) {
                cls = strtoul(cstr, NULL, 16) >> 8;
                if (cls >= class_min && cls <= class_max) {
                    nd = pci_get_device(dom, bus, dev, func);
                    pci_fill_details(nd);
                    dl = g_slist_append(dl, nd);
                }
            }
            g_free(cstr);
            g_free(cf);
        }
    }
    g_dir_close(d);
    return dl;
}

pcid_list pci_get_device_list(uint32_t class_min, uint32_t class_max) {
    pcid_list dl = NULL;
    dl = pci_get_device_list_sysfs(class_min, class_max);
    if (!dl)
        dl = pci_get_device_list_lspci(class_min, class_max);
    return dl;
}

