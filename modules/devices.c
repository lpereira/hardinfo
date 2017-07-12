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
#if defined(ARCH_x86) || defined(ARCH_x86_64)
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
#if defined(ARCH_x86) || defined(ARCH_x86_64)
void scan_dmi(gboolean reload);
void scan_spd(gboolean reload);
#endif
void scan_device_resources(gboolean reload);

gboolean root_required_for_resources(void);

gchar *hi_more_info(gchar *entry);

enum {
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
#endif	/* x86 or x86_64 */
    [ENTRY_RESOURCES] = {N_("Resources"), "resources.png", callback_device_resources, scan_device_resources, MODULE_FLAG_NONE},
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

/* information table from: http://elinux.org/RPi_HardwareHistory */
static struct {
    char *value, *intro, *model, *pcb, *mem, *mfg;
} rpi_boardinfo[] = {
/*  Value        Introduction  Model Name             PCB rev.  Memory       Manufacturer  *
 *                             Raspberry Pi %s                                            */
  { "Beta",      "Q1 2012",    "B (Beta)",            "?",      "256MB",    "(Beta board)" },
  { "0002",      "Q1 2012",    "B",                   "1.0",    "256MB",    "?" },
  { "0003",      "Q3 2012",    "B (ECN0001)",         "1.0",    "256MB",    "(Fuses mod and D14 removed)" },
  { "0004",      "Q3 2012",    "B",                   "2.0",    "256MB",    "Sony"    },
  { "0005",      "Q4 2012",    "B",                   "2.0",    "256MB",    "Qisda"   },
  { "0006",      "Q4 2012",    "B",                   "2.0",    "256MB",    "Egoman"  },
  { "0007",      "Q1 2013",    "A",                   "2.0",    "256MB",    "Egoman"  },
  { "0008",      "Q1 2013",    "A",                   "2.0",    "256MB",    "Sony"    },
  { "0009",      "Q1 2013",    "A",                   "2.0",    "256MB",    "Qisda"   },
  { "000d",      "Q4 2012",    "B",                   "2.0",    "512MB",    "Egoman" },
  { "000e",      "Q4 2012",    "B",                   "2.0",    "512MB",    "Sony" },
  { "000f",      "Q4 2012",    "B",                   "2.0",    "512MB",    "Qisda" },
  { "0010",      "Q3 2014",    "B+",                  "1.0",    "512MB",    "Sony" },
  { "0011",      "Q2 2014",    "Compute Module 1",    "1.0",    "512MB",    "Sony" },
  { "0012",      "Q4 2014",    "A+",                  "1.1",    "256MB",    "Sony" },
  { "0013",      "Q1 2015",    "B+",                  "1.2",    "512MB",    "?" },
  { "0014",      "Q2 2014",    "Compute Module 1",    "1.0",    "512MB",    "Embest" },
  { "0015",      "?",          "A+",                  "1.1",    "256MB/512MB",    "Embest" },
  { "a01040",    "Unknown",    "2 Model B",           "1.0",    "1GB",      "Sony" },
  { "a01041",    "Q1 2015",    "2 Model B",           "1.1",    "1GB",      "Sony" },
  { "a21041",    "Q1 2015",    "2 Model B",           "1.1",    "1GB",      "Embest" },
  { "a22042",    "Q3 2016",    "2 Model B",           "1.2",    "1GB",      "Embest" },  /* (with BCM2837) */
  { "900021",    "Q3 2016",    "A+",                  "1.1",    "512MB",    "Sony" },
  { "900032",    "Q2 2016?",    "B+",                 "1.2",    "512MB",    "Sony" },
  { "900092",    "Q4 2015",    "Zero",                "1.2",    "512MB",    "Sony" },
  { "900093",    "Q2 2016",    "Zero",                "1.3",    "512MB",    "Sony" },
  { "920093",    "Q4 2016?",   "Zero",                "1.3",    "512MB",    "Embest" },
  { "9000c1",    "Q1 2017",    "Zero W",              "1.1",    "512MB",    "Sony" },
  { "a02082",    "Q1 2016",    "3 Model B",           "1.2",    "1GB",      "Sony" },
  { "a020a0",    "Q1 2017",    "Compute Module 3 or CM3 Lite",    "1.0",    "1GB",    "Sony" },
  { "a22082",    "Q1 2016",    "3 Model B",           "1.2",    "1GB",    "Embest" },
  { "a32082",    "Q4 2016",    "3 Model B",           "1.2",    "1GB",    "Sony Japan" },
  { NULL, NULL, NULL, NULL, NULL, NULL }
};

static gchar *rpi_get_boardname(void) {
    int i = 0, c = 0;
    gchar *ret = NULL;
    gchar *soc = NULL;
    gchar *revision = NULL;
    int overvolt = 0;
    FILE *cpuinfo;
    gchar buffer[128];

    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo)
        return NULL;
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);
        tmp[0] = g_strstrip(tmp[0]);
        tmp[1] = g_strstrip(tmp[1]);
        get_str("Revision", revision);
        get_str("Hardware", soc);
    }
    fclose(cpuinfo);

    if (revision == NULL || soc == NULL) {
        g_free(soc);
        g_free(revision);
        return NULL;
    }

    if (g_str_has_prefix(revision, "1000"))
        overvolt = 1;

    while (rpi_boardinfo[i].value != NULL) {
        if (overvolt)
            /* +4 to ignore the 1000 prefix */
            c = g_strcmp0(revision+4, rpi_boardinfo[i].value);
        else
            c = g_strcmp0(revision, rpi_boardinfo[i].value);

        if (c == 0) {
            ret = g_strdup_printf("Raspberry Pi %s (%s) pcb-rev:%s soc:%s mem:%s mfg-by:%s%s",
                rpi_boardinfo[i].model, rpi_boardinfo[i].intro,
                rpi_boardinfo[i].pcb, soc,
                rpi_boardinfo[i].mem, rpi_boardinfo[i].mfg,
                (overvolt) ? " (over-volted)" : "" );
            break;
        }
        i++;
    }
    g_free(soc);
    g_free(revision);
    return ret;
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
#else
    /* use device tree "model" */
    if (g_file_get_contents("/proc/device-tree/model", &board_vendor, NULL, NULL)) {

        /* if a raspberry pi, try and get a more detailed name */
        if (g_str_has_prefix(board_vendor, "Raspberry Pi")) {
            board_name = rpi_get_boardname();
            if (board_name != NULL) {
                g_free(board_vendor);
                return board_name;
            }
        }

        return board_vendor;
    }
#endif

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
    return g_strdup_printf("[Sensors]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ViewType=2\n"
                           "LoadGraphSuffix=\n"
                           "ColumnTitle$TextValue=Sensor\n"
                           "ColumnTitle$Value=Value\n"
                           "ColumnTitle$Extra1=Type\n"
                           "ShowColumnHeaders=true\n"
			   "RescanInterval=5000\n"
			   "%s",
                           sensors, lginterval);
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
            return g_strdup_printf(_("Resource information requires superuser privileges"));
        }
    }
    return NULL;
}
