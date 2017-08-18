/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    This file
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
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
#include "usb_util.h"

usbd *usbd_new() {
    usbd *s = malloc(sizeof(usbd));
    if (s) {
        memset(s, 0, sizeof(usbd));
    }
    return s;
}

void usbd_free(usbd *s) {
    if (s) {
        free(s->vendor);
        free(s->product);
        free(s->usb_version);
        free(s);
    }
}

void usbd_list_free(usbd *s) {
    usbd *n;
    while(s != NULL) {
        n = s->next;
        usbd_free(s);
        s = n;
    }
}

/* returns number of items after append */
int usbd_list_append(usbd *l, usbd *n) {
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

int usbd_list_count(usbd *s) {
    return usbd_list_append(s, NULL);
}

char *_lsusb_lv(char *line, const char *prefix) {
    if ( g_str_has_prefix(line, prefix) ) {
        line += strlen(prefix) + 1;
        return g_strstrip(line);
    } else
        return NULL;
}

int usb_get_device_lsusb(int bus, int dev, usbd *s) {
    gboolean spawned;
    gchar *out, *err, *p, *l, *t, *next_nl;
    gchar *lsusb_cmd = g_strdup_printf("lsusb -s %d:%d -v", bus, dev);

    s->bus = bus;
    s->dev = dev;

    spawned = g_spawn_command_line_sync(lsusb_cmd,
            &out, &err, NULL, NULL);
    if (spawned) {
        if (strstr(err, "information will be missing")) {
            s->user_scan = 1;
        }
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            g_strstrip(p);
            if (l = _lsusb_lv(p, "idVendor")) {
                s->vendor_id = strtol(l, NULL, 0);
                if (t = strchr(l, ' '))
                    s->vendor = strdup(t + 1);
            } else if (l = _lsusb_lv(p, "idProduct")) {
                s->product_id = strtol(l, NULL, 0);
                if (t = strchr(l, ' '))
                    s->product = strdup(t + 1);
            } else if (l = _lsusb_lv(p, "MaxPower")) {
                s->max_curr_ma = atoi(l);
            } else if (l = _lsusb_lv(p, "bcdUSB")) {
                s->usb_version = strdup(l);
            } else if (l = _lsusb_lv(p, "bDeviceClass")) {
                s->dev_class = atoi(l);
            } else if (l = _lsusb_lv(p, "bDeviceSubClass")) {
                s->dev_subclass = atoi(l);
            }
            /* TODO: speed_mbs
             * WISHLIST: interfaces, drivers */
            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }
    g_free(lsusb_cmd);
}

usbd *usb_get_device(int bus, int dev) {
    usbd *s = usbd_new();
    int ok = 0;
    if (s) {
        /* try lsusb */
        ok = usb_get_device_lsusb(bus, dev, s);

        /* TODO: other methods */

        if (!ok) {
            usbd_free(s);
            s = NULL;
        }
    }
    return s;
}

usbd *usb_get_device_list_lsusb() {
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;
    usbd *head = NULL, *nd;
    int bus, dev, vend, prod;

    spawned = g_spawn_command_line_sync("lsusb",
            &out, &err, NULL, NULL);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            if ( sscanf(p, "Bus %d Device %d: ID %x:%x", &bus, &dev, &vend, &prod) ) {
                nd = usb_get_device(bus, dev);
                if (head == NULL) {
                    head = nd;
                } else {
                    usbd_list_append(head, nd);
                }
            }
            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }
    return head;
}

usbd *usb_get_device_list() {
    /* try lsusb */
    return usb_get_device_list_lsusb();

    /* TODO: other methods */
}
