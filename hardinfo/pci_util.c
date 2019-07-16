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

#include "hardinfo.h"
#include "pci_util.h"

pcid *pcid_new() {
    return g_new0(pcid, 1);
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

void pcid_list_free(pcid *s) {
    pcid *n;
    while(s != NULL) {
        n = s->next;
        pcid_free(s);
        s = n;
    }
}

/* returns number of items after append */
static int pcid_list_append(pcid *l, pcid *n) {
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

int pcid_list_count(pcid *s) {
    return pcid_list_append(s, NULL);
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
        if (ec) {
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
        if (ec) s->pcie_speed_max = tf;
        free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "current_link_speed");
    if (tmp) {
        ec = sscanf(tmp, "%f GT/s", &tf);
        if (ec) s->pcie_speed_curr = tf;
        free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "max_link_width");
    if (tmp) {
        s->pcie_width_max = strtoul(tmp, NULL, 0);
        free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "max_link_width");
    if (tmp) {
        s->pcie_width_curr = strtoul(tmp, NULL, 0);
        free(tmp);
    }
    return TRUE;
}

static gboolean pci_get_device_lspci(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, pcid *s) {
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

pcid *pci_get_device(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func) {
    pcid *s = pcid_new();
    gboolean ok = FALSE;
    if (s) {
        ok = pci_get_device_sysfs(dom, bus, dev, func, s);
        ok |= pci_get_device_lspci(dom, bus, dev, func, s);
        /* TODO: other methods */

        if (!ok) {
            pcid_free(s);
            s = NULL;
        }
    }
    return s;
}

static pcid *pci_get_device_list_lspci(uint32_t class_min, uint32_t class_max) {
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;
    pcid *head = NULL, *nd;
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

                    if (head == NULL) {
                        head = nd;
                    } else {
                        pcid_list_append(head, nd);
                    }
                }
            }
            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }
    return head;
}

pcid *pci_get_device_list(uint32_t class_min, uint32_t class_max) {
    /* try lspci */
    return pci_get_device_list_lspci(class_min, class_max);
    /* TODO: other methods */
}

