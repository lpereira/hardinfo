/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

/* { match_string, match_case, name, url } */
static Vendor vendors[] = {
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

static GSList *vendor_list = NULL;

/* sort the vendor list by length of match_string,
 * LONGEST first */
gint vendor_sort (gconstpointer a, gconstpointer b) {
    const Vendor *ap = a, *bp = b;
    int la = 0, lb = 0;
    if (ap && ap->match_string) la = strlen(ap->match_string);
    if (bp && bp->match_string) lb = strlen(bp->match_string);
    if (la == lb) return 0;
    if (la > lb) return -1;
    return 1;
}

static int read_from_vendor_conf(const char *path) {
      GKeyFile *vendors;
      gchar *tmp;
      gint num_vendors, i, count = 0; /* num_vendors is file-reported, count is actual */

      DEBUG("using vendor.conf format loader for %s", path);

      vendors = g_key_file_new();
      if (g_key_file_load_from_file(vendors, path, 0, NULL)) {
        num_vendors = g_key_file_get_integer(vendors, "vendors", "number", NULL);

        for (i = num_vendors - 1; i >= 0; i--) {
          Vendor *v = g_new0(Vendor, 1);

          tmp = g_strdup_printf("vendor%d", i);

          v->match_string = g_key_file_get_string(vendors, tmp, "match_string", NULL);
          if (v->match_string == NULL) {
              /* try old name */
              v->match_string = g_key_file_get_string(vendors, tmp, "id", NULL);
          }
          if (v->match_string) {
              v->match_case = g_key_file_get_integer(vendors, tmp, "match_case", NULL);
              v->name = g_key_file_get_string(vendors, tmp, "name", NULL);
              v->name_short = g_key_file_get_string(vendors, tmp, "name_short", NULL);
              v->url  = g_key_file_get_string(vendors, tmp, "url", NULL);
              v->url_support  = g_key_file_get_string(vendors, tmp, "url_support", NULL);

              vendor_list = g_slist_prepend(vendor_list, v);
              count++;
          } else {
              /* don't add if match_string is null */
              g_free(v);
          }

          g_free(tmp);
        }
        g_key_file_free(vendors);
        DEBUG("... found %d match strings", count);
        return count;
    }
    g_key_file_free(vendors);
    return 0;
}

static int read_from_vendor_ids(const char *path) {
#define VEN_BUFF_SIZE 128
#define VEN_FFWD() while(isspace((unsigned char)*p)) p++;
#define VEN_CHK(TOK) (strncmp(p, TOK, tl = strlen(TOK)) == 0)
    char buff[VEN_BUFF_SIZE] = "";
    char name[VEN_BUFF_SIZE] = "";
    char name_short[VEN_BUFF_SIZE] = "";
    char url[VEN_BUFF_SIZE] = "";
    char url_support[VEN_BUFF_SIZE] = "";
    int count = 0;
    FILE *fd;
    char *p, *b;
    int tl;

    DEBUG("using vendor.ids format loader for %s", path);

    fd = fopen(path, "r");
    if (!fd) return 0;

    while (fgets(buff, VEN_BUFF_SIZE, fd)) {
        b = strchr(buff, '\n');
        if (b) *b = 0;
        p = buff;
        VEN_FFWD();
        if (VEN_CHK("name ")) {
            strncpy(name, p + tl, VEN_BUFF_SIZE - 1);
            strcpy(name_short, "");
            strcpy(url, "");
            strcpy(url_support, "");
        }
        if (VEN_CHK("name_short "))
            strncpy(name_short, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url "))
            strncpy(url, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url_support "))
            strncpy(url_support, p + tl, VEN_BUFF_SIZE - 1);

        if (VEN_CHK("match_string ")) {
            Vendor *v = g_new0(Vendor, 1);
            v->match_string = strdup(p+tl);
            v->match_case = 0;
            v->name = strdup(name);
            v->name_short = strdup(name_short);
            v->url = strdup(url);
            v->url_support = strdup(url_support);
            vendor_list = g_slist_prepend(vendor_list, v);
            count++;
        }

        if (VEN_CHK("match_string_case ")) {
            Vendor *v = g_new0(Vendor, 1);
            v->match_string = strdup(p+tl);
            v->match_case = 1;
            v->name = strdup(name);
            v->name_short = strdup(name_short);
            v->url = strdup(url);
            v->url_support = strdup(url_support);
            vendor_list = g_slist_prepend(vendor_list, v);
            count++;
        }
    }

    fclose(fd);

    DEBUG("... found %d match strings", count);

    return count;
}


void vendor_init(void)
{
    gchar *path;
    static SyncEntry se = {
       .fancy_name = "Update vendor list",
       .name = "RecvVendorList",
       .save_to = "vendor.ids",
       .get_data = NULL
    };

    /* already initialized */
    if (vendor_list) return;

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

        for (i = G_N_ELEMENTS(vendors) - 1; i >= 0; i--) {
            vendor_list = g_slist_prepend(vendor_list, (gpointer) &vendors[i]);
        }
    }

    /* sort the vendor list by length of match string so that short strings are
     * less likely to incorrectly match.
     * example: ST matches ASUSTeK but SEAGATE is not ASUS */
    vendor_list = g_slist_sort(vendor_list, &vendor_sort);

    /* free search location strings */
    n = 0;
    while (file_search_order[n]) {
        free(file_search_order[n]);
        n++;
    }
}

void vendor_cleanup() {
    DEBUG("cleanup vendor list");
    g_slist_free_full(vendor_list, (void (*)(void *))&vendor_free);
}

void vendor_free(Vendor *v) {
    if (v) {
        free(v->name);
        free(v->name_short);
        free(v->url);
        free(v->url_support);
        free(v->match_string);
        free(v);
    }
}

const Vendor *vendor_match(const gchar *id_str, ...) {
    va_list ap, ap2;
    GSList *vlp;
    gchar *tmp = NULL, *p = NULL;
    int tl = 0, c = 0;
    Vendor *ret = NULL;

    if (id_str) {
        c++;
        tl += strlen(id_str);
        tmp = g_malloc0(tl + c + 1);
        strcat(tmp, id_str);
        strcat(tmp, " ");

        va_start(ap, id_str);
        p = va_arg(ap, gchar*);
        while(p) {
            c++;
            tl += strlen(p);
            tmp = g_realloc(tmp, tl + c + 1); /* strings, spaces, null */
            strcat(tmp, p);
            strcat(tmp, " ");
            p = va_arg(ap, gchar*);
        }
        va_end(ap);
    }
    if (!c || tl == 0)
        return NULL;

    DEBUG("full id_str: %s", tmp);

    for (vlp = vendor_list; vlp; vlp = vlp->next) {
        Vendor *v = (Vendor *)vlp->data;

        if (v)
            if (v->match_case) {
                if (v->match_string && strstr(tmp, v->match_string)) {
                    ret = v;
                    break;
                }
            } else {
                if (v->match_string && strcasestr(tmp, v->match_string)) {
                    ret = v;
                    break;
                }
            }
    }

    if (ret)
        DEBUG("ret: match_string: %s -- case: %s -- name: %s", ret->match_string, (ret->match_case ? "yes" : "no"), ret->name);
    else
        DEBUG("ret: not found");

    g_free(tmp);
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
            /* if id_str is shortest, then return as if not found (see below) */
            if (nl <= snl)
                return (sl <= nl) ? id_str : v->name;
            else
                return (sl <= snl) ? id_str : v->name_short;
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
