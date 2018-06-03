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

static void _pci_dev(const pcid *p) {
    gchar *str;
    gchar *class, *vendor, *svendor, *v_str, *sv_str, *product, *sproduct;
    gchar *name, *key;

    class = UNKIFNULL_AC(p->class_str);
    vendor = UNKIFNULL_AC(p->vendor_id_str);
    svendor = UNKIFNULL_AC(p->sub_vendor_id_str);
    product = UNKIFNULL_AC(p->device_id_str);
    sproduct = UNKIFNULL_AC(p->sub_device_id_str);

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
    key = g_strdup_printf("PCI%04x:%02x:%02x.%01x", p->domain, p->bus, p->device, p->function);

    pci_list = h_strdup_cprintf("$%s$%04x:%02x:%02x.%01x=%s\n", pci_list, key, p->domain, p->bus, p->device, p->function, name);

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
}

void scan_pci_do(void) {

    if (pci_list) {
        moreinfo_del_with_prefix("DEV:PCI");
        g_free(pci_list);
    }
    pci_list = g_strdup_printf("[%s]\n", _("PCI Devices"));

    pcid *list = pci_get_device_list(0,0);
    pcid *curr = list;

    int c = pcid_list_count(list);

    if (c > 0) {
        while(curr) {
            _pci_dev(curr);
            curr=curr->next;
        }

        pcid_list_free(list);
        return;
    }

    /* NO PCI? */
    pci_list = g_strconcat(pci_list, _("No PCI devices found"), "=\n", NULL);
}
