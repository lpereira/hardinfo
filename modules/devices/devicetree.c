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
#include <endian.h>
#include "devices.h"

/* some not-quite-working stuff that can be disabled */
#define DTEX_INH_PROPS 0
#define DTEX_PHREFS 0
#define DTEX_GROUP_TUPS 0

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
    DTP_PH,  /* phandle */
    DTP_PH_REF,  /* reference to phandle */
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
    { "phandle", DTP_PH },
    { "interrupts", DTP_HEX },
    { "interrupt-parent", DTP_PH_REF },
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

struct _dt_phandle {
    uint32_t v;
    char *path;
    struct _dt_phandle *next;
};
typedef struct _dt_phandle dt_phandle;

dt_phandle *phandles = NULL;

dt_phandle *dt_phandle_add(uint32_t v, const char *path) {
    dt_phandle *phi = phandles;
    dt_phandle *ph = malloc(sizeof(dt_phandle));
    memset(ph, 0, sizeof(dt_phandle));
    ph->v = v; ph->path = strdup(path);
    if (phi == NULL) {
        phandles = ph;
    } else {
        while(phi->next != NULL) phi = phi->next;
        phi->next = ph;
    }
    return ph;
}

void dt_phandle_free(dt_phandle *ph) {
    if (ph) {
        free(ph->path);
    }
    free(ph);
}

void dt_phandles_free() {
    dt_phandle *phi;
    while(phandles != NULL) {
        phi = phandles->next;
        dt_phandle_free(phandles);
        phandles = phi;
    }
    phandles = NULL;
}

char *dt_phandle_lookup(uint32_t v) {
    dt_phandle *phi = phandles;
    while(phi != NULL) {
        if (phi->v == v)
            return phi->path;
        phi = phi->next;
    }
    return NULL;
}

struct _dt_alias {
    char *label;
    char *path;
    struct _dt_alias *next;
};
typedef struct _dt_alias dt_alias;

dt_alias *aliases;

dt_alias *dt_alias_add(const char *label, const char *path) {
    dt_alias *ali = aliases;
    dt_alias *al = malloc(sizeof(dt_alias));
    memset(al, 0, sizeof(dt_alias));
    al->label = strdup(label); al->path = strdup(path);
    if (ali == NULL) {
        aliases = al;
    } else {
        while(ali->next != NULL) ali = ali->next;
        ali->next = al;
    }
    return al;
}

void dt_alias_free(dt_alias *al) {
    if (al) {
        free(al->label);
        free(al->path);
    }
    free(al);
}
void dt_aliases_free() {
    dt_alias *ali;
    while(aliases != NULL) {
        ali = aliases->next;
        dt_alias_free(aliases);
        aliases = ali;
    }
    aliases = NULL;
}

char *dt_alias_lookup_by_path(const char* path) {
    dt_alias *ali = aliases;
    while(ali != NULL) {
        if (strcmp(ali->path, path) == 0)
            return ali->label;
        ali = ali->next;
    }
    return NULL;
}

void dt_map_phandles(char *np) {
    gchar *dir_path;
    gchar *ftmp, *ntmp, *ptmp;
    const gchar *fn;
    GDir *dir;
    dt_raw *prop, *ph_prop;
    uint32_t phandle;

    if (np == NULL) np = "";
    dir_path = g_strdup_printf("/proc/device-tree/%s", np);

    prop = get_dt_raw(np);
    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            ftmp = g_strdup_printf("%s/%s", dir_path, fn);
            if ( g_file_test(ftmp, G_FILE_TEST_IS_DIR) ) {
                ntmp = g_strdup_printf("%s/%s", np, fn);
                ptmp = g_strdup_printf("%s/phandle", ntmp);
                ph_prop = get_dt_raw(ptmp);
                if (ph_prop != NULL) {
                    phandle = be32toh(*(uint32_t*)ph_prop->data);
                    dt_phandle_add(phandle, ntmp);
                }
                dt_map_phandles(ntmp);
                g_free(ptmp);
                g_free(ntmp);
                dt_raw_free(ph_prop);
            }
            g_free(ftmp);
        }
    }
    g_dir_close(dir);
    dt_raw_free(prop);
}

void dt_read_aliases() {
    gchar *dir_path = g_strdup_printf("/proc/device-tree/aliases");
    gchar *ftmp, *ntmp, *ptmp;
    const gchar *fn;
    GDir *dir;
    dt_raw *prop;

    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            ftmp = g_strdup_printf("%s/%s", dir_path, fn);
            if ( g_file_test(ftmp, G_FILE_TEST_IS_DIR) )
                continue;

            ntmp = g_strdup_printf("aliases/%s", fn);
            prop = get_dt_raw(ntmp);
            dt_alias_add(prop->name, (char*)prop->data);
            g_free(ntmp);
            g_free(ftmp);
        }
    }
    g_dir_close(dir);
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
static char* dt_hex_list(uint32_t *list, unsigned long count, unsigned long tup_len) {
    char *ret, *dest;
    char buff[15] = "";  /* max element: ">, <0x00000000\0" */
    unsigned long i, l, tc;
    l = count * 15 + 1;
    dest = ret = malloc(l);
    memset(ret, 0, l);
    if (tup_len) {
        strcpy(dest, "<");
        dest++;
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

/*cstd*/
static char* dt_byte_list(uint8_t *bytes, unsigned long count) {
    char *ret, *dest;
    char buff[4] = "";  /* max element: " 00\0" */
    uint32_t v;
    unsigned long i, l;
    l = count * 4 + 1;
    ret = malloc(l);
    memset(ret, 0, l);
    strcpy(ret, "[");
    dest = ret + 1;
    for (i = 0; i < count; i++) {
        v = bytes[i];
        sprintf(buff, "%s%02x", (i) ? " " : "", v);
        l = strlen(buff);
        strncpy(dest, buff, l);
        dest += l;
    }
    strcpy(dest, "]");
    return ret;
}

struct {
    const char *name;
    uint32_t def_value;
} inherited_props[] = {
    { "#address-cells", 1 },
    { "#size-cells",    0 },
    { "#clock-cells",   1 },
    { "#gpio-cells",    1 },
    { NULL, 0 },
};
#define INHERITED_PROPS_N 5  /* including term null */
/* requires an un-used int i */
#define INHERITED_PROPS_I(pnm) \
    { i = 0; while (inherited_props[i].name != NULL) { if (strcmp(inherited_props[i].name, pnm) == 0) break; i++; } }

/* find an inherited property by climbing the path */
/*cstd*/
static uint32_t dt_inh_find(dt_raw *prop, const char *inh_prop) {
    char *slash, *tmp, *parent;
    char buff[1024] = "";
    dt_raw *tprop;
    uint32_t ret = 0;
    int found = 0, i;

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
            found = 1;
            break;
        }
    }

    if (!found) {
        INHERITED_PROPS_I(inh_prop); /* sets i */
        ret = inherited_props[i].def_value;
    }

    free(parent);
    return ret;
}

/*cstd*/
static int dt_tup_len(dt_raw *prop) {
    uint32_t address_cells, size_cells,
        clock_cells, gpio_cells;

#if !(DTEX_GROUP_TUPS)
    return 0;
#endif

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
    unsigned long i, l, tl;
    uint32_t phandle;
    char *ph_path, *al_label;

    if (prop == NULL) return NULL;

    if (prop->type == DT_NODE)
        strcpy(ret, "{node}");
    else if (prop->data == NULL)
        strcpy(ret, "{null}");
    else if (prop->length == 0)
        strcpy(ret, "{empty}");
    else {
        i = dt_guess_type(prop);

#if !(DTEX_PHREFS)
        if (i == DTP_PH_REF) i = DTP_HEX;
#endif

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
        } else if (i == DTP_PH && prop->length == 4) {
            phandle = be32toh(*(uint32_t*)prop->data);
            ph_path = dt_phandle_lookup(phandle);
            al_label = dt_alias_lookup_by_path(ph_path);
            if (al_label != NULL)
                sprintf(ret, "0x%x (&%s)", phandle, al_label);
            else
                sprintf(ret, "0x%x", phandle);
        } else if (i == DTP_PH_REF && prop->length == 4) {
            phandle = be32toh(*(uint32_t*)prop->data);
            ph_path = dt_phandle_lookup(phandle);
            if (ph_path != NULL) {
                al_label = dt_alias_lookup_by_path(ph_path);
                if (al_label != NULL)
                    sprintf(ret, "&%s (%s)", al_label, ph_path);
                else
                    sprintf(ret, "0x%x (%s)", phandle, ph_path);
            } else
                sprintf(ret, "<0x%x>", phandle);
        } else {
            if (prop->length > 64) { /* maybe should use #define at the top */
                sprintf(ret, "{data} (%lu bytes)", prop->length);
            } else {
                tmp = dt_byte_list((uint8_t*)prop->data, prop->length);
                strcpy(ret, tmp);
                free(tmp);
            }
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
        prop->path = strdup( p != NULL && strcmp(p, "") ? p : "/" );
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
    gchar *nodes = NULL, *props = NULL, *inh_props = NULL, *ret = NULL;
    gchar *tmp = NULL, *pstr = NULL, *lstr = NULL;
    gchar *dir_path = g_strdup_printf("/proc/device-tree/%s", np);
    gchar *node_path;
    const gchar *fn;
    GDir *dir;
    dt_raw *prop;
    int inh[INHERITED_PROPS_N];
    memset(inh, 0, sizeof(int) * INHERITED_PROPS_N);
    int i;
    uint32_t v;

    props = g_strdup_printf("[%s]\n", _("Properties") );
    inh_props = g_strdup_printf("[%s]\n", _("Inherited Properties") );
    nodes = g_strdup_printf("[%s]\n", _("Children") );

    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            node_path = g_strdup_printf("%s/%s", np, fn);
            prop = get_dt_raw(node_path);
            pstr = hardinfo_clean_value(dt_str(prop), 1);
            lstr = hardinfo_clean_label(fn, 0);
            INHERITED_PROPS_I(prop->name); inh[i] = 1;
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

    prop = get_dt_raw(np);
    for (i = 0; i < INHERITED_PROPS_N - 1; i++)
        if (!inh[i]) {
            v = dt_inh_find(prop, inherited_props[i].name);
            pstr = g_strdup_printf("%u", v);
            lstr = hardinfo_clean_label(inherited_props[i].name, 0);
            tmp = g_strdup_printf("%s%s=%s\n",
                inh_props, lstr, pstr);
            g_free(inh_props);
            inh_props = tmp;
            g_free(pstr);
            g_free(lstr);
        }

#if !(DTEX_INH_PROPS)
    g_free(inh_props); inh_props = g_strdup("");
#endif

    lstr = dt_alias_lookup_by_path(prop->path);
    ret = g_strdup_printf("[%s]\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "%s%s%s",
                    _("Node"),
                    _("Node Path"), prop->path,
                    _("Alias"), (lstr != NULL) ? lstr : _("(None)"),
                    props, inh_props, nodes);

    dt_raw_free(prop);
    g_free(props);
    g_free(inh_props);
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

    dt_read_aliases();
    dt_map_phandles(NULL);

    dtree_info = g_strdup("[Device Tree]\n");
    mi_add("Summary", summary);

    mi_add("/", root_node);
    add_keys("");

    //printf("%s\n", dtree_info);
    dt_aliases_free();
    dt_phandles_free();
}
