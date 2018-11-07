/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
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
#include "usb_util.h"

gchar *usb_list = NULL;

#define UNKIFNULL_AC(f) (f != NULL) ? f : _("(Unknown)");

static void _usb_dev(const usbd *u) {
    gchar *name, *key, *v_str, *str;
    gchar *product, *vendor, *dev_class_str, *dev_subclass_str; /* don't free */
    gchar *if_class_str, *if_subclass_str, *if_protocol_str;    /* don't free */
    gchar *interfaces = strdup("");
    usbi *i;

    vendor = UNKIFNULL_AC(u->vendor);
    product = UNKIFNULL_AC(u->product);
    dev_class_str = UNKIFNULL_AC(u->dev_class_str);
    dev_subclass_str = UNKIFNULL_AC(u->dev_subclass_str);

    name = g_strdup_printf("%s %s", vendor, product);
    key = g_strdup_printf("USB%03d:%03d:%03d", u->bus, u->dev, 0);

    usb_list = h_strdup_cprintf("$%s$%03d:%03d=%s\n", usb_list, key, u->bus, u->dev, name);

    const gchar *v_url = vendor_get_url(vendor);
    const gchar *v_name = vendor_get_name(vendor);
    if (v_url != NULL) {
        v_str = g_strdup_printf("%s (%s)", v_name, v_url);
    } else {
        v_str = g_strdup_printf("%s", vendor );
    }

    if (u->if_list != NULL) {
        i = u->if_list;
        while (i != NULL){
            if_class_str = UNKIFNULL_AC(i->if_class_str);
            if_subclass_str = UNKIFNULL_AC(i->if_subclass_str);
            if_protocol_str = UNKIFNULL_AC(i->if_protocol_str);

            interfaces = h_strdup_cprintf("[%s %d]\n"
                /* Class */       "%s=[%d] %s\n"
                /* Sub-class */   "%s=[%d] %s\n"
                /* Protocol */    "%s=[%d] %s\n",
                    interfaces,
                    _("Interface"), i->if_number,
                    _("Class"), i->if_class, if_class_str,
                    _("Sub-class"), i->if_subclass, if_subclass_str,
                    _("Protocol"), i->if_protocol, if_protocol_str
                );
            i = i->next;
        }
    }

    str = g_strdup_printf("[%s]\n"
             /* Product */      "%s=[0x%04x] %s\n"
             /* Manufacturer */ "%s=[0x%04x] %s\n"
             /* Max Current */  "%s=%d %s\n"
             /* USB Version */ "%s=%s\n"
             /* Class */       "%s=[%d] %s\n"
             /* Sub-class */   "%s=[%d] %s\n"
             /* Dev Version */ "%s=%s\n"
                            "[%s]\n"
             /* Bus */         "%s=%03d\n"
             /* Device */      "%s=%03d\n"
             /* Interfaces */  "%s",
                _("Device Information"),
                _("Product"), u->product_id, product,
                _("Vendor"), u->vendor_id, v_str,
                _("Max Current"), u->max_curr_ma, _("mA"),
                _("USB Version"), u->usb_version,
                _("Class"), u->dev_class, dev_class_str,
                _("Sub-class"), u->dev_subclass, dev_subclass_str,
                _("Device Version"), u->device_version,
                _("Connection"),
                _("Bus"), u->bus,
                _("Device"), u->dev,
                interfaces
                );

    moreinfo_add_with_prefix("DEV", key, str); /* str now owned by morinfo */

    g_free(v_str);
    g_free(name);
    g_free(key);
    g_free(interfaces);
}

void __scan_usb(void) {
    usbd *list = usb_get_device_list();
    usbd *curr = list;

    int c = usbd_list_count(list);

    if (usb_list) {
        moreinfo_del_with_prefix("DEV:USB");
        g_free(usb_list);
    }
    usb_list = g_strdup_printf("[%s]\n", _("USB Devices"));

    if (c > 0) {
        while(curr) {
            _usb_dev(curr);
            curr=curr->next;
        }

        usbd_list_free(list);
    } else {
        /* No USB? */
        usb_list = g_strconcat(usb_list, _("No USB devices found."), "=\n", NULL);
    }
}
