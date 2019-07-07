/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include <gdk/gdkx.h>
#include <string.h>
#include <sys/utsname.h>
#include "hardinfo.h"
#include "computer.h"
#include "distro_flavors.h"

static gchar *
get_libc_version(void)
{
    static const struct {
        const char *test_cmd;
        const char *match_str;
        const char *lib_name;
        gboolean try_ver_str;
        gboolean use_stderr;
    } libs[] = {
        { "ldd --version", "GLIBC", N_("GNU C Library"), TRUE, FALSE},
        { "ldd --version", "GNU libc", N_("GNU C Library"), TRUE, FALSE},
        { "ldconfig -V", "GLIBC", N_("GNU C Library"), TRUE, FALSE},
        { "ldconfig -V", "GNU libc", N_("GNU C Library"), TRUE, FALSE},
        { "ldconfig -v", "uClibc", N_("uClibc or uClibc-ng"), FALSE, FALSE},
        { "diet", "diet version", N_("diet libc"), TRUE, TRUE},
        { NULL }
    };
    int i;

    for (i = 0; libs[i].test_cmd; i++) {
        gboolean spawned;
        gchar *out, *err, *p;

        spawned = g_spawn_command_line_sync(libs[i].test_cmd,
            &out, &err, NULL, NULL);
        if (!spawned)
            continue;

        if (libs[i].use_stderr) {
            p = strend(idle_free(err), '\n');
            g_free(out);
        } else {
            p = strend(idle_free(out), '\n');
            g_free(err);
        }

        if (!p || !strstr(p, libs[i].match_str))
            continue;

        if (libs[i].try_ver_str) {
            /* skip the first word, likely "ldconfig" or name of utility */
            const gchar *ver_str = strchr(p, ' ');

            if (ver_str) {
                return g_strdup_printf("%s / %s", _(libs[i].lib_name),
                    ver_str + 1);
            }
        }

	return g_strdup(_(libs[i].lib_name));
    }

    return g_strdup(_("Unknown"));
}

static gchar *detect_kde_version(void)
{
    const gchar *cmd;
    const gchar *tmp = g_getenv("KDE_SESSION_VERSION");
    gchar *out;
    gboolean spawned;

    if (tmp && tmp[0] == '4') {
        cmd = "kwin --version";
    } else {
        cmd = "kcontrol --version";
    }

    spawned = g_spawn_command_line_sync(cmd, &out, NULL, NULL, NULL);
    if (!spawned)
        return NULL;

    tmp = strstr(idle_free(out), "KDE: ");
    return tmp ? g_strdup(tmp + strlen("KDE: ")) : NULL;
}

static gchar *
detect_gnome_version(void)
{
    gchar *tmp;
    gchar *out;
    gboolean spawned;

    spawned = g_spawn_command_line_sync(
        "gnome-shell --version", &out, NULL, NULL, NULL);
    if (spawned) {
        tmp = strstr(idle_free(out), _("GNOME Shell "));

        if (tmp) {
            tmp += strlen(_("GNOME Shell "));
            return g_strdup_printf("GNOME %s", strend(tmp, '\n'));
        }
    }

    spawned = g_spawn_command_line_sync(
        "gnome-about --gnome-version", &out, NULL, NULL, NULL);
    if (spawned) {
        tmp = strstr(idle_free(out), _("Version: "));

        if (tmp) {
            tmp += strlen(_("Version: "));
            return g_strdup_printf("GNOME %s", strend(tmp, '\n'));
        }
    }

    return NULL;
}


static gchar *
detect_mate_version(void)
{
    gchar *tmp;
    gchar *out;
    gboolean spawned;

    spawned = g_spawn_command_line_sync(
        "mate-about --version", &out, NULL, NULL, NULL);
    if (spawned) {
        tmp = strstr(idle_free(out), _("MATE Desktop Environment "));

        if (tmp) {
            tmp += strlen(_("MATE Desktop Environment "));
            return g_strdup_printf("MATE %s", strend(tmp, '\n'));
        }
    }

    return NULL;
}

static gchar *
detect_window_manager(void)
{
    GdkScreen *screen = gdk_screen_get_default();
    const gchar *windowman;
    const gchar *curdesktop;

    if (!screen || !GDK_IS_SCREEN(screen))
        return NULL;

    windowman = gdk_x11_screen_get_window_manager_name(screen);

    if (g_str_equal(windowman, "Xfwm4"))
        return g_strdup("XFCE 4");

    curdesktop = g_getenv("XDG_CURRENT_DESKTOP");
    if (curdesktop) {
        const gchar *desksession = g_getenv("DESKTOP_SESSION");

        if (desksession && !g_str_equal(curdesktop, desksession))
            return g_strdup(desksession);
    }

    return g_strdup_printf(_("Unknown (Window Manager: %s)"), windowman);
}

static gchar *
desktop_with_session_type(const gchar *desktop_env)
{
    const char *tmp;

    tmp = g_getenv("XDG_SESSION_TYPE");
    if (tmp) {
        if (!g_str_equal(tmp, "unspecified"))
            return g_strdup_printf(_(/*/{desktop environment} on {session type}*/ "%s on %s"), desktop_env, tmp);
    }

    return g_strdup(desktop_env);
}

static gchar *
detect_xdg_environment(const gchar *env_var)
{
    const gchar *tmp;

    tmp = g_getenv(env_var);
    if (!tmp)
        return NULL;

    if (g_str_equal(tmp, "GNOME") || g_str_equal(tmp, "gnome")) {
        gchar *maybe_gnome = detect_gnome_version();

        if (maybe_gnome)
            return maybe_gnome;
    }
    if (g_str_equal(tmp, "KDE") || g_str_equal(tmp, "kde")) {
        gchar *maybe_kde = detect_kde_version();

        if (maybe_kde)
            return maybe_kde;
    }
    if (g_str_equal(tmp, "MATE") || g_str_equal(tmp, "mate")) {
        gchar *maybe_mate = detect_mate_version();

        if (maybe_mate)
            return maybe_mate;
    }

    return g_strdup(tmp);
}

static gchar *
detect_desktop_environment(void)
{
    const gchar *tmp;
    gchar *windowman;

    windowman = detect_xdg_environment("XDG_CURRENT_DESKTOP");
    if (windowman)
        return windowman;
    windowman = detect_xdg_environment("XDG_SESSION_DESKTOP");
    if (windowman)
        return windowman;

    tmp = g_getenv("KDE_FULL_SESSION");
    if (tmp) {
        gchar *maybe_kde = detect_kde_version();

        if (maybe_kde)
            return maybe_kde;
    }
    tmp = g_getenv("GNOME_DESKTOP_SESSION_ID");
    if (tmp) {
        gchar *maybe_gnome = detect_gnome_version();

        if (maybe_gnome)
            return maybe_gnome;
    }

    windowman = detect_window_manager();
    if (windowman)
        return windowman;

    if (!g_getenv("DISPLAY"))
        return g_strdup(_("Terminal"));

    return g_strdup(_("Unknown"));
}

gchar *
computer_get_dmesg_status(void)
{
    gchar *out = NULL, *err = NULL;
    int ex = 1, result = 0;
    g_spawn_command_line_sync("dmesg", &out, &err, &ex, NULL);
    g_free(out);
    g_free(err);
    result += (getuid() == 0) ? 2 : 0;
    result += ex ? 1 : 0;
    switch(result) {
        case 0: /* readable, user */
            return g_strdup(_("User access allowed"));
        case 1: /* unreadable, user */
            return g_strdup(_("User access forbidden"));
        case 2: /* readable, root */
            return g_strdup(_("Access allowed (running as superuser)"));
        case 3: /* unreadable, root */
            return g_strdup(_("Access forbidden? (running as superuser)"));
    }
    return g_strdup(_("(Unknown)"));
}

gchar *
computer_get_aslr(void)
{
    switch (h_sysfs_read_int("/proc/sys/kernel", "randomize_va_space")) {
    case 0:
        return g_strdup(_("Disabled"));
    case 1:
        return g_strdup(_("Partially enabled (mmap base+stack+VDSO base)"));
    case 2:
        return g_strdup(_("Fully enabled (mmap base+stack+VDSO base+heap)"));
    default:
        return g_strdup(_("Unknown"));
    }
}

gchar *
computer_get_entropy_avail(void)
{
    gchar tab_entropy_fstr[][32] = {
      N_(/*/bits of entropy for rng (0)*/              "(None or not available)"),
      N_(/*/bits of entropy for rng (low/poor value)*/  "%d bits (low)"),
      N_(/*/bits of entropy for rng (medium value)*/    "%d bits (medium)"),
      N_(/*/bits of entropy for rng (high/good value)*/ "%d bits (healthy)")
    };
    gint bits = h_sysfs_read_int("/proc/sys/kernel/random", "entropy_avail");
    if (bits > 3000) return g_strdup_printf(_(tab_entropy_fstr[3]), bits);
    if (bits > 200)  return g_strdup_printf(_(tab_entropy_fstr[2]), bits);
    if (bits > 1)    return g_strdup_printf(_(tab_entropy_fstr[1]), bits);
    return g_strdup_printf(_(tab_entropy_fstr[0]), bits);
}

gchar *
computer_get_language(void)
{
    gchar *tab_lang_env[] =
        { "LANGUAGE", "LANG", "LC_ALL", "LC_MESSAGES", NULL };
    gchar *lc = NULL, *env = NULL, *ret = NULL;
    gint i = 0;

    lc = setlocale(LC_ALL, NULL);

    while (tab_lang_env[i] != NULL) {
        env = g_strdup( g_getenv(tab_lang_env[i]) );
        if (env != NULL)  break;
        i++;
    }

    if (env != NULL)
        if (lc != NULL)
            ret = g_strdup_printf("%s (%s)", lc, env);
        else
            ret = g_strdup_printf("%s", env);
    else
        if (lc != NULL)
            ret = g_strdup_printf("%s", lc);

    if (ret == NULL)
        ret = g_strdup( _("(Unknown)") );

    return ret;
}

static Distro
parse_os_release(void)
{
    gchar *pretty_name = NULL;
    gchar *id = NULL;
    gchar **split, *contents, **line;

    if (!g_file_get_contents("/usr/lib/os-release", &contents, NULL, NULL))
        return (Distro) {};

    split = g_strsplit(idle_free(contents), "\n", 0);
    if (!split)
        return (Distro) {};

    for (line = split; *line; line++) {
        if (!strncmp(*line, "ID=", sizeof("ID=") - 1)) {
            id = g_strdup(*line + strlen("ID="));
        } else if (!strncmp(*line, "PRETTY_NAME=", sizeof("PRETTY_NAME=") - 1)) {
            pretty_name = g_strdup(*line +
                                   strlen("PRETTY_NAME=\""));
            strend(pretty_name, '"');
        }
    }

    g_strfreev(split);

    if (pretty_name)
        return (Distro) { .distro = pretty_name, .codename = id };

    g_free(id);
    return (Distro) {};
}

static Distro
parse_lsb_release(void)
{
    gchar *pretty_name = NULL;
    gchar *id = NULL;
    gchar **split, *contents, **line;

    if (!g_spawn_command_line_sync("/usr/bin/lsb_release -di", &contents, NULL, NULL, NULL))
        return (Distro) {};

    split = g_strsplit(idle_free(contents), "\n", 0);
    if (!split)
        return (Distro) {};

    for (line = split; *line; line++) {
        if (!strncmp(*line, "Distributor ID:\t", sizeof("Distributor ID:\t") - 1)) {
            id = g_utf8_strdown(*line + strlen("Distributor ID:\t"), -1);
        } else if (!strncmp(*line, "Description:\t", sizeof("Description:\t") - 1)) {
            pretty_name = g_strdup(*line + strlen("Description:\t"));
        }
    }

    g_strfreev(split);

    if (pretty_name)
        return (Distro) { .distro = pretty_name, .codename = id };

    g_free(id);
    return (Distro) {};
}

static Distro
detect_distro(void)
{
    static const struct {
        const gchar *file;
        const gchar *codename;
        const gchar *override;
    } distro_db[] = {
#define DB_PREFIX "/etc/"
        { DB_PREFIX "arch-release", "arch", "Arch Linux" },
        { DB_PREFIX "fatdog-version", "fatdog" },
        { DB_PREFIX "debian_version", "debian" },
        { DB_PREFIX "slackware-version", "slk" },
        { DB_PREFIX "mandrake-release", "mdk" },
        { DB_PREFIX "mandriva-release", "mdv" },
        { DB_PREFIX "fedora-release", "fedora" },
        { DB_PREFIX "coas", "coas" },
        { DB_PREFIX "environment.corel", "corel"},
        { DB_PREFIX "gentoo-release", "gnt" },
        { DB_PREFIX "conectiva-release", "cnc" },
        { DB_PREFIX "versão-conectiva", "cnc" },
        { DB_PREFIX "turbolinux-release", "tl" },
        { DB_PREFIX "yellowdog-release", "yd" },
        { DB_PREFIX "sabayon-release", "sbn" },
        { DB_PREFIX "enlisy-release", "enlsy" },
        { DB_PREFIX "SuSE-release", "suse" },
        { DB_PREFIX "sun-release", "sun" },
        { DB_PREFIX "zenwalk-version", "zen" },
        { DB_PREFIX "DISTRO_SPECS", "ppy", "Puppy Linux" },
        { DB_PREFIX "puppyversion", "ppy", "Puppy Linux" },
        { DB_PREFIX "distro-release", "fl" },
        { DB_PREFIX "vine-release", "vine" },
        { DB_PREFIX "PartedMagic-version", "pmag" },
         /*
         * RedHat must be the *last* one to be checked, since
         * some distros (like Mandrake) includes a redhat-relase
         * file too.
         */
        { DB_PREFIX "redhat-release", "rh" },
#undef DB_PREFIX
        { NULL, NULL }
    };
    Distro distro;
    gchar *contents;
    int i;

    distro = parse_os_release();
    if (distro.distro)
        return distro;

    distro = parse_lsb_release();
    if (distro.distro)
        return distro;

    for (i = 0; distro_db[i].file; i++) {
        if (!g_file_get_contents(distro_db[i].file, &contents, NULL, NULL))
            continue;

        if (distro_db[i].override) {
            g_free(contents);
            return (Distro) { .distro = g_strdup(distro_db[i].override),
                              .codename = g_strdup(distro_db[i].codename) };
        }

        if (g_str_equal(distro_db[i].codename, "debian")) {
            /* HACK: Some Debian systems doesn't include the distribuition
             * name in /etc/debian_release, so add them here. */
            if (isdigit(contents[0]) || contents[0] != 'D')
                return (Distro) {
                    .distro = g_strdup_printf("Debian GNU/Linux %s", (char*)idle_free(contents)),
                    .codename = g_strdup(distro_db[i].codename)
                };
        }

        if (g_str_equal(distro_db[i].codename, "fatdog")) {
            return (Distro) {
                .distro = g_strdup_printf("Fatdog64 [%.10s]", (char*)idle_free(contents)),
                .codename = g_strdup(distro_db[i].codename)
            };
        }

        return (Distro) { .distro = contents, .codename = g_strdup(distro_db[i].codename) };
    }

    return (Distro) { .distro = g_strdup(_("Unknown")) };
}

OperatingSystem *
computer_get_os(void)
{
    struct utsname utsbuf;
    OperatingSystem *os;
    int i;

    os = g_new0(OperatingSystem, 1);

    Distro distro = detect_distro();
    os->distro = g_strstrip(distro.distro);
    os->distrocode = distro.codename;

    /* Kernel and hostname info */
    uname(&utsbuf);
    os->kernel_version = g_strdup(utsbuf.version);
    os->kernel = g_strdup_printf("%s %s (%s)", utsbuf.sysname,
				 utsbuf.release, utsbuf.machine);
    os->kcmdline = h_sysfs_read_string("/proc", "cmdline");
    os->hostname = g_strdup(utsbuf.nodename);
    os->language = computer_get_language();
    os->homedir = g_strdup(g_get_home_dir());
    os->username = g_strdup_printf("%s (%s)",
				   g_get_user_name(), g_get_real_name());
    os->libc = get_libc_version();
    scan_languages(os);

    os->desktop = detect_desktop_environment();
    if (os->desktop)
        os->desktop = desktop_with_session_type(idle_free(os->desktop));

    os->entropy_avail = computer_get_entropy_avail();

    if (g_strcmp0(os->distrocode, "ubuntu") == 0) {
        GSList *flavs = ubuntu_flavors_scan();
        if (flavs) {
            /* just use the first one */
            os->distro_flavor = (DistroFlavor*)flavs->data;
        }
        g_slist_free(flavs);
    }

    return os;
}

const gchar *
computer_get_selinux(void)
{
    int r;
    gboolean spawned = g_spawn_command_line_sync("selinuxenabled",
                                                 NULL, NULL, &r, NULL);

    if (!spawned)
        return _("Not installed");

    if (r == 0)
        return _("Enabled");

    return _("Disabled");
}

gchar *
computer_get_lsm(void)
{
    gchar *contents;

    if (!g_file_get_contents("/sys/kernel/security/lsm", &contents, NULL, NULL))
        return g_strdup(_("Unknown"));

    return contents;
}
