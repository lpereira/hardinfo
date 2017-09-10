/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
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

    int dev_class;
    int dev_subclass;
    char *dev_class_str;
    char *dev_subclass_str;

    char *usb_version;
    char *device_version; /* bcdDevice */

    int max_curr_ma;

    int speed_mbs; /* TODO: */

    gboolean user_scan; /* not scanned as root */

    struct usbd *next;
} usbd;

usbd *usb_get_device_list();
int usbd_list_count(usbd *);
void usbd_list_free(usbd *);

usbd *usb_get_device(int bus, int dev);
void usbd_free(usbd *);

#endif
