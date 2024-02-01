/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
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

#ifndef __PCI_UTIL_H__
#define __PCI_UTIL_H__

#include <stdint.h>

#define SYSFS_PCI_ROOT "/sys/bus/pci/devices"

char *pci_address_str(uint32_t dom, uint32_t bus, uint32_t  dev, uint32_t func);

typedef struct pcid {
    uint32_t domain;
    uint32_t bus;
    uint32_t device;
    uint32_t function;
    uint32_t class;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t sub_vendor_id;
    uint32_t sub_device_id;
    uint32_t revision;
    char *slot_str;
    char *class_str;
    char *vendor_id_str;
    char *device_id_str;
    char *sub_vendor_id_str;
    char *sub_device_id_str;

    char *driver; /* Kernel driver in use */
    char *driver_list; /* Kernel modules */

    float pcie_speed_max;   /* GT/s */
    float pcie_speed_curr;  /* GT/s */
    uint32_t pcie_width_max;
    uint32_t pcie_width_curr;

    /* ... */

} pcid;

pcid *pci_get_device(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func);
pcid *pci_get_device_str(const char *addy);
#define pcid_new() g_new0(pcid, 1)
gint pcid_cmp_by_addy(gconstpointer a, gconstpointer b);
void pcid_free(pcid *);

typedef GSList* pcid_list;
/* examples:
 * to get all pci devices:
 *    pcid_list list = pci_get_device_list(0, 0);
 * to get all display controllers:
 *    pcid_list list = pci_get_device_list(0x300, 0x3ff);
 */
pcid_list pci_get_device_list(uint32_t class_min, uint32_t class_max);
#define pcid_list_count(l) g_slist_length(l);
#define pcid_list_free(l) g_slist_free_full(l, (GDestroyNotify)pcid_free)

const gchar *find_pci_ids_file();
char *pci_lookup_ids_vendor_str(uint32_t id);

#endif
