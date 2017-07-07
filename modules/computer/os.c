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

#include <string.h>
#include <sys/utsname.h>
#include "hardinfo.h"
#include "computer.h"

static gchar *
get_libc_version(void)
{
    FILE *testp;
    gchar buf[256], *p, *ver_str = NULL, *ret = NULL;

    struct {
        const char *test_cmd;
        const char *match_str;
        const char *lib_name;
    } libs[] = {
        { .test_cmd = "ldconfig -V", .match_str = "GLIBC", .lib_name = "GNU C Library" },
        { .test_cmd = "ldconfig -v", .match_str = "uClibc", .lib_name = "uClibc or uClibc-ng" },
        { .test_cmd = "diet", .match_str = "diet version", .lib_name = "diet libc" },
        { NULL, NULL, NULL },
    };
    int i = 0;
    while (libs[i].test_cmd != NULL) {
        memset(buf, 0, 256);
        testp = popen(libs[i].test_cmd, "r");
        if (testp) {
            (void)fgets(buf, 255, testp);
            pclose(testp);

            /* limit to first line */
            p = strstr(buf, "\n"); if (p) *p = 0;

            if ( strstr(buf, libs[i].match_str) ) {
                ver_str = strstr(buf, " "); /* skip the first word, likely "ldconfig" */
                if (ver_str) ver_str++;
                break;
            }
        }
        i++;
    }

    switch (i) {
        case 0: /* GLIBC */
        case 3: /* diet libc */
            if (ver_str)
                ret = g_strdup_printf("%s / %s", libs[i].lib_name, ver_str );
            else
                ret = g_strdup(libs[i].lib_name);
            break;
        case 1: /* uClibc */
            ret = g_strdup(libs[i].lib_name);
            break;
        default:
            ret = g_strdup(_("Unknown"));
    }

    return ret;
}

#include <gdk/gdkx.h>

void
detect_desktop_environment(OperatingSystem * os)
{
    const gchar *tmp = g_getenv("GNOME_DESKTOP_SESSION_ID");
    FILE *version;
    char vers[16];

    if (tmp) {
	/* FIXME: this might not be true, as the gnome-panel in path
	   may not be the one that's running.
	   see where the user's running panel is and run *that* to
	   obtain the version. */
	version = popen("gnome-about --gnome-version", "r");
	if (version) {
	    (void)fscanf(version, _("Version: %s"), vers);
	    if (pclose(version))
	        goto unknown;
	} else {
	    goto unknown;
	}

	os->desktop = g_strdup_printf("GNOME %s", vers);
    } else if (g_getenv("KDE_FULL_SESSION")) {

	if (g_getenv("KDE_SESSION_VERSION") && strstr(g_getenv("KDE_SESSION_VERSION"),(gchar *)"4")) {
	    version = popen("kwin --version", "r");
	} else {
	    version = popen("kcontrol --version", "r");
	}

	if (version) {
	    char buf[32];

	    (void)fgets(buf, 32, version);

	    (void)fscanf(version, "KDE: %s", vers);
	    if (pclose(version))
	        goto unknown;
	} else {
	    goto unknown;
	}

	os->desktop = g_strdup_printf("KDE %s", vers);
    } else {
      unknown:
        os->desktop = NULL;

	if (!g_getenv("DISPLAY")) {
	    os->desktop = g_strdup(_("Terminal"));
	} else {
            GdkScreen *screen = gdk_screen_get_default();

            if (screen && GDK_IS_SCREEN(screen)) {
              const gchar *windowman;

              windowman = gdk_x11_screen_get_window_manager_name(screen);
              if (g_str_equal(windowman, "Xfwm4")) {
                  /* FIXME: check if xprop -root | grep XFCE_DESKTOP_WINDOW is defined */
                  os->desktop = g_strdup("XFCE 4");
              } else if ((tmp = g_getenv("XDG_CURRENT_DESKTOP"))) {
                  os->desktop = g_strdup(tmp);
                  if ((tmp = g_getenv("DESKTOP_SESSION")) && !g_str_equal(os->desktop, tmp)) {
                      g_free(os->desktop);
                      os->desktop = g_strdup(tmp);
                  }
              }

              if (!os->desktop) {
                  os->desktop = g_strdup_printf(_("Unknown (Window Manager: %s)"),
                                                windowman);
              }
            } else {
                  os->desktop = g_strdup(_("Unknown"));
            }
	}
    }
}

gchar *
computer_get_entropy_avail(void)
{
    gint bits = h_sysfs_read_int("/proc/sys/kernel/random", "entropy_avail");
    if (bits < 200)
        return g_strdup_printf("%d bits (low)", bits);
    if (bits < 3000)
        return g_strdup_printf("%d bits (medium)", bits);
    return g_strdup_printf("%d bits (healthy)", bits);
}

OperatingSystem *
computer_get_os(void)
{
    struct utsname utsbuf;
    OperatingSystem *os;
    int i;

    os = g_new0(OperatingSystem, 1);

    /* Attempt to get the Distribution name; try using /etc/lsb-release first,
       then doing the legacy method (checking for /etc/$DISTRO-release files) */
    if (g_file_test("/etc/lsb-release", G_FILE_TEST_EXISTS)) {
	FILE *release;
	gchar buffer[128];

	release = popen("lsb_release -d", "r");
	if (release) {
            (void)fgets(buffer, 128, release);
            pclose(release);

            os->distro = buffer;
            os->distro = g_strdup(os->distro + strlen("Description:\t"));
        }
    } else if (g_file_test("/etc/arch-release", G_FILE_TEST_EXISTS)) {
        os->distrocode = g_strdup("arch");
        os->distro = g_strdup("Arch Linux");
    } else {
        for (i = 0;; i++) {
            if (distro_db[i].file == NULL) {
                os->distrocode = g_strdup("unk");
                os->distro = g_strdup(_("Unknown distribution"));
                break;
            }

            if (g_file_test(distro_db[i].file, G_FILE_TEST_EXISTS)) {
                FILE *distro_ver;
                char buf[128];

                distro_ver = fopen(distro_db[i].file, "r");
                if (distro_ver) {
                    (void)fgets(buf, 128, distro_ver);
                    fclose(distro_ver);
                } else {
                    continue;
                }

                buf[strlen(buf) - 1] = 0;

                if (!os->distro) {
                    /*
                     * HACK: Some Debian systems doesn't include
                     * the distribuition name in /etc/debian_release,
                     * so add them here.
                     */
                    if (!strncmp(distro_db[i].codename, "deb", 3) &&
                        ((buf[0] >= '0' && buf[0] <= '9') || buf[0] != 'D')) {
                        os->distro = g_strdup_printf
                            ("Debian GNU/Linux %s", buf);
                    } else {
                        os->distro = g_strdup(buf);
                    }
                }

                if (g_str_equal(distro_db[i].codename, "ppy")) {
                  gchar *tmp;
                    tmp = g_strdup_printf("Puppy Linux");
                  g_free(os->distro);
                  os->distro = tmp;
                }

                if (g_str_equal(distro_db[i].codename, "fatdog")) {
                  gchar *tmp;
                    tmp = g_strdup_printf("Fatdog64 [%.10s]", os->distro);
                  g_free(os->distro);
                  os->distro = tmp;
                }

                os->distrocode = g_strdup(distro_db[i].codename);

                break;
            }
        }
    }

    os->distro = g_strstrip(os->distro);

    /* Kernel and hostname info */
    uname(&utsbuf);
    os->kernel_version = g_strdup(utsbuf.version);
    os->kernel = g_strdup_printf("%s %s (%s)", utsbuf.sysname,
				 utsbuf.release, utsbuf.machine);
    os->hostname = g_strdup(utsbuf.nodename);
    os->language = g_strdup(g_getenv("LC_MESSAGES"));
    os->homedir = g_strdup(g_get_home_dir());
    os->username = g_strdup_printf("%s (%s)",
				   g_get_user_name(), g_get_real_name());
    os->libc = get_libc_version();
    scan_languages(os);
    detect_desktop_environment(os);

    os->entropy_avail = computer_get_entropy_avail();

    return os;
}
