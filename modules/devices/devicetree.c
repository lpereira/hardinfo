/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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
/*
 * Device Tree support by Burt P. <pburt0@gmail.com>
 * Sources:
 *   http://elinux.org/Device_Tree_Usage
 *   http://elinux.org/Device_Tree_Mysteries
 */
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include "hardinfo.h"
#include "devices.h"
#include "cpu_util.h"
#include "dt_util.h"
#include "appf.h"

gchar *dtree_info = NULL;
const char *dtree_mem_str = NULL; /* used by memory devices when nothing else is available */

/* These should really go into CMakeLists.txt */
#if defined(__arm__)
#include "devicetree/rpi_data.c"
#elif defined(__powerpc__)
#include "devicetree/pmac_data.c"
#endif

static gchar *get_node(dtr *dt, char *np) {
    gchar *nodes = NULL, *props = NULL, *ret = NULL;
    gchar *tmp = NULL, *pstr = NULL, *lstr = NULL;
    gchar *dir_path;
    gchar *node_path;
    const gchar *fn;
    GDir *dir;
    dtr_obj *node, *child;

    props = g_strdup_printf("[%s]\n", _("Properties") );
    nodes = g_strdup_printf("[%s]\n", _("Children") );
    node = dtr_obj_read(dt, np);
    dir_path = dtr_obj_full_path(node);

    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            child = dtr_get_prop_obj(dt, node, fn);
            pstr = hardinfo_clean_value(dtr_str(child), 1);
            lstr = hardinfo_clean_label(fn, 0);
            if (dtr_obj_type(child) == DT_NODE) {
                tmp = g_strdup_printf("%s%s=%s\n",
                    nodes, lstr, pstr);
                g_free(nodes);
                nodes = tmp;
            } else {
                tmp = g_strdup_printf("%s%s=%s\n",
                    props, lstr, pstr);
                g_free(props);
                props = tmp;
            }
            dtr_obj_free(child);
            g_free(pstr);
            g_free(lstr);
        }
    }
    g_dir_close(dir);
    g_free(dir_path);

    lstr = dtr_obj_alias(node);
    pstr = dtr_obj_symbol(node);
    ret = g_strdup_printf("[%s]\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "%s%s",
                    _("Node"),
                    _("Node Path"), dtr_obj_path(node),
                    _("Alias"), (lstr != NULL) ? lstr : _("(None)"),
                    _("Symbol"), (pstr != NULL) ? pstr : _("(None)"),
                    props, nodes);

    dtr_obj_free(node);
    g_free(props);
    g_free(nodes);

    return ret;
}

/* different from  dtr_get_string() in that it re-uses the existing dt */
static char *get_dt_string(dtr *dt, char *path, gboolean decode) {
    char *ret;

    if (decode) {
        dtr_obj *obj = dtr_get_prop_obj(dt, NULL, path);

        ret = dtr_str(obj);

        dtr_obj_free(obj);
    } else {
        ret = dtr_get_prop_str(dt, NULL, path);
    }

    return ret;
}

static gchar *get_summary(dtr *dt) {
    char *model = NULL, *compat = NULL;
    char *ret = NULL;

    model = get_dt_string(dt, "/model", 0);
    compat = get_dt_string(dt, "/compatible", 1);
    UNKIFNULL(model);
    EMPIFNULL(compat);

#if defined(__arm__)
    /* Expand on the DT information from known machines, like RPi.
     * RPi stores a revision value in /proc/cpuinfo that can be used
     * to look up details. This is just a nice place to pull it all
     * together for DT machines, with a nice fallback.
     * PPC Macs could be handled this way too. They store
     * machine identifiers in /proc/cpuinfo. */
    if (strstr(model, "Raspberry Pi")
        || strstr(compat, "raspberrypi")) {
        gchar *gpu_compat = get_dt_string(dt, "/soc/gpu/compatible", 1);
        gchar *rpi_details = rpi_board_details();
        gchar *basic_info;

        basic_info = g_strdup_printf(
                "[%s]\n"
                "%s=%s\n"
                "%s=%s\n",
                _("Platform"),
                _("Compatible"), compat,
                _("GPU-compatible"), gpu_compat);

        if (rpi_details) {
            ret = g_strconcat(rpi_details, basic_info, NULL);

            g_free(rpi_details);
        } else {
            gchar *serial_number = get_dt_string(dt, "/serial-number", 1);

            ret = g_strdup_printf(
                "[%s]\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s",
                _("Raspberry Pi or Compatible"),
                _("Model"), model,
                _("Serial Number"), serial_number,
                _("RCode"), _("No revision code available; unable to lookup model details."),
                basic_info);

            g_free(serial_number);
        }

        g_free(gpu_compat);
        g_free(basic_info);
    }
#endif

#if defined(__powerpc__)
    /* Power Macintosh */
    if (strstr(compat, "PowerBook") != NULL
         || strstr(compat, "MacRISC") != NULL
         || strstr(compat, "Power Macintosh") != NULL) {
        gchar *mac_details = ppc_mac_details();

        if (mac_details) {
            gchar *serial_number = get_dt_string(dt, "/serial-number", 1);

            ret = g_strdup_printf(
                "%s[%s]\n"
                "%s=%s\n",
                mac_details,
                _("More"),
                _("Serial Number"), serial_number);

            free(mac_details);
            free(serial_number);
        }
    }
#endif

    /* fallback */
    if (!ret) {
        gchar *serial_number = get_dt_string(dt, "/serial-number", 1);
        EMPIFNULL(serial_number);
        ret = g_strdup_printf(
                "[%s]\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n",
                _("Board"),
                _("Model"), model,
                _("Serial Number"), serial_number,
                _("Compatible"), compat);
        free(serial_number);
    }

    free(model);
    free(compat);

    return ret;
}

static void mi_add(const char *key, const char *value, int report_details) {
    gchar *ckey, *rkey;

    ckey = hardinfo_clean_label(key, 0);
    rkey = g_strdup_printf("%s:%s", "DTREE", ckey);

    dtree_info = h_strdup_cprintf("$%s%s$%s=\n", dtree_info,
        (report_details) ? "!" : "", rkey, ckey);
    moreinfo_add_with_prefix("DEV", rkey, g_strdup(value));

    g_free(ckey);
    g_free(rkey);
}

static void add_keys(dtr *dt, char *np) {
    gchar *dir_path, *dt_path;
    gchar *ftmp, *ntmp;
    gchar *n_info;
    const gchar *fn;
    GDir *dir;
    dtr_obj *obj;

    /* add self */
    obj = dtr_obj_read(dt, np);
    dt_path = dtr_obj_path(obj);
    n_info = get_node(dt, dt_path);
    mi_add(dt_path, n_info, 0);

    dir_path = g_strdup_printf("%s/%s", dtr_base_path(dt), np);
    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            ftmp = g_strdup_printf("%s/%s", dir_path, fn);
            if ( g_file_test(ftmp, G_FILE_TEST_IS_DIR) ) {
                if (strcmp(np, "/") == 0)
                    ntmp = g_strdup_printf("/%s", fn);
                else
                    ntmp = g_strdup_printf("%s/%s", np, fn);
                add_keys(dt, ntmp);
                g_free(ntmp);
            }
            g_free(ftmp);
        }
        g_dir_close(dir);
    }
}

static char *msg_section(dtr *dt, int dump) {
    gchar *aslbl = NULL;
    gchar *messages = dtr_messages(dt);
    gchar *ret = g_strdup_printf("[%s]", _("Messages"));
    gchar **lines = g_strsplit(messages, "\n", 0);
    int i = 0;
    while(lines[i] != NULL) {
        aslbl = hardinfo_clean_label(lines[i], 0);
        ret = appfnl(ret, "%s=", aslbl);
        g_free(aslbl);
        i++;
    }
    g_strfreev(lines);
    if (dump)
        printf("%s", messages);
    g_free(messages);
    return ret;
}

void __scan_dtree()
{
    dtr *dt = dtr_new(NULL);
    gchar *summary = get_summary(dt);
    gchar *maps = dtr_maps_info(dt);
    gchar *messages = NULL;

    dtree_info = g_strdup("[Device Tree]\n");
    mi_add("Summary", summary, 1);
    mi_add("Maps", maps, 0);

    if(dtr_was_found(dt))
        add_keys(dt, "/");
    messages = msg_section(dt, 0);
    mi_add("Messages", messages, 0);

    g_free(summary);
    g_free(maps);
    g_free(messages);
    dtr_free(dt);
}
