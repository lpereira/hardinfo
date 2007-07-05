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
void
__scan_usb(void)
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
	return;

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
  	            product = g_strdup_printf("Unknown USB %.2f Device (class %d)",
                                              ver, classid);
                }
	    }
	    

	    if (classid == 9) {	/* hub */
    	        usb_list = h_strdup_cprintf("[%s#%d]\n",
		      		           usb_list, product, n);
            } else { /* everything else */
    	        usb_list = h_strdup_cprintf("$%s$%s=\n",
		      		           usb_list, tmp, product);

                const gchar *url = vendor_get_url(manuf);
                if (url) {
                    gchar *tmp = g_strdup_printf("%s (%s)", manuf, url);
                    g_free(manuf);
                    manuf = tmp;
                }

                gchar *strhash = g_strdup_printf("[Device Information]\n"
                                                 "Product=%s\n"
                                                 "Manufacturer=%s\n"
                                                 "[Port #%d]\n"
                                                 "Speed=%.2fMbit/s\n"
                                                 "Max Current=%s\n"
                                                 "[Misc]\n"
                                                 "USB Version=%.2f\n"
                                                 "Revision=%.2f\n"
                                                 "Class=0x%x\n"
                                                 "Vendor=0x%x\n"
                                                 "Product ID=0x%x\n"
                                                 "Bus=%d\n" "Level=%d\n",
                                                 product, manuf,
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
}
