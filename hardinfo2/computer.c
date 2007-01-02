/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

#include <vendor.h>

enum {
    COMPUTER_SUMMARY,
    COMPUTER_OPERATING_SYSTEM,
    COMPUTER_KERNEL_MODULES,
    COMPUTER_LANGUAGE,
    COMPUTER_FILESYSTEMS,
    COMPUTER_SHARES,
    COMPUTER_DISPLAY,
    COMPUTER_NETWORK,
    COMPUTER_USERS,
} Entries;

/* Callbacks */
gchar *callback_summary();
gchar *callback_os();
gchar *callback_modules();
gchar *callback_locales();
gchar *callback_fs();
gchar *callback_shares();
gchar *callback_display();
gchar *callback_network();
gchar *callback_users();

/* Scan callbacks */
void scan_summary(gboolean reload);
void scan_os(gboolean reload);
void scan_modules(gboolean reload);
void scan_locales(gboolean reload);
void scan_fs(gboolean reload);
void scan_shares(gboolean reload);
void scan_display(gboolean reload);
void scan_network(gboolean reload);
void scan_users(gboolean reload);

static ModuleEntry entries[] = {
    { "Summary",		"summary.png",		callback_summary,	scan_summary },
    { "Operating System",	"os.png",		callback_os,		scan_os },
    { "Kernel Modules",		"module.png",		callback_modules,	scan_modules },
    { "Languages",		"language.png",		callback_locales,	scan_locales },
    { "Filesystems",		"dev_removable.png",	callback_fs,		scan_fs },
    { "Shared Directories",	"shares.png",		callback_shares,	scan_shares },
    { "Display",		"monitor.png",		callback_display,	scan_display },
    { "Network Interfaces",	"network.png",		callback_network,	scan_network },
    { "Users",			"users.png",		callback_users,		scan_users },
    { NULL },
};

#include "computer.h"

static GHashTable *moreinfo = NULL;
static gchar *module_list = NULL;
static Computer *computer = NULL;

#include <arch/this/modules.h>
#include <arch/common/languages.h>
#include <arch/this/alsa.h>
#include <arch/common/display.h>
#include <arch/this/loadavg.h>
#include <arch/this/memory.h>
#include <arch/this/uptime.h>
#include <arch/this/os.h>
#include <arch/this/filesystem.h>
#include <arch/this/samba.h>
#include <arch/this/nfs.h>
#include <arch/this/net.h>
#include <arch/common/users.h>

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

    if (g_str_equal(field, "Memory")) {
	MemoryInfo *mi;

	mi = computer_get_memory();
	tmp = g_strdup_printf("%dMB (%dMB used)", mi->total, mi->used);

	g_free(mi);
    } else if (g_str_equal(field, "Uptime")) {
	tmp = computer_get_formatted_uptime();
    } else if (g_str_equal(field, "Date/Time")) {
	time_t t = time(NULL);

	tmp = g_new0(gchar, 32);
	strftime(tmp, 32, "%D / %R", localtime(&t));
    } else if (g_str_equal(field, "Load Average")) {
	tmp = computer_get_formatted_loadavg();
    } else {
	tmp = g_strdup("");
    }

    return tmp;
}

void scan_summary(gboolean reload)
{
    SCAN_START();
    module_entry_scan_all_except(entries, COMPUTER_SUMMARY);
    computer->alsa = computer_get_alsainfo();
    SCAN_END();
}

void scan_os(gboolean reload)
{
    SCAN_START();
    computer->os = computer_get_os();
    SCAN_END();
}

void scan_modules(gboolean reload)
{
    SCAN_START();
    scan_modules_do();
    SCAN_END();
}

void scan_locales(gboolean reload)
{
    SCAN_START();
    scan_languages(computer->os);
    SCAN_END();
}

void scan_fs(gboolean reload)
{
    SCAN_START();
    scan_filesystems();
    SCAN_END();
}

void scan_shares(gboolean reload)
{
    SCAN_START();
    scan_samba_shared_directories();
    scan_nfs_shared_directories();
    SCAN_END();
}

void scan_display(gboolean reload)
{
    SCAN_START();
    computer->display = computer_get_display();
    SCAN_END();
}

void scan_network(gboolean reload)
{
    SCAN_START();
    scan_net_interfaces();
    SCAN_END();
}

void scan_users(gboolean reload)
{
    SCAN_START();
    scan_users_do();
    SCAN_END();
}

gchar *callback_summary()
{
    return g_strdup_printf("[$ShellParam$]\n"
                           "UpdateInterval$Memory=1000\n"
                           "UpdateInterval$Date/Time=1000\n"
                           "#ReloadInterval=5000\n"
                           "[Computer]\n"
                           "Processor=%s\n"
                           "Memory=...\n"
                           "Operating System=%s\n"
                           "User Name=%s\n"
                           "Date/Time=...\n"
                           "[Display]\n"
                           "Resolution=%dx%d pixels\n"
                           "OpenGL Renderer=%s\n"
                           "X11 Vendor=%s\n"
                           "[Multimedia]\n"
                           "%s\n"
                           "[Input Devices]\n%s\n"
                           "%s\n"
                           "%s\n",
                           (gchar*)idle_free(module_call_method("devices::getProcessorName")),
                           computer->os->distro,
                           computer->os->username,
                           computer->display->width,
                           computer->display->height,
                           computer->display->ogl_renderer,
                           computer->display->vendor,
                           (gchar*)idle_free(computer_get_alsacards(computer)),
                           (gchar*)idle_free(module_call_method("devices::getInputDevices")),
                           (gchar*)idle_free(module_call_method("devices::getPrinters")),
                           (gchar*)idle_free(module_call_method("devices::getStorageDevices")));
}

gchar *callback_os()
{
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
                           "Uptime=...\n"
                           "Load Average=...",
                           computer->os->kernel,
                           computer->os->compiled_date,
                           computer->os->libc,
                           computer->os->distro,
                           computer->os->hostname,
                           computer->os->username,
                           computer->os->language,
                           computer->os->homedir,
                           computer->os->desktop);
}

gchar *callback_modules()
{
    return g_strdup_printf("[Loaded Modules]\n"
                           "%s"
                           "[$ShellParam$]\n"
                           "ViewType=1",
                           module_list);
}

gchar *callback_locales()
{
    return g_strdup_printf("[$ShellParam$]\n"
                           "ViewType=1\n"
                           "[Available Languages]\n"
                           "%s", computer->os->languages);
}

gchar *callback_fs()
{
    return g_strdup_printf("[$ShellParam$]\n"
                           "ViewType=1\n"
                           "ReloadInterval=5000\n"
                           "[Mounted File Systems]\n%s\n", fs_list);
}

gchar *callback_shares()
{
    return g_strdup_printf("[SAMBA]\n"
                           "%s\n"
                           "[NFS]\n"
                           "%s", smb_shares_list, nfs_shares_list);
}

gchar *callback_display()
{
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
                           "Version=%s\n"
                           "Direct Rendering=%s\n",
                           computer->display->width,
                           computer->display->height,
                           computer->display->vendor,
                           computer->display->version,
                           computer->display->monitors,
                           computer->display->extensions,
                           computer->display->ogl_vendor,
                           computer->display->ogl_renderer,
                           computer->display->ogl_version,
                           computer->display->dri ? "Yes" : "No");
}

gchar *callback_network()
{
    return g_strdup_printf("[$ShellParam$]\n"
                           "ReloadInterval=3000\n"
                           "ViewType=1\n"
                           "%s", network_interfaces);
}

gchar *callback_users()
{
    return g_strdup_printf("[$ShellParam$]\n"
                           "ReloadInterval=10000\n"
                           "ViewType=1\n"
                           "[Human Users]\n"
                           "%s\n"
                           "[System Users]\n"
                           "%s\n", human_users, sys_users);
}

ModuleEntry *
hi_module_get_entries(void)
{
    return entries;
}

gchar *
hi_module_get_name(void)
{
    return g_strdup("Computer");
}

guchar
hi_module_get_weight(void)
{
    return 80;
}

gchar **
hi_module_get_dependencies(void)
{
    static gchar *deps[] = { "devices.so", NULL };
    
    return deps;
}

void
hi_module_init(void)
{
    computer = g_new0(Computer, 1);
    moreinfo = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}
