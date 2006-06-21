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

#include <gtk/gtk.h>
#include <config.h>
#include <string.h>

#include <hardinfo.h>
#include <shell.h>
#include <iconcache.h>

enum {
    DEVICES_KERNEL_MODULES,
    DEVICES_PCI,
    DEVICES_USB,
    DEVICES_PRINTERS,
    DEVICES_INPUT,
    DEVICES_STORAGE,
} Entries;

static ModuleEntry hi_entries[] = {
    {"Kernel Modules",	"module.png"},
    {"PCI Devices",	"devices.png"},
    {"USB Devices",	"usb.png"},
    {"Printers",	"printer.png"},
    {"Input Devices",	"keyboard.png"},
    {"Storage",		"hdd.png"},
};

static GHashTable *devices = NULL;
static gchar *module_list = "";
static gchar *printer_list = NULL;
static gchar *pci_list = "";
static gchar *input_list = NULL;
static gchar *storage_list = "";

#define WALK_UNTIL(x)   while((*buf != '\0') && (*buf != x)) buf++

#define GET_STR(field_name,ptr)      \
  if (strstr(tmp[0], field_name)) {  \
    ptr = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));          \
    g_strfreev(tmp);                 \
    continue;                        \
  }

#include <vendor.h>

#include <arch/this/pci.h>
#include <arch/this/modules.h>
#include <arch/common/printers.h>
#include <arch/this/inputdevices.h>
#include <arch/this/usb.h>
#include <arch/this/storage.h>

static void
detect_devices(void)
{
    devices = g_hash_table_new(g_str_hash, g_str_equal);

    shell_status_update("Getting loaded modules information...");
    scan_modules();

    shell_status_update("Scanning PCI devices...");
    scan_pci();

    shell_status_update("Searching for printers...");
    scan_printers();

    shell_status_update("Scanning input devices...");
    scan_inputdevices();

    shell_status_update("Scanning USB devices...");
    scan_usb();

    shell_status_update("Scanning IDE devices...");
    scan_ide();

    shell_status_update("Scanning SCSI devices...");
    scan_scsi();
}

gchar *
hi_more_info(gchar * entry)
{
    gchar *info = (gchar *) g_hash_table_lookup(devices, entry);

    if (info)
	return g_strdup(info);
    return g_strdup("[Empty]");
}

void
hi_reload(gint entry)
{
    switch (entry) {
    case DEVICES_INPUT:
	scan_inputdevices();
	break;
    case DEVICES_PRINTERS:
	scan_printers();
	break;
    case DEVICES_USB:
	scan_usb();
	break;
    case DEVICES_STORAGE:
	if (storage_list) {
	    g_free(storage_list);
	    g_free(storage_icons);
	    storage_list = g_strdup("");
	    storage_icons = g_strdup("");
	}
	scan_ide();
	scan_scsi();
	break;
    }
}

gchar *
hi_info(gint entry)
{
    if (!devices) {
	detect_devices();
    }

    switch (entry) {
    case DEVICES_KERNEL_MODULES:
	return g_strdup_printf("[Loaded Modules]\n"
			       "%s"
			       "[$ShellParam$]\n"
			       "ViewType=1",
			       module_list);
    case DEVICES_PCI:
	return g_strdup_printf("[PCI Devices]\n"
			       "%s"
			       "[$ShellParam$]\n"
			       "ViewType=1\n",
			       pci_list);
    case DEVICES_PRINTERS:
	return g_strdup_printf("%s\n"
			       "[$ShellParam$]\n"
			       "ReloadInterval=5000", printer_list);
    case DEVICES_STORAGE:
	return g_strdup_printf("%s\n"
			       "[$ShellParam$]\n"
			       "ReloadInterval=5000\n"
			       "ViewType=1\n%s", storage_list, storage_icons);
    case DEVICES_INPUT:
	return g_strdup_printf("[Input Devices]\n"
			       "%s"
			       "[$ShellParam$]\n"
			       "ViewType=1\n"
			       "ReloadInterval=5000\n%s", input_list, input_icons);
    case DEVICES_USB:
	return g_strdup_printf("%s"
			       "[$ShellParam$]\n"
			       "ViewType=1\n"
			       "ReloadInterval=5000\n",
			       usb_list);
    default:
	return g_strdup("[Empty]\nNo info available=");
    }
}

gint
hi_n_entries(void)
{
    return G_N_ELEMENTS(hi_entries) - 1;
}

GdkPixbuf *
hi_icon(gint entry)
{
    return icon_cache_get_pixbuf(hi_entries[entry].icon);
}

gchar *
hi_name(gint entry)
{
    return hi_entries[entry].name;
}
