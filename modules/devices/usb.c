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
gchar *usb_icons = NULL;

#define UNKIFNULL_AC(f) (f != NULL) ? f : _("(Unknown)")
#define IARR_END -2
#define IARR_ANY -1

static struct {
    int class;
    char *icon;
} usb_class_icons[] = {
    { 0x1, "audio"},            /* Audio  */
    { 0x2, "modem"},            /* Communications and CDC Control */
    { 0x3, "inputdevices"},     /* HID (Human Interface Device) */
    { 0x6, "camera-photo"},     /* Still Imaging */
    { 0x7, "printer"},          /* Printer */
    { 0x8, "media-removable"},  /* Mass storage */
    { 0x9, "usb"},              /* Hub */
    { 0xe, "camera-web"},       /* Video */
    {IARR_END, NULL}
};

static struct {
    int class, subclass, protocol;
    char *icon;
} usb_type_icons[] = {
    { 0x2,          0x6, IARR_ANY, "network-interface"},  /* Ethernet Networking Control Model */
    { 0x3,          0x1,      0x1, "keyboard"},           /* Keyboard */
    { 0x3,          0x1,      0x2, "mouse"},              /* Mouse */
    {0xe0,          0x1,      0x1, "bluetooth"},          /* Bluetooth Programming Interface */
    {IARR_END, IARR_END, IARR_END, NULL},
};

static const char* get_class_icon(int class){
    int i = 0;
    while (usb_class_icons[i].class != IARR_END) {
        if (usb_class_icons[i].class == class) {
            return usb_class_icons[i].icon;
        }
        i++;
    }
    return NULL;
}

static const char* get_usbif_icon(const usbi *usbif) {
    int i = 0;
    while (usb_type_icons[i].class != IARR_END) {
        if (usb_type_icons[i].class == usbif->if_class && usb_type_icons[i].subclass == usbif->if_subclass &&
            (usb_type_icons[i].protocol == IARR_ANY || usb_type_icons[i].protocol == usbif->if_protocol)) {

            return usb_type_icons[i].icon;
        }
        i++;
    }

    return get_class_icon(usbif->if_class);
}

static const char* get_usbdev_icon(const usbd *u) {
    const char * icon = NULL;
    usbi *curr_if;

    curr_if = u->if_list;
    while (icon == NULL && curr_if != NULL){
        icon = get_usbif_icon(curr_if);
        curr_if = curr_if->next;
    }

    if (icon == NULL){
        icon = get_class_icon(u->dev_class);
    }

    return icon;
}

static void _usb_dev(const usbd *u) {
    gchar *name, *key, *label, *str, *speed;
    gchar *product, *vendor, *manufacturer, *device;  /* don't free */
    gchar *interfaces = strdup("");
    usbi *i;
    const char* icon;

    vendor = UNKIFNULL_AC(u->vendor);
    product = UNKIFNULL_AC(u->product);
    manufacturer = UNKIFNULL_AC(u->manufacturer);
    device = UNKIFNULL_AC(u->device);

    name = g_strdup_printf("%s %s", u->vendor? vendor: manufacturer, u->product? product: device);
    key = g_strdup_printf("USB%03d:%03d:%03d", u->bus, u->dev, 0);
    label = g_strdup_printf("%03d:%03d", u->bus, u->dev);
    icon = get_usbdev_icon(u);

    usb_list = h_strdup_cprintf("$%s$%s=%s\n", usb_list, key, label, name);
    usb_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n", usb_icons, key, label, icon ? icon: "usb");

    if (u->if_list != NULL) {
        i = u->if_list;
        while (i != NULL){
            interfaces = h_strdup_cprintf("[%s %d %s]\n"
                /* Class */       "%s=[%d] %s\n"
                /* Sub-class */   "%s=[%d] %s\n"
                /* Protocol */    "%s=[%d] %s\n"
                /* Driver */      "%s=%s\n",
                    interfaces,
                    _("Interface"), i->if_number, i->if_label? i->if_label: "",
                    _("Class"), i->if_class, UNKIFNULL_AC(i->if_class_str),
                    _("Sub-class"), i->if_subclass, UNKIFNULL_AC(i->if_subclass_str),
                    _("Protocol"), i->if_protocol, UNKIFNULL_AC(i->if_protocol_str),
                    _("Driver"), UNKIFNULL_AC(i->driver)
                );
            i = i->next;
        }
    }

    if (u->speed_mbs > 0){
        speed = g_strdup_printf("%d %s", u->speed_mbs, _("Mb/s"));
    }
    else{
        speed = g_strdup(_("Unknown"));
    }

    str = g_strdup_printf("[%s]\n"
             /* Product */      "%s=[0x%04x] %s\n"
             /* Vendor */       "$^$%s=[0x%04x] %s\n"
             /* Device */       "%s=%s\n"
             /* Manufacturer */ "$^$%s=%s\n"
             /* Max Current */  "%s=%d %s\n"
             /* USB Version */ "%s=%s\n"
             /* Speed */       "%s=%s\n"
             /* Class */       "%s=[%d] %s\n"
             /* Sub-class */   "%s=[%d] %s\n"
             /* Protocol */    "%s=[%d] %s\n"
             /* Dev Version */ "%s=%s\n"
             /* Serial */      "%s=%s\n"
                            "[%s]\n"
             /* Bus */         "%s=%03d\n"
             /* Device */      "%s=%03d\n"
             /* Interfaces */  "%s",
                _("Device Information"),
                _("Product"), u->product_id, product,
                _("Vendor"), u->vendor_id, vendor,
                _("Device"), device,
                _("Manufacturer"), manufacturer,
                _("Max Current"), u->max_curr_ma, _("mA"),
                _("USB Version"), u->usb_version,
                _("Speed"), speed,
                _("Class"), u->dev_class, UNKIFNULL_AC(u->dev_class_str),
                _("Sub-class"), u->dev_subclass, UNKIFNULL_AC(u->dev_subclass_str),
                _("Protocol"), u->dev_protocol, UNKIFNULL_AC(u->dev_protocol_str),
                _("Device Version"), UNKIFNULL_AC(u->device_version),
                _("Serial Number"), UNKIFNULL_AC(u->serial),
                _("Connection"),
                _("Bus"), u->bus,
                _("Device"), u->dev,
                interfaces
                );

    moreinfo_add_with_prefix("DEV", key, str); /* str now owned by morinfo */

    g_free(speed);
    g_free(name);
    g_free(key);
    g_free(label);
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
    if (usb_icons){
       g_free(usb_icons);
       usb_icons = NULL;
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
