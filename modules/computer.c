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

#include <config.h>
#include <gtk/gtk.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <hardinfo.h>
#include <iconcache.h>
#include <shell.h>

#include <vendor.h>

#include "computer.h"

#include "dmi_util.h" /* for dmi_get_str() */
#include "dt_util.h" /* for dtr_get_string() */

#include "info.h"

/* Callbacks */
gchar *callback_summary(void);
gchar *callback_os(void);
gchar *callback_modules(void);
gchar *callback_boots(void);
gchar *callback_locales(void);
gchar *callback_fs(void);
gchar *callback_display(void);
gchar *callback_network(void);
gchar *callback_users(void);
gchar *callback_groups(void);
gchar *callback_env_var(void);
#if GLIB_CHECK_VERSION(2,14,0)
gchar *callback_dev(void);
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

    if (g_str_equal(field, _("Memory"))) {
        MemoryInfo *mi = computer_get_memory();
        tmp = g_strdup_printf(_("%dMB (%dMB used)"), mi->total, mi->used);
        g_free(mi);
    } else if (g_str_equal(field, _("Uptime"))) {
        tmp = computer_get_formatted_uptime();
    } else if (g_str_equal(field, _("Date/Time"))) {
        time_t t = time(NULL);

        tmp = g_new0(gchar, 64);
        strftime(tmp, 64, "%c", localtime(&t));
    } else if (g_str_equal(field, _("Load Average"))) {
        tmp = computer_get_formatted_loadavg();
    } else if (g_str_equal(field, _("Available entropy in /dev/random"))) {
        tmp = computer_get_entropy_avail();
    } else {
        tmp = g_strdup_printf("Unknown field: %s", field);
    }
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
       { N_("Gambas3 (gbr3)"), "gbr3 --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Python"), "python -V", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("Python2"), "python2 -V", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("Python3"), "python3 -V", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Perl"), "perl -v", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Perl6 (VM)"), "perl6 -v", "(?<=This is ).*", TRUE },
       { N_("Perl6"), "perl6 -v", "(?<=implementing Perl )\\w*\\.\\w*", TRUE },
       { N_("PHP"), "php --version", "\\d+\\.\\d+\\.\\S+", TRUE},
       { N_("Ruby"), "ruby --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Bash"), "bash --version", "\\d+\\.\\d+\\.\\S+", TRUE},
       { N_("Compilers"), NULL, FALSE },
       { N_("C (GCC)"), "gcc -v", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("C (Clang)"), "clang -v", "\\d+\\.\\d+", FALSE },
       { N_("D (dmd)"), "dmd --help", "\\d+\\.\\d+", TRUE },
       { N_("Gambas3 (gbc3)"), "gbc3 --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Java"), "javac -version", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("CSharp (Mono, old)"), "mcs --version", "\\d+\\.\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("CSharp (Mono)"), "gmcs --version", "\\d+\\.\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Vala"), "valac --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Haskell (GHC)"), "ghc -v", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("FreePascal"), "fpc -iV", "\\d+\\.\\d+\\.?\\d*", TRUE },
       { N_("Go"), "go version", "\\d+\\.\\d+\\.?\\d* ", TRUE },
       { N_("Tools"), NULL, FALSE },
       { N_("make"), "make --version", "\\d+\\.\\d+", TRUE },
       { N_("GDB"), "gdb --version", "(?<=^GNU gdb ).*", TRUE },
       { N_("strace"), "strace -V", "\\d+\\.\\d+\\.?\\d*", TRUE },
       { N_("valgrind"), "valgrind --version", "\\d+\\.\\d+\\.\\S+", TRUE },
       { N_("QMake"), "qmake --version", "\\d+\\.\\S+", TRUE},
       { N_("CMake"), "cmake --version", "\\d+\\.\\d+\\.?\\d*", TRUE},
       { N_("Gambas3 IDE"), "gambas3 --version", "\\d+\\.\\d+\\.\\d+", TRUE },
    };

    g_free(dev_list);

    dev_list = g_strdup("");

    for (i = 0; i < G_N_ELEMENTS(detect_lang); i++) {
       gchar *version = NULL;
       gchar *output, *ignored;
       gchar *temp;
       GRegex *regex;
       GMatchInfo *match_info;
       gboolean found;

       if (!detect_lang[i].regex) {
            dev_list = h_strdup_cprintf("[%s]\n", dev_list, _(detect_lang[i].compiler_name));
            continue;
       }

       if (detect_lang[i].stdout) {
            found = g_spawn_command_line_sync(detect_lang[i].version_command, &output, &ignored, NULL, NULL);
       } else {
            found = g_spawn_command_line_sync(detect_lang[i].version_command, &ignored, &output, NULL, NULL);
       }
       g_free(ignored);

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

       if (version == NULL)
           version = strdup(_("Not found"));

       dev_list = h_strdup_cprintf("%s=%s\n", dev_list, detect_lang[i].compiler_name, version);
       g_free(version);

       temp = g_strdup_printf(_("Detecting version: %s"),
                              detect_lang[i].compiler_name);
       shell_status_update(temp);
       g_free(temp);
    }

    SCAN_END();
}

gchar *callback_dev(void)
{
    return g_strdup_printf(
                "[$ShellParam$]\n"
                "ColumnTitle$TextValue=%s\n" /* Program */
                "ColumnTitle$Value=%s\n" /* Version */
                "ShowColumnHeaders=true\n"
                "%s",
                _("Program"), _("Version"),
                dev_list);
}
#endif /* GLIB_CHECK_VERSION(2,14,0) */

static gchar *detect_machine_type(void)
{
    GDir *dir;
    gchar *chassis;

    chassis = dmi_chassis_type_str(0);
    if (chassis != NULL)
        return chassis;

    chassis = dtr_get_string("/model", 0);
    if (chassis) {
        if (strstr(chassis, "Raspberry Pi") != NULL
            || strstr(chassis, "ODROID") != NULL
            /* etc */ ) {
                g_free(chassis);
                return g_strdup(_("Single-board computer"));
        }
        g_free(chassis);
    }

    if (g_file_test("/proc/pmu/info", G_FILE_TEST_EXISTS))
        return g_strdup(_("Laptop"));

    dir = g_dir_open("/proc/acpi/battery", 0, NULL);
    if (dir) {
        const gchar *name = g_dir_read_name(dir);

        g_dir_close(dir);

        if (name)
            return g_strdup(_("Laptop"));
    }

    dir = g_dir_open("/sys/class/power_supply", 0, NULL);
    if (dir) {
        const gchar *name;

        while ((name = g_dir_read_name(dir))) {
            gchar *contents;
            gchar type[PATH_MAX];
            int r;

            r = snprintf(type, sizeof(type), "%s/%s/type",
                         "/sys/class/power_supply", name);
            if (r < 0 || r > PATH_MAX)
                continue;

            if (g_file_get_contents(type, &contents, NULL, NULL)) {
                if (g_str_has_prefix(contents, "Battery")) {
                    g_free(contents);
                    g_dir_close(dir);

                    return g_strdup(_("Laptop"));
                }

                g_free(contents);
            }
        }

        g_dir_close(dir);
    }

    /* FIXME: check if batteries are found using /proc/apm */

    return g_strdup(_("Unknown physical machine type"));
}

/* Table based off imvirt by Thomas Liske <liske@ibh.de>
   Copyright (c) 2008 IBH IT-Service GmbH under GPLv2. */
gchar *computer_get_virtualization(void)
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
        { "VMware", N_("Virtual (VMware)") },
        { ": VMware Virtual IDE CDROM Drive", N_("Virtual (VMware)") },
        /* QEMU */
        { "QEMU", N_("Virtual (QEMU)") },
        { "QEMU Virtual CPU", N_("Virtual (QEMU)") },
        { ": QEMU HARDDISK", N_("Virtual (QEMU)") },
        { ": QEMU CD-ROM", N_("Virtual (QEMU)") },
        /* Generic Virtual Machine */
        { ": Virtual HD,", N_("Virtual (Unknown)") },
        { ": Virtual CD,", N_("Virtual (Unknown)") },
        /* Virtual Box */
        { "VBOX", N_("Virtual (VirtualBox)") },
        { ": VBOX HARDDISK", N_("Virtual (VirtualBox)") },
        { ": VBOX CD-ROM", N_("Virtual (VirtualBox)") },
        /* Xen */
        { "Xen virtual console", N_("Virtual (Xen)") },
        { "Xen reported: ", N_("Virtual (Xen)") },
        { "xen-vbd: registered block device", N_("Virtual (Xen)") },
        /* Generic */
        { " hypervisor", N_("Virtual (hypervisor present)") } ,
        { NULL }
    };
    gchar *tmp;

    DEBUG("Detecting virtual machine");

    if (g_file_test("/proc/xen", G_FILE_TEST_EXISTS)) {
         DEBUG("/proc/xen found; assuming Xen");
         return g_strdup(_("Virtual (Xen)"));
    }

    tmp = module_call_method("devices::getMotherboard");
    if (strstr(tmp, "VirtualBox") != NULL) {
        g_free(tmp);
        return g_strdup(_("Virtual (VirtualBox)"));
    }
    g_free(tmp);

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
                  return g_strdup(_(vm_types[j].vmtype));
              }
         }

    }

    DEBUG("no virtual machine detected; assuming physical machine");

    return detect_machine_type();
}

gchar *callback_summary(void)
{
    struct Info *info = info_new();

    info_add_group(info, _("Computer"),
        info_field_printf(_("Processor"), "%s",
            module_call_method("devices::getProcessorName")),
        info_field_update(_("Memory"), 1000),
        info_field_printf(_("Machine Type"), "%s",
            computer_get_virtualization()),
        info_field(_("Operating System"), computer->os->distro),
        info_field(_("User Name"), computer->os->username),
        info_field_update(_("Date/Time"), 1000),
        info_field_last());

    info_add_group(info, _("Display"),
        info_field_printf(_("Resolution"), _(/* label for resolution */ "%dx%d pixels"),
            computer->display->width, computer->display->height),
        info_field(_("OpenGL Renderer"), computer->display->ogl_renderer),
        info_field(_("X11 Vendor"), computer->display->vendor),
        info_field_last());

    info_add_computed_group(info, _("Audio Devices"),
        idle_free(computer_get_alsacards(computer)));
    info_add_computed_group(info, _("Input Devices"),
        idle_free(module_call_method("devices::getInputDevices")));
    info_add_computed_group(info, NULL, /* getPrinters provides group headers */
        idle_free(module_call_method("devices::getPrinters")));
    info_add_computed_group(info, NULL,  /* getStorageDevices provides group headers */
        idle_free(module_call_method("devices::getStorageDevices")));

    return info_flatten(info);
}

gchar *callback_os(void)
{
    struct Info *info = info_new();

    info_add_group(info, _("Version"),
        info_field(_("Kernel"), computer->os->kernel),
        info_field(_("Version"), computer->os->kernel_version),
        info_field(_("C Library"), computer->os->libc),
        info_field(_("Distribution"), computer->os->distro),
        info_field_last());

    info_add_group(info, _("Current Session"),
        info_field(_("Computer Name"), computer->os->hostname),
        info_field(_("User Name"), computer->os->username),
        info_field(_("Language"), computer->os->language),
        info_field(_("Home Directory"), computer->os->homedir),
        info_field_last());

    info_add_group(info, _("Misc"),
        info_field_update(_("Uptime"), 1000),
        info_field_update(_("Load Average"), 10000),
        info_field_update(_("Available entropy in /dev/random"), 1000),
        info_field_last());

    return info_flatten(info);
}

gchar *callback_modules(void)
{
    struct Info *info = info_new();

    info_add_computed_group(info, _("Loaded Modules"), module_list);

    info_set_column_title(info, "TextValue", _("Name"));
    info_set_column_title(info, "Value", _("Description"));
    info_set_column_headers_visible(info, TRUE);
    info_set_view_type(info, SHELL_VIEW_DUAL);

    return info_flatten(info);
}

gchar *callback_boots(void)
{
    struct Info *info = info_new();

    info_add_computed_group(info, _("Boots"), computer->os->boots);

    info_set_column_title(info, "TextValue", _("Date & Time"));
    info_set_column_title(info, "Value", _("Kernel Version"));
    info_set_column_headers_visible(info, TRUE);

    return info_flatten(info);
}

gchar *callback_locales(void)
{
    struct Info *info = info_new();

    info_add_computed_group(info, _("Available Languages"), computer->os->languages);

    info_set_column_title(info, "TextValue", _("Language Code"));
    info_set_column_title(info, "Value", _("Name"));
    info_set_view_type(info, SHELL_VIEW_DUAL);
    info_set_column_headers_visible(info, TRUE);

    return info_flatten(info);
}

gchar *callback_fs(void)
{
    struct Info *info = info_new();

    info_add_computed_group(info, _("Mounted File Systems"), fs_list);

    info_set_column_title(info, "Extra1", _("Mount Point"));
    info_set_column_title(info, "Progress", _("Usage"));
    info_set_column_title(info, "TextValue", _("Device"));
    info_set_column_headers_visible(info, TRUE);
    info_set_view_type(info, SHELL_VIEW_PROGRESS_DUAL);
    info_set_zebra_visible(info, TRUE);
    info_set_normalize_percentage(info, FALSE);

    return info_flatten(info);
}

gchar *callback_display(void)
{
    struct Info *info = info_new();

    info_add_group(info, _("Display"),
        info_field_printf(_("Resolution"), _(/* resolution WxH unit */ "%dx%d pixels"),
                computer->display->width, computer->display->height),
        info_field(_("Vendor"), computer->display->vendor),
        info_field(_("Version"), computer->display->version),
        info_field_last());

    info_add_computed_group(info, _("Monitors"), computer->display->monitors);

    info_add_group(info, _("OpenGL"),
        info_field(_("Vendor"), computer->display->ogl_vendor),
        info_field(_("Renderer"), computer->display->ogl_renderer),
        info_field(_("Version"), computer->display->ogl_version),
        info_field(_("Direct Rendering"),
            computer->display->dri ? _("Yes") : _("No")),
        info_field_last());

    info_add_computed_group(info, _("Extensions"), computer->display->extensions);

    return info_flatten(info);
}

gchar *callback_users(void)
{
    struct Info *info = info_new();

    info_add_computed_group(info, _("Users"), users);
    info_set_view_type(info, SHELL_VIEW_DUAL);
    info_set_reload_interval(info, 10000);

    return info_flatten(info);
}

gchar *callback_groups(void)
{
    struct Info *info = info_new();

    info_add_computed_group(info, _("Group"), groups);

    info_set_column_title(info, "TextValue", _("Name"));
    info_set_column_title(info, "Value", _("Group ID"));
    info_set_column_headers_visible(info, TRUE);
    info_set_reload_interval(info, 10000);

    return info_flatten(info);
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

gchar *get_ogl_renderer(void)
{
    scan_display(FALSE);

    return g_strdup(computer->display->ogl_renderer);
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
        {"getOGLRenderer", get_ogl_renderer},
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
    return g_strdup_printf("[%s]\n"
                    "Icon=os.png\n"
                    "Method=computer::getOS\n"
                    "[%s]\n"
                    "Icon=processor.png\n"
                    "Method=devices::getProcessorNameAndDesc\n"
                    "[%s]\n"
                    "Icon=memory.png\n"
                    "Method=devices::getMemoryTotal\n"
                    "[%s]\n"
                    "Icon=module.png\n"
                    "Method=devices::getMotherboard\n"
                    "[%s]\n"
                    "Icon=monitor.png\n"
                    "Method=computer::getDisplaySummary\n"
                    "[%s]\n"
                    "Icon=hdd.png\n"
                    "Method=devices::getStorageDevices\n"
                    "[%s]\n"
                    "Icon=printer.png\n"
                    "Method=devices::getPrinters\n"
                    "[%s]\n"
                    "Icon=audio.png\n"
                    "Method=computer::getAudioCards\n",
                    _("Operating System"),
                    _("CPU"), _("RAM"), _("Motherboard"), _("Graphics"),
                    _("Storage"), _("Printers"), _("Audio")
                    );
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

