/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>

#include "nice_name.h"
#include "util_sysobj.h"

/* export */
/* replaces the extra chars with spaces, then when done with a series of
 * str_shorten()s, use util_compress_space() to squeeze. */
gboolean str_shorten(gchar *str, const gchar *find, const gchar *replace) {
    if (!str || !find || !replace) return FALSE;
    long unsigned lf = strlen(find);
    long unsigned lr = strlen(replace);
    gchar *p = strstr(str, find);
    if (p) {
        if (lr > lf) lr = lf;
        gchar *buff = g_strnfill(lf, ' ');
        strncpy(buff, replace, lr);
        strncpy(p, buff, lf);
        g_free(buff);
        return TRUE;
    }
    return FALSE;
}

gboolean str_shorten_anycase(gchar *str, const gchar *find, const gchar *replace) {
    if (!str || !find || !replace) return FALSE;
    long unsigned lf = strlen(find);
    long unsigned lr = strlen(replace);
    gchar *p = strcasestr(str, find);
    if (p) {
        if (lr > lf) lr = lf;
        gchar *buff = g_strnfill(lf, ' ');
        strncpy(buff, replace, lr);
        strncpy(p, buff, lf);
        g_free(buff);
        return TRUE;
    }
    return FALSE;
}

void nice_name_x86_cpuid_model_string(char *cpuid_model_string) {
    static gboolean move_vendor_to_front = TRUE;
    static gboolean remove_long_core_count = TRUE;
    static gboolean remove_amd_compute_cores = TRUE;
    static gboolean remove_amd_xn_ncore_redundancy = TRUE;
    static gboolean remove_processor_cpu_apu_etc = TRUE;
    static gboolean remove_mhz_ghz = TRUE;
    static gboolean remove_radeon = TRUE;

    if (!cpuid_model_string) return;
    g_strstrip(cpuid_model_string);

    while(str_shorten(cpuid_model_string, "Genuine Intel", "Intel")) {};
    while(str_shorten_anycase(cpuid_model_string, "(R)", "")) {};
    while(str_shorten_anycase(cpuid_model_string, "(TM)", "")) {};
    while(str_shorten_anycase(cpuid_model_string, "(C)", "")) {};
    while(str_shorten(cpuid_model_string, "@", "")) {};

    if (move_vendor_to_front) {
        /* vendor not at the beginning, try to move there.
         * ex: Mobile AMD Sempron(tm) Processor 3600+
         * ex: Dual Core AMD Opteron(tm) Processor 165 */
        char *intel = strstr(cpuid_model_string, "Intel ");
        char *amd = strstr(cpuid_model_string, "AMD ");
        if (amd || intel) {
            if (amd && !intel) {
                if (amd != cpuid_model_string) {
                    int l = amd - cpuid_model_string;
                    memmove(cpuid_model_string+4, cpuid_model_string, l);
                    memcpy(cpuid_model_string, "AMD ", 4);
                }
            } else if (intel && !amd) {
                int l = intel - cpuid_model_string;
                memmove(cpuid_model_string+6, cpuid_model_string, l);
                memcpy(cpuid_model_string, "Intel ", 6);
            }
        }
    }

    if (g_str_has_prefix(cpuid_model_string, "AMD")) {
        while(str_shorten(cpuid_model_string, "Mobile Technology", "Mobile")) {};

        if (remove_radeon) {
            char *radeon = strcasestr(cpuid_model_string, "with Radeon");
            if (!radeon)
                radeon = strcasestr(cpuid_model_string, "Radeon");
            if (radeon) *radeon = 0;
        }

        if (remove_amd_compute_cores) {
            if (strcasestr(cpuid_model_string, " COMPUTE CORES ")) {
                /* ex: AMD FX-9800P RADEON R7, 12 COMPUTE CORES 4C+8G */
                char *comma = strchr(cpuid_model_string, ',');
                if (comma) *comma = 0;
            }
        }

        if (remove_amd_xn_ncore_redundancy) {
            /* remove Xn n-core redundancy */
            if (strstr(cpuid_model_string, "X2")) {
                str_shorten(cpuid_model_string, "Dual Core", "");
                str_shorten(cpuid_model_string, "Dual-Core", "");
            }
            if (strstr(cpuid_model_string, "X3"))
                str_shorten(cpuid_model_string, "Triple-Core", "");
            if (strstr(cpuid_model_string, "X4"))
                str_shorten(cpuid_model_string, "Quad-Core", "");
        }
    }

    if (g_str_has_prefix(cpuid_model_string, "Cyrix")) {
        /* ex: Cyrix MediaGXtm MMXtm Enhanced */
        while(str_shorten(cpuid_model_string, "tm ", "")) {};
    }

    if (remove_processor_cpu_apu_etc) {
        while(str_shorten(cpuid_model_string, " CPU", "")) {};
        while(str_shorten(cpuid_model_string, " APU", "")) {};
        while(str_shorten_anycase(cpuid_model_string, " Integrated Processor", "")) {};
        while(str_shorten_anycase(cpuid_model_string, " Processor", "")) {};
    } else {
        while(str_shorten(cpuid_model_string, " processor", " Processor")) {};
    }

    if (remove_mhz_ghz) {
        /* 1400MHz, 1.6+ GHz, etc */
        char *u = NULL;
        while((u = strcasestr(cpuid_model_string, "GHz"))
            || (u = strcasestr(cpuid_model_string, "MHz")) ) {
            if (u[3] == '+') u[3] = ' ';
            strncpy(u, " ", 3);
            while(isspace(*u)) {u--;}
            while (isdigit(*u) || *u == '.' || *u == '+')
                { *u = ' '; u--;}
        }
    }

    if (remove_long_core_count) {
        /* note: "Intel Core 2 Duo" and "Intel Core 2 Quad"
         * are the marketing names, don't remove those. */
        static char *count_strings[] = {
            "Dual", "Triple", "Quad", "Six",
            "Eight", "Octal", "Twelve", "Sixteen",
            "8", "16", "24", "32", "48", "56", "64",
        };
        char buffer[] = "Enough room for the longest... -Core";
        char *dash = strchr(buffer, '-');
        char *m = NULL;

        unsigned int i = 0;
        for(; i < G_N_ELEMENTS(count_strings); i++) {
            int l = strlen(count_strings[i]);
            m = dash-l;
            memcpy(m, count_strings[i], l);
            *dash = '-';
            while(str_shorten_anycase(cpuid_model_string, m, "")) {};
            *dash = ' ';
            while(str_shorten_anycase(cpuid_model_string, m, "")) {};
        }
    }

    /* finalize */
    util_compress_space(cpuid_model_string);
    g_strstrip(cpuid_model_string);
}

/* Intel Graphics may have very long names,
 * like "Intel Corporation Seventh Generation Something Core Something Something Integrated Graphics Processor Revision Ninety-four" */
void nice_name_intel_gpu_device(char *pci_ids_device_string) {
    while(str_shorten_anycase(pci_ids_device_string, "(R)", "")) {}; /* Intel(R) -> Intel */
    str_shorten(pci_ids_device_string, "Graphics Controller", "Graphics");
    str_shorten(pci_ids_device_string, "Graphics Device", "Graphics");
    str_shorten(pci_ids_device_string, "Generation", "Gen");
    str_shorten(pci_ids_device_string, "Core Processor", "Core");
    str_shorten(pci_ids_device_string, "Atom Processor", "Atom");
    str_shorten(pci_ids_device_string, "Xeon Processor", "Xeon");
    str_shorten(pci_ids_device_string, "Celeron Processor", "Celeron");
    str_shorten(pci_ids_device_string, "Pentium Processor", "Pentium");
    util_compress_space(pci_ids_device_string);
    g_strstrip(pci_ids_device_string);
}
