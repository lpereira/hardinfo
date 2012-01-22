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
/*
 * FIXME:
 * - listing with sysfs does not generate device hierarchy
 */

#include <string.h>

#include "hardinfo.h"
#include "devices.h"

gchar *usb_list = NULL;

static gboolean
remove_usb_devices(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "USB");
}

void __scan_usb_sysfs_add_device(gchar * endpoint, int n)
{
    gchar *manufacturer, *product, *mxpwr, *tmp, *strhash;
    gint bus, classid, vendor, prodid;
    gfloat version, speed;

    classid = h_sysfs_read_int(endpoint, "bDeviceClass");
    vendor = h_sysfs_read_int(endpoint, "idVendor");
    prodid = h_sysfs_read_int(endpoint, "idProduct");
    bus = h_sysfs_read_int(endpoint, "busnum");
    speed = h_sysfs_read_float(endpoint, "speed");
    version = h_sysfs_read_float(endpoint, "version");

    if (!(mxpwr = h_sysfs_read_string(endpoint, "bMaxPower"))) {
    	mxpwr = g_strdup("0 mA");
    }

    if (!(manufacturer = h_sysfs_read_string(endpoint, "manufacturer"))) {
    	manufacturer = g_strdup("Unknown");
    }

    if (!(product = h_sysfs_read_string(endpoint, "product"))) {
	if (classid == 9) {
	    product = g_strdup_printf("USB %.2f Hub", version);
	} else {
	    product = g_strdup_printf("Unknown USB %.2f Device (class %d)", version, classid);
	}
    }

    const gchar *url = vendor_get_url(manufacturer);
    if (url) {
	tmp = g_strdup_printf("%s (%s)", vendor_get_name(manufacturer), url);
	
	g_free(manufacturer);
	manufacturer = tmp;
    }

    tmp = g_strdup_printf("USB%d", n);
    usb_list = h_strdup_cprintf("$%s$%s=\n", usb_list, tmp, product);

    strhash = g_strdup_printf("[Device Information]\n"
			      "Product=%s\n"
			      "Manufacturer=%s\n"
			      "Speed=%.2fMbit/s\n"
			      "Max Current=%s\n"
			      "[Misc]\n"
			      "USB Version=%.2f\n"
			      "Class=0x%x\n"
			      "Vendor=0x%x\n"
			      "Product ID=0x%x\n"
			      "Bus=%d\n",
			      product,
			      manufacturer,
			      speed,
			      mxpwr,
			      version, classid, vendor, prodid, bus);

    g_hash_table_insert(moreinfo, tmp, strhash);

    g_free(manufacturer);
    g_free(product);
    g_free(mxpwr);
}

gboolean __scan_usb_sysfs(void)
{
    GDir *sysfs;
    gchar *filename;
    const gchar *sysfs_path = "/sys/class/usb_endpoint";
    gint usb_device_number = 0;

    if (!(sysfs = g_dir_open(sysfs_path, 0, NULL))) {
	return FALSE;
    }

    if (usb_list) {
	g_hash_table_foreach_remove(moreinfo, remove_usb_devices, NULL);
	g_free(usb_list);
    }
    usb_list = g_strdup("[USB Devices]\n");

    while ((filename = (gchar *) g_dir_read_name(sysfs))) {
	gchar *endpoint =
	    g_build_filename(sysfs_path, filename, "device", NULL);
	gchar *temp;

	temp = g_build_filename(endpoint, "idVendor", NULL);
	if (g_file_test(temp, G_FILE_TEST_EXISTS)) {
	    __scan_usb_sysfs_add_device(endpoint, ++usb_device_number);
	}

	g_free(temp);
	g_free(endpoint);
    }

    g_dir_close(sysfs);

    return usb_device_number > 0;
}

gboolean __scan_usb_procfs(void)
{
    FILE *dev;
    gchar buffer[128];
    gchar *tmp, *manuf = NULL, *product = NULL, *mxpwr;
    gint bus, level, port = 0, classid = 0, trash;
    gint vendor, prodid;
    gfloat ver, rev, speed;
    int n = 0;

    dev = fopen("/proc/bus/usb/devices", "r");
    if (!dev)
	return 0;

    if (usb_list) {
	g_hash_table_foreach_remove(moreinfo, remove_usb_devices, NULL);
	g_free(usb_list);
    }
    usb_list = g_strdup("[USB Devices]\n");

    while (fgets(buffer, 128, dev)) {
	tmp = buffer;

	switch (*tmp) {
	case 'T':
	    sscanf(tmp,
		   "T:  Bus=%d Lev=%d Prnt=%d Port=%d Cnt=%d Dev#=%d Spd=%f",
		   &bus, &level, &trash, &port, &trash, &trash, &speed);
	    break;
	case 'D':
	    sscanf(tmp, "D:  Ver=%f Cls=%x", &ver, &classid);
	    break;
	case 'P':
	    sscanf(tmp, "P:  Vendor=%x ProdID=%x Rev=%f",
		   &vendor, &prodid, &rev);
	    break;
	case 'S':
	    if (strstr(tmp, "Manufacturer=")) {
		manuf = g_strdup(strchr(tmp, '=') + 1);
		remove_linefeed(manuf);
	    } else if (strstr(tmp, "Product=")) {
		product = g_strdup(strchr(tmp, '=') + 1);
		remove_linefeed(product);
	    }
	    break;
	case 'C':
	    mxpwr = strstr(buffer, "MxPwr=") + 6;

	    tmp = g_strdup_printf("USB%d", ++n);

	    if (product && *product == '\0') {
		g_free(product);
		if (classid == 9) {
		    product = g_strdup_printf("USB %.2f Hub", ver);
		} else {
		    product =
			g_strdup_printf
			("Unknown USB %.2f Device (class %d)", ver,
			 classid);
		}
	    }

	    if (classid == 9) {	/* hub */
		usb_list = h_strdup_cprintf("[%s#%d]\n",
					    usb_list, product, n);
	    } else {		/* everything else */
		usb_list = h_strdup_cprintf("$%s$%s=\n",
					    usb_list, tmp, product);

		const gchar *url = vendor_get_url(manuf);
		if (url) {
		    gchar *tmp =
			g_strdup_printf("%s (%s)", vendor_get_name(manuf),
					url);
		    g_free(manuf);
		    manuf = tmp;
		}

		gchar *strhash = g_strdup_printf("[Device Information]\n"
						 "Product=%s\n",
						 product);
		if (manuf && strlen(manuf))
		    strhash = h_strdup_cprintf("Manufacturer=%s\n",
					       strhash, manuf);

		strhash = h_strdup_cprintf("[Port #%d]\n"
					   "Speed=%.2fMbit/s\n"
					   "Max Current=%s\n"
					   "[Misc]\n"
					   "USB Version=%.2f\n"
					   "Revision=%.2f\n"
					   "Class=0x%x\n"
					   "Vendor=0x%x\n"
					   "Product ID=0x%x\n"
					   "Bus=%d\n" "Level=%d\n",
					   strhash,
					   port, speed, mxpwr,
					   ver, rev, classid,
					   vendor, prodid, bus, level);

		g_hash_table_insert(moreinfo, tmp, strhash);
	    }

	    g_free(manuf);
	    g_free(product);
	    manuf = g_strdup("");
	    product = g_strdup("");
	    port = classid = 0;
	}
    }

    fclose(dev);

    return n > 0;
}

void __scan_usb_lsusb_add_device(char *buffer, FILE *lsusb, int usb_device_number)
{
    gint bus, device, vendor_id, product_id;
    gchar *version = NULL, *product = NULL, *vendor = NULL, *dev_class = NULL, *int_class = NULL;
    gchar *max_power = NULL;
    gchar *tmp, *strhash;
    long position;

    sscanf(buffer, "Bus %d Device %d: ID %x:%x",
           &bus, &device, &vendor_id, &product_id);

    for (position = ftell(lsusb); fgets(buffer, 512, lsusb); position = ftell(lsusb)) {
        g_strstrip(buffer);

        if (g_str_has_prefix(buffer, "idVendor")) {
            g_free(vendor);
            vendor = g_strdup(buffer + 26);
        } else if (g_str_has_prefix(buffer, "idProduct")) {
            g_free(product);
            product = g_strdup(buffer + 26);
        } else if (g_str_has_prefix(buffer, "MaxPower")) {
            g_free(max_power);
            max_power = g_strdup(buffer + 9);
        } else if (g_str_has_prefix(buffer, "bcdUSB")) {
            g_free(version);
            version = g_strdup(buffer + 7);
        } else if (g_str_has_prefix(buffer, "bDeviceClass")) {
            g_free(dev_class);
            dev_class = g_strdup(buffer + 14);
        } else if (g_str_has_prefix(buffer, "bInterfaceClass")) {
            g_free(int_class);
            int_class = g_strdup(buffer + 16);
        } else if (g_str_has_prefix(buffer, "Bus ")) {
            /* device separator */
            fseek(lsusb, position, SEEK_SET);
            break;
        }
    }

    if (strstr(dev_class, "0 (Defined at Interface level)")) {
        g_free(dev_class);
        if (int_class) {
            dev_class = int_class;
        } else {
            dev_class = g_strdup("Unknown");
        }
    }

    tmp = g_strdup_printf("USB%d", usb_device_number);
    usb_list = h_strdup_cprintf("$%s$%s=\n", usb_list, tmp, product ? product : "Unknown");

    strhash = g_strdup_printf("[Device Information]\n"
			      "Product=%s\n"
			      "Manufacturer=%s\n"
			      "Max Current=%s\n"
			      "[Misc]\n"
			      "USB Version=%s\n"
			      "Class=%s\n"
			      "Vendor=0x%x\n"
			      "Product ID=0x%x\n"
			      "Bus=%d\n",
			      product   ? g_strstrip(product)   : "Unknown",
			      vendor    ? g_strstrip(vendor)    : "Unknown",
			      max_power ? g_strstrip(max_power) : "Unknown",
			      version   ? g_strstrip(version)   : "Unknown",
			      dev_class ? g_strstrip(dev_class) : "Unknown",
			      vendor_id, product_id, bus);

    g_hash_table_insert(moreinfo, tmp, strhash);

    g_free(vendor);
    g_free(product);
    g_free(max_power);
    g_free(dev_class);
    g_free(version);
}

gboolean __scan_usb_lsusb(void)
{
    static gchar *lsusb_path = NULL;
    int usb_device_number = 0;
    FILE *lsusb;
    char buffer[512], *temp;

    if (!lsusb_path) {
        if (!(lsusb_path = find_program("lsusb"))) {
            DEBUG("lsusb not found");

            return FALSE;
        }
    }

    temp = g_strdup_printf("%s -v", lsusb_path);
    if (!(lsusb = popen(temp, "r"))) {
        DEBUG("cannot run %s", lsusb_path);

        g_free(temp);
        return FALSE;
    }

    g_free(temp);

    if (usb_list) {
	g_hash_table_foreach_remove(moreinfo, remove_usb_devices, NULL);
	g_free(usb_list);
    }
    usb_list = g_strdup("[USB Devices]\n");

    while (fgets(buffer, sizeof(buffer), lsusb)) {
        if (g_str_has_prefix(buffer, "Bus ")) {
           __scan_usb_lsusb_add_device(buffer, lsusb, ++usb_device_number);
        }
    }
    
    pclose(lsusb);
    
    return usb_device_number > 0;
}

void __scan_usb(void)
{
    if (!__scan_usb_procfs()) {
	if (!__scan_usb_sysfs()) {
             __scan_usb_lsusb();
        }
    }
}
