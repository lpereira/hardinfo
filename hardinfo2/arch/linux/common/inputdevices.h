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

static gchar *input_icons = NULL;

static gboolean
remove_input_devices(gpointer key, gpointer value, gpointer data)
{
    if (!strncmp((gchar *) key, "INP", 3)) {
	g_free((gchar *) key);
	g_free((GtkTreeIter *) value);
	return TRUE;
    }

    return FALSE;
}

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
scan_inputdevices(void)
{
    FILE *dev;
    gchar buffer[128];
    gchar *tmp, *name = NULL, *phys = NULL;
    gint bus, vendor, product, version;
    int d = 0, n = 0;

    dev = fopen("/proc/bus/input/devices", "r");
    if (!dev)
	return;

    if (input_list) {
	g_hash_table_foreach_remove(devices, remove_input_devices, NULL);
	g_free(input_list);
	g_free(input_icons);
    }
    input_list = g_strdup("");
    input_icons = g_strdup("");

    while (fgets(buffer, 128, dev)) {
	tmp = buffer;

	switch (*tmp) {
	case 'N':
	    name = g_strdup(tmp + strlen("N: Name="));
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
	    if (strstr(name, "PC Speaker")) {
	      d = 3;		// INPUT_PCSPKR
	    }
	
	    tmp = g_strdup_printf("INP%d", ++n);
	    input_list = g_strdup_printf("%s$%s$%s=\n",
					 input_list,
					 tmp, name);
	    input_icons = g_strdup_printf("%sIcon$%s$%s=%s\n",
				 	  input_icons,
					  tmp, name,
					  input_devices[d].icon);
	    gchar *strhash = g_strdup_printf("[Device Information]\n"
					     "Name=%s\n"
					     "Type=%s\n"
					     "Bus=0x%x\n",
					     name,
					     input_devices[d].name,
					     bus);

	    const gchar *url = vendor_get_url(name);
	    if (url) {
	    	strhash = g_strdup_printf("%s"
					  "Vendor=%s (%s)\n",
					  strhash,
					  vendor_get_name(name),
					  url);
	    } else {
	    	strhash = g_strdup_printf("%s"
					  "Vendor=%x\n",
					  strhash,
					  vendor);
	    }

	    strhash = g_strdup_printf("%s"
				      "Product=0x%x\n"
				      "Version=0x%x\n"
				      "Connected to=%s\n",
				      strhash, product, version, phys);
	    g_hash_table_insert(devices, tmp, strhash);

	    g_free(phys);
	    g_free(name);
	}
    }

    fclose(dev);
}
