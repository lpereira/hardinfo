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
    	mxpwr = g_strdup_printf("%d %s", 0 , _("mA") );
    }

    if (!(manufacturer = h_sysfs_read_string(endpoint, "manufacturer"))) {
    	manufacturer = g_strdup(_("(Unknown)"));
    }

    if (!(product = h_sysfs_read_string(endpoint, "product"))) {
        if (classid == 9) {
            product = g_strdup_printf(_(/*/%.2f is version*/ "USB %.2f Hub"), version);
        } else {
            product = g_strdup_printf(_("Unknown USB %.2f Device (class %d)"), version, classid);
        }
    }

    const gchar *v_url = vendor_get_url(manufacturer);
    const gchar *v_name = vendor_get_name(manufacturer);
    gchar *v_str;
    if (v_url != NULL) {
        v_str = g_strdup_printf("%s (%s)", v_name, v_url);
    } else {
        v_str = g_strdup_printf("%s", manufacturer);
    }

    tmp = g_strdup_printf("USB%d", n);
    usb_list = h_strdup_cprintf("$%s$%s=\n", usb_list, tmp, product);

    strhash = g_strdup_printf("[%s]\n"
             /* Product */      "%s=%s\n"
             /* Manufacturer */ "%s=%s\n"
             /* Speed */        "%s=%.2f %s\n"
             /* Max Current */  "%s=%s\n"
                  "[%s]\n"
             /* USB Version */  "%s=%.2f\n"
             /* Class */        "%s=0x%x\n"
             /* Vendor */       "%s=0x%x\n"
             /* Product ID */   "%s=0x%x\n"
             /* Bus */          "%s=%d\n",
                  _("Device Information"),
                  _("Product"), product,
                  _("Manufacturer"), v_str,
                  _("Speed"), speed, _("Mbit/s"),
                  _("Max Current"), mxpwr,
                  _("Misc"),
                  _("USB Version"), version,
                  _("Class"), classid,
                  _("Vendor ID"), vendor,
                  _("Product ID"), prodid,
                  _("Bus"), bus);

    moreinfo_add_with_prefix("DEV", tmp, strhash);
    g_free(tmp);
    g_free(v_str);
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
       moreinfo_del_with_prefix("DEV:USB");
	g_free(usb_list);
    }
    usb_list = g_strdup_printf("[%s]\n", _("USB Devices"));

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
    gchar *tmp, *manuf = NULL, *product = NULL, *mxpwr = NULL;
    gint bus = 0, level = 0, port = 0, classid = 0, trash;
    gint vendor = 0, prodid = 0;
    gfloat ver = 0.0f, rev = 0.0f, speed = 0.0f;
    int n = 0;

    dev = fopen("/proc/bus/usb/devices", "r");
    if (!dev)
	return 0;

    if (usb_list) {
	moreinfo_del_with_prefix("DEV:USB");
	g_free(usb_list);
    }
    usb_list = g_strdup_printf("[%s]\n", _("USB Devices"));

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
	    sscanf(tmp, "P:  Vendor=%x ProdID=%x Rev=%f", &vendor, &prodid, &rev);
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
		    product = g_strdup_printf(_("USB %.2f Hub"), ver);
		} else {
		    product = g_strdup_printf(_("Unknown USB %.2f Device (class %d)"), ver, classid);
		}
	    }

        if (classid == 9) {	/* hub */
            usb_list = h_strdup_cprintf("[%s#%d]\n", usb_list, product, n);
        } else {		/* everything else */
            usb_list = h_strdup_cprintf("$%s$%s=\n", usb_list, tmp, product);

        EMPIFNULL(manuf);
        const gchar *v_url = vendor_get_url(manuf);
        const gchar *v_name = vendor_get_name(manuf);
        gchar *v_str = NULL;
        if (strlen(manuf)) {
            if (v_url != NULL)
                v_str = g_strdup_printf("%s (%s)", v_name, v_url);
            else
                v_str = g_strdup_printf("%s", manuf);
        }
        UNKIFNULL(v_str);
        UNKIFNULL(product);

        gchar *strhash = g_strdup_printf("[%s]\n" "%s=%s\n" "%s=%s\n",
                        _("Device Information"),
                        _("Product"), product,
                        _("Manufacturer"), v_str);

        strhash = h_strdup_cprintf("[%s #%d]\n"
                  /* Speed */       "%s=%.2f %s\n"
                  /* Max Current */ "%s=%s\n"
                       "[%s]\n"
                  /* USB Version */ "%s=%.2f\n"
                  /* Revision */    "%s=%.2f\n"
                  /* Class */       "%s=0x%x\n"
                  /* Vendor */      "%s=0x%x\n"
                  /* Product ID */  "%s=0x%x\n"
                  /* Bus */         "%s=%d\n"
                  /* Level */       "%s=%d\n",
                       strhash,
                       _("Port"), port,
                       _("Speed"), speed, _("Mbit/s"),
                       _("Max Current"), mxpwr,
                       _("Misc"),
                       _("USB Version"), ver,
                       _("Revision"), rev,
                       _("Class"), classid,
                       _("Vendor ID"), vendor,
                       _("Product ID"), prodid,
                       _("Bus"), bus,
                       _("Level"), level);

        moreinfo_add_with_prefix("DEV", tmp, strhash);
        g_free(v_str);
        g_free(tmp);
        }

	    g_free(manuf);
	    g_free(product);
	    manuf = NULL;
	    product = NULL;
	    port = classid = 0;
	}
    }

    fclose(dev);

    return n > 0;
}


void __scan_usb_lsusb_add_device(char *buffer, int bufsize, FILE * lsusb, int usb_device_number)
{
    gint bus, device, vendor_id, product_id;
    gchar *version = NULL, *product = NULL, *vendor = NULL, *dev_class = NULL, *int_class = NULL;
    gchar *max_power = NULL, *name = NULL;
    gchar *tmp, *strhash;
    long position = 0;

    g_strstrip(buffer);
    sscanf(buffer, "Bus %d Device %d: ID %x:%x", &bus, &device, &vendor_id, &product_id);
    name = g_strdup(buffer + 33);

    for (fgets(buffer, bufsize, lsusb); position >= 0 && fgets(buffer, bufsize, lsusb); position = ftell(lsusb)) {
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

    if (dev_class && strstr(dev_class, "0 (Defined at Interface level)")) {
        g_free(dev_class);
        if (int_class) {
            dev_class = int_class;
        } else {
            dev_class = g_strdup(_("(Unknown)"));
        }
    } else
    dev_class = g_strdup(_("(Unknown)"));

    tmp = g_strdup_printf("USB%d", usb_device_number);
    usb_list = h_strdup_cprintf("$%s$%s=\n", usb_list, tmp, name);

    const gchar *v_url = vendor_get_url(vendor);
    const gchar *v_name = vendor_get_name(vendor);
    gchar *v_str;
    if (v_url != NULL) {
        v_str = g_strdup_printf("%s (%s)", v_name, v_url);
    } else {
        v_str = g_strdup_printf("%s", g_strstrip(vendor) );
    }

    if (max_power != NULL) {
        int mA = atoi(g_strstrip(max_power));
        gchar *trent_steel = g_strdup_printf("%d %s", mA, _("mA"));
        g_free(max_power);
        max_power = trent_steel;
    }

    UNKIFNULL(product);
    UNKIFNULL(v_str);
    UNKIFNULL(max_power);
    UNKIFNULL(version);
    UNKIFNULL(dev_class);

    strhash = g_strdup_printf("[%s]\n"
             /* Product */      "%s=%s\n"
             /* Manufacturer */ "%s=%s\n"
             /* Max Current */  "%s=%s\n"
                            "[%s]\n"
             /* USB Version */ "%s=%s\n"
             /* Class */       "%s=%s\n"
             /* Vendor ID */   "%s=0x%x\n"
             /* Product ID */  "%s=0x%x\n"
             /* Bus */         "%s=%d\n",
                _("Device Information"),
                _("Product"), g_strstrip(product),
                _("Vendor"), v_str,
                _("Max Current"), g_strstrip(max_power),
                _("Misc"),
                _("USB Version"), g_strstrip(version),
                _("Class"), g_strstrip(dev_class),
                _("Vendor ID"), vendor_id,
                _("Product ID"), product_id,
                _("Bus"), bus);

    moreinfo_add_with_prefix("DEV", tmp, strhash);
    g_free(v_str);
    g_free(vendor);
    g_free(product);
    g_free(max_power);
    g_free(dev_class);
    g_free(version);
    g_free(tmp);
    g_free(name);
}

gboolean __scan_usb_lsusb(void)
{
    static gchar *lsusb_path = NULL;
    int usb_device_number = 0;
    FILE *lsusb;
    FILE *temp_lsusb;
    char buffer[512], *temp;

    if (!lsusb_path) {
        if (!(lsusb_path = find_program("lsusb"))) {
            DEBUG("lsusb not found");

            return FALSE;
        }
    }

    temp = g_strdup_printf("%s -v | tr '[]' '()'", lsusb_path);
    if (!(lsusb = popen(temp, "r"))) {
        DEBUG("cannot run %s", lsusb_path);

        g_free(temp);
        return FALSE;
    }

    temp_lsusb = tmpfile();
    if (!temp_lsusb) {
        DEBUG("cannot create temporary file for lsusb");
        pclose(lsusb);
        return FALSE;
    }

    while (fgets(buffer, sizeof(buffer), lsusb)) {
        fputs(buffer, temp_lsusb);
    }

    pclose(lsusb);

    // rewind file so we can read from it
    fseek(temp_lsusb, 0, SEEK_SET);

    g_free(temp);

    if (usb_list) {
       moreinfo_del_with_prefix("DEV:USB");
        g_free(usb_list);
    }
    usb_list = g_strdup_printf("[%s]\n", _("USB Devices"));

    while (fgets(buffer, sizeof(buffer), temp_lsusb)) {
        if (g_str_has_prefix(buffer, "Bus ")) {
           __scan_usb_lsusb_add_device(buffer, sizeof(buffer), temp_lsusb, ++usb_device_number);
        }
    }

    fclose(temp_lsusb);

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
