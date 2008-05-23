/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

static gboolean
remove_usb_devices(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "USB");
}

static gchar *usb_list = NULL;

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
    	product = 
	    g_strdup_printf("Unknown USB %.2f device (class %d)", version,
			    classid);
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

void __scan_usb_sysfs(void)
{
    GDir *sysfs;
    gchar *filename;
    const gchar *sysfs_path = "/sys/class/usb_endpoint";
    gint usb_device_number = 0;

    if (usb_list) {
	g_hash_table_foreach_remove(moreinfo, remove_usb_devices, NULL);
	g_free(usb_list);
    }
    usb_list = g_strdup("[USB Devices]\n");

    if (!(sysfs = g_dir_open(sysfs_path, 0, NULL))) {
	return;
    }

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
}

int __scan_usb_procfs(void)
{
    FILE *dev;
    gchar buffer[128];
    gchar *tmp, *manuf = NULL, *product = NULL, *mxpwr;
    gint bus, level, port, classid, trash;
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
    usb_list = g_strdup("");

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

	    if (*product == '\0') {
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
	}
    }

    fclose(dev);

    return n;
}

void __scan_usb(void)
{
    if (!__scan_usb_procfs())
	__scan_usb_sysfs();
}
