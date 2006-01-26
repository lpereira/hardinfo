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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <config.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#include <hardinfo.h>
#include <iconcache.h>
#include <shell.h>

#include <expr.h>

enum {
    COMPUTER_SUMMARY,
    COMPUTER_PROCESSORS,
    COMPUTER_OPERATING_SYSTEM,
    COMPUTER_LANGUAGE,
    COMPUTER_SENSORS,
    COMPUTER_FILESYSTEMS,
    COMPUTER_SHARES,
    COMPUTER_DISPLAY,
    COMPUTER_NETWORK,
/*    COMPUTER_LOADGRAPH,*/
} Entries;

static ModuleEntry hi_entries[] = {
    {"Summary",			"summary.png"},
    {"Processor",		"processor.png"},
    {"Operating System",	"os.png"},
    {"Languages",		"language.png"},
    {"Sensors",			"therm.png"},
    {"Filesystems",		"dev_removable.png"},
    {"Shared Directories",	"shares.png"},
    {"Display",			"monitor.png"},
    {"Network Interfaces",	"network.png"},
/*    {"<s>LoadGraph</s>",	"summary.png"}*/
};

#include "computer.h"

static GHashTable *moreinfo = NULL;

#include <arch/common/languages.h>
#include <arch/this/alsa.h>
#include <arch/common/display.h>
#include <arch/this/loadavg.h>
#include <arch/this/memory.h>
#include <arch/this/uptime.h>
#include <arch/this/processor.h>
#include <arch/this/os.h>
#include <arch/this/filesystem.h>
#include <arch/this/samba.h>
#include <arch/this/sensors.h>
#include <arch/this/net.h>

static Computer *
computer_get_info(void)
{
    Computer *computer;

    computer = g_new0(Computer, 1);
    
    if (moreinfo) {
#ifdef g_hash_table_unref
	g_hash_table_unref(moreinfo);
#else
	g_free(moreinfo);
#endif
    }

    moreinfo = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    shell_status_update("Getting processor information...");
    computer->processor = computer_get_processor();

    shell_status_update("Getting memory information...");
    computer->memory = computer_get_memory();

    shell_status_update("Getting operating system information...");
    computer->os = computer_get_os();

    shell_status_update("Getting display information...");
    computer->display = computer_get_display();

    shell_status_update("Getting sound card information...");
    computer->alsa = computer_get_alsainfo();

    shell_status_update("Getting mounted file system information...");
    scan_filesystems();

    shell_status_update("Getting shared directories...");
    scan_shared_directories();
    
    shell_status_update("Reading sensors...");
    read_sensors();

    shell_status_update("Obtaining network information...");
    scan_net_interfaces();

    computer->date_time = "...";
    return computer;
}

void
hi_reload(gint entry)
{
    switch (entry) {
    case COMPUTER_FILESYSTEMS:
	scan_filesystems();
	break;
    case COMPUTER_NETWORK:
	scan_net_interfaces();
	break;
    case COMPUTER_SENSORS:
	read_sensors();
	break;
    }
}

gchar *
hi_more_info(gchar * entry)
{
    gchar *info = (gchar *) g_hash_table_lookup(moreinfo, entry);

    if (info)
	return g_strdup(info);

    return g_strdup_printf("[Empty %s]", entry);
}

gchar *
hi_get_field(gchar * field)
{
    gchar *tmp;

    if (!strcmp(field, "Memory")) {
	MemoryInfo *mi;

	mi = computer_get_memory();
	tmp = g_strdup_printf("%dMB (%dMB used)", mi->total, mi->used);

	g_free(mi);
    } else if (!strcmp(field, "Random")) {
        return g_strdup_printf("%d", rand() % 200);
    /*} else if (!strcmp(field, "Used Memory")) {
	MemoryInfo *mi;
	
	mi = computer_get_memory();
	tmp = g_strdup_printf("%d", mi->used);

	g_free(mi);*/
    } else if (!strcmp(field, "Uptime")) {
	tmp = computer_get_formatted_uptime();
    } else if (!strcmp(field, "Date/Time")) {
	time_t t = time(NULL);

	tmp = g_new0(gchar, 32);
	strftime(tmp, 32, "%D / %R", localtime(&t));
    } else if (!strcmp(field, "Load Average")) {
	tmp = computer_get_formatted_loadavg();
    } else {
	tmp = g_strdup("");
    }

    return tmp;
}

gchar *
hi_info(gint entry)
{
    static Computer *computer = NULL;
    static gchar *tmp = NULL;

    /*if (tmp != NULL) {
       g_free(tmp);
       tmp = NULL;
    } */

    if (!computer) {
	computer = computer_get_info();
    }

    switch (entry) {
    case COMPUTER_NETWORK:
        return g_strdup_printf("[$ShellParam$]\n"
                               "ReloadInterval=3000\n"
                               "ViewType=1\n"
                               "%s", network_interfaces);
    case COMPUTER_SENSORS:
        return g_strdup_printf("[$ShellParam$]\n"
                               "ReloadInterval=3000\n"
                               "%s", sensors);
    case COMPUTER_SHARES:
        return g_strdup_printf("[SAMBA]\n"
                               "%s", shares_list);
/*    case COMPUTER_LOADGRAPH:
	return g_strdup_printf("[$ShellParam$]\n"
			       "ViewType=2\n"
                               "UpdateInterval$Used Memory=500\n"
                               "LoadGraphInterval$Used Memory=50\n"
                               "LoadGraphInterval$Random=50\n"
                               "[Doh]\n"
                               "Used Memory=bleh\n"
                               "Random=Select me! /o/");*/
    case COMPUTER_FILESYSTEMS:
	return g_strdup_printf("[$ShellParam$]\n"
			       "ViewType=1\n"
			       "ReloadInterval=5000\n"
			       "[Mounted File Systems]\n%s\n", fs_list);
    case COMPUTER_SUMMARY:
	tmp = computer_get_alsacards(computer);
	return g_strdup_printf("[$ShellParam$]\n"
			       "UpdateInterval$Memory=1000\n"
			       "UpdateInterval$Date/Time=1000\n"
			       "[Computer]\n"
			       "Processor=%s\n"
			       "Memory=...\n"
			       "Operating System=%s\n"
			       "User Name=%s\n"
			       "Date/Time=%s\n"
			       "[Display]\n"
			       "Resolution=%dx%d pixels\n"
			       "OpenGL Renderer=%s\n"
			       "X11 Vendor=%s\n"
			       "[Multimedia]\n"
			       "%s\n"
			       "#[Storage]\n"
			       "#IDE Controller=\n"
			       "#SCSI Controller=\n"
			       "#Floppy Drive=\n"
			       "#Disk Drive=\n",
			       computer->processor->model_name,
			       computer->os->distro,
			       computer->os->username,
			       computer->date_time,
			       computer->display->width,
			       computer->display->height,
			       computer->display->ogl_renderer,
			       computer->display->vendor,
			       tmp);
    case COMPUTER_DISPLAY:
        return g_strdup_printf("[Display]\n"
                               "Resolution=%dx%d pixels\n"
                               "Vendor=%s\n"
                               "Version=%s\n"
                               "[Monitors]\n"
                               "%s"
                               "[Extensions]\n"
                               "%s"
                               "[OpenGL]\n"
                               "Vendor=%s\n"
                               "Renderer=%s\n"
                               "Version=%s\n",
                               computer->display->width,
                               computer->display->height,
                               computer->display->vendor,
                               computer->display->version,
                               computer->display->monitors,
                               computer->display->extensions,
                               computer->display->ogl_vendor,
                               computer->display->ogl_renderer,
                               computer->display->ogl_version);
    case COMPUTER_OPERATING_SYSTEM:
	tmp = computer_get_formatted_uptime();
	return g_strdup_printf("[$ShellParam$]\n"
			       "UpdateInterval$Uptime=10000\n"
			       "UpdateInterval$Load Average=1000\n"
			       "[Version]\n"
			       "Kernel=%s\n"
			       "Compiled=%s\n"
			       "C Library=%s\n"
			       "Distribution=%s\n"
			       "[Current Session]\n"
			       "Computer Name=%s\n"
			       "User Name=%s\n"
			       "#Language=%s\n"
			       "Home Directory=%s\n"
			       "Desktop Environment=%s\n"
			       "[Misc]\n"
			       "Uptime=%s\n"
			       "Load Average=...",
			       computer->os->kernel,
			       computer->os->compiled_date,
			       computer->os->libc,
			       computer->os->distro,
			       computer->os->hostname,
			       computer->os->username,
			       computer->os->language,
			       computer->os->homedir,
			       computer->os->desktop, tmp);
    case COMPUTER_LANGUAGE:
	return g_strdup_printf("[$ShellParam$]\n"
			       "ViewType=1\n"
			       "[Available Languages]\n"
			       "%s", computer->os->languages);
    case COMPUTER_PROCESSORS:
	tmp = processor_get_capabilities_from_flags(computer->processor->
						  flags);
	return g_strdup_printf("[Processor]\n"
	                       "Name=%s\n"
	                       "Specification=%s\n"
                               "Family, model, stepping=%d, %d, %d\n"
			       "Vendor=%s\n"
			       "Cache Size=%dkb\n"
			       "Frequency=%.2fMHz\n"
			       "BogoMips=%.2f\n"
			       "Byte Order=%s\n"
			       "[Features]\n"
			       "FDIV Bug=%s\n"
			       "HLT Bug=%s\n"
			       "F00F Bug=%s\n"
			       "Coma Bug=%s\n"
			       "Has FPU=%s\n"
			       "[Capabilities]\n" "%s",
			       computer->processor->strmodel,
			       computer->processor->model_name,
			       computer->processor->family,
			       computer->processor->model,
			       computer->processor->stepping,
			       computer->processor->vendor_id,
			       computer->processor->cache_size,
			       computer->processor->cpu_mhz,
			       computer->processor->bogomips,
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                               "Little Endian",
#else
                               "Big Endian",
#endif
			       computer->processor->bug_fdiv,
			       computer->processor->bug_hlt,
			       computer->processor->bug_f00f,
			       computer->processor->bug_coma,
			       computer->processor->has_fpu,
			       tmp);
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
