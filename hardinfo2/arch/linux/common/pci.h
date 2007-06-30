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

void
__scan_pci(void)
{
    FILE *lspci;
    gchar buffer[256], *buf, *strhash = NULL, *strdevice = NULL;
    gchar *category = NULL, *name = NULL;
    gint n = 0;

    if (!(lspci = popen(LSPCI, "r"))) {
      goto pci_error;
    }

    gchar *icon;
    
    int x = 0;			/* unique Memory, Capability and I/O port */
    while (fgets(buffer, 256, lspci)) {
	buf = g_strstrip(buffer);

	if (!strncmp(buf, "Flags", 5)) {
	    gint irq = 0, freq = 0, latency = 0, i;
	    gchar **list;
	    gboolean bus_master;

	    buf += 7;

	    bus_master = FALSE;

	    list = g_strsplit(buf, ", ", 10);
	    for (i = 0; i <= 10; i++) {
		if (!list[i])
		    break;

		if (!strncmp(list[i], "IRQ", 3))
		    sscanf(list[i], "IRQ %d", &irq);
		else if (strstr(list[i], "Mhz"))
		    sscanf(list[i], "%dMhz", &freq);
		else if (!strncmp(list[i], "bus master", 10))
		    bus_master = TRUE;
		else if (!strncmp(list[i], "latency", 7))
		    sscanf(list[i], "latency %d", &latency);
	    }
	    g_strfreev(list);

	    if (irq)
		strdevice = g_strdup_printf("%sIRQ=%d\n", strdevice, irq);
	    if (freq)
		strdevice =
		    g_strdup_printf("%sFrequency=%dMHz\n", strdevice,
				    freq);
	    if (latency)
		strdevice =
		    g_strdup_printf("%sLatency=%d\n", strdevice, latency);

	    strdevice =
		g_strdup_printf("%sBus Master=%s\n", strdevice,
				bus_master ? "Yes" : "No");
	} else if (!strncmp(buf, "Subsystem", 9)) {
	    WALK_UNTIL(' ');
	    buf++;
	    strdevice =
		g_strdup_printf("%sOEM Vendor=%s\n", strdevice, buf);
	} else if (!strncmp(buf, "Capabilities", 12)
		   && !strstr(buf, "only to root") && 
		      !strstr(buf, "access denied")) {
	    WALK_UNTIL(' ');
	    WALK_UNTIL(']');
	    buf++;
	    strdevice =
		g_strdup_printf("%sCapability#%d=%s\n", strdevice, ++x,
				buf);
	} else if (!strncmp(buf, "Memory at", 9) && strstr(buf, "[size=")) {
	    gint mem;
	    gchar unit;
	    gboolean prefetch;
	    gboolean _32bit;

	    prefetch = strstr(buf, "non-prefetchable") ? FALSE : TRUE;
	    _32bit = strstr(buf, "32-bit") ? TRUE : FALSE;

	    WALK_UNTIL('[');
	    sscanf(buf, "[size=%d%c", &mem, &unit);

	    strdevice = g_strdup_printf("%sMemory#%d=%d%cB (%s%s)\n",
					strdevice, ++x,
					mem,
					(unit == ']') ? ' ' : unit,
					_32bit ? "32-bit, " : "",
					prefetch ? "prefetchable" :
					"non-prefetchable");

	} else if (!strncmp(buf, "I/O", 3)) {
	    guint io_addr, io_size;

	    sscanf(buf, "I/O ports at %x [size=%d]", &io_addr, &io_size);

	    strdevice =
		g_strdup_printf("%sI/O ports at#%d=0x%x - 0x%x\n",
				strdevice, ++x, io_addr,
				io_addr + io_size);
	} else if ((buf[0] >= '0' && buf[0] <= '9') && (buf[4] == ':' || buf[2] == ':')) {
	    gint bus, device, function, domain;
	    gpointer start, end;

	    if (strdevice != NULL && strhash != NULL) {
		g_hash_table_insert(moreinfo, strhash, strdevice);
                g_free(category);
                g_free(name);
	    }

	    if (buf[4] == ':') {
		sscanf(buf, "%x:%x:%x.%d", &domain, &bus, &device, &function);
	    } else {
	    	/* lspci without domain field */
	    	sscanf(buf, "%x:%x.%x", &bus, &device, &function);
	    	domain = 0;
	    }

	    WALK_UNTIL(' ');

	    start = buf;

	    WALK_UNTIL(':');
	    end = buf + 1;
	    *buf = 0;

	    buf = start + 1;
	    category = g_strdup(buf);

	    buf = end;
	    start = buf;
	    WALK_UNTIL('(');
	    *buf = 0;
	    buf = start + 1;

            if (strstr(category, "RAM memory")) icon = "mem";
            else if (strstr(category, "Multimedia")) icon = "media";
            else if (strstr(category, "USB")) icon = "usb";
            else icon = "pci";
            
	    name = g_strdup(buf);

	    strhash = g_strdup_printf("PCI%d", n);
	    strdevice = g_strdup_printf("[Device Information]\n"
					"Name=%s\n"
					"Class=%s\n"
					"Domain=%d\n"
					"Bus, device, function=%d, %d, %d\n",
					name, category, domain, bus,
					device, function);
            
            const gchar *url = vendor_get_url(name);
            if (url) {
                strdevice = g_strdup_printf("%s"
                                            "Vendor=%s (%s)\n",
                                            strdevice,
                                            vendor_get_name(name),
                                            url);
            }
            
            
	    pci_list = g_strdup_printf("%s$PCI%d$%s=%s\n", pci_list, n, category,
				name);

	    n++;
	}
    }
    
    if (pclose(lspci)) {
pci_error:
        /* error (no pci, perhaps?) */
        pci_list = g_strconcat(pci_list, "No PCI devices found=\n", NULL);
    } else if (strhash) {
	/* insert the last device */
        g_hash_table_insert(moreinfo, strhash, strdevice);
        g_free(category);
        g_free(name);
    }
}
