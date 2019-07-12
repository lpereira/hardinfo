/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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
#include "pci_util.h"

#define UNKIFNULL_AC(f) (f != NULL) ? f : _("(Unknown)");

static const struct {
    const gchar *icon;
    uint32_t class;
} class2icon[] = {
    { .class = 0x0200, .icon = "network-interface.png" },
    { .class = 0x0c03, .icon = "usb.png" },
    { .class = 0x0403, .icon = "audio.png" },
    { .class = 0x0805, .icon = "usbfldisk.png" },
    { .class = 0x0d11, .icon = "bluetooth.png" },
    { .class = 0x0703, .icon = "modem.png" },
    { .class = 0x01, .icon = "hdd.png" },
    { .class = 0x02, .icon = "network.png" },
    { .class = 0x03, .icon = "monitor.png" },
    { .class = 0x05, .icon = "memory.png" },
    { .class = 0x07, .icon = "network-connections.png" },
    { .class = 0x09, .icon = "inputdevices.png" },
    { .class = 0x10, .icon = "cryptohash.png" },
};

static const gchar *find_icon_for_class(uint32_t class)
{
    int i;

    for (i = 0; i < G_N_ELEMENTS(class2icon); i++) {
	if (class2icon[i].class <= 0xff) {
		if ((class & 0xff00) == class2icon[i].class << 8)
		    return class2icon[i].icon;
	} else if (class == class2icon[i].class) {
            return class2icon[i].icon;
	}
    }

    return "devices.png";
}

static gchar *_pci_dev(const pcid *p, gchar *icons) {
    gchar *str;
    gchar *class, *vendor, *svendor, *v_str, *sv_str, *product, *sproduct;
    gchar *name, *key;

    class = UNKIFNULL_AC(p->class_str);
    vendor = UNKIFNULL_AC(p->vendor_id_str);
    svendor = UNKIFNULL_AC(p->sub_vendor_id_str);
    product = UNKIFNULL_AC(p->device_id_str);
    sproduct = UNKIFNULL_AC(p->sub_device_id_str);

    v_str = vendor_get_link(vendor);
    sv_str = vendor_get_link(svendor);

    name = g_strdup_printf("%s %s", vendor, product);
    key = g_strdup_printf("PCI%04x:%02x:%02x.%01x", p->domain, p->bus, p->device, p->function);

    pci_list = h_strdup_cprintf("$%s$%04x:%02x:%02x.%01x=%s\n", pci_list, key, p->domain, p->bus, p->device, p->function, name);
    icons = h_strdup_cprintf("Icon$%s$%04x:%02x:%02x.%01x=%s\n", icons, key, p->domain, p->bus, p->device, p->function, find_icon_for_class(p->class));

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
                     /* Width */        "%s=x%u\n"
                     /* Width (max) */  "%s=x%u\n"
                     /* Speed */        "%s=%0.1f %s\n"
                     /* Speed (max) */  "%s=%0.1f %s\n",
                    _("PCI Express"),
                    _("Link Width"), p->pcie_width_curr,
                    _("Maximum Link Width"), p->pcie_width_max,
                    _("Link Speed"), p->pcie_speed_curr,  _("GT/s"),
                    _("Maximum Link Speed"), p->pcie_speed_max, _("GT/s") );
    } else
        pcie_str = strdup("");

    str = g_strdup_printf("[%s]\n"
             /* Class */     "%s=[%04x] %s\n"
                             "%s"
             /* Revision */  "%s=%02x\n"
             /* PCIE? */     "%s"
                             "[%s]\n"
            /* Driver */     "%s=%s\n"
            /* Modules */    "%s=%s\n"
                             "[%s]\n"
            /* Domain */     "%s=%04x\n"
            /* Bus */        "%s=%02x\n"
            /* Device */     "%s=%02x\n"
            /* Function */   "%s=%01x\n",
                _("Device Information"),
                _("Class"), p->class, class,
                vendor_device_str,
                _("Revision"), p->revision,
                pcie_str,
                _("Driver"),
                _("In Use"), (p->driver) ? p->driver : _("(Unknown)"),
                _("Kernel Modules"), (p->driver_list) ? p->driver_list : _("(Unknown)"),
                _("Connection"),
                _("Domain"), p->domain,
                _("Bus"), p->bus,
                _("Device"), p->device,
                _("Function"), p->function
                );

    g_free(pcie_str);

    moreinfo_add_with_prefix("DEV", key, str); /* str now owned by morinfo */

    g_free(vendor_device_str);
    g_free(v_str);
    g_free(sv_str);
    g_free(name);
    g_free(key);

    return icons;
}

void scan_pci_do(void) {

    if (pci_list) {
        moreinfo_del_with_prefix("DEV:PCI");
        g_free(pci_list);
    }
    pci_list = g_strdup_printf("[%s]\n", _("PCI Devices"));

    gchar *pci_icons = g_strdup("");

    pcid *list = pci_get_device_list(0,0);
    pcid *curr = list;

    int c = pcid_list_count(list);

    if (c > 0) {
        while(curr) {
            pci_icons = _pci_dev(curr, pci_icons);
            curr=curr->next;
        }

        pcid_list_free(list);
    }

    if (c) {
        pci_list = g_strconcat(pci_list, "[$ShellParam$]\n", "ViewType=1\n", pci_icons, NULL);
    } else  {
        /* NO PCI? */
        pci_list = g_strconcat(pci_list, _("No PCI devices found"), "=\n", NULL);
    }

    g_free(pci_icons);
}
