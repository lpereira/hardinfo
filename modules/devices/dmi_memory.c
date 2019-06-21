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

gboolean no_handles = FALSE;

gchar *dmi_mem_socket_info() {
    static const char empty_mem_str[] = "No Module Installed";
    gchar *ret = strdup("");
    unsigned long dtm = 17, i;
    dmi_handle_list *hlm = dmidecode_handles(&dtm);
    if (!hlm) {
        no_handles = TRUE;
        ret = g_strdup_printf("[%s]\n%s=%s\n",
                _("Socket Information"), _("Result"),
                (getuid() == 0)
                ? _("(Not available)")
                : _("(Not available; Perhaps try running HardInfo as root.)") );
    } else {
        no_handles = FALSE;
        if (hlm) {
            for(i = 0; i < hlm->count; i++) {
                unsigned long h = hlm->handles[i];
                gchar *locator = dmidecode_match("Locator", &dtm, &h);
                gchar *size_str = dmidecode_match("Size", &dtm, &h);

                if (strcmp(size_str, empty_mem_str) == 0) {
                    ret = h_strdup_cprintf("[%s (%lu) %s]\n"
                                    "%s=0x%x\n"
                                    "%s=%s\n",
                                    ret,
                                    _("Memory Socket"), i, locator,
                                    _("DMI Handle"), h,
                                    _("Size"), _("(Empty)") );
                } else {
                    gchar *form_factor = dmidecode_match("Form Factor", &dtm, &h);
                    gchar *type = dmidecode_match("Type", &dtm, &h);
                    gchar *type_detail = dmidecode_match("Type Detail", &dtm, &h);
                    gchar *speed_str = dmidecode_match("Speed", &dtm, &h);
                    gchar *configured_clock_str = dmidecode_match("Configured Clock Speed", &dtm, &h);
                    gchar *voltage_min_str = dmidecode_match("Minimum Voltage", &dtm, &h);
                    gchar *voltage_max_str = dmidecode_match("Maximum Voltage", &dtm, &h);
                    gchar *voltage_conf_str = dmidecode_match("Configured Voltage", &dtm, &h);
                    gchar *mfgr = dmidecode_match("Manufacturer", &dtm, &h);
                    gchar *partno = dmidecode_match("Part Number", &dtm, &h);

                    gchar *vendor_str = NULL;
                    if (mfgr) {
                        const gchar *v_url = vendor_get_url(mfgr);
                        if (v_url)
                            vendor_str = g_strdup_printf(" (%s, %s)",
                                vendor_get_name(mfgr), v_url );
                        else
                            vendor_str = g_strdup("");
                    }

#define UNKIFNULL2(f) ((f) ? f : _("(Unknown)"))

                    ret = h_strdup_cprintf("[%s (%lu) %s]\n"
                                    "%s=0x%x\n"
                                    "%s=%s\n"
                                    "%s=%s%s\n"
                                    "%s=%s\n"
                                    "%s=%s / %s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n",
                                    ret,
                                    _("Memory Socket"), i, locator,
                                    _("DMI Handle"), h,
                                    _("Form Factor"), UNKIFNULL2(form_factor),
                                    _("Manufacturer"), UNKIFNULL2(mfgr), vendor_str,
                                    _("Part Number"), UNKIFNULL2(partno),
                                    _("Type"), UNKIFNULL2(type), UNKIFNULL2(type_detail),
                                    _("Size"), UNKIFNULL2(size_str),
                                    _("Speed"), UNKIFNULL2(speed_str),
                                    _("Configured Clock Frequency"), UNKIFNULL2(configured_clock_str),
                                    _("Minimum Voltage"), UNKIFNULL2(voltage_min_str),
                                    _("Maximum Voltage"), UNKIFNULL2(voltage_max_str),
                                    _("Configured Voltage"), UNKIFNULL2(voltage_conf_str)
                                    );
                    g_free(type);
                    g_free(form_factor);
                    g_free(speed_str);
                    g_free(configured_clock_str);
                    g_free(voltage_min_str);
                    g_free(voltage_max_str);
                    g_free(voltage_conf_str);
                    g_free(mfgr);
                    g_free(partno);
                    g_free(vendor_str);
                }
                g_free(size_str);
                g_free(locator);
            }
            dmi_handle_list_free(hlm);
        }
    }

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
