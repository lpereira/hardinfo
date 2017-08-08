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
#include <endian.h>
#include "hardinfo.h"
#include "dt_util.h"

static struct {
    char *name; int type;
} prop_types[] = {
    { "name", DTP_STR },
    { "compatible", DTP_STR },
    { "model", DTP_STR },
    { "reg", DTP_REG },
    { "clocks", DTP_CLOCKS },
    { "gpios", DTP_GPIOS },
    { "cs-gpios", DTP_GPIOS },
    { "phandle", DTP_PH },
    { "interrupts", DTP_INTRUPT },
    { "interrupts-extended", DTP_INTRUPT_EX },
    { "interrupt-parent", DTP_PH_REF },
    { "interrupt-controller", DTP_EMPTY },
    { "regulator-min-microvolt", DTP_UINT },
    { "regulator-max-microvolt", DTP_UINT },
    { "clock-frequency", DTP_UINT },
    { "dmas", DTP_DMAS },
    { "dma-channels", DTP_UINT },
    { "dma-requests", DTP_UINT },
    { NULL, 0 },
};

static struct {
    char *name; uint32_t v;
} default_values[] = {
    { "#address-cells", 2 },
    { "#size-cells", 1 },
    { "#dma-cells", 1 },
    { NULL, 0 },
};

struct _dtr_map {
    uint32_t v;  /* phandle */
    char *label;  /* alias */
    char *path;
    struct _dtr_map *next;
};
typedef struct _dtr_map dtr_map;

struct _dtr {
    dtr_map *aliases;
    dtr_map *symbols;
    dtr_map *phandles;
    char *base_path;
    char *log;
};

struct _dtr_obj {
    char *path;
    union {
        void *data;
        char *data_str;
        dt_uint *data_int;
    };
    char *name;
    uint32_t length;
    int type;
    char *prefix;        /* if the name has a manufacturer's prefix or null */
    char *np_name;       /* the name without any prefix. points into .prefix or .name, do not free */
    const char *alias;  /* null until first dtr_obj_alias(). do not free */
    const char *symbol; /* null until first dtr_obj_symbol(). do not free */
    dtr *dt;
};

dtr_map *dtr_map_add(dtr_map *map, uint32_t v, const char *label, const char *path) {
    dtr_map *it;
    dtr_map *nmap = malloc(sizeof(dtr_map));
    memset(nmap, 0, sizeof(dtr_map));
    nmap->v = v;

    if (label != NULL) nmap->label = strdup(label);
    if (path != NULL) nmap->path = strdup(path);
    if (map == NULL)
        return nmap;

    it = map;
    while(it->next != NULL)
        it = it->next;
    it->next = nmap;

    return nmap;
}

void dtr_map_free(dtr_map *map) {
    dtr_map *it;
    while(map != NULL) {
        it = map->next;
        free(map->label);
        free(map->path);
        free(map);
        map = it;
    }
}

/* simple sort for maps
 * sv: 1 = sort by v, 0 = sort by label */
void dtr_map_sort(dtr_map *map, int sv)
{
    int done = 0, cmp;
    dtr_map *this, *next, *top, *next_top;
    uint32_t tmp_v;
    char *tmp_l, *tmp_p;
    if (map == NULL) return;
    do {
        this = map;
        next_top = NULL;
        while(this != NULL) {
            next = this->next;
            if (this == top)
                break;
            if (next == NULL)
                break;
            if (sv)
                cmp = (this->v > next->v) ? 1 : 0;
            else
                cmp = strcmp(this->label, next->label);
            if (cmp > 0) {
                tmp_v = this->v; this->v = next->v; next->v = tmp_v;
                tmp_l = this->label; this->label = next->label; next->label = tmp_l;
                tmp_p = this->path; this->path = next->path; next->path = tmp_p;
                next_top = this;
            }
            this = next;
        }
        if (next_top == NULL)
            done = 1;
        else
            top = next_top;
    } while (!done);
}

const char *dtr_phandle_lookup(dtr *s, uint32_t v) {
    dtr_map *phi = s->phandles;
    /* 0 and 0xffffffff are invalid phandle values */
    /* TODO: perhaps "INVALID" or something */
    if (v == 0 || v == 0xffffffff)
        return NULL;
    while(phi != NULL) {
        if (phi->v == v)
            return phi->path;
        phi = phi->next;
    }
    return NULL;
}

const char *dtr_alias_lookup(dtr *s, const char* label) {
    dtr_map *ali = s->aliases;
    while(ali != NULL) {
        if (strcmp(ali->label, label) == 0)
            return ali->path;
        ali = ali->next;
    }
    return NULL;
}

const char *dtr_alias_lookup_by_path(dtr *s, const char* path) {
    dtr_map *ali = s->aliases;
    while(ali != NULL) {
        if (strcmp(ali->path, path) == 0)
            return ali->label;
        ali = ali->next;
    }
    return NULL;
}

const char *dtr_symbol_lookup_by_path(dtr *s, const char* path) {
    dtr_map *ali = s->symbols;
    while(ali != NULL) {
        if (strcmp(ali->path, path) == 0)
            return ali->label;
        ali = ali->next;
    }
    return NULL;
}

void _dtr_read_aliases(dtr *);
void _dtr_read_symbols(dtr *);
void _dtr_map_phandles(dtr *, char *np);

const char *dtr_find_device_tree_root() {
    char *candidates[] = {
        "/proc/device-tree",
        "/sys/firmware/devicetree/base",
        /* others? */
        NULL
    };
    int i = 0;
    while (candidates[i] != NULL) {
        if(access(candidates[i], F_OK) != -1)
            return candidates[i];
        i++;
    }
    return NULL;
}

dtr *dtr_new_x(const char *base_path, int fast) {
    dtr *dt = malloc(sizeof(dtr));
    if (dt != NULL) {
        memset(dt, 0, sizeof(dtr));
        dt->log = strdup("");

        if (base_path == NULL)
            base_path = DTR_ROOT;

        if (base_path != NULL)
            dt->base_path = strdup(base_path);
        else {
            dtr_msg(dt, "%s", "Device Tree not found.");
            return dt;
        }

        /* build alias and phandle lists */
        dt->aliases = NULL;
        dt->symbols = NULL;
        dt->phandles = NULL;
        if (!fast) {
            _dtr_read_aliases(dt);
            _dtr_read_symbols(dt);
            _dtr_map_phandles(dt, "");
        }
    }
    return dt;
}

dtr *dtr_new(const char *base_path) {
    return dtr_new_x(base_path, 0);
}

void dtr_free(dtr *s) {
    if (s != NULL) {
        dtr_map_free(s->aliases);
        dtr_map_free(s->symbols);
        dtr_map_free(s->phandles);
        free(s->base_path);
        free(s->log);
        free(s);
    }
}

int dtr_was_found(dtr *s) {
    if (s != NULL) {
        if (s->base_path != NULL)
            return 1;
    }
    return 0;
}

void dtr_msg(dtr *s, char *fmt, ...) {
    gchar *buf, *tmp;
    va_list args;

    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);

    tmp = g_strdup_printf("%s%s", s->log, buf);
    g_free(s->log);
    s->log = tmp;
}

char *dtr_messages(dtr *s) {
    if (s != NULL)
        return strdup(s->log);
    else
        return NULL;
}

const char *dtr_base_path(dtr *s) {
    if (s)
        return s->base_path;
    return NULL;
}

/*glib, but _dt_obj *prop uses malloc() and std types */
dtr_obj *dtr_obj_read(dtr *s, const char *dtp) {
    char *full_path;
    char *slash, *coma;
    dtr_obj *obj;

    if (dtp == NULL)
        return NULL;

    obj = malloc(sizeof(dtr_obj));
    if (obj != NULL) {
        memset(obj, 0, sizeof(dtr_obj));

        obj->dt = s;
        if (*dtp != '/') {
            /* doesn't start with slash, use alias */
            obj->path = (char*)dtr_alias_lookup(s, dtp);
            if (obj->path != NULL)
                obj->path = strdup(obj->path);
            else {
                dtr_obj_free(obj);
                return NULL;
            }
        } else
            obj->path = strdup(dtp);

        /* find name after last slash, or start */
        slash = strrchr(obj->path, '/');
        if (slash != NULL)
            obj->name = strdup(slash + 1);
        else
            obj->name = strdup(obj->path);

        /* find manufacturer prefix */
        obj->prefix = strdup(obj->name);
        coma = strchr(obj->prefix, ',');
        if (coma != NULL) {
            /* coma -> null; .np_name starts after */
            *coma = 0;
            obj->np_name = coma + 1;
        } else {
            obj->np_name = obj->name;
            free(obj->prefix);
            obj->prefix = NULL;
        }

        /* read data */
        full_path = g_strdup_printf("%s%s", s->base_path, obj->path);
        if ( g_file_test(full_path, G_FILE_TEST_IS_DIR) ) {
            obj->type = DT_NODE;
        } else {
            if (!g_file_get_contents(full_path, (gchar**)&obj->data, (gsize*)&obj->length, NULL)) {
                dtr_obj_free(obj);
                g_free(full_path);
                return NULL;
            }
            obj->type = dtr_guess_type(obj);
        }
        g_free(full_path);

        return obj;
    }
    return NULL;
}

void dtr_obj_free(dtr_obj *s) {
    if (s != NULL) {
        free(s->path);
        free(s->name);
        free(s->prefix);
        free(s->data);
        free(s);
    }
}

int dtr_obj_type(dtr_obj *s) {
    if (s)
        return s->type;
    return DT_TYPE_ERR;
}

char *dtr_obj_alias(dtr_obj *s) {
    if (s) {
        if (s->alias == NULL)
            s->alias = dtr_alias_lookup_by_path(s->dt, s->path);
        return (char*)s->alias;
    }
    return NULL;
}

char *dtr_obj_symbol(dtr_obj *s) {
    if (s) {
        if (s->symbol == NULL)
            s->symbol = dtr_symbol_lookup_by_path(s->dt, s->path);
        return (char*)s->symbol;
    }
    return NULL;
}

char *dtr_obj_path(dtr_obj *s) {
    if (s)
        return s->path;
    return NULL;
}

char *dtr_obj_full_path(dtr_obj *s) {
    if (s) {
        if (strcmp(s->path, "/") == 0)
            return g_strdup_printf("%s", s->dt->base_path);
        else
            return g_strdup_printf("%s%s", s->dt->base_path, s->path);
    }
    return NULL;
}

dtr_obj *dtr_get_prop_obj(dtr *s, dtr_obj *node, const char *name) {
    dtr_obj *prop;
    char *ptmp;
    ptmp = g_strdup_printf("%s/%s", (node == NULL) ? "" : node->path, name);
    prop = dtr_obj_read(s, ptmp);
    g_free(ptmp);
    return prop;
}

char *dtr_get_prop_str(dtr *s, dtr_obj *node, const char *name) {
    dtr_obj *prop;
    char *ptmp;
    char *ret = NULL;

    ptmp = g_strdup_printf("%s/%s", (node == NULL) ? "" : node->path, name);
    prop = dtr_obj_read(s, ptmp);
    if (prop != NULL && prop->data != NULL) {
        ret = strdup(prop->data_str);
        dtr_obj_free(prop);
    }
    g_free(ptmp);
    return ret;
}

char *dtr_get_string(const char *p, int decode) {
    dtr *dt = dtr_new_x(NULL, 1);
    dtr_obj *obj;
    char *ret = NULL;
    if (decode) {
        obj = dtr_get_prop_obj(dt, NULL, p);
        ret = dtr_str(obj);
        dtr_obj_free(obj);
    } else
        ret = dtr_get_prop_str(dt, NULL, p);
    dtr_free(dt);
    return ret;
}

uint32_t dtr_get_prop_u32(dtr *s, dtr_obj *node, const char *name) {
    dtr_obj *prop;
    char *ptmp;
    uint32_t ret = 0;

    ptmp = g_strdup_printf("%s/%s", (node == NULL) ? "" : node->path, name);
    prop = dtr_obj_read(s, ptmp);
    if (prop != NULL && prop->data != NULL) {
        ret = be32toh(*prop->data_int);
        dtr_obj_free(prop);
    }
    g_free(ptmp);
    return ret;
}

int dtr_guess_type(dtr_obj *obj) {
    char *tmp, *dash;
    int i = 0, anc = 0, might_be_str = 1;

    if (obj->length == 0)
        return DTP_EMPTY;

    /* special #(.*)-cells names are UINT */
    if (*obj->name == '#') {
        dash = strrchr(obj->name, '-');
        if (dash != NULL) {
            if (strcmp(dash, "-cells") == 0)
                return DTP_UINT;
        }
    }

    /* /aliases/* and /__symbols__/* are always strings */
    if (strncmp(obj->path, "/aliases/", strlen("/aliases/") ) == 0)
        return DTP_STR;
    if (strncmp(obj->path, "/__symbols__/", strlen("/__symbols__/") ) == 0)
        return DTP_STR;

    /* /__overrides__/* */
    if (strncmp(obj->path, "/__overrides__/", strlen("/__overrides__/") ) == 0)
        if (strcmp(obj->name, "name") != 0)
            return DTP_OVR;

    /* lookup known type */
    while (prop_types[i].name != NULL) {
        if (strcmp(obj->name, prop_types[i].name) == 0)
            return prop_types[i].type;
        i++;
    }

    /* maybe a string? */
    for (i = 0; i < obj->length; i++) {
        tmp = obj->data_str + i;
        if ( isalnum(*tmp) ) anc++; /* count the alpha-nums */
        if ( isprint(*tmp) || *tmp == 0 )
            continue;
        might_be_str = 0;
        break;
    }
    if (might_be_str &&
        ( anc >= obj->length - 2 /* all alpha-nums but ^/ and \0$ */
          || anc >= 5 ) /*arbitrary*/)
        return DTP_STR;

    /* multiple of 4 bytes, try list of hex values */
    if (!(obj->length % 4))
        return DTP_HEX;

    return DTP_UNK;
}

char *dtr_elem_phref(dtr *s, dt_uint e, int show_path) {
    const char *ph_path, *al_label;
    char *ret = NULL;
    ph_path = dtr_phandle_lookup(s, be32toh(e));
    if (ph_path != NULL) {
        /* TODO: alias or symbol? */
        al_label = dtr_symbol_lookup_by_path(s, ph_path);
        if (al_label != NULL) {
            if (show_path)
                ret = g_strdup_printf("&%s (%s)", al_label, ph_path);
            else
                ret = g_strdup_printf("&%s", al_label);
        } else {
            if (show_path)
                ret = g_strdup_printf("0x%x (%s)", be32toh(e), ph_path);
        }
    }
    if (ret == NULL)
        ret = dtr_elem_hex(e);
    return ret;
}

char *dtr_elem_hex(dt_uint e) {
    return g_strdup_printf("0x%x", be32toh(e) );
}

char *dtr_elem_byte(uint8_t e) {
    return g_strdup_printf("%x", e);
}

char *dtr_elem_uint(dt_uint e) {
    return g_strdup_printf("%u", be32toh(e) );
}

char *dtr_list_byte(uint8_t *bytes, unsigned long count) {
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

char *dtr_list_hex(dt_uint *list, unsigned long count) {
    char *ret, *dest;
    char buff[12] = "";  /* max element: " 0x00000000\0" */
    unsigned long i, l;
    l = count * 12 + 1;
    dest = ret = malloc(l);
    memset(ret, 0, l);
    for (i = 0; i < count; i++) {
        sprintf(buff, "%s0x%x", (i) ? " " : "", be32toh(list[i]));
        l = strlen(buff);
        strncpy(dest, buff, l);
        dest += l;
    }
    return ret;
}

/*cstd, except for g_strescape()*/
char *dtr_list_str0(const char *data, uint32_t length) {
    char *tmp, *esc, *next_str;
    char ret[1024] = "";
    uint32_t l, tl;

    /* treat as null-separated string list */
    tl = 0;
    strcpy(ret, "");
    tmp = ret;
    next_str = (char*)data;
    while(next_str != NULL) {
        l = strlen(next_str);
        esc = g_strescape(next_str, NULL);
        sprintf(tmp, "%s\"%s\"",
                strlen(ret) ? ", " : "", esc);
        free(esc);
        tmp += strlen(tmp);
        tl += l + 1; next_str += l + 1;
        if (tl >= length) break;
    }

    return strdup(ret);
}

char *dtr_list_override(dtr_obj *obj) {
    /* <phref, string "property:value"> ... */
    char *ret = NULL;
    char *ph, *str;
    char *src;
    int l, consumed = 0;
    src = obj->data_str;
    while (consumed + 5 <= obj->length) {
        ph = dtr_elem_phref(obj->dt, *(dt_uint*)src, 1);
        src += 4; consumed += 4;
        l = strlen(src) + 1; /* consume the null */
        str = dtr_list_str0(src, l);
        ret = appf(ret, "<%s -> %s>", ph, str);
        src += l; consumed += l;
        free(ph);
        free(str);
    }
    if (consumed < obj->length) {
        str = dtr_list_byte(src, obj->length - consumed);
        ret = appf(ret, "%s", str);
        free(str);
    }
    return ret;
}

uint32_t dtr_get_phref_prop(dtr *s, uint32_t phandle, char *prop) {
    const char *ph_path;
    char *tmp;
    uint32_t ret;
    ph_path = dtr_phandle_lookup(s, phandle);
    tmp = g_strdup_printf("%s/%s", ph_path, prop);
    ret = dtr_get_prop_u32(s, NULL, tmp);
    free(tmp);
    return ret;
}

char *dtr_list_phref(dtr_obj *obj, char *ext_cell_prop) {
    /* <phref, #XXX-cells> */
    int count = obj->length / 4;
    int i = 0, ext_cells = 0;
    char *ph_path;
    char *ph, *ext, *ret = NULL;
    while (i < count) {
        if (ext_cell_prop == NULL)
            ext_cells = 0;
        else
            ext_cells = dtr_get_phref_prop(obj->dt, be32toh(obj->data_int[i]), ext_cell_prop);
        ph = dtr_elem_phref(obj->dt, obj->data_int[i], 0); i++;
        if (ext_cells > count - i) ext_cells = count - i;
        ext = dtr_list_hex((obj->data_int + i), ext_cells); i+=ext_cells;
        ret = appf(ret, "<%s%s%s>",
            ph, (ext_cells) ? " " : "", ext);
        g_free(ph);
        g_free(ext);
    }
    return ret;
}

char *dtr_list_interrupts(dtr_obj *obj) {
    char *ext, *ret = NULL;
    uint32_t iparent, icells;
    int count, i = 0;

    iparent = dtr_inh_find(obj, "interrupt-parent", 0);
    if (!iparent) {
        dtr_msg(obj->dt, "Did not find an interrupt-parent for %s", obj->path);
        goto intr_err;
    }
    icells = dtr_get_phref_prop(obj->dt, iparent, "#interrupt-cells");
    if (!icells) {
        dtr_msg(obj->dt, "Invalid #interrupt-cells value %d for %s", icells, obj->path);
        goto intr_err;
    }

    count = obj->length / 4;
    while (i < count) {
        icells = MIN(icells, count - i);
        ext = dtr_list_hex((obj->data_int + i), icells); i+=icells;
        ret = appf(ret, "<%s>", ext);
    }
    return ret;

intr_err:
    return dtr_list_hex(obj->data_int, obj->length);

}

char *dtr_list_reg(dtr_obj *obj) {
    char *tup_str, *ret = NULL;
    uint32_t acells, scells, tup_len;
    uint32_t tups, extra, consumed; /* extra and consumed are bytes */
    uint32_t *next;

    acells = dtr_inh_find(obj, "#address-cells", 2);
    scells = dtr_inh_find(obj, "#size-cells", 2);
    tup_len = acells + scells;
    tups = obj->length / (tup_len * 4);
    extra = obj->length % (tup_len * 4);
    consumed = 0; /* bytes */

    //printf("list reg: %s\n ... acells: %u, scells: %u, extra: %u\n", obj->path, acells, scells, extra);

    if (extra) {
        /* error: length is not a multiple of tuples */
        dtr_msg(obj->dt, "Data length (%u) is not a multiple of (#address-cells:%u + #size-cells:%u) for %s\n",
            obj->length, acells, scells, obj->path);
        return dtr_list_hex(obj->data, obj->length / 4);
    }

    next = obj->data_int;
    consumed = 0;
    while (consumed + (tup_len * 4) <= obj->length) {
        tup_str = dtr_list_hex(next, tup_len);
        ret = appf(ret, "<%s>", tup_str);
        free(tup_str);
        consumed += (tup_len * 4);
        next += tup_len;
    }

    //printf(" ... %s\n", ret);
    return ret;
}

char* dtr_str(dtr_obj *obj) {
    char *ret;
    int type;

    if (obj == NULL) return NULL;
    type = obj->type;

    if (type == DTP_PH_REF) {
        if (!DTEX_PHREFS || obj->length != 4)
            type = DTP_HEX;
    }

    switch(type) {
        case DT_NODE:
            ret = strdup("{node}");
            break;
        case DTP_EMPTY:
            ret = strdup("{empty}");
            break;
        case DTP_STR:
            ret = dtr_list_str0(obj->data_str, obj->length);
            break;
        case DTP_OVR:
            ret = dtr_list_override(obj);
            break;
        case DTP_REG:
            /* <#address-cells #size-cells> */
            ret = dtr_list_reg(obj);
            break;
        case DTP_INTRUPT:
            ret = dtr_list_interrupts(obj);
            break;
        case DTP_INTRUPT_EX:
            /* <phref, #interrupt-cells"> */
            ret = dtr_list_phref(obj, "#interrupt-cells");
            break;
        case DTP_CLOCKS:
            /* <phref, #clock-cells"> */
            ret = dtr_list_phref(obj, "#clock-cells");
            break;
        case DTP_GPIOS:
            /* <phref, #gpio-cells"> */
            ret = dtr_list_phref(obj, "#gpio-cells");
            break;
        case DTP_DMAS:
            /* <phref, #dma-cells"> */
            ret = dtr_list_phref(obj, "#dma-cells");
            break;
        case DTP_PH:
        case DTP_HEX:
            if (obj->length % 4)
                ret = dtr_list_byte((uint8_t*)obj->data, obj->length);
            else
                ret = dtr_list_hex(obj->data, obj->length / 4);
            break;
        case DTP_PH_REF:
            ret = dtr_elem_phref(obj->dt, *obj->data_int, 1);
            break;
        case DTP_UINT:
            ret = dtr_elem_uint(*obj->data_int);
            break;
        case DTP_UNK:
        default:
            if (obj->length > 64) /* maybe should use #define at the top */
                ret = g_strdup_printf(ret, "{data} (%lu bytes)", obj->length);
            else
                ret = dtr_list_byte((uint8_t*)obj->data, obj->length);
            break;
    }

    return ret;
}

dtr_obj *dtr_get_parent_obj(dtr_obj *obj) {
    char *slash, *parent;
    dtr_obj *ret = NULL;

    if (obj == NULL)
        return NULL;

    parent = strdup(obj->path);
    slash = strrchr(parent, '/');
    if (slash != NULL) {
        *slash = 0;
        if (strlen(parent) > 0)
            ret = dtr_obj_read(obj->dt, parent);
        else
            ret = dtr_obj_read(obj->dt, "/");
    }
    free(parent);
    return ret;
}

/* find the value of a path-inherited property by climbing the path */
int dtr_inh_find(dtr_obj *obj, char *qprop, int limit) {
    dtr_obj *tobj, *pobj, *qobj;
    uint32_t ret = 0;
    int i, found = 0;

    if (!limit) limit = 1000;

    tobj = obj;
    while (tobj != NULL) {
        pobj = dtr_get_parent_obj(tobj);
        if (tobj != obj)
            dtr_obj_free(tobj);
        if (!limit || pobj == NULL) break;
        qobj = dtr_get_prop_obj(obj->dt, pobj, qprop);
        if (qobj != NULL) {
            ret = be32toh(*qobj->data_int);
            found = 1;
            dtr_obj_free(qobj);
            break;
        }
        tobj = pobj;
        limit--;
    }
    dtr_obj_free(pobj);

    if (!found) {
        i = 0;
        while(default_values[i].name != NULL) {
            if (strcmp(default_values[i].name, qprop) == 0) {
                ret = default_values[i].v;
                dtr_msg(obj->dt, "Using default value %d for %s in %s\n", ret, qprop, obj->path);
                break;
            }
            i++;
        }
    }

    return ret;
}

void _dtr_read_aliases(dtr *s) {
    gchar *dir_path;
    GDir *dir;
    const gchar *fn;
    dtr_obj *anode, *prop;
    dtr_map *al;
    anode = dtr_obj_read(s, "/aliases");

    dir_path = g_strdup_printf("%s/aliases", s->base_path);
    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            prop = dtr_get_prop_obj(s, anode, fn);
            if (prop->type == DTP_STR) {
                if (*prop->data_str == '/') {
                    al = dtr_map_add(s->aliases, 0, prop->name, prop->data_str);
                    if (s->aliases == NULL)
                        s->aliases = al;
                }
            }
            dtr_obj_free(prop);
        }
    }
    g_dir_close(dir);
    g_free(dir_path);
    dtr_obj_free(anode);
    dtr_map_sort(s->aliases, 0);
}

void _dtr_read_symbols(dtr *s) {
    gchar *dir_path;
    GDir *dir;
    const gchar *fn;
    dtr_obj *anode, *prop;
    dtr_map *al;
    anode = dtr_obj_read(s, "/__symbols__");

    dir_path = g_strdup_printf("%s/__symbols__", s->base_path);
    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            prop = dtr_get_prop_obj(s, anode, fn);
            if (prop->type == DTP_STR) {
                if (*prop->data_str == '/') {
                    al = dtr_map_add(s->symbols, 0, prop->name, prop->data_str);
                    if (s->symbols == NULL)
                        s->symbols = al;
                }
            }
            dtr_obj_free(prop);
        }
    }
    g_dir_close(dir);
    g_free(dir_path);
    dtr_obj_free(anode);
    dtr_map_sort(s->symbols, 0);
}

/* TODO: rewrite */
void _dtr_map_phandles(dtr *s, char *np) {
    gchar *dir_path;
    gchar *ftmp, *ntmp, *ptmp;
    const gchar *fn;
    GDir *dir;
    dtr_obj *prop, *ph_prop;
    dtr_map *ph;
    uint32_t phandle;

    if (np == NULL) np = "";
    dir_path = g_strdup_printf("%s/%s", s->base_path, np);

    prop = dtr_obj_read(s, np);
    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            ftmp = g_strdup_printf("%s/%s", dir_path, fn);
            if ( g_file_test(ftmp, G_FILE_TEST_IS_DIR) ) {
                ntmp = g_strdup_printf("%s/%s", np, fn);
                ptmp = g_strdup_printf("%s/phandle", ntmp);
                ph_prop = dtr_obj_read(s, ptmp);
                if (ph_prop != NULL) {
                    ph = dtr_map_add(s->phandles, be32toh(*ph_prop->data_int), NULL, ntmp);
                    if (s->phandles == NULL)
                        s->phandles = ph;
                }
                _dtr_map_phandles(s, ntmp);
                g_free(ptmp);
                g_free(ntmp);
                dtr_obj_free(ph_prop);
            }
            g_free(ftmp);
        }
    }
    g_dir_close(dir);
    dtr_obj_free(prop);
    dtr_map_sort(s->phandles, 1);
}

/*
 * Maybe these should move to devicetree.c, but would have to expose
 * struct internals.
 */

/* kvl: 0 = key is label, 1 = key is v */
char *dtr_map_info_section(dtr *s, dtr_map *map, char *title, int kvl) {
    gchar *tmp, *ret;
    const gchar *sym;
    ret = g_strdup_printf("[%s]\n", _(title));
    dtr_map *it = map;
    while(it != NULL) {
        if (kvl) {
            sym = dtr_symbol_lookup_by_path(s, it->path);
            if (sym != NULL)
                tmp = g_strdup_printf("%s0x%x (%s)=%s\n", ret,
                    it->v, sym, it->path);
            else
                tmp = g_strdup_printf("%s0x%x=%s\n", ret,
                    it->v, it->path);
        } else
            tmp = g_strdup_printf("%s%s=%s\n", ret,
                it->label, it->path);
        g_free(ret);
        ret = tmp;
        it = it->next;
    }

    return ret;
}

char *dtr_maps_info(dtr *s) {
    gchar *ph_map, *al_map, *sy_map, *ret;

    ph_map = dtr_map_info_section(s, s->phandles, _("phandle Map"), 1);
    al_map = dtr_map_info_section(s, s->aliases, _("Alias Map"), 0);
    sy_map = dtr_map_info_section(s, s->symbols, _("Symbol Map"), 0);
    ret = g_strdup_printf("%s%s%s", ph_map, sy_map, al_map);
    g_free(ph_map);
    g_free(al_map);
    g_free(sy_map);
    return ret;
}

char *appf(char *src, char *fmt, ...) {
    gchar *buf, *ret;
    va_list args;

    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);

    if (src != NULL) {
        ret = g_strdup_printf("%s%s%s", src, sp_sep(src), buf);
        g_free(buf);
        g_free(src);
    } else
        ret = buf;

    return ret;
}

