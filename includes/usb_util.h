/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
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

#ifndef __USB_UTIL_H__
#define __USB_UTIL_H__

/* this is a linked list */
typedef struct usbd {
    int bus, dev;
    int vendor_id, product_id;
    char *vendor;
    char *product;
    char *manufacturer;
    char *device;
    char *serial;

    int dev_class;
    int dev_subclass;
    int dev_protocol;
    char *dev_class_str;
    char *dev_subclass_str;
    char *dev_protocol_str;

    char *usb_version;
    char *device_version; /* bcdDevice */

    int max_curr_ma;

    int speed_mbs;

    vendor_list vendors;

    gboolean user_scan; /* not scanned as root */
    struct usbi *if_list;

    struct usbd *next;
} usbd;

/* another linked list */
typedef struct usbi {
    int if_number;
    int if_class;
    int if_subclass;
    int if_protocol;
    char *if_label;
    char *if_class_str;
    char *if_subclass_str;
    char *if_protocol_str;
    char *driver;

    struct usbi *next;
} usbi;

usbd *usb_get_device_list();
int usbd_list_count(usbd *);
void usbd_list_free(usbd *);
void usb_lookup_ids_vendor_product_str(gint vendor_id, gint product_id,
                                       gchar **vendor_str, gchar **product_str);

usbd *usb_get_device(int bus, int dev, const gchar* sysfspath);
void usbd_free(usbd *);

#endif
