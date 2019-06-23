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

gboolean no_handles = FALSE;

/* strings from dmidecode */
static const char empty_mem_str[] = "No Module Installed";
static const char unknown_mfgr_str[] = "<BAD INDEX>";
static const unsigned long dtm = 17;

#define UNKIFNULL2(f) ((f) ? f : _("(Unknown)"))

typedef struct {
    unsigned long handle;
    gboolean populated;
    gchar *locator;
    gchar *size_str;

    gchar *type;
    gchar *type_detail;
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
    //TODO: gboolean spd_matched;
} dmi_mem;

dmi_mem *dmi_mem_new(unsigned long h) {
    dmi_mem *s = g_new0(dmi_mem, 1);
    s->handle = h;
    s->locator = dmidecode_match("Locator", &dtm, &h);
    s->size_str = dmidecode_match("Size", &dtm, &h);
    if (!g_str_has_prefix(s->size_str, empty_mem_str)) {
        s->populated = 1;

        s->bank_locator = dmidecode_match("Bank Locator", &dtm, &h);
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

void dmi_mem_free(dmi_mem* s) {
    if (s) {
        g_free(s->locator);
        g_free(s->size_str);
        g_free(s->type);
        g_free(s->type_detail);
        g_free(s->bank_locator);
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

GSList *get_dmi_mem_list() {
    GSList *ret = NULL;
    dmi_handle_list *hlm = dmidecode_handles(&dtm);
    if (hlm) {
        unsigned long i = 0;
        for(i = 0; i < hlm->count; i++) {
            unsigned long h = hlm->handles[i];
            ret = g_slist_append(ret, dmi_mem_new(h));
        }
    }
    return ret;
}

gchar *dmi_mem_socket_info() {
    unsigned long i = 0;
    gchar *ret = strdup("");
    GSList *mems = get_dmi_mem_list();
    GSList *l = mems;
    for(; l; l = l->next) {
        dmi_mem *s = (dmi_mem*)l->data;
        if (s->populated) {
            gchar *vendor_str = NULL;
            if (s->vendor) {
                if (s->vendor->url)
                    vendor_str = g_strdup_printf(" (%s, %s)",
                        s->vendor->name, s->vendor->url );
            }

            ret = h_strdup_cprintf("[%s (%lu) %s]\n"
                            "%s=0x%lx\n"
                            "%s=%s\n"
                            "%s=%s%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n",
                            ret,
                            _("Memory Socket"), i, s->locator,
                            _("DMI Handle"), s->handle,
                            _("Form Factor"), UNKIFNULL2(s->form_factor),
                            _("Manufacturer"), UNKIFNULL2(s->mfgr), vendor_str ? vendor_str : "",
                            _("Part Number"), UNKIFNULL2(s->partno),
                            _("Type"), UNKIFNULL2(s->type), UNKIFNULL2(s->type_detail),
                            _("Size"), UNKIFNULL2(s->size_str),
                            _("Bank Locator"), UNKIFNULL2(s->bank_locator),
                            _("Speed"), UNKIFNULL2(s->speed_str),
                            _("Configured Speed"), UNKIFNULL2(s->configured_clock_str),
                            _("Data Width/Total Width"), UNKIFNULL2(s->data_width), UNKIFNULL2(s->total_width),
                            _("Minimum Voltage"), UNKIFNULL2(s->voltage_min_str),
                            _("Maximum Voltage"), UNKIFNULL2(s->voltage_max_str),
                            _("Configured Voltage"), UNKIFNULL2(s->voltage_conf_str)
                            );

            g_free(vendor_str);
        } else {
            ret = h_strdup_cprintf("[%s (%lu) %s]\n"
                            "%s=0x%x\n"
                            "%s=%s\n",
                            ret,
                            _("Memory Socket"), i, s->locator,
                            _("DMI Handle"), s->handle,
                            _("Size"), _("(Empty)") );
        }
        i++;
    }

    no_handles = FALSE;
    if(!mems) {
        no_handles = TRUE;
        ret = g_strdup_printf("[%s]\n%s=%s\n",
                _("Socket Information"), _("Result"),
                (getuid() == 0)
                ? _("(Not available)")
                : _("(Not available; Perhaps try running HardInfo as root.)") );
    }

    g_slist_free_full(mems, (GDestroyNotify)dmi_mem_free);
    return ret;
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
