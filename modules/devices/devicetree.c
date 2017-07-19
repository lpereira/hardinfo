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
#include "devices.h"
#include "dt_util.h"

/* Hardinfo labels that have # are truncated and/or hidden.
 * Labels can't have $ because that is the delimiter in
 * moreinfo. */
gchar *hardinfo_clean_label(const gchar *v, int replacing) {
    gchar *clean, *p;

    p = clean = g_strdup(v);
    while (*p != 0) {
        switch(*p) {
            case '#': case '$':
                *p = '_';
                break;
            default:
                break;
        }
        p++;
    }
    if (replacing)
        g_free((gpointer)v);
    return clean;
}

/* hardinfo uses the values as {ht,x}ml, apparently */
gchar *hardinfo_clean_value(const gchar *v, int replacing) {
    gchar *clean, *tmp;
    gchar **vl;
    if (v == NULL) return NULL;

    vl = g_strsplit(v, "&", -1);
    if (g_strv_length(vl) > 1)
        clean = g_strjoinv("&amp;", vl);
    else
        clean = g_strdup(v);
    g_strfreev(vl);

    vl = g_strsplit(clean, "<", -1);
    if (g_strv_length(vl) > 1) {
        tmp = g_strjoinv("&lt;", vl);
        g_free(clean);
        clean = tmp;
    }
    g_strfreev(vl);

    vl = g_strsplit(clean, ">", -1);
    if (g_strv_length(vl) > 1) {
        tmp = g_strjoinv("&gt;", vl);
        g_free(clean);
        clean = tmp;
    }
    g_strfreev(vl);

    if (replacing)
        g_free((gpointer)v);
    return clean;
}

#include "devicetree/rpi_data.c"

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
            pstr = hardinfo_clean_value(dtr_str(dt, child), 1);
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
    ret = g_strdup_printf("[%s]\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "%s%s",
                    _("Node"),
                    _("Node Path"), dtr_obj_path(node),
                    _("Alias"), (lstr != NULL) ? lstr : _("(None)"),
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
        ret = dtr_str(dt, obj);
        dtr_obj_free(obj);
    } else
        ret = dtr_get_prop_str(dt, NULL, path);
    return ret;
}

gchar *get_summary() {
    char *model = NULL, *serial = NULL, *compat = NULL, *ret = NULL;

    model = get_dt_string("/model", 0);
    serial = get_dt_string("/serial-number", 0);
    compat = get_dt_string("/compatible", 1);

    /* Expand on the DT information from known machines, like RPi.
     * RPi stores a revision value in /proc/cpuinfo that can be used
     * to look up details. This is just a nice place to pull it all
     * together for DT machines, with a nice fallback.
     * PPC Macs could be handled this way too. They store
     * machine identifiers in /proc/cpuinfo. */
    if ( strstr(model, "Raspberry Pi") != NULL )
        ret = rpi_board_details();

    if (ret == NULL) {
        ret = g_strdup_printf(
                "[%s]\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n",
                _("Board"),
                _("Model"), model,
                _("Serial Number"), serial,
                _("Compatible"), compat);
    }

    free(model);
    free(serial);
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

void __scan_dtree()
{
    dt = dtr_new(NULL);
    gchar *summary = get_summary();

    dtree_info = g_strdup("[Device Tree]\n");
    mi_add("Summary", summary);

    add_keys("/");

    //printf("%s\n", dtree_info);
    dtr_free(dt);
}
