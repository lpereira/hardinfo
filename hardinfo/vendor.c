/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 L. A. F. Pereira <l@tia.mat.br>
 *
 *    List of vendors based on GtkSysInfo (c) Pissens Sebastien.
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
#include <gtk/gtk.h>

#include "vendor.h"
#include "syncmanager.h"
#include "config.h"
#include "hardinfo.h"
#include "strstr_word.h"
#include "util_sysobj.h" /* for appfsp() and SEQ() */

/* { match_string, match_rule, name, url } */
static Vendor vendors_builtin[] = {
    /* BIOS manufacturers */
    { "American Megatrends", 0, "American Megatrends", "www.ami.com"},
    { "Award", 0, "Award Software International", "www.award-bios.com"},
    { "Phoenix", 0, "Phoenix Technologies", "www.phoenix.com"},
    /* x86 vendor strings */
    { "AMDisbetter!", 0, "Advanced Micro Devices", "www.amd.com" },
    { "AuthenticAMD", 0, "Advanced Micro Devices", "www.amd.com" },
    { "CentaurHauls", 0, "VIA (formerly Centaur Technology)", "www.via.tw" },
    { "CyrixInstead", 0, "Cyrix", "" },
    { "GenuineIntel", 0, "Intel", "www.intel.com" },
    { "TransmetaCPU", 0, "Transmeta", "" },
    { "GenuineTMx86", 0, "Transmeta", "" },
    { "Geode by NSC", 0, "National Semiconductor", "" },
    { "NexGenDriven", 0, "NexGen", "" },
    { "RiseRiseRise", 0, "Rise Technology", "" },
    { "SiS SiS SiS", 0, "Silicon Integrated Systems", "" },
    { "UMC UMC UMC", 0, "United Microelectronics Corporation", "" },
    { "VIA VIA VIA", 0, "VIA", "www.via.tw" },
    { "Vortex86 SoC", 0, "DMP Electronics", "" },
    /* x86 VM vendor strings */
    { "KVMKVMKVM", 0, "KVM", "" },
    { "Microsoft Hv", 0, "Microsoft Hyper-V", "www.microsoft.com" },
    { "lrpepyh vr", 0, "Parallels", "" },
    { "VMwareVMware", 0, "VMware", "" },
    { "XenVMMXenVMM", 0, "Xen HVM", "" },
};

#define ven_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/
#define ven_file_err(msg, ...) {       \
    ven_msg(msg, ##__VA_ARGS__);       \
    fflush(stderr);                    \
    if (vendor_die_on_error) exit(-1); }

static vendor_list vendors = NULL;
const vendor_list get_vendors_list() { return vendors; }
gboolean vendor_die_on_error = FALSE;

/* sort the vendor list by length of match_string,
 * LONGEST first */
int vendor_sort (const Vendor *ap, const Vendor *bp) {
    int la = 0, lb = 0;
    if (ap) la = ap->weight;
    if (bp) lb = bp->weight;
    return lb-la;
}

static int read_from_vendor_conf(const char *path) {
      GKeyFile *vendors_file;
      gchar *tmp;
      gint num_vendors, i, count = 0; /* num_vendors is file-reported, count is actual */

      DEBUG("using vendor.conf format loader for %s", path);

      vendors_file = g_key_file_new();
      if (g_key_file_load_from_file(vendors_file, path, 0, NULL)) {
        num_vendors = g_key_file_get_integer(vendors_file, "vendors", "number", NULL);

        for (i = num_vendors - 1; i >= 0; i--) {
          Vendor *v = g_new0(Vendor, 1);

          tmp = g_strdup_printf("vendor%d", i);

          v->match_string = g_key_file_get_string(vendors_file, tmp, "match_string", NULL);
          if (v->match_string == NULL) {
              /* try old name */
              v->match_string = g_key_file_get_string(vendors_file, tmp, "id", NULL);
          }
          if (v->match_string) {
              v->match_rule = g_key_file_get_integer(vendors_file, tmp, "match_case", NULL);
              v->name = g_key_file_get_string(vendors_file, tmp, "name", NULL);
              v->name_short = g_key_file_get_string(vendors_file, tmp, "name_short", NULL);
              v->url  = g_key_file_get_string(vendors_file, tmp, "url", NULL);
              v->url_support  = g_key_file_get_string(vendors_file, tmp, "url_support", NULL);

              vendors = g_slist_prepend(vendors, v);
              count++;
          } else {
              /* don't add if match_string is null */
              g_free(v);
          }

          g_free(tmp);
        }
        g_key_file_free(vendors_file);
        DEBUG("... found %d match strings", count);
        return count;
    }
    g_key_file_free(vendors_file);
    return 0;
}

static int read_from_vendor_ids(const char *path) {
#define VEN_BUFF_SIZE 128
#define VEN_FFWD() while(isspace((unsigned char)*p)) p++;
#define VEN_CHK(TOK) (strncmp(p, TOK, tl = strlen(TOK)) == 0 && (ok = 1))
    char buff[VEN_BUFF_SIZE] = "";

    char vars[7][VEN_BUFF_SIZE];
    char *name = vars[0];
    char *name_short = vars[1];
    char *url = vars[2];
    char *url_support = vars[3];
    char *wikipedia = vars[4];
    char *note = vars[5];
    char *ansi_color = vars[6];
    int name_rule_count = -1;

    int count = 0;
    FILE *fd;
    char *p, *b;
    int tl, line = -1, ok = 0;

    DEBUG("using vendor.ids format loader for %s", path);

    fd = fopen(path, "r");
    if (!fd) return 0;

    while (fgets(buff, VEN_BUFF_SIZE, fd)) {
        ok = 0;
        line++;

        b = strchr(buff, '\n');
        if (b)
            *b = 0;
        else
            ven_file_err("%s:%d: line longer than VEN_BUFF_SIZE (%lu)", path, line, (unsigned long)VEN_BUFF_SIZE);

        b = strchr(buff, '#');
        if (b) *b = 0; /* line ends at comment */

        p = buff;
        VEN_FFWD();
        if (VEN_CHK("name ")) {
            if (name_rule_count == 0)
                ven_file_err("%s:%d: name \"%s\" had no match rules", path, line, name);
            strncpy(name, p + tl, VEN_BUFF_SIZE - 1);
            strcpy(name_short, "");
            strcpy(url, "");
            strcpy(url_support, "");
            strcpy(wikipedia, "");
            strcpy(note, "");
            strcpy(ansi_color, "");
            name_rule_count = 0;
        }
        if (VEN_CHK("name_short "))
            strncpy(name_short, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url "))
            strncpy(url, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url_support "))
            strncpy(url_support, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("wikipedia "))
            strncpy(wikipedia, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("note "))
            strncpy(note, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("ansi_color "))
            strncpy(ansi_color, p + tl, VEN_BUFF_SIZE - 1);

#define dup_if_not_empty(s) (strlen(s) ? g_strdup(s) : NULL)

        int mrule = -1;
        if (VEN_CHK("match_string "))
            mrule = VENDOR_MATCH_RULE_WORD_IGNORE_CASE;
        else if (VEN_CHK("match_string_case "))
            mrule = VENDOR_MATCH_RULE_WORD_MATCH_CASE;
        else if (VEN_CHK("match_string_exact "))
            mrule = VENDOR_MATCH_RULE_EXACT;
        else if (VEN_CHK("match_string_prefix "))
            mrule = VENDOR_MATCH_RULE_WORD_PREFIX_IGNORE_CASE;
        else if (VEN_CHK("match_string_prefix_case "))
            mrule = VENDOR_MATCH_RULE_WORD_PREFIX_MATCH_CASE;
        else if (VEN_CHK("match_string_suffix "))
            mrule = VENDOR_MATCH_RULE_WORD_PREFIX_IGNORE_CASE;
        else if (VEN_CHK("match_string_suffix_case "))
            mrule = VENDOR_MATCH_RULE_WORD_PREFIX_MATCH_CASE;
        else if (VEN_CHK("match_string_num_prefix "))
            mrule = VENDOR_MATCH_RULE_NUM_PREFIX_IGNORE_CASE;
        else if (VEN_CHK("match_string_num_prefix_case "))
            mrule = VENDOR_MATCH_RULE_NUM_PREFIX_MATCH_CASE;

        if (mrule >= 0) {
            Vendor *v = g_new0(Vendor, 1);
            v->file_line = line;
            v->match_string = g_strdup(p+tl);
            v->ms_length = strlen(v->match_string);
            v->match_rule = mrule;
            v->name = g_strdup(name);
            v->name_short = dup_if_not_empty(name_short);
            v->url = dup_if_not_empty(url);
            v->url_support = dup_if_not_empty(url_support);
            v->wikipedia = dup_if_not_empty(wikipedia);
            v->note = dup_if_not_empty(note);
            v->ansi_color = dup_if_not_empty(ansi_color);

            v->weight = v->ms_length;
            /* NUM_PREFIX rules consider +1 characters */
            if (v->match_rule == VENDOR_MATCH_RULE_NUM_PREFIX_MATCH_CASE
                || v->match_rule == VENDOR_MATCH_RULE_NUM_PREFIX_IGNORE_CASE)
                v->weight++;

            v->has_parens = g_utf8_strchr(v->match_string, -1, '(') ? TRUE : FALSE;

            vendors = g_slist_prepend(vendors, v);
            name_rule_count++;
            count++;
        }

        g_strstrip(buff);
        if (!ok && *buff != 0)
            ven_file_err("unrecognised item at %s:%d, %s", path, line, buff);
    }

    fclose(fd);

    DEBUG("... found %d match strings", count);

    return count;
}

void vendor_init(void)
{
    gchar *path;
    static SyncEntry se = {
       .name = N_("Update vendor list"),
       .file_name = "vendor.ids",
    };

    /* already initialized */
    if (vendors) return;

    DEBUG("initializing vendor list");
    sync_manager_add_entry(&se);

    char *file_search_order[] = {
        /* new format */
        g_build_filename(g_get_user_config_dir(), "hardinfo", "vendor.ids", NULL),
        g_build_filename(params.path_data, "vendor.ids", NULL),
        /* old format */
        g_build_filename(g_get_user_config_dir(), "hardinfo", "vendor.conf", NULL),
        g_build_filename(g_get_home_dir(), ".hardinfo", "vendor.conf", NULL), /* old place */
        g_build_filename(params.path_data, "vendor.conf", NULL),
        NULL
    };

    int n = 0;
    while (file_search_order[n]) {
        DEBUG("searching for vendor data at %s ...", file_search_order[n]);
        if (!access(file_search_order[n], R_OK)) {
            path = file_search_order[n];
            DEBUG("... file exists.");
            break;
        }
        n++;
    }

    int fail = 1;

    /* new format */
    if (path && strstr(path, "vendor.ids")) {
        fail = !read_from_vendor_ids(path);
    }

    /* old format */
    if (path && strstr(path, "vendor.conf")) {
        fail = !read_from_vendor_conf(path);
    }

    if (fail) {
        gint i;

        DEBUG("vendor data not found, using internal database");

        for (i = G_N_ELEMENTS(vendors_builtin) - 1; i >= 0; i--) {
            vendors = g_slist_prepend(vendors, (gpointer) &vendors_builtin[i]);
        }
    }

    /* sort the vendor list by length of match string so that short strings are
     * less likely to incorrectly match.
     * example: ST matches ASUSTeK but SEAGATE is not ASUS */
    vendors = g_slist_sort(vendors, (GCompareFunc)vendor_sort);

    /* free search location strings */
    n = 0;
    while (file_search_order[n]) {
        free(file_search_order[n]);
        n++;
    }
}

void vendor_cleanup() {
    DEBUG("cleanup vendor list");
    g_slist_free_full(vendors, (GDestroyNotify)vendor_free);
}

void vendor_free(Vendor *v) {
    if (v) {
        g_free(v->name);
        g_free(v->name_short);
        g_free(v->url);
        g_free(v->url_support);
        g_free(v->ansi_color);
        g_free(v->match_string);
        g_free(v);
    }
}

const Vendor *vendor_match(const gchar *id_str, ...) {
    Vendor *ret = NULL;
    va_list ap, ap2;
    gchar *tmp = NULL, *p = NULL;
    int tl = 0, c = 0;

    if (id_str) {
        c++;
        tl += strlen(id_str);
        tmp = appfsp(tmp, "%s", id_str);

        va_start(ap, id_str);
        p = va_arg(ap, gchar*);
        while(p) {
            c++;
            tl += strlen(p);
            tmp = appfsp(tmp, "%s", p);
            p = va_arg(ap, gchar*);
        }
        va_end(ap);
    }
    if (!c || tl == 0)
        return NULL;

    vendor_list vl = vendors_match_core(tmp, 1);
    if (vl) {
        ret = (Vendor*)vl->data;
        vendor_list_free(vl);
    }
    return ret;
}

static const gchar *vendor_get_name_ex(const gchar * id_str, short shortest) {
    const Vendor *v = vendor_match(id_str, NULL);

    if (v) {
        if (shortest) {
            int sl = strlen(id_str);
            int nl = (v->name) ? strlen(v->name) : 0;
            int snl = (v->name_short) ? strlen(v->name_short) : 0;
            if (!nl) nl = 9999;
            if (!snl) snl = 9999;
            /* if id_str is shortest, then return as if
             *   not found (see below).
             * if all equal then prefer
             *   name_short > name > id_str. */
            if (nl < snl)
                return (sl < nl) ? id_str : v->name;
            else
                return (sl < snl) ? id_str : v->name_short;
        } else
            return v->name;
    }

    return id_str; /* Preserve an old behavior, but what about const? */
}

const gchar *vendor_get_name(const gchar * id_str) {
    return vendor_get_name_ex(id_str, 0);
}

const gchar *vendor_get_shortest_name(const gchar * id_str) {
    return vendor_get_name_ex(id_str, 1);
}

const gchar *vendor_get_url(const gchar * id_str) {
    const Vendor *v = vendor_match(id_str, NULL);

    if (v)
        return v->url;

    return NULL;
}

gchar *vendor_get_link(const gchar *id_str)
{
    const Vendor *v = vendor_match(id_str, NULL);

    if (!v) {
        return g_strdup(id_str);
    }

    return vendor_get_link_from_vendor(v);
}

gchar *vendor_get_link_from_vendor(const Vendor *v)
{
#if GTK_CHECK_VERSION(2, 18, 0)
    gboolean gtk_label_link_ok = TRUE;
#else
    gboolean gtk_label_link_ok = FALSE;
#endif
    gboolean link_ok = params.markup_ok && gtk_label_link_ok;
    /* If using a real link, and wikipedia is available,
     * target that instead of url. There's usually much more
     * information there, plus easily click through to company url. */
    gboolean link_wikipedia = TRUE;

    if (!v)
        return g_strdup(_("Unknown"));

    gchar *url = NULL;

    if (link_ok && link_wikipedia && v->wikipedia
        || v->wikipedia && !v->url)
        url = g_strdup_printf("http://wikipedia.com/wiki/%s", v->wikipedia);
    else if (v->url)
        url = g_strdup(v->url);

    if (!url)
        return g_strdup(v->name);

    auto_free(url);

    if (link_ok) {
        const gchar *prefix;

        if (!strncmp(url, "http://", sizeof("http://") - 1) ||
            !strncmp(url, "https://", sizeof("https://") - 1)) {
            prefix = "";
        } else {
            prefix = "http://";
        }

        return g_strdup_printf("<a href=\"%s%s\">%s</a>", prefix, url, v->name);
    }

    return g_strdup_printf("%s (%s)", v->name, url);
}

vendor_list vendor_list_concat_va(int count, vendor_list vl, ...) {
    vendor_list ret = vl, p = NULL;
    va_list ap;
    va_start(ap, vl);
    if (count > 0) {
        count--; /* includes vl */
        while (count) {
            p = va_arg(ap, vendor_list);
            ret = g_slist_concat(ret, p);
            count--;
        }
    } else {
        p = va_arg(ap, vendor_list);
        while (p) {
            ret = g_slist_concat(ret, p);
            p = va_arg(ap, vendor_list);
        }
    }
    va_end(ap);
    return ret;
}

int vendor_cmp_deep(const Vendor *a, const Vendor *b) {
    int r;
    if (a && !b) return 1;
    if (!a && b) return -1;
    if (!a && !b) return 0;
    r = g_strcmp0(a->name, b->name);
    if (!!r) return r;
    r = g_strcmp0(a->name_short, b->name_short);
    if (!!r) return r;
    r = g_strcmp0(a->ansi_color, b->ansi_color);
    if (!!r) return r;
    r = g_strcmp0(a->url, b->url);
    if (!!r) return r;
    r = g_strcmp0(a->url_support, b->url_support);
    if (!!r) return r;
    r = g_strcmp0(a->wikipedia, b->wikipedia);
    if (!!r) return r;
    return 0;
}

vendor_list vendor_list_remove_duplicates_deep(vendor_list vl) {
    /* vendor_list is GSList* */
    GSList *tvl = vl;
    GSList *evl = NULL;
    while(tvl) {
        const Vendor *tv = tvl->data;
        evl = tvl->next;
        while(evl) {
            const Vendor *ev = evl->data;
            if (vendor_cmp_deep(ev, tv) == 0) {
                GSList *next = evl->next;
                vl = g_slist_delete_link(vl, evl);
                evl = next;
            } else
                evl = evl->next;
        }
        tvl = tvl->next;
    }
    return vl;
}

vendor_list vendors_match(const gchar *id_str, ...) {
    va_list ap, ap2;
    gchar *tmp = NULL, *p = NULL;
    int tl = 0, c = 0;

    if (id_str) {
        c++;
        tl += strlen(id_str);
        tmp = appfsp(tmp, "%s", id_str);

        va_start(ap, id_str);
        p = va_arg(ap, gchar*);
        while(p) {
            c++;
            tl += strlen(p);
            tmp = appfsp(tmp, "%s", p);
            p = va_arg(ap, gchar*);
        }
        va_end(ap);
    }
    if (!c || tl == 0)
        return NULL;

    return vendors_match_core(tmp, -1);
}

vendor_list vendors_match_core(const gchar *str, int limit) {
    gchar *p = NULL;
    GSList *vlp;
    int found = 0;
    vendor_list ret = NULL;

    /* pass [array_index]: function
     * 1st [3]: only check match strings that have () in them
     * 2nd [2]: ignore text in (), like (formerly ...) or (nee ...),
     *      but unfortunately also (now ...)
     * 3rd [1]: (passes[0]): full text */
    gchar *passes[3] = { g_strdup(str), g_strdup(str), g_strdup(str) };
    int pass = 1; p = passes[1];
    while((p = strchr(p, '('))) {
        pass = 3; p++;
        while(*p && *p != ')') { *p = ' '; p++; }
    }

    for (; pass > 0; pass--) {
        for (vlp = vendors; vlp; vlp = vlp->next) {
            Vendor *v = (Vendor *)vlp->data;
            char *m = NULL;

            if (!v) continue;
            if (!v->match_string) continue;

            if (v->has_parens)
                if (pass != 3) continue;

            //ven_msg("pass:%d <<%s>> EXAMINE: \"%s\"", pass, v->match_string, passes[pass-1]);
            int epass;

#define standard_match_work_inner(fn) {                       \
    /* clear so it doesn't match again */                     \
    for(epass = pass; epass > 0; epass--)                     \
    {   char *s = passes[epass-1] + (m - passes[pass-1]);     \
        char *e = s + v->ms_length;                           \
        for(; s < e; s++) *s = ' ';                           \
        g_strstrip(passes[epass-1]);  }                       \
    /* add to return list */                                  \
    ret = vendor_list_append(ret, v);                         \
    found++;                                                  \
    if (*passes[0] == 0)                                      \
        goto vendors_match_core_finish;                       \
    if (limit > 0 && found >= limit)                          \
        goto vendors_match_core_finish; }
#define standard_match_work(fn)                               \
    if ((m = fn(passes[pass-1], v->match_string)))            \
        standard_match_work_inner();

            switch(v->match_rule) {
                case VENDOR_MATCH_RULE_EXACT:
                    if (SEQ(passes[pass-1], v->match_string) ) {
                        /* add to return list */
                        ret = vendor_list_append(ret, v);
                        found++;
                        goto vendors_match_core_finish; /* no way for any other match to happen */
                    }
                    break;
                case VENDOR_MATCH_RULE_WORD_IGNORE_CASE:
                    standard_match_work(strcasestr_word);
                    break;
                case VENDOR_MATCH_RULE_WORD_MATCH_CASE:
                    standard_match_work(strstr_word);
                    break;
                case VENDOR_MATCH_RULE_WORD_PREFIX_MATCH_CASE:
                    standard_match_work(strstr_word_prefix);
                    break;
                case VENDOR_MATCH_RULE_WORD_PREFIX_IGNORE_CASE:
                    standard_match_work(strcasestr_word_prefix);
                    break;
                case VENDOR_MATCH_RULE_WORD_SUFFIX_MATCH_CASE:
                    standard_match_work(strstr_word_suffix);
                    break;
                case VENDOR_MATCH_RULE_WORD_SUFFIX_IGNORE_CASE:
                    standard_match_work(strcasestr_word_suffix);
                    break;
                case VENDOR_MATCH_RULE_NUM_PREFIX_IGNORE_CASE:
                    if ((m = strstr_word_prefix(passes[pass-1], v->match_string)))
                        if (isdigit(m[v->ms_length]))
                            standard_match_work_inner();
                    break;
                case VENDOR_MATCH_RULE_NUM_PREFIX_MATCH_CASE:
                    if ((m = strcasestr_word_prefix(passes[pass-1], v->match_string)))
                        if (isdigit(m[v->ms_length]))
                            standard_match_work_inner();
                    break;
            }
        }
    }

vendors_match_core_finish:

    g_free(passes[0]);
    g_free(passes[1]);
    g_free(passes[2]);
    return ret;
}
