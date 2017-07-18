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
 * Sources: http://elinux.org/Device_Tree_Mysteries
 */
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <endian.h>
#include "devices.h"

enum {
    DT_NODE,
    DT_PROPERTY,
};
enum {
    DTP_UNK,
    DTP_STR, /* null-delimited list of strings */
    DTP_INT,
    DTP_UINT,
    DTP_HEX, /* list of 32-bit values displayed in hex */
};

typedef struct {
    char *path;
    char *name;
    int type;
    unsigned long length;
    void *data;
} dt_raw;

static struct {
    char *name; int type;
} prop_types[] = {
    { "name", DTP_STR },
    { "compatible", DTP_STR },
    { "model", DTP_STR },
    { "reg", DTP_HEX },
    { "clocks", DTP_HEX },
    { "gpios", DTP_HEX },
    { "phandle", DTP_HEX },
    { "interrupts", DTP_HEX },
    { "regulator-min-microvolt", DTP_UINT },
    { "regulator-max-microvolt", DTP_UINT },
    { "clock-frequency", DTP_UINT },
    { NULL, 0 },
};

static dt_raw *get_dt_raw(char *);
static void dt_raw_free(dt_raw *);

/* Hardinfo labels that have # are truncated and/or hidden.
 * Labels can't have $ because that is the delimiter in
 * moreinfo. */
gchar *hardinfo_clean_label(const gchar *v, int replacing) {
    gchar *clean, *p;

    p = clean = g_strdup(v);
    while (*p != 0) {
        switch(*p) {
            case '#': case '$': case '&':
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

#define DT_CHECK_NAME(prop, nm) (strcmp(prop->name, nm) == 0)

/*cstd*/
static int dt_guess_type(dt_raw *prop) {
    char *tmp, *dash;
    int i = 0, anc = 0, might_be_str = 1;

    /* special #(.*)-cells names are UINT */
    if (*prop->name == '#') {
        dash = strrchr(prop->name, '-');
        if (dash != NULL) {
            if (strcmp(dash, "-cells") == 0)
                return DTP_UINT;
        }
    }

    /* lookup known type */
    while (prop_types[i].name != NULL) {
        if (strcmp(prop->name, prop_types[i].name) == 0)
            return prop_types[i].type;
        i++;
    }

    /* maybe a string? */
    for (i = 0; i < prop->length; i++) {
        tmp = (char*)prop->data + i;
        if ( isalnum(*tmp) ) anc++; /* count the alpha-nums */
        if ( isprint(*tmp) || *tmp == 0 )
            continue;
        might_be_str = 0;
        break;
    }
    if (might_be_str &&
        ( anc >= prop->length - 2 /* all alpha-nums but ^/ and \0$ */
          || anc >= 5 ) /*arbitrary*/)
        return DTP_STR;

    if (!(prop->length % 4)) /* multiple of 4 bytes, try list of hex values */
        return DTP_HEX;

    return DTP_UNK;
}

/*cstd*/
static char* dt_hex_list(uint32_t *list, int count, int tup_len) {
    char *ret, *dest;
    char buff[15] = "";  /* max element: ">, <0x00000000\0" */
    int i, l, tc;
    l = count * 15 + 1;
    dest = ret = malloc(l);
    memset(ret, 0, l);
    if (tup_len) {
        strcpy(dest, "<");
        dest = ret + 1;
    }
    tc = 0;
    for (i = 0; i < count; i++) {
        if (tup_len) {
            if (tc == tup_len) {
                sprintf(buff, ">, <0x%x", be32toh(list[i]));
                tc = 1;
            } else {
                sprintf(buff, "%s0x%x", (i) ? " " : "", be32toh(list[i]));
                tc++;
            }
        } else
            sprintf(buff, "%s0x%x", (i) ? " " : "", be32toh(list[i]));
        l = strlen(buff);
        strncpy(dest, buff, l);
        dest += l;
    }
    if (tup_len)
        strcpy(dest, ">");
    return ret;
}

/* find an inherited property by climbing the path */
/*cstd*/
static int dt_inh_find(dt_raw *prop, char *inh_prop) {
    char *slash, *tmp, *parent;
    char buff[1024] = "";
    dt_raw *tprop;
    uint32_t ret = 0;

    if (prop == NULL)
        return 0;

    parent = strdup(prop->path);
    while ( slash = strrchr(parent, '/') ) {
        *slash = 0;
        sprintf(buff, "%s/%s", parent, inh_prop);
        tprop = get_dt_raw(buff);
        if (tprop != NULL) {
            ret = be32toh(*(uint32_t*)tprop->data);
            dt_raw_free(tprop);
            break;
        }
    }

    free(parent);
    return ret;
}

/*cstd*/
static int dt_tup_len(dt_raw *prop) {
    int address_cells, size_cells,
        clock_cells, gpio_cells;

    if (prop == NULL)
        return 0;

    if DT_CHECK_NAME(prop, "reg") {
        address_cells = dt_inh_find(prop, "#address-cells");
        size_cells = dt_inh_find(prop, "#size-cells");
        return address_cells + size_cells;
    }

    if DT_CHECK_NAME(prop, "gpios") {
        gpio_cells = dt_inh_find(prop, "#gpio-cells");
        if (gpio_cells == 0) gpio_cells = 1;
        return gpio_cells;
    }

    if DT_CHECK_NAME(prop, "clocks") {
        clock_cells = dt_inh_find(prop, "#clock-cells");
        return 1 + clock_cells;
    }

    return 0;
}

/*cstd, except for g_strescape()*/
static char* dt_str(dt_raw *prop) {
    char *tmp, *esc, *next_str;
    char ret[1024] = "";
    int i, l, tl;

    if (prop == NULL) return NULL;

    if (prop->type == DT_NODE)
        strcpy(ret, "{node}");
    else if (prop->data == NULL)
        strcpy(ret, "{null}");
    else if (prop->length == 0)
        strcpy(ret, "{empty}");
    else {
        i = dt_guess_type(prop);
        if (i == DTP_STR) {
            /* treat as null-separated string list */
            tl = 0;
            strcpy(ret, "");
            tmp = ret;
            next_str = prop->data;
            while(next_str != NULL) {
                l = strlen(next_str);
                esc = g_strescape(next_str, NULL);
                sprintf(tmp, "%s\"%s\"",
                        strlen(ret) ? ", " : "", esc);
                free(esc);
                tmp += strlen(tmp);
                tl += l + 1; next_str += l + 1;
                if (tl >= prop->length) break;
            }
        } else if (i == DTP_INT && prop->length == 4) {
            /* still use uint32_t for the byte-order conversion */
            sprintf(ret, "%d", be32toh(*(uint32_t*)prop->data) );
        } else if (i == DTP_UINT && prop->length == 4) {
            sprintf(ret, "%u", be32toh(*(uint32_t*)prop->data) );
        } else if (i == DTP_HEX && !(prop->length % 4)) {
            l = prop->length / 4;
            tmp = dt_hex_list((uint32_t*)prop->data, l, dt_tup_len(prop));
            strcpy(ret, tmp);
            free(tmp);
        } else {
            sprintf(ret, "{data} (%lu bytes)", prop->length);
        }
    }
    return strdup(ret);
}

/*glib, but dt_raw *prop uses malloc() and std types */
static dt_raw *get_dt_raw(char *p) {
    gchar *full_path;
    dt_raw *prop;
    prop = malloc(sizeof(dt_raw));
    if (prop != NULL) {
        memset(prop, 0, sizeof(dt_raw));
        full_path = g_strdup_printf("/proc/device-tree/%s", p);
        prop->path = strdup(p);
        if ( g_file_test(full_path, G_FILE_TEST_IS_DIR) ) {
            prop->type = DT_NODE;
        } else {
            prop->type = DT_PROPERTY;
            if (!g_file_get_contents(full_path, (gchar**)&prop->data, (gsize*)&prop->length, NULL)) {
                dt_raw_free(prop);
                return NULL;
            }
        }

        /* find name after last slash, or start */
        char *slash = strrchr(prop->path, '/');
        if (slash != NULL)
            prop->name = strdup(slash + 1);
        else
            prop->name = strdup(prop->path);

        return prop;
    }
    return NULL;
}

/*cstd*/
void dt_raw_free(dt_raw *s) {
    if (s != NULL) {
        free(s->path);
        free(s->name);
        free(s->data);
    }
    free(s);
}

/*cstd*/
static char *get_dt_string(char *p, int decode) {
    dt_raw *prop;
    char *ret, *rep;
    prop = get_dt_raw(p);
    if (prop != NULL) {
        if (decode)
            ret = dt_str(prop);
        else {
            ret = strdup(prop->data);
            if (ret)
                while((rep = strchr(ret, '\n'))) *rep = ' ';
        }
        dt_raw_free(prop);
        return ret;
    }
    return NULL;
}

#include "devicetree/rpi_data.c"

gchar *dtree_info = NULL;

gchar *get_node(char *np) {
    gchar *nodes = NULL, *props = NULL, *ret = NULL;
    gchar *tmp = NULL, *pstr = NULL, *lstr = NULL;
    gchar *dir_path = g_strdup_printf("/proc/device-tree/%s", np);
    gchar *node_path;
    const gchar *fn;
    GDir *dir;
    dt_raw *prop;

    props = g_strdup_printf("[%s]\n", _("Properties") );
    nodes = g_strdup_printf("[%s]\n", _("Children") );

    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            node_path = g_strdup_printf("%s/%s", np, fn);
            prop = get_dt_raw(node_path);
            pstr = hardinfo_clean_value(dt_str(prop), 1);
            lstr = hardinfo_clean_label(fn, 0);
            if (prop->type == DT_NODE) {
                tmp = g_strdup_printf("%s%s=%s\n",
                    nodes, lstr, pstr);
                g_free(nodes);
                nodes = tmp;
            } else if (prop->type == DT_PROPERTY) {
                tmp = g_strdup_printf("%s%s=%s\n",
                    props, lstr, pstr);
                g_free(props);
                props = tmp;
            }
            dt_raw_free(prop);
            g_free(pstr);
            g_free(lstr);
            g_free(node_path);
        }
    }
    g_dir_close(dir);
    g_free(dir_path);
    ret = g_strdup_printf("[Node]\n"
                    "%s=%s\n"
                    "%s%s",
                    _("Node Path"), strcmp(np, "") ? np : "/",
                    props, nodes);
    g_free(props);
    g_free(nodes);
    return ret;
}

gchar *get_summary() {
    char *model = NULL, *serial = NULL, *compat = NULL, *ret = NULL;
    model = get_dt_string("model", 0);
    serial = get_dt_string("serial-number", 0);
    compat = get_dt_string("compatible", 1);

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
    gchar *dir_path = g_strdup_printf("/proc/device-tree/%s", np);
    gchar *ftmp, *ntmp, *ptmp;
    gchar *n_name, *n_info;
    const gchar *fn;
    GDir *dir;

    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            ftmp = g_strdup_printf("%s/%s", dir_path, fn);
            if ( g_file_test(ftmp, G_FILE_TEST_IS_DIR) ) {
                ntmp = g_strdup_printf("%s/%s", np, fn);
                ptmp = g_strdup_printf("%s/name", ntmp);
                n_name = get_dt_string(ptmp, 0);
                n_info = get_node(ntmp);
                mi_add(ntmp, n_info);
                g_free(n_name);
                g_free(n_info);
                g_free(ptmp);

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
    gchar *summary = get_summary();
    gchar *root_node = get_node("");

    dtree_info = g_strdup("[Device Tree]\n");
    mi_add("Summary", summary);

    mi_add("/", root_node);
    add_keys("");

    //printf("%s\n", dtree_info);
}
