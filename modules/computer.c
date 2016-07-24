/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
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
#include <sys/stat.h>

#include <hardinfo.h>
#include <iconcache.h>
#include <shell.h>

#include <vendor.h>

#include "computer.h"

/* Callbacks */
gchar *callback_summary();
gchar *callback_os();
gchar *callback_modules();
gchar *callback_boots();
gchar *callback_locales();
gchar *callback_fs();
gchar *callback_display();
gchar *callback_network();
gchar *callback_users();
gchar *callback_groups();
gchar *callback_env_var();
#if GLIB_CHECK_VERSION(2,14,0)
gchar *callback_dev();
#endif /* GLIB_CHECK_VERSION(2,14,0) */

/* Scan callbacks */
void scan_summary(gboolean reload);
void scan_os(gboolean reload);
void scan_modules(gboolean reload);
void scan_boots(gboolean reload);
void scan_locales(gboolean reload);
void scan_fs(gboolean reload);
void scan_display(gboolean reload);
void scan_network(gboolean reload);
void scan_users(gboolean reload);
void scan_groups(gboolean reload);
void scan_env_var(gboolean reload);
#if GLIB_CHECK_VERSION(2,14,0)
void scan_dev(gboolean reload);
#endif /* GLIB_CHECK_VERSION(2,14,0) */

static ModuleEntry entries[] = {
    {N_("Summary"), "summary.png", callback_summary, scan_summary, MODULE_FLAG_NONE},
    {N_("Operating System"), "os.png", callback_os, scan_os, MODULE_FLAG_NONE},
    {N_("Kernel Modules"), "module.png", callback_modules, scan_modules, MODULE_FLAG_NONE},
    {N_("Boots"), "boot.png", callback_boots, scan_boots, MODULE_FLAG_NONE},
    {N_("Languages"), "language.png", callback_locales, scan_locales, MODULE_FLAG_NONE},
    {N_("Filesystems"), "dev_removable.png", callback_fs, scan_fs, MODULE_FLAG_NONE},
    {N_("Display"), "monitor.png", callback_display, scan_display, MODULE_FLAG_NONE},
    {N_("Environment Variables"), "environment.png", callback_env_var, scan_env_var, MODULE_FLAG_NONE},
#if GLIB_CHECK_VERSION(2,14,0)
    {N_("Development"), "devel.png", callback_dev, scan_dev, MODULE_FLAG_NONE},
#endif /* GLIB_CHECK_VERSION(2,14,0) */
    {N_("Users"), "users.png", callback_users, scan_users, MODULE_FLAG_NONE},
    {N_("Groups"), "users.png", callback_groups, scan_groups, MODULE_FLAG_NONE},
    {NULL},
};

gchar *module_list = NULL;
Computer *computer = NULL;

gchar *hi_more_info(gchar * entry)
{
    gchar *info = moreinfo_lookup_with_prefix("COMP", entry);

    if (info)
	return g_strdup(info);

    return g_strdup_printf("[%s]", entry);
}

gchar *hi_get_field(gchar * field)
{
    gchar *tmp;

    setlocale(LC_ALL, "C"); //Load Average is not updated if locale is not C, switch locale to C

    if (g_str_equal(field, "Memory")) {
	MemoryInfo *mi = computer_get_memory();
	tmp = g_strdup_printf(_("%dMB (%dMB used)"), mi->total, mi->used);
	g_free(mi);
    } else if (g_str_equal(field, "Uptime")) {
	tmp = computer_get_formatted_uptime();
    } else if (g_str_equal(field, "Date/Time")) {
	time_t t = time(NULL);

	tmp = g_new0(gchar, 64);
	strftime(tmp, 64, "%c", localtime(&t));
    } else if (g_str_equal(field, "Load Average")) {
	tmp = computer_get_formatted_loadavg();
    } else if (g_str_equal(field, "Available entropy in /dev/random")) {
	tmp = computer_get_entropy_avail();
    } else {
	tmp = g_strdup("");
    }
    setlocale(LC_ALL, "");// switch locale back to normal
    return tmp;
}

void scan_summary(gboolean reload)
{
    SCAN_START();
    module_entry_scan_all_except(entries, 0);
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

void scan_boots(gboolean reload)
{
    SCAN_START();
    scan_boots_real();
    SCAN_END();
}

void scan_locales(gboolean reload)
{
    SCAN_START();
    scan_os(FALSE);
    scan_languages(computer->os);
    SCAN_END();
}

void scan_fs(gboolean reload)
{
    SCAN_START();
    scan_filesystems();
    SCAN_END();
}

void scan_display(gboolean reload)
{
    SCAN_START();
    computer->display = computer_get_display();
    SCAN_END();
}

void scan_users(gboolean reload)
{
    SCAN_START();
    scan_users_do();
    SCAN_END();
}

void scan_groups(gboolean reload)
{
    SCAN_START();
    scan_groups_do();
    SCAN_END();
}

#if GLIB_CHECK_VERSION(2,14,0)
static gchar *dev_list = NULL;
void scan_dev(gboolean reload)
{
    SCAN_START();
    
    int i;
    struct {
       gchar *compiler_name;
       gchar *version_command;
       gchar *regex;
       gboolean stdout;
    } detect_lang[] = {
       { N_("Scripting Languages"), NULL, FALSE },
       { N_("CPython"), "python -V", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("Perl"), "perl -v", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("PHP"), "php --version", "\\d+\\.\\d+\\.\\S+", TRUE},
       { N_("Ruby"), "ruby --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Bash"), "bash --version", "\\d+\\.\\d+\\.\\S+", TRUE},
       { N_("Compilers"), NULL, FALSE },
       { N_("C (GCC)"), "gcc -v", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("C (Clang)"), "clang -v", "\\d+\\.\\d+", FALSE },
       { N_("D (dmd)"), "dmd --help", "\\d+\\.\\d+", TRUE },
       { N_("Java"), "javac -version", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("CSharp (Mono, old)"), "mcs --version", "\\d+\\.\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("CSharp (Mono)"), "gmcs --version", "\\d+\\.\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Vala"), "valac --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Haskell (GHC)"), "ghc -v", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("FreePascal"), "fpc -iV", "\\d+\\.\\d+\\.?\\d*", TRUE },
       { N_("Go"), "go version", "\\d+\\.\\d+\\.?\\d* ", TRUE },
       { N_("Tools"), NULL, FALSE },
       { N_("make"), "make --version", "\\d+\\.\\d+", TRUE },
       { N_("GDB"), "gdb --version", "\\d+\\.\\S+", TRUE },
       { N_("strace"), "strace -V", "\\d+\\.\\d+\\.?\\d*", TRUE },
       { N_("valgrind"), "valgrind --version", "\\d+\\.\\d+\\.\\S+", TRUE },
       { N_("QMake"), "qmake --version", "\\d+\\.\\S+", TRUE},
       { N_("CMake"), "cmake --version", "\\d+\\.\\d+\\.?\\d*", TRUE},
    };
    
    g_free(dev_list);
    
    dev_list = g_strdup("");
    
    for (i = 0; i < G_N_ELEMENTS(detect_lang); i++) {
       gchar *version = NULL;
       gchar *output;
       gchar *temp;
       GRegex *regex;
       GMatchInfo *match_info;
       gboolean found;
       
       if (!detect_lang[i].regex) {
            dev_list = h_strdup_cprintf("[%s]\n", dev_list, detect_lang[i].compiler_name);
            continue;
       }
       
       if (detect_lang[i].stdout) {
            found = g_spawn_command_line_sync(detect_lang[i].version_command, &output, NULL, NULL, NULL);
       } else {
            found = g_spawn_command_line_sync(detect_lang[i].version_command, NULL, &output, NULL, NULL);
       }
       
       if (found) {
           regex = g_regex_new(detect_lang[i].regex, 0, 0, NULL);

           g_regex_match(regex, output, 0, &match_info);
           if (g_match_info_matches(match_info)) {
               version = g_match_info_fetch(match_info, 0);
           }
           
           g_match_info_free(match_info);
           g_regex_unref(regex);
           g_free(output);
       }
       
       if (version) {
           dev_list = h_strdup_cprintf("%s=%s\n", dev_list, detect_lang[i].compiler_name, version);
           g_free(version);
       } else {
           dev_list = h_strdup_cprintf(_("%s=Not found\n"), dev_list, detect_lang[i].compiler_name);
       }
       
       temp = g_strdup_printf(_("Detecting version: %s"),
                              detect_lang[i].compiler_name);
       shell_status_update(temp);
       g_free(temp);
    }
    
    SCAN_END();
}

gchar *callback_dev()
{
    return g_strdup_printf(_("[$ShellParam$]\n"
			   "ColumnTitle$TextValue=Program\n"
			   "ColumnTitle$Value=Version\n"
			   "ShowColumnHeaders=true\n"
                           "%s"), dev_list);
}
#endif /* GLIB_CHECK_VERSION(2,14,0) */

/* Table based off imvirt by Thomas Liske <liske@ibh.de>
   Copyright (c) 2008 IBH IT-Service GmbH under GPLv2. */
gchar *computer_get_virtualization()
{
    gboolean found = FALSE;
    gint i, j;
    gchar *files[] = {
        "/proc/scsi/scsi",
        "/proc/cpuinfo",
        "/var/log/dmesg",
        NULL
    };
    const static struct {
        gchar *str;
        gchar *vmtype;
    } vm_types[] = {
        /* VMware */
        { "VMware", "Virtual (VMware)" },
        { ": VMware Virtual IDE CDROM Drive", "Virtual (VMware)" },
        /* QEMU */
        { "QEMU", "Virtual (QEMU)" },
        { "QEMU Virtual CPU", "Virtual (QEMU)" },
        { ": QEMU HARDDISK", "Virtual (QEMU)" },
        { ": QEMU CD-ROM", "Virtual (QEMU)" },
        /* Generic Virtual Machine */
        { ": Virtual HD,", "Virtual (Unknown)" },
        { ": Virtual CD,", "Virtual (Unknown)" },
        /* Virtual Box */
        { "VBOX", "Virtual (VirtualBox)" },
        { ": VBOX HARDDISK", "Virtual (VirtualBox)" },
        { ": VBOX CD-ROM", "Virtual (VirtualBox)" },
        /* Xen */
        { "Xen virtual console", "Virtual (Xen)" },
        { "Xen reported: ", "Virtual (Xen)" },
        { "xen-vbd: registered block device", "Virtual (Xen)" },
        { NULL }
    };
    
    DEBUG("Detecting virtual machine");

    if (g_file_test("/proc/xen", G_FILE_TEST_EXISTS)) {
         DEBUG("/proc/xen found; assuming Xen");
         return g_strdup("Xen");
    }
    
    for (i = 0; files[i+1]; i++) {
         gchar buffer[512];
         FILE *file;
         
         if ((file = fopen(files[i], "r"))) {
              while (!found && fgets(buffer, 512, file)) {
                  for (j = 0; vm_types[j+1].str; j++) {
                      if (strstr(buffer, vm_types[j].str)) {
                         found = TRUE;
                         break;
                      }
                  }
              }
              
              fclose(file);
              
              if (found) {
                  DEBUG("%s found (by reading file %s)",
                        vm_types[j].vmtype, files[i]);
                  return g_strdup(vm_types[j].vmtype);
              }
         }
         
    }
    
    DEBUG("no virtual machine detected; assuming physical machine");
    
    return g_strdup(_("Physical machine"));
}

gchar *callback_summary()
{
    gchar *processor_name, *alsa_cards;
    gchar *input_devices, *printers;
    gchar *storage_devices, *summary;
    gchar *virt;
    
    processor_name  = module_call_method("devices::getProcessorName");
    alsa_cards      = computer_get_alsacards(computer);
    input_devices   = module_call_method("devices::getInputDevices");
    printers        = module_call_method("devices::getPrinters");
    storage_devices = module_call_method("devices::getStorageDevices");
    virt            = computer_get_virtualization();

    summary = g_strdup_printf(_("[$ShellParam$]\n"
			      "UpdateInterval$Memory=1000\n"
			      "UpdateInterval$Date/Time=1000\n"
			      "#ReloadInterval=5000\n"
			      "[Computer]\n"
			      "Processor=%s\n"
			      "Memory=...\n"
			      "Machine Type=%s\n"
			      "Operating System=%s\n"
			      "User Name=%s\n"
			      "Date/Time=...\n"
			      "[Display]\n"
			      "Resolution=%dx%d pixels\n"
			      "OpenGL Renderer=%s\n"
			      "X11 Vendor=%s\n"
			      "\n%s\n"
			      "[Input Devices]\n%s\n"
			      "\n%s\n"
			      "\n%s\n"),
			      processor_name,
			      virt,
			      computer->os->distro,
			      computer->os->username,
			      computer->display->width,
			      computer->display->height,
			      computer->display->ogl_renderer,
			      computer->display->vendor,
			      alsa_cards,
			      input_devices, printers, storage_devices);

    g_free(processor_name);
    g_free(alsa_cards);
    g_free(input_devices);
    g_free(printers);
    g_free(storage_devices);
    g_free(virt);

    return summary;
}

gchar *callback_os()
{
    return g_strdup_printf(_("[$ShellParam$]\n"
			   "UpdateInterval$Uptime=10000\n"
			   "UpdateInterval$Load Average=1000\n"
			   "UpdateInterval$Available entropy in /dev/random=1000\n"
			   "[Version]\n"
			   "Kernel=%s\n"
			   "Version=%s\n"
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
			   "Load Average=...\n"
			   "Available entropy in /dev/random=..."),
			   computer->os->kernel,
			   computer->os->kernel_version,
			   computer->os->libc,
			   computer->os->distro,
			   computer->os->hostname,
			   computer->os->username,
			   computer->os->language,
			   computer->os->homedir, computer->os->desktop,
			   computer->os->entropy_avail);
}

gchar *callback_modules()
{
    return g_strdup_printf(_("[Loaded Modules]\n"
			   "%s"
			   "[$ShellParam$]\n"
			   "ViewType=1\n"
			   "ColumnTitle$TextValue=Name\n"
			   "ColumnTitle$Value=Description\n"
			   "ShowColumnHeaders=true\n"), module_list);
}

gchar *callback_boots()
{
    return g_strdup_printf(_("[$ShellParam$]\n"
			   "ColumnTitle$TextValue=Date & Time\n"
			   "ColumnTitle$Value=Kernel Version\n"
			   "ShowColumnHeaders=true\n"
			   "\n"
			   "%s"), computer->os->boots);
}

gchar *callback_locales()
{
    return g_strdup_printf(_("[$ShellParam$]\n"
			   "ViewType=1\n"
			   "ColumnTitle$TextValue=Language Code\n"
			   "ColumnTitle$Value=Name\n"
			   "ShowColumnHeaders=true\n"
			   "[Available Languages]\n"
			   "%s"), computer->os->languages);
}

gchar *callback_fs()
{
    return g_strdup_printf(_("[$ShellParam$]\n"
			   "ViewType=4\n"
			   "ReloadInterval=5000\n"
			   "Zebra=1\n"
			   "NormalizePercentage=false\n"
			   "ColumnTitle$Extra1=Mount Point\n"
			   "ColumnTitle$Progress=Usage\n"
			   "ColumnTitle$TextValue=Device\n"
			   "ShowColumnHeaders=true\n"
			   "[Mounted File Systems]\n%s\n"), fs_list);
}

gchar *callback_display()
{
    return g_strdup_printf(_("[Display]\n"
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
			   "Direct Rendering=%s\n"),
			   computer->display->width,
			   computer->display->height,
			   computer->display->vendor,
			   computer->display->version,
			   computer->display->monitors,
			   computer->display->extensions,
			   computer->display->ogl_vendor,
			   computer->display->ogl_renderer,
			   computer->display->ogl_version,
			   computer->display->dri ? _("Y_es") : _("No"));
}

gchar *callback_users()
{
    return g_strdup_printf("[$ShellParam$]\n"
			   "ReloadInterval=10000\n"
			   "ViewType=1\n"
			   "[Users]\n"
			   "%s\n", users);
}

gchar *callback_groups()
{
    return g_strdup_printf(_("[$ShellParam$]\n"
			   "ReloadInterval=10000\n"
			   "ColumnTitle$TextValue=Name\n"
			   "ColumnTitle$Value=Group ID\n"
			   "ShowColumnHeaders=true\n"
			   "[Groups]\n"
			   "%s\n"), groups);
}

gchar *get_os_kernel(void)
{
    scan_os(FALSE);
    return g_strdup(computer->os->kernel);
}

gchar *get_os(void)
{
    scan_os(FALSE);
    return g_strdup(computer->os->distro);
}

gchar *get_display_summary(void)
{
    scan_display(FALSE);

    return g_strdup_printf("%dx%d\n"
                           "%s\n"
                           "%s",
                           computer->display->width,
                           computer->display->height,
                           computer->display->ogl_renderer,
                           computer->display->vendor);
}

gchar *get_kernel_module_description(gchar *module)
{
    gchar *description;
    
    if (!_module_hash_table) {
        scan_modules(FALSE);
    }
    
    description = g_hash_table_lookup(_module_hash_table, module);
    if (!description) {
        return NULL;
    }
    
    return g_strdup(description);
}

gchar *get_audio_cards(void)
{
    if (!computer->alsa) {
      computer->alsa = computer_get_alsainfo();
    }
    
    return computer_get_alsacards(computer);
}

ShellModuleMethod *hi_exported_methods(void)
{
    static ShellModuleMethod m[] = {
	{"getOSKernel", get_os_kernel},
	{"getOS", get_os},
	{"getDisplaySummary", get_display_summary},
	{"getAudioCards", get_audio_cards},
	{"getKernelModuleDescription", get_kernel_module_description},
	{NULL}
    };

    return m;
}

ModuleEntry *hi_module_get_entries(void)
{
    return entries;
}

gchar *hi_module_get_name(void)
{
    return g_strdup(_("Computer"));
}

guchar hi_module_get_weight(void)
{
    return 80;
}

gchar **hi_module_get_dependencies(void)
{
    static gchar *deps[] = { "devices.so", NULL };

    return deps;
}

gchar *hi_module_get_summary(void)
{
    return g_strdup("[Operating System]\n"
                    "Icon=os.png\n"
                    "Method=computer::getOS\n"
                    "[CPU]\n"
                    "Icon=processor.png\n"
                    "Method=devices::getProcessorName\n"
                    "[RAM]\n"
                    "Icon=memory.png\n"
                    "Method=devices::getMemoryTotal\n"
                    "[Motherboard]\n"
                    "Icon=module.png\n"
                    "Method=devices::getMotherboard\n"
                    "[Graphics]\n"
                    "Icon=monitor.png\n"
                    "Method=computer::getDisplaySummary\n"
                    "[Storage]\n"
                    "Icon=hdd.png\n"
                    "Method=devices::getStorageDevices\n"
                    "[Printers]\n"
                    "Icon=printer.png\n"
                    "Method=devices::getPrinters\n"
                    "[Audio]\n"
                    "Icon=audio.png\n"
                    "Method=computer::getAudioCards\n");
}

void hi_module_deinit(void)
{
    if (computer->os) {
        g_free(computer->os->kernel);
        g_free(computer->os->libc);
        g_free(computer->os->distrocode);
        g_free(computer->os->distro);
        g_free(computer->os->hostname);
        g_free(computer->os->language);
        g_free(computer->os->homedir);
        g_free(computer->os->kernel_version);
        g_free(computer->os->languages);
        g_free(computer->os->desktop);
        g_free(computer->os->username);
        g_free(computer->os->boots);
        g_free(computer->os);
    }
    
    if (computer->display) {
        g_free(computer->display->ogl_vendor);
        g_free(computer->display->ogl_renderer);
        g_free(computer->display->ogl_version);
        g_free(computer->display->display_name);
        g_free(computer->display->vendor);
        g_free(computer->display->version);
        g_free(computer->display->extensions);
        g_free(computer->display->monitors);
        g_free(computer->display);
    }
    
    if (computer->alsa) {
        g_slist_free(computer->alsa->cards);
        g_free(computer->alsa);
    }
    
    g_free(computer->date_time);
    g_free(computer);
    
    moreinfo_del_with_prefix("COMP");
}

void hi_module_init(void)
{
    computer = g_new0(Computer, 1);
}

ModuleAbout *hi_module_get_about(void)
{
    static ModuleAbout ma[] = {
	{
	 .author = "Leandro A. F. Pereira",
	 .description = N_("Gathers high-level computer information"),
	 .version = VERSION,
	 .license = "GNU GPL version 2"}
    };

    return ma;
}

