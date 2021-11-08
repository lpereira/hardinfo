/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2019 L. A. F. Pereira <l@tia.mat.br>
 *    This file Burt P. <pburt0@gmail.com>
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

#include <hardinfo.h>
#include "distro_flavors.h"
#include "util_sysobj.h" /* for appfsp() */

static const UbuntuFlavor ubuntu_flavors[] = {
    { "Vanilla Server", "distros/ubuntu.svg", "https://ubuntu.org", "ubuntu-server" },
    { "Vanilla Desktop", "distros/ubuntu.svg", "https://ubuntu.org", "ubuntu-desktop" },
    { "Xubuntu", "distros/xubuntu.svg", "https://xubuntu.org", "xubuntu-desktop" },
    { "Kubuntu", "distros/kubuntu.png", "https://kubuntu.org", "kubuntu-desktop" },
    { "Lubuntu", "distros/lubuntu.png", "https://lubuntu.me", "lubuntu-desktop" }, /* formerly or also lubuntu.net? */
    { "Ubuntu MATE", "distros/ubuntu-mate.png", "https://ubuntu-mate.org", "ubuntu-mate-desktop" },
    { "Ubuntu Budgie", "distros/ubuntu-budgie.png", "https://ubuntubudgie.org", "ubuntu-budgie-desktop" },
    { "UbuntuKylin (做最有中国味的操作系统)", "distros/ubuntu-kylin.png", "https://www.ubuntukylin.com", "ubuntukylin-desktop" },
    { "UbuntuStudio", "distros/ubuntu-studio.png", "https://ubuntustudio.org", "ubuntustudio-desktop" },
    /* Deprecated flavors */
    { "Ubuntu GNOME", "distros/ubuntu-gnome.png", "https://ubuntugnome.org", "ubuntu-gnome-desktop" },
    // Edubuntu
    // Mythbuntu
    { NULL }
};

static const UbuntuFlavor *_find_flavor(const gchar *pkg) {
    int i = 0;
    for(; ubuntu_flavors[i].base.name; i++) {
        if (SEQ(ubuntu_flavors[i].package, pkg))
            return &ubuntu_flavors[i];
    }
    return NULL;
}

/* items are const; free with g_slist_free() */
GSList *ubuntu_flavors_scan(void) {
    GSList *ret = NULL;
    gboolean spawned;
    gchar *out, *err, *p, *next_nl;
    gint exit_status;
    const UbuntuFlavor *f = NULL;
    gchar *cmd_line = g_strdup("apt-cache policy");
    int i;
    for(i = 0; ubuntu_flavors[i].base.name; i++) {
        cmd_line = appfsp(cmd_line, "%s", ubuntu_flavors[i].package);
    }
    if (!i)
        return NULL;

    spawned = hardinfo_spawn_command_line_sync(cmd_line,
            &out, &err, &exit_status, NULL);
    if (spawned) {
        p = out;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            int mc = 0;
            char pkg[32] = "";
            if (*p != ' ' && *p != '\t')
                mc = sscanf(p, "%31s", pkg);
            if (mc == 1) {
                strend(pkg, ':');
                f = _find_flavor(pkg);
            } else if
                (g_strstr_len(p, -1, "Installed:")
                && !g_strstr_len(p, -1, "(none)") ) {
                ret = g_slist_append(ret, (void*)f);
            }
            p = next_nl + 1;
        }
        g_free(out);
        g_free(err);
    }
    g_free(cmd_line);
    return ret;
}
