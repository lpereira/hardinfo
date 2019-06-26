/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2019 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    Copyright (C) 2019 Burt P. <pburt0@gmail.com>
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

#include "hardinfo.h"
#include "devices.h"
#include "vendor.h"
#include <inttypes.h>

gboolean no_handles = FALSE;

/* strings from dmidecode */
static const char empty_mem_str[] = "No Module Installed";
static const char unknown_mfgr_str[] = "<BAD INDEX>";
static const char mobo_location[] = "System Board Or Motherboard";
static const char mobo_shorter[] = "Mainboard";
static const unsigned long dta = 16; /* array */
static const unsigned long dtm = 17; /* socket */

#define UNKIFNULL2(f) ((f) ? f : _("(Unknown)"))
#define SEQ(s,m) (g_strcmp0(s, m) == 0)

typedef struct {
    unsigned long array_handle;
    gchar *locator;
    gchar *use;
    gchar *ecc;
    int devs;
    int devs_populated;
    long int size_MiB_max;
    long int size_MiB_present;
} dmi_mem_array;

dmi_mem_array *dmi_mem_array_new(unsigned long h) {
    dmi_mem_array *s = g_new0(dmi_mem_array, 1);
    s->array_handle = h;
    s->use = dmidecode_match("Use", &dta, &h);
    s->ecc = dmidecode_match("Error Correction Type", &dta, &h);
    s->locator = dmidecode_match("Location", &dta, &h);
    if (SEQ(s->locator, mobo_location)) {
        g_free(s->locator);
        s->locator = g_strdup(mobo_shorter);
    }
    gchar *array_max_size = dmidecode_match("Maximum Capacity", &dta, &h);
    if (array_max_size) {
        long int v = 0;
        char l[3] = "";
        int mc = sscanf(array_max_size, "%"PRId64" %2s", &v, l);
        if (mc == 2) {
            if (SEQ(l, "TB")) s->size_MiB_max = v * 1024 * 1024;
            else if (SEQ(l, "GB")) s->size_MiB_max = v * 1024;
            else if (SEQ(l, "MB")) s->size_MiB_max = v;
        }
        g_free(array_max_size);
    }
    gchar *array_devs = dmidecode_match("Number Of Devices", &dta, &h);
    s->devs = strtol(array_devs, NULL, 10);
    g_free(array_devs);
    return s;
}

void dmi_mem_array_free(dmi_mem_array* s) {
    if (s) {
        g_free(s->locator);
        g_free(s->use);
        g_free(s->ecc);
        g_free(s);
    }
}

typedef struct {
    unsigned long handle;
    unsigned long array_handle;
    gboolean populated;
    gchar *locator;
    gchar *full_locator;
    gchar *size_str;
    long int size_MiB;

    gchar *type;
    gchar *type_detail;
    gchar *array_locator;
    gchar *bank_locator;
    gchar *form_factor;
    gchar *speed_str;
    gchar *configured_clock_str;
    gchar *voltage_min_str;
    gchar *voltage_max_str;
    gchar *voltage_conf_str;
    gchar *partno;
    gchar *data_width;
    gchar *total_width;
    gchar *mfgr;

    const Vendor *vendor;
} dmi_mem_socket;

typedef struct {
    gboolean empty;
    GSList *arrays;
    GSList *sockets;
} dmi_mem;

dmi_mem_socket *dmi_mem_socket_new(unsigned long h) {
    dmi_mem_socket *s = g_new0(dmi_mem_socket, 1);
    s->handle = h;
    s->locator = dmidecode_match("Locator", &dtm, &h);
    s->size_str = dmidecode_match("Size", &dtm, &h);
    if (s->size_str) {
        long int v = 0;
        char l[3] = "";
        int mc = sscanf(s->size_str, "%"PRId64" %2s", &v, l);
        if (mc == 2) {
            if (SEQ(l, "TB")) s->size_MiB = v * 1024 * 1024;
            else if (SEQ(l, "GB")) s->size_MiB = v * 1024;
            else if (SEQ(l, "MB")) s->size_MiB = v;
        }
    }
    s->bank_locator = dmidecode_match("Bank Locator", &dtm, &h);
    gchar *ah = dmidecode_match("Array Handle", &dtm, &h);
    if (ah) {
        s->array_handle = strtol(ah, NULL, 16);
        g_free(ah);
        s->array_locator = dmidecode_match("Location", &dta, &s->array_handle);
        if (SEQ(s->array_locator, mobo_location)) {
            g_free(s->array_locator);
            s->array_locator = g_strdup(mobo_shorter);
        }
    }

    gchar *ah_str = g_strdup_printf("0x%lx", s->array_handle);
    gchar *h_str = g_strdup_printf("0x%lx", s->handle);
    s->full_locator = g_strdup_printf("%s \u27A4 %s",
            s->array_locator ? s->array_locator : ah_str,
            s->locator ? s->locator : h_str);
    g_free(ah_str);
    g_free(h_str);

    if (!g_str_has_prefix(s->size_str, empty_mem_str)) {
        s->populated = 1;

        s->form_factor = dmidecode_match("Form Factor", &dtm, &h);
        s->type = dmidecode_match("Type", &dtm, &h);
        s->type_detail = dmidecode_match("Type Detail", &dtm, &h);
        s->speed_str = dmidecode_match("Speed", &dtm, &h);
        s->configured_clock_str = dmidecode_match("Configured Clock Speed", &dtm, &h);
        if (!s->configured_clock_str)
            s->configured_clock_str = dmidecode_match("Configured Memory Speed", &dtm, &h);
        s->voltage_min_str = dmidecode_match("Minimum Voltage", &dtm, &h);
        s->voltage_max_str = dmidecode_match("Maximum Voltage", &dtm, &h);
        s->voltage_conf_str = dmidecode_match("Configured Voltage", &dtm, &h);
        s->partno = dmidecode_match("Part Number", &dtm, &h);

        s->data_width = dmidecode_match("Data Width", &dtm, &h);
        s->total_width = dmidecode_match("Total Width", &dtm, &h);

        s->mfgr = dmidecode_match("Manufacturer", &dtm, &h);
        if (g_str_has_prefix(s->mfgr, unknown_mfgr_str)) {
            /* the manufacturer code is unknown to dmidecode */
            g_free(s->mfgr);
            s->mfgr = NULL;
        }

        s->vendor = vendor_match(s->mfgr, NULL);
    }
    return s;
}

void dmi_mem_socket_free(dmi_mem_socket* s) {
    if (s) {
        g_free(s->locator);
        g_free(s->full_locator);
        g_free(s->size_str);
        g_free(s->type);
        g_free(s->type_detail);
        g_free(s->bank_locator);
        g_free(s->array_locator);
        g_free(s->form_factor);
        g_free(s->speed_str);
        g_free(s->configured_clock_str);
        g_free(s->voltage_min_str);
        g_free(s->voltage_max_str);
        g_free(s->voltage_conf_str);
        g_free(s->partno);
        g_free(s->data_width);
        g_free(s->total_width);
        g_free(s->mfgr);

        g_free(s);
    }
}

dmi_mem_array *dmi_mem_find_array(dmi_mem *s, unsigned int handle) {
    GSList *l = NULL;
    for(l = s->arrays; l; l = l->next) {
        dmi_mem_array *a = (dmi_mem_array*)l->data;
        if (a->array_handle == handle)
            return a;
    }
    return NULL;
}

dmi_mem *dmi_mem_new() {
    dmi_mem *m = g_new0(dmi_mem, 1);

    dmi_handle_list *hla = dmidecode_handles(&dta);
    if (hla) {
        unsigned long i = 0;
        for(i = 0; i < hla->count; i++) {
            unsigned long h = hla->handles[i];
            m->arrays = g_slist_append(m->arrays, dmi_mem_array_new(h));
        }
        dmi_handle_list_free(hla);
    }

    dmi_handle_list *hlm = dmidecode_handles(&dtm);
    if (hlm) {
        unsigned long i = 0;
        for(i = 0; i < hlm->count; i++) {
            unsigned long h = hlm->handles[i];
            m->sockets = g_slist_append(m->sockets, dmi_mem_socket_new(h));
        }
        dmi_handle_list_free(hlm);
    }

    if (!m->sockets && !m->arrays) {
        m->empty = 1;
        return m;
    }

    /* update array present devices/size */
    GSList *l = NULL;
    for(l = m->sockets; l; l = l->next) {
        dmi_mem_socket *s = (dmi_mem_socket*)l->data;
        dmi_mem_array *a = dmi_mem_find_array(m, s->array_handle);
        if (a) {
            a->size_MiB_present += s->size_MiB;
            if (s->populated)
                a->devs_populated++;
        }
    }

    return m;
}

void dmi_mem_free(dmi_mem* s) {
    if (s) {
        g_slist_free_full(s->arrays, (GDestroyNotify)dmi_mem_array_free);
        g_slist_free_full(s->sockets, (GDestroyNotify)dmi_mem_socket_free);
        g_free(s);
    }
}

gchar *dmi_mem_socket_info_view0() {
    gchar *ret = g_strdup("[$ShellParam$]\nViewType=0\n");
    GSList *l = NULL;

    dmi_mem *mem = dmi_mem_new();

    /* Arrays */
    for(l = mem->arrays; l; l = l->next) {
        dmi_mem_array *a = (dmi_mem_array*)l->data;
        gchar *size_str = NULL;

        if (a->size_MiB_max > 1024 && (a->size_MiB_max % 1024 == 0)
            && a->size_MiB_present > 1024 && (a->size_MiB_present % 1024 == 0) )
            size_str = g_strdup_printf("%"PRId64" / %"PRId64" %s", a->size_MiB_present / 1024, a->size_MiB_max / 1024, _("GiB"));
        else
            size_str = g_strdup_printf("%"PRId64" / %"PRId64" %s", a->size_MiB_present, a->size_MiB_max, _("MiB"));
        ret = h_strdup_cprintf("[%s %s]\n"
                        "%s=0x%x\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%d / %d\n",
                        ret,
                        _("Memory Array"), a->locator ? a->locator : ".",
                        _("Array DMI Handle"), a->array_handle,
                        _("Use"), UNKIFNULL2(a->use),
                        _("Error Correction Type"), UNKIFNULL2(a->ecc),
                        _("Size (Present / Max)"), size_str,
                        _("Devices (Populated / Sockets)"), a->devs_populated, a->devs
                        );
        g_free(size_str);
    }

    /* Sockets */
    for(l = mem->sockets; l; l = l->next) {
        dmi_mem_socket *s = (dmi_mem_socket*)l->data;

        if (s->populated) {
            gchar *vendor_str = NULL;
            if (s->vendor) {
                if (s->vendor->url)
                    vendor_str = g_strdup_printf(" (%s, %s)",
                        s->vendor->name, s->vendor->url );
            }
            gchar *size_str = NULL;
            if (!s->size_str)
                size_str = g_strdup(_("(Unknown)"));
            else if (!s->size_MiB)
                size_str = g_strdup(s->size_str);
            else
                size_str = g_strdup_printf("%ld %s", s->size_MiB, _("MiB") );

            ret = h_strdup_cprintf("[%s %s]\n"
                            "%s=0x%lx, 0x%lx\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n",
                            ret,
                            _("Memory Socket"), s->full_locator,
                            _("DMI Handles (Array, Socket)"), s->array_handle, s->handle,
                            _("Bank Locator"), UNKIFNULL2(s->bank_locator),
                            _("Form Factor"), UNKIFNULL2(s->form_factor),
                            _("Manufacturer"), UNKIFNULL2(s->mfgr), vendor_str ? vendor_str : "",
                            _("Part Number"), UNKIFNULL2(s->partno),
                            _("Type"), UNKIFNULL2(s->type), UNKIFNULL2(s->type_detail),
                            _("Size"), size_str,
                            _("Rated Speed"), UNKIFNULL2(s->speed_str),
                            _("Configured Speed"), UNKIFNULL2(s->configured_clock_str),
                            _("Data Width/Total Width"), UNKIFNULL2(s->data_width), UNKIFNULL2(s->total_width),
                            _("Minimum Voltage"), UNKIFNULL2(s->voltage_min_str),
                            _("Maximum Voltage"), UNKIFNULL2(s->voltage_max_str),
                            _("Configured Voltage"), UNKIFNULL2(s->voltage_conf_str)
                            );

            g_free(vendor_str);
            g_free(size_str);
        } else {
            ret = h_strdup_cprintf("[%s %s]\n"
                            "%s=0x%x, 0x%x\n"
                            "%s=%s\n"
                            "%s=%s\n",
                            ret,
                            _("Memory Socket"), s->full_locator,
                            _("DMI Handles (Array, Socket)"), s->array_handle, s->handle,
                            _("Bank Locator"), UNKIFNULL2(s->bank_locator),
                            _("Size"), _("(Empty)")
                            );
        }
    }

    no_handles = FALSE;
    if(mem->empty) {
        no_handles = TRUE;
        ret = g_strdup_printf("[%s]\n%s=%s\n" "[$ShellParam$]\nViewType=0\n",
                _("Socket Information"), _("Result"),
                (getuid() == 0)
                ? _("(Not available)")
                : _("(Not available; Perhaps try running HardInfo as root.)") );
    }

    dmi_mem_free(mem);
    return ret;
}

gchar *dmi_mem_socket_info_view1() {
    gchar *ret = g_strdup_printf(
        "[$ShellParam$]\nViewType=1\n"
        "ColumnTitle$TextValue=%s\n" /* Locator */
        "ColumnTitle$Extra1=%s\n"  /* Size */
        "ColumnTitle$Extra2=%s\n"  /* Manufacturer */
        "ColumnTitle$Value=%s\n"     /* Part */
        "ShowColumnHeaders=true\n"
        "[%s]\n",
        _("Locator"),
        _("Size"),
        _("Manufacturer"),
        _("Part"),
        _("Memory DMI")
        );
    GSList *l = NULL;
    gchar key_prefix[] = "DEV";

    dmi_mem *mem = dmi_mem_new();

    /* Arrays */
    for(l = mem->arrays; l; l = l->next) {
        dmi_mem_array *a = (dmi_mem_array*)l->data;
        gchar *key = g_strdup_printf("%s", a->locator);
        gchar *size_str = NULL;

        if (a->size_MiB_max > 1024 && (a->size_MiB_max % 1024 == 0)
            && a->size_MiB_present > 1024 && (a->size_MiB_present % 1024 == 0) )
            size_str = g_strdup_printf("%"PRId64" / %"PRId64" %s", a->size_MiB_present / 1024, a->size_MiB_max / 1024, _("GiB"));
        else
            size_str = g_strdup_printf("%"PRId64" / %"PRId64" %s", a->size_MiB_present, a->size_MiB_max, _("MiB"));
        gchar *details = g_strdup_printf("[%s]\n"
                        "%s=0x%lx\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%d / %d\n",
                        _("Memory Array"),
                        _("DMI Handle"), a->array_handle,
                        _("Locator"), a->locator ? a->locator : ".",
                        _("Use"), UNKIFNULL2(a->use),
                        _("Error Correction Type"), UNKIFNULL2(a->ecc),
                        _("Size (Present / Max)"), size_str,
                        _("Devices (Populated / Sockets)"), a->devs_populated, a->devs
                        );
        moreinfo_add_with_prefix(key_prefix, key, details); /* moreinfo now owns *details */
        ret = h_strdup_cprintf("$!%s$%s=|%s\n",
                ret,
                key, a->locator, size_str
                );
        g_free(size_str);
        g_free(key);
    }

    /* Sockets */
    for(l = mem->sockets; l; l = l->next) {
        dmi_mem_socket *s = (dmi_mem_socket*)l->data;
        gchar *key = g_strdup_printf("%s", s->full_locator);

        if (s->populated) {
            gchar *vendor_str = NULL;
            if (s->vendor) {
                if (s->vendor->url)
                    vendor_str = g_strdup_printf(" (%s, %s)",
                        s->vendor->name, s->vendor->url );
            }
            gchar *size_str = NULL;
            if (!s->size_str)
                size_str = g_strdup(_("(Unknown)"));
            else if (!s->size_MiB)
                size_str = g_strdup(s->size_str);
            else
                size_str = g_strdup_printf("%ld %s", s->size_MiB, _("MiB") );

            gchar *details = g_strdup_printf("[%s]\n"
                            "%s=0x%lx, 0x%lx\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n",
                            _("Memory Socket"),
                            _("DMI Handles (Array, Socket)"), s->array_handle, s->handle,
                            _("Locator"), s->full_locator,
                            _("Bank Locator"), UNKIFNULL2(s->bank_locator),
                            _("Form Factor"), UNKIFNULL2(s->form_factor),
                            _("Manufacturer"), UNKIFNULL2(s->mfgr), vendor_str ? vendor_str : "",
                            _("Part Number"), UNKIFNULL2(s->partno),
                            _("Type"), UNKIFNULL2(s->type), UNKIFNULL2(s->type_detail),
                            _("Size"), size_str,
                            _("Rated Speed"), UNKIFNULL2(s->speed_str),
                            _("Configured Speed"), UNKIFNULL2(s->configured_clock_str),
                            _("Data Width/Total Width"), UNKIFNULL2(s->data_width), UNKIFNULL2(s->total_width),
                            _("Minimum Voltage"), UNKIFNULL2(s->voltage_min_str),
                            _("Maximum Voltage"), UNKIFNULL2(s->voltage_max_str),
                            _("Configured Voltage"), UNKIFNULL2(s->voltage_conf_str)
                            );
            moreinfo_add_with_prefix(key_prefix, key, details); /* moreinfo now owns *details */
            ret = h_strdup_cprintf("$!%s$%s=%s|%s|%s\n",
                    ret,
                    key, s->full_locator, UNKIFNULL2(s->partno), size_str, UNKIFNULL2(s->mfgr)
                    );
            g_free(vendor_str);
            g_free(size_str);
        } else {
            gchar *details = g_strdup_printf("[%s]\n"
                            "%s=0x%lx, 0x%lx\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n",
                            _("Memory Socket"),
                            _("DMI Handles (Array, Socket)"), s->array_handle, s->handle,
                            _("Locator"), s->full_locator,
                            _("Bank Locator"), UNKIFNULL2(s->bank_locator),
                            _("Size"), _("(Empty)")
                            );
            moreinfo_add_with_prefix(key_prefix, key, details); /* moreinfo now owns *details */
            ret = h_strdup_cprintf("$%s$%s=|%s\n",
                    ret,
                    key, s->full_locator, _("(Empty)")
                    );
        }
        g_free(key);
    }

    no_handles = FALSE;
    if(mem->empty) {
        no_handles = TRUE;
        ret = g_strdup_printf("[%s]\n%s=%s\n" "[$ShellParam$]\nViewType=0\n",
                _("Socket Information"), _("Result"),
                (getuid() == 0)
                ? _("(Not available)")
                : _("(Not available; Perhaps try running HardInfo as root.)") );
    }

    dmi_mem_free(mem);
    return ret;
}

gchar *dmi_mem_socket_info() {
    // alternative shell view types
    //return dmi_mem_socket_info_view0();
    return dmi_mem_socket_info_view1();
}

gboolean dmi_mem_show_hinote(const char **msg) {
    if (no_handles) {
        if (getuid() == 0) {
            *msg = g_strdup(
                _("To view DMI memory information the <b><i>dmidecode</i></b> utility must be\n"
                  "available."));
        } else {
            *msg = g_strdup(
                _("To view DMI memory information the <b><i>dmidecode</i></b> utility must be\n"
                  "available, and HardInfo must be run with superuser privileges."));
        }
        return TRUE;
    }
    return FALSE;
}
