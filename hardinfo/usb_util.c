/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    This file
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
 *    Copyright (C) 2019 Ondrej Čerman
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

#define SYSFS_DIR_USB_DEVICES "/sys/bus/usb/devices"

usbi *usbi_new() {
    return g_new0(usbi, 1);
}

void usbi_free(usbi *s) {
    if (s) {
        g_free(s->if_class_str);
        g_free(s->if_subclass_str);
        g_free(s->if_protocol_str);
        g_free(s->driver);
        g_free(s);
    }
}

void usbi_list_free(usbi *s) {
    usbi *n;
    while(s != NULL) {
        n = s->next;
        usbi_free(s);
        s = n;
    }
}

usbd *usbd_new() {
    return g_new0(usbd, 1);
}

void usbd_free(usbd *s) {
    if (s) {
        usbi_list_free(s->if_list);
        g_free(s->vendor);
        g_free(s->product);
        g_free(s->manufacturer);
        g_free(s->device);
        g_free(s->usb_version);
        g_free(s->device_version);
        g_free(s->dev_class_str);
        g_free(s->dev_subclass_str);
        g_free(s);
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
static int usbd_list_append(usbd *l, usbd *n) {
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

void usbd_append_interface(usbd *dev, usbi *new_if){
    usbi *curr_if;
    if (dev->if_list == NULL){
        dev->if_list = new_if;
    return;
    }

    curr_if = dev->if_list;
    while(curr_if->next != NULL) {
        curr_if = curr_if->next;
    }
    curr_if->next = new_if;
}

int usbd_list_count(usbd *s) {
    return usbd_list_append(s, NULL);
}

static char *lsusb_line_value(char *line, const char *prefix) {
    if (g_str_has_prefix(line, prefix)) {
        line += strlen(prefix) + 1;
        return g_strstrip(line);
    } else
        return NULL;
}

static gboolean usb_get_device_lsusb(int bus, int dev, usbd *s) {
    gboolean spawned;
    gchar *out, *err, *p, *l, *t, *next_nl;
    gchar *lsusb_cmd = g_strdup_printf("lsusb -s %d:%d -v", bus, dev);
    usbi *curr_if = NULL; // do not free

    s->bus = bus;
    s->dev = dev;

    spawned = g_spawn_command_line_sync(lsusb_cmd,
            &out, &err, NULL, NULL);
    g_free(lsusb_cmd);
    if (spawned) {
        if (strstr(err, "information will be missing")) {
            s->user_scan = TRUE;
        }
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            g_strstrip(p);

            // device info
            if (l = lsusb_line_value(p, "idVendor")) {
                s->vendor_id = strtol(l, NULL, 0);
                if (t = strchr(l, ' '))
                    s->vendor = g_strdup(t + 1);
            } else if (l = lsusb_line_value(p, "idProduct")) {
                s->product_id = strtol(l, NULL, 0);
                if (t = strchr(l, ' '))
                    s->product = g_strdup(t + 1);
            } else if (l = lsusb_line_value(p, "MaxPower")) {
                s->max_curr_ma = atoi(l);
            } else if (l = lsusb_line_value(p, "bcdUSB")) {
                s->usb_version = g_strdup(l);
            } else if (l = lsusb_line_value(p, "bcdDevice")) {
                s->device_version = g_strdup(l);
            } else if (l = lsusb_line_value(p, "bDeviceClass")) {
                s->dev_class = atoi(l);
                if (t = strchr(l, ' '))
                    s->dev_class_str = g_strdup(t + 1);
            } else if (l = lsusb_line_value(p, "bDeviceSubClass")) {
                s->dev_subclass = atoi(l);
                if (t = strchr(l, ' '))
                    s->dev_subclass_str = g_strdup(t + 1);

            // interface info
            } else if (l = lsusb_line_value(p, "bInterfaceNumber")) {
                curr_if = usbi_new();
                curr_if->if_number = atoi(l);
                usbd_append_interface(s, curr_if);
            } else if (l = lsusb_line_value(p, "bInterfaceClass")) {
                if (curr_if != NULL){
                    curr_if->if_class = atoi(l);
                    if (t = strchr(l, ' '))
                        curr_if->if_class_str = g_strdup(t + 1);
                }
            } else if (l = lsusb_line_value(p, "bInterfaceSubClass")) {
                if (curr_if != NULL){
                    curr_if->if_subclass = atoi(l);
                    if (t = strchr(l, ' '))
                        curr_if->if_subclass_str = g_strdup(t + 1);
                }
            } else if (l = lsusb_line_value(p, "bInterfaceProtocol")) {
                if (curr_if != NULL){
                    curr_if->if_protocol = atoi(l);
                    if (t = strchr(l, ' '))
                        curr_if->if_protocol_str = g_strdup(t + 1);
                }
            }

            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
        return TRUE;
    }
    return FALSE;
}

static gboolean usb_get_interface_sysfs(int conf, int number,
                                        const char* devpath, usbi *intf){
    gchar *ifpath, *drvpath, *tmp;

    ifpath = g_strdup_printf("%s:%d.%d", devpath, conf, number);
    if (!g_file_test(ifpath, G_FILE_TEST_EXISTS)){
        return FALSE;
    }

    tmp = g_strdup_printf("%s/driver", ifpath);
    drvpath = g_file_read_link(tmp, NULL);
    g_free(tmp);
    if (drvpath){
        intf->driver = g_path_get_basename(drvpath);
        g_free(drvpath);
    }
    else{
        intf->driver = g_strdup(_("(None)"));
    }

    intf->if_number = number;
    intf->if_class = h_sysfs_read_hex(ifpath, "bInterfaceClass");
    intf->if_subclass = h_sysfs_read_hex(ifpath, "bInterfaceSubClass");
    intf->if_protocol = h_sysfs_read_hex(ifpath, "bInterfaceProtocol");

    g_free(ifpath);
    return TRUE;
}

static gboolean usb_get_device_sysfs(int bus, int dev, const char* sysfspath, usbd *s) {
    usbi *intf;
    gboolean ok;
    int i, if_count = 0, conf = 0;
    if (sysfspath == NULL)
        return FALSE;

    s->bus = bus;
    s->dev = dev;
    s->dev_class = h_sysfs_read_hex(sysfspath, "bDeviceClass");
    s->vendor_id = h_sysfs_read_hex(sysfspath, "idVendor");
    s->product_id = h_sysfs_read_hex(sysfspath, "idProduct");
    s->manufacturer = h_sysfs_read_string(sysfspath, "manufacturer");
    s->device = h_sysfs_read_string(sysfspath, "product");
    s->max_curr_ma = h_sysfs_read_int(sysfspath, "bMaxPower");
    s->dev_class = h_sysfs_read_hex(sysfspath, "bDeviceClass");
    s->dev_subclass = h_sysfs_read_hex(sysfspath, "bDeviceSubClass");
    s->speed_mbs = h_sysfs_read_int(sysfspath, "speed");

    conf = h_sysfs_read_hex(sysfspath, "bConfigurationValue");

    if (s->usb_version == NULL)
        s->usb_version = h_sysfs_read_string(sysfspath, "version");

    if (s->if_list == NULL){ // create interfaces list
        if_count = h_sysfs_read_int(sysfspath, "bNumInterfaces");
        for (i = 0; i <= if_count; i++){
            intf = usbi_new();
            ok = usb_get_interface_sysfs(conf, i, sysfspath, intf);
            if (ok){
                usbd_append_interface(s, intf);
            }
            else{
                usbi_free(intf);
            }
        }
    }
    else{ // improve existing list
        intf = s->if_list;
        while (intf){
            usb_get_interface_sysfs(conf, intf->if_number, sysfspath, intf);
            intf = intf->next;
        }
    }

    return TRUE;
}

usbd *usb_get_device(int bus, int dev, const gchar* sysfspath) {
    usbd *s = usbd_new();
    int ok = 0;
    if (s) {
        /* try lsusb */
        ok = usb_get_device_lsusb(bus, dev, s);
        /* try sysfs */
        ok |= usb_get_device_sysfs(bus, dev, sysfspath, s);

        if (!ok) {
            usbd_free(s);
            s = NULL;
        }
    }
    return s;
}

static usbd *usb_get_device_list_lsusb() {
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;
    usbd *head = NULL, *nd;
    int bus, dev, vend, prod, ec;

    spawned = g_spawn_command_line_sync("lsusb",
            &out, &err, NULL, NULL);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            ec = sscanf(p, "Bus %d Device %d: ID %x:%x", &bus, &dev, &vend, &prod);
            if (ec == 4) {
                nd = usb_get_device(bus, dev, NULL);
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

static usbd *usb_get_device_list_sysfs() {
    GDir *dir;
    GRegex *regex;
    GMatchInfo *match_info;
    usbd *head = NULL, *nd;
    gchar *devpath;
    const char *entry;
    int bus, dev;

    regex = g_regex_new("^([0-9]+-[0-9]+([.][0-9]+)*)|(usb[0-9]+)$", 0, 0, NULL);
    if (!regex){
        return NULL;
    }

    dir = g_dir_open(SYSFS_DIR_USB_DEVICES, 0, NULL);
    if (!dir){
        return NULL;
    }

    while ((entry = g_dir_read_name(dir))) {
        g_regex_match(regex, entry, 0, &match_info);

        if (g_match_info_matches(match_info)) {
            devpath = g_build_filename(SYSFS_DIR_USB_DEVICES, entry, NULL);
            bus = h_sysfs_read_int(devpath, "busnum");
            dev = h_sysfs_read_int(devpath, "devnum");

            if (bus > 0 && dev > 0){
                nd = usb_get_device(bus, dev, devpath);
                if (head == NULL) {
                    head = nd;
                } else {
                    usbd_list_append(head, nd);
                }
            }
            g_free(devpath);
        }
        g_match_info_free(match_info);
    }

    g_dir_close(dir);
    g_regex_unref(regex);

    return head;
}

usbd *usb_get_device_list() {
    usbd *lst;

    lst = usb_get_device_list_sysfs();
    if (lst == NULL)
        lst = usb_get_device_list_lsusb();

    return lst;
}
