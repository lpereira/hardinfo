/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif /* __USE_XOPEN */

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif /* _XOPEN_SOURCE */

#include <gtk/gtk.h>
#include <config.h>
#include <string.h>

#include <hardinfo.h>
#include <shell.h>
#include <iconcache.h>
#include <syncmanager.h>

#include <expr.h>
#include <socket.h>

#include "devices.h"

gchar *callback_processors();
gchar *callback_memory();
gchar *callback_battery();
gchar *callback_pci();
gchar *callback_sensors();
gchar *callback_printers();
gchar *callback_storage();
gchar *callback_input();
gchar *callback_usb();
#if defined(ARCH_x86)
gchar *callback_dmi();
gchar *callback_spd();
#endif
gchar *callback_device_resources();

void scan_processors(gboolean reload);
void scan_memory(gboolean reload);
void scan_battery(gboolean reload);
void scan_pci(gboolean reload);
void scan_sensors(gboolean reload);
void scan_printers(gboolean reload);
void scan_storage(gboolean reload);
void scan_input(gboolean reload);
void scan_usb(gboolean reload);
#if defined(ARCH_x86)
void scan_dmi(gboolean reload);
void scan_spd(gboolean reload);
#endif
void scan_device_resources(gboolean reload);

gchar *hi_more_info(gchar *entry);

static ModuleEntry entries[] = {
    {"Processor", "processor.png", callback_processors, scan_processors, MODULE_FLAG_NONE},
    {"Memory", "memory.png", callback_memory, scan_memory, MODULE_FLAG_NONE},
    {"PCI Devices", "devices.png", callback_pci, scan_pci, MODULE_FLAG_NONE},
    {"USB Devices", "usb.png", callback_usb, scan_usb, MODULE_FLAG_NONE},
    {"Printers", "printer.png", callback_printers, scan_printers, MODULE_FLAG_NONE},
    {"Battery", "battery.png", callback_battery, scan_battery, MODULE_FLAG_NONE},
    {"Sensors", "therm.png", callback_sensors, scan_sensors, MODULE_FLAG_NONE},
    {"Input Devices", "inputdevices.png", callback_input, scan_input, MODULE_FLAG_NONE},
    {"Storage", "hdd.png", callback_storage, scan_storage, MODULE_FLAG_NONE},
#if defined(ARCH_x86)
    {"DMI", "computer.png", callback_dmi, scan_dmi, MODULE_FLAG_NONE},
    {"Memory SPD", "memory.png", callback_spd, scan_spd, MODULE_FLAG_NONE},
#endif	/* x86 or x86_64 */
    {"Resources", "resources.png", callback_device_resources, scan_device_resources, MODULE_FLAG_NONE},
    {NULL}
};

static GSList *processors = NULL;
gchar *printer_list = NULL;
gchar *printer_icons = NULL;
gchar *pci_list = NULL;
gchar *input_list = NULL;
gchar *storage_list = NULL;
gchar *battery_list = NULL;
gchar *meminfo = NULL;
gchar *lginterval = NULL;

GHashTable *moreinfo = NULL;

#include <vendor.h>

gchar *get_processor_name(void)
{
    scan_processors(FALSE);

    Processor *p = (Processor *) processors->data;

    if (g_slist_length(processors) > 1) {
	return idle_free(g_strdup_printf("%dx %s",
					 g_slist_length(processors),
					 p->model_name));
    } else {
	return p->model_name;
    }
}

gchar *get_storage_devices(void)
{
    scan_storage(FALSE);

    return storage_list;
}

gchar *get_printers(void)
{
    scan_printers(FALSE);

    return printer_list;
}

gchar *get_input_devices(void)
{
    scan_input(FALSE);

    return input_list;
}

gchar *get_processor_count(void)
{
    scan_processors(FALSE);

    return g_strdup_printf("%d", g_slist_length(processors));
}

gchar *get_processor_frequency(void)
{
    Processor *p;

    scan_processors(FALSE);

    p = (Processor *)processors->data;
    if (p->cpu_mhz == 0.0f) {
        return g_strdup("Unknown");
    } else {
        return g_strdup_printf("%.0f", p->cpu_mhz);
    }
}

gchar *get_pci_device_description(gchar *pci_id)
{
    gchar *description;
    
    if (!_pci_devices) {
        scan_pci(FALSE);
    }
    
    if ((description = g_hash_table_lookup(_pci_devices, pci_id))) {
        return g_strdup(description);
    }
    
    return NULL;
}

gchar *get_memory_total(void)
{
    scan_memory(FALSE);
    return hi_more_info("Total Memory");    
}

ShellModuleMethod *hi_exported_methods(void)
{
    static ShellModuleMethod m[] = {
        {"getProcessorCount", get_processor_count},
	{"getProcessorName", get_processor_name},
	{"getProcessorFrequency", get_processor_frequency},
	{"getMemoryTotal", get_memory_total},
	{"getStorageDevices", get_storage_devices},
	{"getPrinters", get_printers},
	{"getInputDevices", get_input_devices},
	{"getPCIDeviceDescription", get_pci_device_description},
	{NULL}
    };

    return m;
}

gchar *hi_more_info(gchar * entry)
{
    gchar *info = (gchar *) g_hash_table_lookup(moreinfo, entry);
    
    if (info)
	return g_strdup(info);

    return g_strdup("?");
}

gchar *hi_get_field(gchar * field)
{
    gchar *info = (gchar *) g_hash_table_lookup(moreinfo, field);

    if (info)
	return g_strdup(info);

    return g_strdup(field);
}

#if defined(ARCH_x86)
void scan_dmi(gboolean reload)
{
    SCAN_START();
    __scan_dmi();
    SCAN_END();
}

void scan_spd(gboolean reload)
{
    SCAN_START();
    scan_spd_do();
    SCAN_END();
}
#endif

void scan_processors(gboolean reload)
{
    SCAN_START();
    if (!processors)
	processors = processor_scan();
    SCAN_END();
}

void scan_memory(gboolean reload)
{
    SCAN_START();
    scan_memory_do();
    SCAN_END();
}

void scan_battery(gboolean reload)
{
    SCAN_START();
    scan_battery_do();
    SCAN_END();
}

void scan_pci(gboolean reload)
{
    SCAN_START();
    scan_pci_do();
    SCAN_END();
}

void scan_sensors(gboolean reload)
{
    SCAN_START();
    scan_sensors_do();
    SCAN_END();
}

void scan_printers(gboolean reload)
{
    SCAN_START();
    scan_printers_do();
    SCAN_END();
}

void scan_storage(gboolean reload)
{
    SCAN_START();
    g_free(storage_list);
    storage_list = g_strdup("");

    __scan_ide_devices();
    __scan_scsi_devices();
    SCAN_END();
}

void scan_input(gboolean reload)
{
    SCAN_START();
    __scan_input_devices();
    SCAN_END();
}

void scan_usb(gboolean reload)
{
    SCAN_START();
    __scan_usb();
    SCAN_END();
}

gchar *callback_processors()
{
    return processor_get_info(processors);
}

#if defined(ARCH_x86)
gchar *callback_dmi()
{
    return g_strdup(dmi_info);
}

gchar *callback_spd()
{
    return g_strdup(spd_info);
}
#endif

gchar *callback_memory()
{
    return g_strdup_printf("[Memory]\n"
			   "%s\n"
			   "[$ShellParam$]\n"
			   "ViewType=2\n"
			   "LoadGraphSuffix= kB\n"
			   "RescanInterval=2000\n"
			   "%s\n", meminfo, lginterval);
}

gchar *callback_battery()
{
    return g_strdup_printf("%s\n"
			   "[$ShellParam$]\n"
			   "ReloadInterval=4000\n", battery_list);
}

gchar *callback_pci()
{
    return g_strdup_printf("[PCI Devices]\n"
			   "%s"
			   "[$ShellParam$]\n" "ViewType=1\n", pci_list);
}

gchar *callback_sensors()
{
    return g_strdup_printf("[$ShellParam$]\n"
			   "ReloadInterval=5000\n" "%s", sensors);
}

gchar *callback_printers()
{
    return g_strdup_printf("%s\n"
                           "[$ShellParam$]\n"
                           "ViewType=1\n"
			   "ReloadInterval=5000\n"
			   "%s", printer_list, printer_icons);
}

gchar *callback_storage()
{
    return g_strdup_printf("%s\n"
			   "[$ShellParam$]\n"
			   "ReloadInterval=5000\n"
			   "ViewType=1\n%s", storage_list, storage_icons);
}

gchar *callback_input()
{
    return g_strdup_printf("[Input Devices]\n"
			   "%s"
			   "[$ShellParam$]\n"
			   "ViewType=1\n"
			   "ReloadInterval=5000\n%s", input_list,
			   input_icons);
}

gchar *callback_usb()
{
    return g_strdup_printf("%s"
			   "[$ShellParam$]\n"
			   "ViewType=1\n"
			   "ReloadInterval=5000\n", usb_list);
}

ModuleEntry *hi_module_get_entries(void)
{
    return entries;
}

gchar *hi_module_get_name(void)
{
    return g_strdup("Devices");
}

guchar hi_module_get_weight(void)
{
    return 85;
}

void hi_module_init(void)
{
    if (!g_file_test("/usr/share/misc/pci.ids", G_FILE_TEST_EXISTS)) {
        static SyncEntry se = {
             .fancy_name = "Update PCI ID listing",
             .name = "GetPCIIds",
             .save_to = "pci.ids",
             .get_data = NULL
        };

        sync_manager_add_entry(&se);
    }

#if defined(ARCH_x86)
    {
      static SyncEntry se = {
        .fancy_name = "Update CPU feature database",
        .name = "RecvCPUFlags",
        .save_to = "cpuflags.conf",
        .get_data = NULL
      };
      
      sync_manager_add_entry(&se);
    }
#endif	/* defined(ARCH_x86) */

    moreinfo = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    init_memory_labels();
    init_cups();
}

void hi_module_deinit(void)
{
    g_hash_table_destroy(moreinfo);
    g_hash_table_destroy(memlabels);
    g_module_close(cups);
}

ModuleAbout *hi_module_get_about(void)
{
    static ModuleAbout ma[] = {
	{
	 .author = "Leandro A. F. Pereira",
	 .description = "Gathers information about hardware devices",
	 .version = VERSION,
	 .license = "GNU GPL version 2"}
    };

    return ma;
}

gchar **hi_module_get_dependencies(void)
{
    static gchar *deps[] = { "libcomputer.so", NULL };

    return deps;
}
