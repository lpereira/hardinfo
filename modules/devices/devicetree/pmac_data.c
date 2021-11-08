/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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

#include "cpu_util.h" /* for PROC_CPUINFO */

static gchar *ppc_mac_details(void) {
    int i = 0;
    gchar *ret = NULL;
    gchar *platform = NULL;
    gchar *model = NULL;
    gchar *machine = NULL;
    gchar *motherboard = NULL;
    gchar *detected_as = NULL;
    gchar *pmac_flags = NULL;
    gchar *l2_cache = NULL;
    gchar *pmac_gen = NULL;

    FILE *cpuinfo;
    gchar buffer[128];

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
        return NULL;
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);
        if (tmp[1] == NULL) {
            g_strfreev(tmp);
            continue;
        }
        tmp[0] = g_strstrip(tmp[0]);
        tmp[1] = g_strstrip(tmp[1]);
        get_str("platform", platform);
        get_str("model", model);
        get_str("machine", machine);
        get_str("motherboard", motherboard);
        get_str("detected as", detected_as);
        get_str("pmac flags", pmac_flags);
        get_str("L2 cache", l2_cache);
        get_str("pmac-generation", pmac_gen);
    }
    fclose(cpuinfo);

    if (machine == NULL)
        goto pmd_exit;

    UNKIFNULL(platform);
    UNKIFNULL(model);
    UNKIFNULL(motherboard);
    UNKIFNULL(detected_as);
    UNKIFNULL(pmac_flags);
    UNKIFNULL(l2_cache);
    UNKIFNULL(pmac_gen);

    ret = g_strdup_printf("[%s]\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n",
                _("Apple Power Macintosh"),
                _("Platform"), platform,
                _("Model"), model,
                _("Machine"), machine,
                _("Motherboard"), motherboard,
                _("Detected as"), detected_as,
                _("PMAC Flags"), pmac_flags,
                _("L2 Cache"), l2_cache,
                _("PMAC Generation"), pmac_gen );

pmd_exit:
    g_free(platform);
    g_free(model);
    g_free(machine);
    g_free(motherboard);
    g_free(detected_as);
    g_free(pmac_flags);
    g_free(l2_cache);
    g_free(pmac_gen);
    return ret;
}
