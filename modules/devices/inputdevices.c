/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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

gchar *input_icons = NULL;

static struct {
    char *name;
    char *icon;
} input_devices[] = {
    { "Keyboard", "keyboard.png" },
    { "Joystick", "joystick.png" },
    { "Mouse",    "mouse.png"    },
    { "Speaker",  "audio.png"  },
    { "Unknown",  "module.png"   },
};

void
__scan_input_devices(void)
{
    FILE *dev;
    gchar buffer[1024];
    gchar *tmp, *name = NULL, *phys = NULL;
    gchar *vendor_str = NULL, *product_str = NULL, *vendor_tags = NULL;
    gint bus = 0, vendor = 0, product = 0, version = 0;
    int d = 0, n = 0;

    dev = fopen("/proc/bus/input/devices", "r");
    if (!dev)
	return;

    if (input_list) {
        moreinfo_del_with_prefix("DEV:INP");
	g_free(input_list);
	g_free(input_icons);
    }
    input_list = g_strdup("");
    input_icons = g_strdup("");

    while (fgets(buffer, sizeof(buffer), dev)) {
	tmp = buffer;

	switch (*tmp) {
	case 'N':
	    tmp = strreplacechr(tmp + strlen("N: Name="), "=", ':');
	    name = g_strdup(tmp);
	    remove_quotes(name);
	    break;
	case 'P':
	    phys = g_strdup(tmp + strlen("P: Phys="));
	    break;
	case 'I':
	    sscanf(tmp, "I: Bus=%x Vendor=%x Product=%x Version=%x",
		   &bus, &vendor, &product, &version);
	    break;
	case 'H':
	    if (strstr(tmp, "kbd"))
		d = 0;		//INPUT_KEYBOARD;
	    else if (strstr(tmp, "js"))
		d = 1;		//INPUT_JOYSTICK;
	    else if (strstr(tmp, "mouse"))
		d = 2;		//INPUT_MOUSE;
	    else
		d = 4;		//INPUT_UNKNOWN;
	    break;
	case '\n':
	    if (name && strstr(name, "PC Speaker")) {
	      d = 3;		// INPUT_PCSPKR
	    }

        if (vendor > 0 && product > 0 && g_str_has_prefix(phys, "usb-")) {
            usb_lookup_ids_vendor_product_str(vendor, product, &vendor_str, &product_str);
        }
        
        tmp = g_strdup_printf("INP%d", ++n);
        input_list = h_strdup_cprintf("$%s$%s=\n",
                     input_list,
                     tmp, name);
        input_icons = h_strdup_cprintf("Icon$%s$%s=%s\n",
                      input_icons,
                      tmp, name,
                      input_devices[d].icon);

        gchar *strhash = g_strdup_printf("[%s]\n"
                /* Name */   "$^$%s=%s\n"
                /* Type */   "%s=%s\n"
                /* Bus */    "%s=0x%x\n"
                /* Vendor */ "$^$%s=[0x%x] %s\n"
                /* Product */"%s=[0x%x] %s\n"
                /* Version */"%s=0x%x\n",
                        _("Device Information"),
                        _("Name"), name,
                        _("Type"), input_devices[d].name,
                        _("Bus"), bus,
                        _("Vendor"), vendor, vendor_str ? vendor_str: _("(Unknown)"),
                        _("Product"), product, product_str ? product_str: _("(Unknown)"),
                        _("Version"), version );

        if (phys && phys[1] != 0) {
             strhash = h_strdup_cprintf("%s=%s\n", strhash, _("Connected to"), phys);
        }

        if (phys && strstr(phys, "ir")) {
            strhash = h_strdup_cprintf("%s=%s\n", strhash, _("InfraRed port"), _("Yes") );
        }

        moreinfo_add_with_prefix("DEV", tmp, strhash);
        g_free(tmp);
        g_free(phys);
        g_free(name);
        g_free(vendor_str);
        g_free(product_str);
        vendor_str = NULL;
        product_str = NULL;
    }
    }

    fclose(dev);
}
