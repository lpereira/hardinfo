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

#include "devicetree/rpi_data.c"
#include "devicetree/pmac_data.c"

dtr *dt;
gchar *dtree_info = NULL;

gchar *get_node(char *np) {
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

char *get_dt_string(char *path, int decode) {
    dtr_obj *obj;
    char *ret = NULL;
    if (decode) {
        obj = dtr_get_prop_obj(dt, NULL, path);
        ret = dtr_str(obj);
        dtr_obj_free(obj);
    } else
        ret = dtr_get_prop_str(dt, NULL, path);
    return ret;
}

gchar *get_summary() {
    char *model = NULL, *compat = NULL;
    char *tmp[10];
    char *ret = NULL;

    model = get_dt_string("/model", 0);
    compat = get_dt_string("/compatible", 1);
    UNKIFNULL(model);
    EMPIFNULL(compat);

    /* Expand on the DT information from known machines, like RPi.
     * RPi stores a revision value in /proc/cpuinfo that can be used
     * to look up details. This is just a nice place to pull it all
     * together for DT machines, with a nice fallback.
     * PPC Macs could be handled this way too. They store
     * machine identifiers in /proc/cpuinfo. */
    if ( strstr(model, "Raspberry Pi") != NULL
        || strstr(compat, "raspberrypi") != NULL ) {
        tmp[0] = get_dt_string("/serial-number", 1);
        tmp[1] = get_dt_string("/soc/gpu/compatible", 1);
        tmp[9] = rpi_board_details();
        tmp[8] = g_strdup_printf(
                "[%s]\n" "%s=%s\n" "%s=%s\n",
                _("Platform"),
                _("Compatible"), compat,
                _("GPU-compatible"), tmp[1] );
        if (tmp[9] != NULL) {
            ret = g_strdup_printf("%s%s", tmp[9], tmp[8]);
        } else {
            ret = g_strdup_printf(
                "[%s]\n" "%s=%s\n" "%s=%s\n" "%s=%s\n" "%s",
                _("Raspberry Pi or Compatible"),
                _("Model"), model,
                _("Serial Number"), tmp[0],
                _("RCode"), _("No revision code available; unable to lookup model details."),
                tmp[8]);
        }
        free(tmp[0]); free(tmp[1]);
        free(tmp[9]); free(tmp[8]);
    }

    /* Power Macintosh */
    if (strstr(compat, "PowerBook") != NULL
         || strstr(compat, "MacRISC") != NULL
         || strstr(compat, "Power Macintosh") != NULL) {
        tmp[9] =  ppc_mac_details();
        if (tmp[9] != NULL) {
            tmp[0] = get_dt_string("/serial-number", 1);
            ret = g_strdup_printf(
                "%s[%s]\n" "%s=%s\n", tmp[9],
                _("More"),
                _("Serial Number"), tmp[0] );
            free(tmp[0]);
        }
        free(tmp[9]);
    }

    /* fallback */
    if (ret == NULL) {
        tmp[0] = get_dt_string("/serial-number", 1);
        EMPIFNULL(tmp[0]);
        ret = g_strdup_printf(
                "[%s]\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n",
                _("Board"),
                _("Model"), model,
                _("Serial Number"), tmp[0],
                _("Compatible"), compat);
        free(tmp[0]);
    }
    free(model);
    return ret;
}

void mi_add(const char *key, const char *value) {
    gchar *ckey, *rkey;

    ckey = hardinfo_clean_label(key, 0);
    rkey = g_strdup_printf("%s:%s", "DTREE", ckey);

    dtree_info = g_strdup_printf("%s$%s$%s=\n", dtree_info, rkey, ckey);
    moreinfo_add_with_prefix("DEV", rkey, g_strdup(value));

    g_free(ckey);
    g_free(rkey);
}

void add_keys(char *np) {
    gchar *dir_path, *dt_path;
    gchar *ftmp, *ntmp;
    gchar *n_info;
    const gchar *fn;
    GDir *dir;
    dtr_obj *obj;

    /* add self */
    obj = dtr_obj_read(dt, np);
    dt_path = dtr_obj_path(obj);
    n_info = get_node(dt_path);
    mi_add(dt_path, n_info);

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
                add_keys(ntmp);
                g_free(ntmp);
            }
            g_free(ftmp);
        }
    }
    g_dir_close(dir);
}

char *msg_section(int dump) {
    gchar *aslbl = NULL;
    gchar *messages = dtr_messages(dt);
    gchar *ret = g_strdup_printf("[%s]\n", _("Messages"));
    gchar **lines = g_strsplit(messages, "\n", 0);
    int i = 0;
    while(lines[i] != NULL) {
        aslbl = hardinfo_clean_label(lines[i], 0);
        ret = appf(ret, "%s=\n", aslbl);
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
    dt = dtr_new(NULL);
    gchar *summary = get_summary();
    gchar *maps = dtr_maps_info(dt);
    gchar *messages = NULL;

    dtree_info = g_strdup("[Device Tree]\n");
    mi_add("Summary", summary);
    mi_add("Maps", maps);

    if(dtr_was_found(dt))
        add_keys("/");
    messages = msg_section(0);
    mi_add("Messages", messages);

    //printf("%s\n", dtree_info);

    g_free(summary);
    g_free(maps);
    g_free(messages);
    dtr_free(dt);
}
