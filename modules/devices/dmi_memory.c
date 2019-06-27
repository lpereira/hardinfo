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

#include "dt_util.h" /* for appf() */

#include "spd-decode2.c"

gboolean no_handles = FALSE;
gboolean sketchy_info = FALSE;

/* strings from dmidecode */
static const char empty_mem_str[] = "No Module Installed";
static const char unknown_mfgr_str[] = "<BAD INDEX>";
static const char mobo_location[] = "System Board Or Motherboard";
static const char mobo_shorter[] = "Mainboard";
static const unsigned long dta = 16; /* array */
static const unsigned long dtm = 17; /* socket */

#define UNKIFNULL2(f) ((f) ? f : _("(Unknown)"))
#define UNKIFEMPTY2(f) ((*f) ? f : _("(Unknown)"))
#define SEQ(s,m) (g_strcmp0(s, m) == 0)

const char *problem_marker() {
    static const char as_markup[] = "<big><b>\u26A0</b></big>";
    static const char as_text[] = "(!)";
    if (params.markup_ok)
        return as_markup;
    else
        return as_text;
}

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
    spd_data *spd;
} dmi_mem_socket;

typedef struct {
    gboolean empty;
    GSList *arrays;
    GSList *sockets;
    GSList *spd;
} dmi_mem;

gboolean null_if_empty(gchar **str) {
    if (str && *str) {
        gchar *p = *str;
        while(p && *p) {
            if (isalnum(*p))
                return FALSE;
            p++;
        }
        *str = NULL;
    }
    return TRUE;
}

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

#define STR_IGNORE(str, ignore) if (SEQ(str, ignore)) { *str = 0; null_if_empty(&str); }

        s->form_factor = dmidecode_match("Form Factor", &dtm, &h);
        s->type = dmidecode_match("Type", &dtm, &h);
        STR_IGNORE(s->type, "Unknown");
        s->type_detail = dmidecode_match("Type Detail", &dtm, &h);
        STR_IGNORE(s->type_detail, "None");

        s->speed_str = dmidecode_match("Speed", &dtm, &h);
        s->configured_clock_str = dmidecode_match("Configured Clock Speed", &dtm, &h);
        if (!s->configured_clock_str)
            s->configured_clock_str = dmidecode_match("Configured Memory Speed", &dtm, &h);

        s->voltage_min_str = dmidecode_match("Minimum Voltage", &dtm, &h);
        s->voltage_max_str = dmidecode_match("Maximum Voltage", &dtm, &h);
        s->voltage_conf_str = dmidecode_match("Configured Voltage", &dtm, &h);
        STR_IGNORE(s->voltage_min_str, "Unknown");
        STR_IGNORE(s->voltage_max_str, "Unknown");
        STR_IGNORE(s->voltage_conf_str, "Unknown");

        s->partno = dmidecode_match("Part Number", &dtm, &h);

        s->data_width = dmidecode_match("Data Width", &dtm, &h);
        s->total_width = dmidecode_match("Total Width", &dtm, &h);

        s->mfgr = dmidecode_match("Manufacturer", &dtm, &h);
        if (g_str_has_prefix(s->mfgr, unknown_mfgr_str)) {
            /* the manufacturer code is unknown to dmidecode */
            g_free(s->mfgr);
            s->mfgr = NULL;
        }
        null_if_empty(&s->mfgr);
        null_if_empty(&s->partno);

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

    m->spd = spd_scan();

    if (!m->sockets && !m->arrays && !m->spd) {
        m->empty = 1;
        return m;
    }

    GSList *l = NULL, *l2 = NULL;
    for(l = m->sockets; l; l = l->next) {
        dmi_mem_socket *s = (dmi_mem_socket*)l->data;

        /* update array present devices/size */
        dmi_mem_array *a = dmi_mem_find_array(m, s->array_handle);
        if (a) {
            a->size_MiB_present += s->size_MiB;
            if (s->populated)
                a->devs_populated++;
        }

        if (!s->populated) continue;
        if (!m->spd) continue;

        /* match SPD */
        spd_data *best = NULL;
        int best_score = 0;
        for(l2 = m->spd; l2; l2 = l2->next) {
            spd_data *e = (spd_data*)l2->data;
            int score = 0;
            if (!e->claimed_by_dmi) {
                if (SEQ(s->partno, e->partno))
                    score += 20;
                if (s->size_MiB == e->size_MiB)
                    score += 10;
                if (s->vendor == e->vendor)
                    score += 5;

                if (score > best_score)
                    best = e;
            }
        }
        if (best) {
            s->spd = best;
            best->claimed_by_dmi = 1;

            /* fill in missing from SPD */
            if (!s->mfgr && s->spd->vendor_str) {
                s->mfgr = g_strdup(s->spd->vendor_str);
                s->vendor = s->spd->vendor;
            }

            if (!s->partno && s->spd->partno)
                s->partno = g_strdup(s->spd->partno);

            if (!s->form_factor && s->spd->form_factor)
                s->form_factor = g_strdup(s->spd->form_factor);

            if (!s->type_detail && s->spd->type_detail)
                s->type_detail = g_strdup(s->spd->type_detail);
        }
    }

    return m;
}

void dmi_mem_free(dmi_mem* s) {
    if (s) {
        g_slist_free_full(s->arrays, (GDestroyNotify)dmi_mem_array_free);
        g_slist_free_full(s->sockets, (GDestroyNotify)dmi_mem_socket_free);
        g_slist_free_full(s->spd, (GDestroyNotify)spd_data_free);
        g_free(s);
    }
}

gchar *make_spd_section(spd_data *spd) {
    gchar *ret = NULL;
    if (spd) {
        gchar *full_spd = NULL;
        switch(spd->type) {
            case SDR_SDRAM:
                full_spd = decode_sdr_sdram_extra(spd->bytes);
                break;
            case DDR_SDRAM:
                full_spd = decode_ddr_sdram_extra(spd->bytes);
                break;
            case DDR2_SDRAM:
                full_spd = decode_ddr2_sdram_extra(spd->bytes);
                break;
            case DDR3_SDRAM:
                full_spd = decode_ddr3_sdram_extra(spd->bytes);
                break;
            case DDR4_SDRAM:
                full_spd = decode_ddr4_sdram_extra(spd->bytes, spd->spd_size);
                break;
            default:
                DEBUG("blug for type: %d %s\n", spd->type, ram_types[spd->type]);
        }
        gchar *vendor_str = NULL;
        if (spd->vendor) {
            if (spd->vendor->url)
                vendor_str = g_strdup_printf(" (%s, %s)",
                    spd->vendor->name, spd->vendor->url );
        }
        gchar *size_str = NULL;
        if (!spd->size_MiB)
            size_str = g_strdup(_("(Unknown)"));
        else
            size_str = g_strdup_printf("%d %s", spd->size_MiB, _("MiB") );

        ret = g_strdup_printf("[%s]\n"
                    "%s=%s (%s)%s\n"
                    "%s=%d.%d\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "%s=%s%s\n" /* vendor */
                    "%s=%s\n" /* part */
                    "%s=%s\n" /* size */
                    "%s",
                    _("Serial Presence Detect (SPD)"),
                    _("Source"), spd->dev, spd->spd_driver ? "ee1004" : "eeprom",
                        (spd->type == DDR4_SDRAM && !spd->spd_driver) ? problem_marker() : "",
                    _("SPD Revision"), spd->spd_rev_major, spd->spd_rev_minor,
                    _("Form Factor"), UNKIFNULL2(spd->form_factor),
                    _("Type"), UNKIFEMPTY2(spd->type_detail),
                    _("Vendor"), UNKIFNULL2(spd->vendor_str), vendor_str ? vendor_str : "",
                    _("Part Number"), UNKIFEMPTY2(spd->partno),
                    _("Size"), size_str,
                    full_spd ? full_spd : ""
                    );
        g_free(full_spd);
        g_free(vendor_str);
        g_free(size_str);
    }
    return ret;
}

gchar *dmi_mem_socket_info() {
    gchar *ret = g_strdup_printf(
        "[$ShellParam$]\nViewType=1\n"
        "ColumnTitle$TextValue=%s\n" /* Locator */
        "ColumnTitle$Extra1=%s\n"  /* Size */
        "ColumnTitle$Extra2=%s\n"  /* Vendor */
        "ColumnTitle$Value=%s\n"     /* Part */
        "ShowColumnHeaders=true\n"
        "[%s]\n",
        _("Locator"),
        _("Size"),
        _("Vendor"),
        _("Part"),
        _("Memory Device List")
        );
    GSList *l = NULL;
    sketchy_info = FALSE;
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

        if (a->size_MiB_max < a->size_MiB_present) {
            sketchy_info = TRUE;
            size_str = h_strdup_cprintf(" %s", size_str, problem_marker());
        }

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

            gchar *spd = s->spd ? make_spd_section(s->spd) : NULL;

            gchar *details = g_strdup_printf("[%s]\n"
                            "%s=0x%lx, 0x%lx\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s / %s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s", /* spd */
                            _("Memory Socket"),
                            _("DMI Handles (Array, Socket)"), s->array_handle, s->handle,
                            _("Locator"), s->full_locator,
                            _("Bank Locator"), UNKIFNULL2(s->bank_locator),
                            _("Form Factor"), UNKIFNULL2(s->form_factor),
                            _("Type"), UNKIFNULL2(s->type), UNKIFNULL2(s->type_detail),
                            _("Vendor"), UNKIFNULL2(s->mfgr), vendor_str ? vendor_str : "",
                            _("Part Number"), UNKIFNULL2(s->partno),
                            _("Size"), size_str,
                            _("Rated Speed"), UNKIFNULL2(s->speed_str),
                            _("Configured Speed"), UNKIFNULL2(s->configured_clock_str),
                            _("Data Width/Total Width"), UNKIFNULL2(s->data_width), UNKIFNULL2(s->total_width),
                            _("Minimum Voltage"), UNKIFNULL2(s->voltage_min_str),
                            _("Maximum Voltage"), UNKIFNULL2(s->voltage_max_str),
                            _("Configured Voltage"), UNKIFNULL2(s->voltage_conf_str),
                            spd ? spd : ""
                            );
            g_free(spd);
            moreinfo_add_with_prefix(key_prefix, key, details); /* moreinfo now owns *details */
            const gchar *mfgr = s->mfgr ? vendor_get_shortest_name(s->mfgr) : NULL;
            ret = h_strdup_cprintf("$!%s$%s=%s|%s|%s\n",
                    ret,
                    key, s->full_locator, UNKIFNULL2(s->partno), size_str, UNKIFNULL2(mfgr)
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

    /* Unmatched SPD */
    for(l = mem->spd; l; l = l->next) {
        spd_data *s = (spd_data*)l->data;
        if (s->claimed_by_dmi) continue;
        gchar *key = g_strdup_printf("SPD:%s", s->dev);
        gchar *vendor_str = NULL;
        if (s->vendor) {
            if (s->vendor->url)
                vendor_str = g_strdup_printf(" (%s, %s)",
                    s->vendor->name, s->vendor->url );
        }
        gchar *size_str = NULL;
        if (!s->size_MiB)
            size_str = g_strdup(_("(Unknown)"));
        else
            size_str = g_strdup_printf("%d %s", s->size_MiB, _("MiB") );

        gchar *details = make_spd_section(s);

        moreinfo_add_with_prefix(key_prefix, key, details); /* moreinfo now owns *details */
        const gchar *mfgr = s->vendor_str ? vendor_get_shortest_name(s->vendor_str) : NULL;
        ret = h_strdup_cprintf("$!%s$%s%s=%s|%s|%s\n",
                ret,
                key, key, problem_marker(), UNKIFNULL2(s->partno), size_str, UNKIFNULL2(mfgr)
                );
        g_free(vendor_str);
        g_free(size_str);
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

static gchar *note_state = NULL;

gboolean dmi_mem_show_hinote(const char **msg) {

    gchar *want_dmi    = _(" <b><i>dmidecode</i></b> utility available\n");
    gchar *want_root   = _(" ... <i>and</i> HardInfo running with superuser privileges\n");
    gchar *want_eeprom = _(" <b><i>eeprom</i></b> module loaded (for SDR, DDR, DDR2, DDR3)\n");
    gchar *want_ee1004 = _(" ... <i>or</i> <b><i>ee1004</i></b> module loaded <b>and configured!</b> (for DDR4)");

    gboolean has_root = (getuid() == 0);
    gboolean has_dmi = !no_handles;
    gboolean has_eeprom = g_file_test("/sys/bus/i2c/drivers/eeprom", G_FILE_TEST_IS_DIR);
    gboolean has_ee1004 = g_file_test("/sys/bus/i2c/drivers/ee1004", G_FILE_TEST_IS_DIR);

    char *bullet_yes = "<big><b>\u2713</b></big>";
    char *bullet_no = "<big><b>\u2022<tt> </tt></b></big>";

    g_free(note_state);
    note_state = g_strdup(_("Memory information requires <b>one or both</b> of the following:\n"));
    note_state = appf(note_state, "<tt>1. </tt>%s%s", has_dmi ? bullet_yes : bullet_no, want_dmi);
    note_state = appf(note_state, "<tt>   </tt>%s%s", has_root ? bullet_yes : bullet_no, want_root);
    note_state = appf(note_state, "<tt>2. </tt>%s%s", has_eeprom ? bullet_yes : bullet_no, want_eeprom);
    note_state = appf(note_state, "<tt>   </tt>%s%s", has_ee1004 ? bullet_yes : bullet_no, want_ee1004);

    gboolean best_state = FALSE;
    if (has_dmi && has_root &&
        ((has_eeprom && !spd_ddr4_partial_data)
        || has_ee1004) )
        best_state = TRUE;

    if (!best_state) {
        *msg = note_state;
        return TRUE;
    }

    if (sketchy_info) {
        *msg = g_strdup(
            _("\"More often than not, information contained in the DMI tables is inaccurate,\n"
              "incomplete or simply wrong.\" -<i><b>dmidecode</b></i> manual page"));
        return TRUE;
    }

    return FALSE;
}
