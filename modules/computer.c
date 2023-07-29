/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 L. A. F. Pereira <l@tia.mat.br>
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

#define THISORUNK(t) ( (t) ? t : _("(Unknown)") )

/* Callbacks */
gchar *callback_summary(void);
gchar *callback_os(void);
gchar *callback_security(void);
gchar *callback_modules(void);
gchar *callback_boots(void);
gchar *callback_locales(void);
gchar *callback_memory_usage();
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
void scan_security(gboolean reload);
void scan_modules(gboolean reload);
void scan_boots(gboolean reload);
void scan_locales(gboolean reload);
void scan_fs(gboolean reload);
void scan_memory_usage(gboolean reload);
void scan_display(gboolean reload);
void scan_network(gboolean reload);
void scan_users(gboolean reload);
void scan_groups(gboolean reload);
void scan_env_var(gboolean reload);
#if GLIB_CHECK_VERSION(2,14,0)
void scan_dev(gboolean reload);
#endif /* GLIB_CHECK_VERSION(2,14,0) */

enum {
    ENTRY_SUMMARY,
    ENTRY_OS,
    ENTRY_SECURITY,
    ENTRY_KMOD,
    ENTRY_BOOTS,
    ENTRY_LANGUAGES,
    ENTRY_MEMORY_USAGE,
    ENTRY_FS,
    ENTRY_DISPLAY,
    ENTRY_ENV,
    ENTRY_DEVEL,
    ENTRY_USERS,
    ENTRY_GROUPS
};

static ModuleEntry entries[] = {
    [ENTRY_SUMMARY] = {N_("Summary"), "summary.png", callback_summary, scan_summary, MODULE_FLAG_NONE},
    [ENTRY_OS] = {N_("Operating System"), "os.png", callback_os, scan_os, MODULE_FLAG_NONE},
    [ENTRY_SECURITY] = {N_("Security"), "security.png", callback_security, scan_security, MODULE_FLAG_NONE},
    [ENTRY_KMOD] = {N_("Kernel Modules"), "module.png", callback_modules, scan_modules, MODULE_FLAG_NONE},
    [ENTRY_BOOTS] = {N_("Boots"), "boot.png", callback_boots, scan_boots, MODULE_FLAG_NONE},
    [ENTRY_LANGUAGES] = {N_("Languages"), "language.png", callback_locales, scan_locales, MODULE_FLAG_NONE},
    [ENTRY_MEMORY_USAGE] = {N_("Memory Usage"), "memory.png", callback_memory_usage, scan_memory_usage, MODULE_FLAG_NONE},
    [ENTRY_FS] = {N_("Filesystems"), "dev_removable.png", callback_fs, scan_fs, MODULE_FLAG_NONE},
    [ENTRY_DISPLAY] = {N_("Display"), "monitor.png", callback_display, scan_display, MODULE_FLAG_NONE},
    [ENTRY_ENV] = {N_("Environment Variables"), "environment.png", callback_env_var, scan_env_var, MODULE_FLAG_NONE},
#if GLIB_CHECK_VERSION(2,14,0)
    [ENTRY_DEVEL] = {N_("Development"), "devel.png", callback_dev, scan_dev, MODULE_FLAG_NONE},
#else
    [ENTRY_DEVEL] = {N_("Development"), "devel.png", callback_dev, scan_dev, MODULE_FLAG_HIDE},
#endif /* GLIB_CHECK_VERSION(2,14,0) */
    [ENTRY_USERS] = {N_("Users"), "users.png", callback_users, scan_users, MODULE_FLAG_NONE},
    [ENTRY_GROUPS] = {N_("Groups"), "users.png", callback_groups, scan_groups, MODULE_FLAG_NONE},
    {NULL},
};

gchar *module_list = NULL;
Computer *computer = NULL;
gchar *meminfo = NULL;

gchar *hi_more_info(gchar * entry)
{
    gchar *info = moreinfo_lookup_with_prefix("COMP", entry);

    if (info)
        return g_strdup(info);

    return g_strdup_printf("[%s]", entry);
}

/* a g_str_equal() where either may be null */
#define g_str_equal0(a,b) (g_strcmp0(a,b) == 0)

gchar *hi_get_field(gchar * field)
{
    gchar *tag, *label;
    key_get_components(field, NULL, &tag, NULL, &label, NULL, TRUE);

    gchar *tmp;

    if (g_str_equal0(label, _("Memory"))) {
        MemoryInfo *mi = computer_get_memory();
        tmp = g_strdup_printf(_("%dMB (%dMB used)"), mi->total, mi->used);
        g_free(mi);
    } else if (g_str_equal0(label, _("Uptime"))) {
        tmp = computer_get_formatted_uptime();
    } else if (g_str_equal0(label, _("Date/Time"))) {
        time_t t = time(NULL);

        tmp = g_new0(gchar, 64);
        strftime(tmp, 64, "%c", localtime(&t));
    } else if (g_str_equal0(label, _("Load Average"))) {
        tmp = computer_get_formatted_loadavg();
    } else if (g_str_equal0(tag, "entropy")) {
        tmp = computer_get_entropy_avail();
    } else {
        gchar *info = NULL;
        if (tag)
            info = moreinfo_lookup_with_prefix("DEV", tag);
        else if (label)
            info = moreinfo_lookup_with_prefix("DEV", label);

        if (info)
            tmp = g_strdup(info);
        else
            tmp = g_strdup_printf("Unknown field: [tag: %s] label: %s", tag ? tag : "(none)", label ? label : "(empty)");
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

void scan_security(gboolean reload)
{
    SCAN_START();
    //nothing to do here yet
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

void scan_memory_usage(gboolean reload)
{
    SCAN_START();
    scan_memory_do();
    SCAN_END();
}

void scan_display(gboolean reload)
{
    SCAN_START();
    if (computer->display)
        computer_free_display(computer->display);
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
       gboolean read_stdout;
    } detect_lang[] = {
       { N_("Scripting Languages"), NULL, FALSE },
       { N_("Gambas3 (gbr3)"), "gbr3 --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Python (default)"), "python -V", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("Python2"), "python2 -V", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("Python3"), "python3 -V", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Perl"), "perl -v", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Perl6 (VM)"), "perl6 -v", "(?<=This is ).*", TRUE },
       { N_("Perl6"), "perl6 -v", "(?<=implementing Perl )\\w*\\.\\w*", TRUE },
       { N_("PHP"), "php --version", "\\d+\\.\\d+\\.\\S+", TRUE},
       { N_("Ruby"), "ruby --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Bash"), "bash --version", "\\d+\\.\\d+\\.\\S+", TRUE},
       { N_("JavaScript (Node.js)"), "node --version", "(?<=v)(\\d\\.?)+", TRUE },
       { N_("awk"), "awk --version", "GNU Awk \\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Compilers"), NULL, FALSE },
       { N_("C (GCC)"), "gcc -v", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("C (Clang)"), "clang -v", "\\d+\\.\\d+", FALSE },
       { N_("D (dmd)"), "dmd --help", "\\d+\\.\\d+", TRUE },
       { N_("Gambas3 (gbc3)"), "gbc3 --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Java"), "javac -version", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("C♯ (mcs)"), "mcs --version", "\\d+\\.\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Vala"), "valac --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Haskell (GHC)"), "ghc -v", "\\d+\\.\\d+\\.\\d+", FALSE },
       { N_("FreePascal"), "fpc -iV", "\\d+\\.\\d+\\.?\\d*", TRUE },
       { N_("Go"), "go version", "\\d+\\.\\d+\\.?\\d* ", TRUE },
       { N_("Rust"), "rustc --version", "(?<=rustc )(\\d\\.?)+", TRUE },
       { N_("Tools"), NULL, FALSE },
       { N_("make"), "make --version", "\\d+\\.\\d+", TRUE },
       { N_("ninja"), "ninja --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("GDB"), "gdb --version", "(?<=^GNU gdb ).*", TRUE },
       { N_("LLDB"), "lldb --version", "(?<=lldb version )(\\d\\.?)+", TRUE },
       { N_("strace"), "strace -V", "\\d+\\.\\d+\\.?\\d*", TRUE },
       { N_("valgrind"), "valgrind --version", "\\d+\\.\\d+\\.\\S+", TRUE },
       { N_("QMake"), "qmake --version", "\\d+\\.\\S+", TRUE},
       { N_("CMake"), "cmake --version", "\\d+\\.\\d+\\.?\\d*", TRUE},
       { N_("Gambas3 IDE"), "gambas3 --version", "\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Radare2"), "radare2 -v", "(?<=radare2 )(\\d+\\.?)+(-git)?", TRUE },
       { N_("ltrace"), "ltrace --version", "(?<=ltrace version )\\d+\\.\\d+\\.\\d+", TRUE },
       { N_("Powershell"), "pwsh --version", "\\d+\\.\\d+\\.\\d+", TRUE },
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

       if (detect_lang[i].read_stdout) {
            found = hardinfo_spawn_command_line_sync(detect_lang[i].version_command, &output, &ignored, NULL, NULL);
       } else {
            found = hardinfo_spawn_command_line_sync(detect_lang[i].version_command, &ignored, &output, NULL, NULL);
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
                "ViewType=5\n"
                "ColumnTitle$TextValue=%s\n" /* Program */
                "ColumnTitle$Value=%s\n" /* Version */
                "ShowColumnHeaders=true\n"
                "%s",
                _("Program"), _("Version"),
                dev_list);
}
#endif /* GLIB_CHECK_VERSION(2,14,0) */

gchar *callback_memory_usage()
{
    extern gchar *lginterval;
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

static gchar *detect_machine_type(void)
{
    GDir *dir;
    gchar *chassis;

    chassis = dmi_chassis_type_str(-1, 0);
    if (chassis)
        return chassis;

    chassis = dtr_get_string("/model", 0);
    if (chassis) {
        if (strstr(chassis, "Raspberry Pi") != NULL
            || strstr(chassis, "ODROID") != NULL
            || strstr(chassis, "Firefly ROC") != NULL
            /* FIXME: consider making a table when adding more models */ ) {
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

    info_set_view_type(info, SHELL_VIEW_DETAIL);

    info_add_group(info, _("Computer"),
        info_field(_("Processor"),
            idle_free(module_call_method("devices::getProcessorNameAndDesc"))),
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
        info_field(_("Display Adapter"),
            idle_free(module_call_method("devices::getGPUList"))),
        info_field(_("OpenGL Renderer"), THISORUNK(computer->display->xi->glx->ogl_renderer)),
        info_field(_("Session Display Server"), THISORUNK(computer->display->display_server)),
        info_field_last());

    info_add_computed_group(info, _("Audio Devices"),
        idle_free(computer_get_alsacards(computer)));
    info_add_computed_group_wo_extra(info, _("Input Devices"),
        idle_free(module_call_method("devices::getInputDevices")));
    info_add_computed_group(info, NULL, /* getPrinters provides group headers */
        idle_free(module_call_method("devices::getPrinters")));
    info_add_computed_group_wo_extra(info, NULL,  /* getStorageDevices provides group headers */
        idle_free(module_call_method("devices::getStorageDevices")));

    return info_flatten(info);
}

gchar *callback_os(void)
{
    struct Info *info = info_new();
    gchar *distro_icon;
    gchar *distro;

    info_set_view_type(info, SHELL_VIEW_DETAIL);

    distro_icon = computer->os->distroid
                      ? idle_free(g_strdup_printf("distros/%s.svg",
                                                  computer->os->distroid))
                      : NULL;
    distro = computer->os->distrocode
                      ? idle_free(g_strdup_printf("%s (%s)",
                                  computer->os->distro, computer->os->distrocode))
                      : computer->os->distro;

    struct InfoGroup *version_group =
    info_add_group(
        info, _("Version"), info_field(_("Kernel"), computer->os->kernel),
        info_field(_("Command Line"), computer->os->kcmdline ?: _("Unknown")),
        info_field(_("Version"), computer->os->kernel_version),
        info_field(_("C Library"), computer->os->libc),
        info_field(_("Distribution"), distro,
                   .value_has_vendor = TRUE,
                   .icon = distro_icon),
        info_field_last());

    if (computer->os->distro_flavor) {
        info_group_add_field(version_group,
            info_field(_("Spin/Flavor"), computer->os->distro_flavor->name,
                .value_has_vendor = TRUE,
                .icon = computer->os->distro_flavor->icon) );
    }

    info_add_group(info, _("Current Session"),
        info_field(_("Computer Name"), computer->os->hostname),
        info_field(_("User Name"), computer->os->username),
        info_field(_("Language"), computer->os->language),
        info_field(_("Home Directory"), computer->os->homedir),
        info_field(_("Desktop Environment"), computer->os->desktop),
        info_field_last());

    info_add_group(info, _("Misc"), info_field_update(_("Uptime"), 1000),
                   info_field_update(_("Load Average"), 10000),
                   info_field_last());

    return info_flatten(info);
}

gchar *callback_security(void)
{
    struct Info *info = info_new();

    info_set_view_type(info, SHELL_VIEW_DETAIL);

    info_add_group(info, _("HardInfo"),
                   info_field(_("HardInfo running as"),
                              (getuid() == 0) ? _("Superuser") : _("User")),
                   info_field_last());

    info_add_group(
        info, _("Health"),
        info_field_update(_("Available entropy in /dev/random"), 1000, .tag = g_strdup("entropy") ),
        info_field_last());

    info_add_group(
        info, _("Hardening Features"),
        info_field(_("ASLR"), idle_free(computer_get_aslr())),
        info_field(_("dmesg"), idle_free(computer_get_dmesg_status())),
        info_field_last());

    info_add_group(
        info, _("Linux Security Modules"),
        info_field(_("Modules available"), idle_free(computer_get_lsm())),
        info_field(_("SELinux status"), computer_get_selinux()),
        info_field_last());

    GDir *dir = g_dir_open("/sys/devices/system/cpu/vulnerabilities", 0, NULL);
    if (dir) {
        struct InfoGroup *vulns =
            info_add_group(info, _("CPU Vulnerabilities"), info_field_last());
        vulns->sort = INFO_GROUP_SORT_NAME_ASCENDING;
        const gchar *vuln;

        while ((vuln = g_dir_read_name(dir))) {
            gchar *contents = h_sysfs_read_string(
                "/sys/devices/system/cpu/vulnerabilities", vuln);
            if (!contents)
                continue;

            const gchar *icon = NULL;
            if (g_strstr_len(contents, -1, "Vulnerable") ||
                g_strstr_len(contents, -1, "vulnerable"))
                icon = "dialog-warning.png";

            info_group_add_fields(vulns,
                                  info_field(g_strdup(vuln),
                                             idle_free(contents), .icon = icon,
                                             .free_name_on_flatten = TRUE),
                                  info_field_last());
        }

        g_dir_close(dir);
    }

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
    int n = 0;
    gchar *screens_str = strdup(""), *outputs_str = strdup("");
    xinfo *xi = computer->display->xi;
    xrr_info *xrr = xi->xrr;
    glx_info *glx = xi->glx;
    wl_info *wl = computer->display->wl;

    struct Info *info = info_new();

    info_set_view_type(info, SHELL_VIEW_DETAIL);

    info_add_group(info, _("Session"),
        info_field(_("Type"), THISORUNK(computer->display->session_type)),
        info_field_last());

    info_add_group(info, _("Wayland"),
        info_field(_("Current Display Name"),
                    (wl->display_name) ? (wl->display_name) : _("(Not Available)")),
        info_field_last());

    info_add_group(info, _("X Server"),
        info_field(_("Current Display Name"), THISORUNK(xi->display_name) ),
        info_field(_("Vendor"), THISORUNK(xi->vendor), .value_has_vendor = TRUE ),
        info_field(_("Version"), THISORUNK(xi->version) ),
        info_field(_("Release Number"), THISORUNK(xi->release_number) ),
        info_field_last());

    for (n = 0; n < xrr->screen_count; n++) {
        gchar *dims = g_strdup_printf(_(/* resolution WxH unit */ "%dx%d pixels"), xrr->screens[n].px_width, xrr->screens[n].px_height);
        screens_str = h_strdup_cprintf("Screen %d=%s\n", screens_str, xrr->screens[n].number, dims);
        g_free(dims);
    }
    info_add_computed_group(info, _("Screens"), screens_str);

    for (n = 0; n < xrr->output_count; n++) {
        gchar *connection = NULL;
        switch (xrr->outputs[n].connected) {
            case 0:
                connection = _("Disconnected");
                break;
            case 1:
                connection = _("Connected");
                break;
            case -1:
            default:
                connection = _("Unknown");
                break;
        }
        gchar *dims = (xrr->outputs[n].screen == -1)
            ? g_strdup(_("Unused"))
            : g_strdup_printf(_("%dx%d pixels, offset (%d, %d)"),
                    xrr->outputs[n].px_width, xrr->outputs[n].px_height,
                    xrr->outputs[n].px_offset_x, xrr->outputs[n].px_offset_y);

        outputs_str = h_strdup_cprintf("%s=%s; %s\n", outputs_str,
            xrr->outputs[n].name, connection, dims);

        g_free(dims);
    }
    info_add_computed_group(info, _("Outputs (XRandR)"), outputs_str);

    info_add_group(info, _("OpenGL (GLX)"),
        info_field(_("Vendor"), THISORUNK(glx->ogl_vendor), .value_has_vendor = TRUE ),
        info_field(_("Renderer"), THISORUNK(glx->ogl_renderer) ),
        info_field(_("Direct Rendering"),
            glx->direct_rendering ? _("Yes") : _("No")),
        info_field(_("Version (Compatibility)"), THISORUNK(glx->ogl_version) ),
        info_field(_("Shading Language Version (Compatibility)"), THISORUNK(glx->ogl_sl_version) ),
        info_field(_("Version (Core)"), THISORUNK(glx->ogl_core_version) ),
        info_field(_("Shading Language Version (Core)"), THISORUNK(glx->ogl_core_sl_version) ),
        info_field(_("Version (ES)"), THISORUNK(glx->ogles_version) ),
        info_field(_("Shading Language Version (ES)"), THISORUNK(glx->ogles_sl_version) ),
        info_field(_("GLX Version"), THISORUNK(glx->glx_version) ),
        info_field_last());

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

    return g_strdup(computer->display->xi->glx->ogl_renderer);
}

gchar *get_display_summary(void)
{
    scan_display(FALSE);

    gchar *gpu_list = module_call_method("devices::getGPUList");

    gchar *ret = g_strdup_printf(
                           "%s\n"
                           "%dx%d\n"
                           "%s\n"
                           "%s",
                           gpu_list,
                           computer->display->width, computer->display->height,
                           computer->display->display_server,
                           (computer->display->xi->glx->ogl_renderer)
                              ? computer->display->xi->glx->ogl_renderer
                              : "" );
    g_free(gpu_list);
    return ret;
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

/* the returned string must stay in kB as it is used
 * elsewhere with that expectation */
gchar *get_memory_total(void)
{
    scan_memory_usage(FALSE);
    return moreinfo_lookup ("DEV:MemTotal");
}

gchar *memory_devices_get_system_memory_str(); /* in dmi_memory.c */
gchar *memory_devices_get_system_memory_types_str();
/* Note 1: moreinfo_lookup() results should not be freed because
 *         they are pointers into a GHash.
 *         module_call_method() g_strdup()s it's return value. */
const gchar *get_memory_desc(void) // [1] const (as to say "don't free")
{
    scan_memory_usage(FALSE);
    gchar *avail = g_strdup(moreinfo_lookup("DEV:MemTotal")); // [1] g_strdup()
    double k = avail ? (double)strtol(avail, NULL, 10) : 0;
    if (k) {
        g_free(avail);
        avail = NULL;
        const char *fmt = _(/*/ <value> <unit> "usable memory" */ "%0.1f %s available to Linux");
        if (k > (2048 * 1024))
            avail = g_strdup_printf(fmt, k / (1024*1024), _("GiB") );
        else if (k > 2048)
            avail = g_strdup_printf(fmt, k / 1024, _("MiB") );
        else
            avail = g_strdup_printf(fmt, k, _("KiB") );
    }
    gchar *mem = memory_devices_get_system_memory_str();
    if (mem) {
        gchar *types = memory_devices_get_system_memory_types_str();
        gchar *ret = g_strdup_printf("%s %s\n%s", mem, types, avail ? avail : "");
        g_free(avail);
        g_free(mem);
        g_free(types);
        return (gchar*)idle_free(ret); // [1] idle_free()
    }
    return (gchar*)idle_free(avail); // [1] idle_free()
}

static gchar *get_machine_type(void)
{
    return computer_get_virtualization();
}

const ShellModuleMethod *hi_exported_methods(void)
{
    static const ShellModuleMethod m[] = {
        {"getOSKernel", get_os_kernel},
        {"getOS", get_os},
        {"getDisplaySummary", get_display_summary},
        {"getOGLRenderer", get_ogl_renderer},
        {"getAudioCards", get_audio_cards},
        {"getKernelModuleDescription", get_kernel_module_description},
        {"getMemoryTotal", get_memory_total},
        {"getMemoryDesc", get_memory_desc},
        {"getMachineType", get_machine_type},
        {NULL},
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
    gchar *virt = computer_get_virtualization();
    gchar *machine_type = g_strdup_printf("%s (%s)",
                                          _("Motherboard"),
                                          (char*)idle_free(virt));

    return g_strdup_printf("[%s]\n"
                    "Icon=os.png\n"
                    "Method=computer::getOS\n"
                    "[%s]\n"
                    "Icon=processor.png\n"
                    "Method=devices::getProcessorNameAndDesc\n"
                    "[%s]\n"
                    "Icon=memory.png\n"
                    "Method=computer::getMemoryDesc\n"
                    "[%s]\n"
                    "Icon=module.png\n"
                    "Method=devices::getMotherboard\n"
                    "[%s]\n"
                    "Icon=monitor.png\n"
                    "Method=computer::getDisplaySummary\n"
                    "[%s]\n"
                    "Icon=hdd.png\n"
                    "Method=devices::getStorageDevicesSimple\n"
                    "[%s]\n"
                    "Icon=printer.png\n"
                    "Method=devices::getPrinters\n"
                    "[%s]\n"
                    "Icon=audio.png\n"
                    "Method=computer::getAudioCards\n",
                    _("Operating System"),
                    _("Processor"), _("Memory"), (char*)idle_free(machine_type), _("Graphics"),
                    _("Storage"), _("Printers"), _("Audio")
                    );
}

void hi_module_deinit(void)
{
    g_hash_table_destroy(memlabels);

    if (computer->os) {
        g_free(computer->os->kernel);
        g_free(computer->os->kcmdline);
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

    computer_free_display(computer->display);

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
    init_memory_labels();
    kernel_module_icon_init();
}

const ModuleAbout *hi_module_get_about(void)
{
    static const ModuleAbout ma = {
        .author = "L. A. F. Pereira",
        .description = N_("Gathers high-level computer information"),
        .version = VERSION,
        .license = "GNU GPL version 2",
    };

    return &ma;
}

static const gchar *hinote_kmod() {
    static gchar note[note_max_len] = "";
    gboolean ok = TRUE;
    *note = 0; /* clear */
    ok &= note_require_tool("lsmod", note, _("<i><b>lsmod</b></i> is required."));
    return ok ? NULL : g_strstrip(note); /* remove last \n */
}

static const gchar *hinote_display() {
    static gchar note[note_max_len] = "";
    gboolean ok = TRUE;
    *note = 0; /* clear */
    ok &= note_require_tool("xrandr", note, _("X.org's <i><b>xrandr</b></i> utility provides additional details when available."));
    ok &= note_require_tool("glxinfo", note, _("Mesa's <i><b>glxinfo</b></i> utility is required for OpenGL information."));
    return ok ? NULL : g_strstrip(note); /* remove last \n */
}

const gchar *hi_note_func(gint entry)
{
    if (entry == ENTRY_KMOD) {
        return hinote_kmod();
    }
    else if (entry == ENTRY_DISPLAY) {
        return hinote_display();
    }
    return NULL;
}
