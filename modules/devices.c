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
#include "dt_util.h"

gchar *callback_processors();
gchar *callback_memory();
gchar *callback_battery();
gchar *callback_pci();
gchar *callback_sensors();
gchar *callback_printers();
gchar *callback_storage();
gchar *callback_input();
gchar *callback_usb();
#if defined(ARCH_x86) || defined(ARCH_x86_64)
gchar *callback_dmi();
gchar *callback_spd();
#endif
gchar *callback_dtree();
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
#if defined(ARCH_x86) || defined(ARCH_x86_64)
void scan_dmi(gboolean reload);
void scan_spd(gboolean reload);
#endif
void scan_dtree(gboolean reload);
void scan_device_resources(gboolean reload);

gboolean root_required_for_resources(void);

gchar *hi_more_info(gchar *entry);

enum {
    ENTRY_DTREE,
    ENTRY_PROCESSOR,
    ENTRY_MEMORY,
    ENTRY_PCI,
    ENTRY_USB,
    ENTRY_PRINTERS,
    ENTRY_BATTERY,
    ENTRY_SENSORS,
    ENTRY_INPUT,
    ENTRY_STORAGE,
    ENTRY_DMI,
    ENTRY_SPD,
    ENTRY_RESOURCES
};

static ModuleEntry entries[] = {
    [ENTRY_PROCESSOR] = {N_("Processor"), "processor.png", callback_processors, scan_processors, MODULE_FLAG_NONE},
    [ENTRY_MEMORY] = {N_("Memory"), "memory.png", callback_memory, scan_memory, MODULE_FLAG_NONE},
    [ENTRY_PCI] = {N_("PCI Devices"), "devices.png", callback_pci, scan_pci, MODULE_FLAG_NONE},
    [ENTRY_USB] = {N_("USB Devices"), "usb.png", callback_usb, scan_usb, MODULE_FLAG_NONE},
    [ENTRY_PRINTERS] = {N_("Printers"), "printer.png", callback_printers, scan_printers, MODULE_FLAG_NONE},
    [ENTRY_BATTERY] = {N_("Battery"), "battery.png", callback_battery, scan_battery, MODULE_FLAG_NONE},
    [ENTRY_SENSORS] = {N_("Sensors"), "therm.png", callback_sensors, scan_sensors, MODULE_FLAG_NONE},
    [ENTRY_INPUT] = {N_("Input Devices"), "inputdevices.png", callback_input, scan_input, MODULE_FLAG_NONE},
    [ENTRY_STORAGE] = {N_("Storage"), "hdd.png", callback_storage, scan_storage, MODULE_FLAG_NONE},
#if defined(ARCH_x86) || defined(ARCH_x86_64)
    [ENTRY_DMI] = {N_("DMI"), "computer.png", callback_dmi, scan_dmi, MODULE_FLAG_NONE},
    [ENTRY_SPD] = {N_("Memory SPD"), "memory.png", callback_spd, scan_spd, MODULE_FLAG_NONE},
    [ENTRY_DTREE] = {"#"},
#else
    [ENTRY_DMI] = {"#"},
    [ENTRY_SPD] = {"#"},
    [ENTRY_DTREE] = {N_("Device Tree"), "devices.png", callback_dtree, scan_dtree, MODULE_FLAG_NONE},
#endif	/* x86 or x86_64 */
    [ENTRY_RESOURCES] = {N_("Resources"), "resources.png", callback_device_resources, scan_device_resources, MODULE_FLAG_NONE},
    { NULL }
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

#include <vendor.h>

gint proc_cmp (Processor *a, Processor *b) {
    return g_strcmp0(a->model_name, b->model_name);
}

gchar *processor_describe(GSList * processors)
{
    gchar *ret = g_strdup("");
    GSList *tmp, *l;
    Processor *p;
    gchar *cur_str = NULL;
    gint cur_count = 0;

    tmp = g_slist_copy(processors);
    tmp = g_slist_sort(tmp, (GCompareFunc)proc_cmp);

    for (l = tmp; l; l = l->next) {
        p = (Processor*)l->data;
        if (cur_str == NULL) {
            cur_str = p->model_name;
            cur_count = 1;
        } else {
            if(g_strcmp0(cur_str, p->model_name)) {
                ret = h_strdup_cprintf("%s%dx %s", ret, strlen(ret) ? " + " : "", cur_count, cur_str);
                cur_str = p->model_name;
                cur_count = 1;
            } else {
                cur_count++;
            }
        }
    }
    ret = h_strdup_cprintf("%s%dx %s", ret, strlen(ret) ? " + " : "", cur_count, cur_str);
    g_slist_free(tmp);
    return ret;
}

gchar *get_processor_name(void)
{
    scan_processors(FALSE);
    return processor_describe(processors);
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
        return g_strdup(N_("Unknown"));
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
    return moreinfo_lookup ("DEV:MemTotal");
}

gchar *get_motherboard(void)
{
    char *board_name, *board_vendor;
#if defined(ARCH_x86) || defined(ARCH_x86_64)
    scan_dmi(FALSE);

    board_name = moreinfo_lookup("DEV:DMI:Board:Name");
    board_vendor = moreinfo_lookup("DEV:DMI:Board:Vendor");

    if (board_name && board_vendor && *board_name && *board_vendor)
       return g_strconcat(board_vendor, " ", board_name, NULL);
    else if (board_name && *board_name)
       return g_strconcat(board_name, _(" (vendor unknown)"), NULL);
    else if (board_vendor && *board_vendor)
       return g_strconcat(board_vendor, _(" (model unknown)"), NULL);
#endif

    /* use device tree "model" */
    board_vendor = dtr_get_string("/model", 0);
    if (board_vendor != NULL)
        return board_vendor;

    return g_strdup(_("Unknown"));
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
	{"getMotherboard", get_motherboard},
	{NULL}
    };

    return m;
}

gchar *hi_more_info(gchar * entry)
{
    gchar *info = moreinfo_lookup_with_prefix("DEV", entry);

    if (info)
	return g_strdup(info);

    return g_strdup("?");
}

gchar *hi_get_field(gchar * field)
{
    gchar *info = moreinfo_lookup_with_prefix("DEV", field);

    if (info)
	return g_strdup(info);

    return g_strdup(field);
}

#if defined(ARCH_x86) || defined(ARCH_x86_64)
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

void scan_dtree(gboolean reload)
{
    SCAN_START();
    __scan_dtree();
    SCAN_END();
}

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

#if defined(ARCH_x86) || defined(ARCH_x86_64)
gchar *callback_dmi()
{
    return g_strdup(dmi_info);
}

gchar *callback_spd()
{
    return g_strdup(spd_info);
}
#endif

gchar *callback_dtree()
{
    return g_strdup_printf("%s"
        "[$ShellParam$]\n"
        "ViewType=1\n", dtree_info);
}

gchar *callback_memory()
{
    return g_strdup_printf("[Memory]\n"
               "%s\n"
               "[$ShellParam$]\n"
               "ViewType=2\n"
               "LoadGraphSuffix= kB\n"
               "RescanInterval=2000\n"
               "ColumnTitle$TextValue=%s\n"
               "ColumnTitle$Extra1=%s\n"
               "ColumnTitle$Value=%s\n"
               "ShowColumnHeaders=true\n"
               "%s\n", meminfo,
               _("Field"), _("Description"), _("Value"), /* column labels */
               lginterval);
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
    return g_strdup_printf("[Sensors]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ViewType=2\n"
                           "LoadGraphSuffix=\n"
                           "ColumnTitle$TextValue=%s\n"
                           "ColumnTitle$Value=%s\n"
                           "ColumnTitle$Extra1=%s\n"
                           "ShowColumnHeaders=true\n"
                           "RescanInterval=5000\n"
                           "%s",
                           sensors,
                           _("Sensor"), _("Value"), _("Type"), /* column labels */
                           lginterval);
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
    return g_strdup(_("Devices"));
}

guchar hi_module_get_weight(void)
{
    return 85;
}

void hi_module_init(void)
{
    if (!g_file_test("/usr/share/misc/pci.ids", G_FILE_TEST_EXISTS)) {
        static SyncEntry se = {
             .fancy_name = N_("Update PCI ID listing"),
             .name = "GetPCIIds",
             .save_to = "pci.ids",
             .get_data = NULL
        };

        sync_manager_add_entry(&se);
    }

#if defined(ARCH_x86) || defined(ARCH_x86_64)
    {
      static SyncEntry se = {
        .fancy_name = N_("Update CPU feature database"),
        .name = "RecvCPUFlags",
        .save_to = "cpuflags.conf",
        .get_data = NULL
      };

      sync_manager_add_entry(&se);
    }
#endif	/* defined(ARCH_x86) */

    init_memory_labels();
    init_cups();
    sensors_init();
}

void hi_module_deinit(void)
{
    moreinfo_del_with_prefix("DEV");
    sensors_shutdown();
    g_hash_table_destroy(memlabels);
    g_module_close(cups);
}

ModuleAbout *hi_module_get_about(void)
{
    static ModuleAbout ma[] = {
	{
	 .author = "Leandro A. F. Pereira",
	 .description = N_("Gathers information about hardware devices"),
	 .version = VERSION,
	 .license = "GNU GPL version 2"}
    };

    return ma;
}

gchar **hi_module_get_dependencies(void)
{
    static gchar *deps[] = { "computer.so", NULL };

    return deps;
}

const gchar *hi_note_func(gint entry)
{
    if (entry == ENTRY_RESOURCES) {
        if (root_required_for_resources()) {
            return g_strdup(_("Resource information requires superuser privileges"));
        }
    }
    return NULL;
}
