/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
 *    This file
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
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
#include "dmi_util.h"

static const char *dmi_type_strings[] = {
    [0] = N_("BIOS Information"),
    [1] = N_("System"),
    [2] = N_("Base Board"),
    [3] = N_("Chassis"),
    [4] = N_("Processor"),
    [5] = N_("Memory Controller"),
    [6] = N_("Memory Module"),
    [7] = N_("Cache"),
    [8] = N_("Port Connector"),
    [9] = N_("System Slots"),
    [10] = N_("On Board Devices"),
    [11] = N_("OEM Strings"),
    [12] = N_("System Configuration Options"),
    [13] = N_("BIOS Language"),
    [14] = N_("Group Associations"),
    [15] = N_("System Event Log"),
    [16] = N_("Physical Memory Array"),
    [17] = N_("Memory Device"),
    [18] = N_("32-bit Memory Error"),
    [19] = N_("Memory Array Mapped Address"),
    [20] = N_("Memory Device Mapped Address"),
    [21] = N_("Built-in Pointing Device"),
    [22] = N_("Portable Battery"),
    [23] = N_("System Reset"),
    [24] = N_("Hardware Security"),
    [25] = N_("System Power Controls"),
    [26] = N_("Voltage Probe"),
    [27] = N_("Cooling Device"),
    [28] = N_("Temperature Probe"),
    [29] = N_("Electrical Current Probe"),
    [30] = N_("Out-of-band Remote Access"),
    [31] = N_("Boot Integrity Services"),
    [32] = N_("System Boot"),
    [33] = N_("64-bit Memory Error"),
    [34] = N_("Management Device"),
    [35] = N_("Management Device Component"),
    [36] = N_("Management Device Threshold Data"),
    [37] = N_("Memory Channel"),
    [38] = N_("IPMI Device"),
    [39] = N_("Power Supply"),
    [40] = N_("Additional Information"),
    [41] = N_("Onboard Device"),
    //127 = End of Table
};

/* frees the string and sets it NULL if it is to be ignored
 * returns -1 if error, 0 if ok, 1 if ignored */
static int ignore_placeholder_strings(gchar **pstr) {
    gchar *chk, *p;
    chk = g_strdup(*pstr);

    if (pstr == NULL || *pstr == NULL)
        return -1;
#define DMI_IGNORE(m) if (strcasecmp(m, *pstr) == 0) { g_free(chk); g_free(*pstr); *pstr = NULL; return 1; }
    DMI_IGNORE("To be filled by O.E.M.");
    DMI_IGNORE("Default String");
    DMI_IGNORE("System Product Name");
    DMI_IGNORE("System Manufacturer");
    DMI_IGNORE("System Version");
    DMI_IGNORE("System Serial Number");
    DMI_IGNORE("Rev X.0x"); /* ASUS board version nonsense */
    DMI_IGNORE("x.x");      /* Gigabyte board version nonsense */
    DMI_IGNORE("NA");
    DMI_IGNORE("SKU");

    /* noticed on an HP x360 with Insyde BIOS */
    DMI_IGNORE("Type2 - Board Asset Tag");
    DMI_IGNORE("Type1ProductConfigId");

    /* Toshiba Laptop with Insyde BIOS */
    DMI_IGNORE("Base Board Version");
    DMI_IGNORE("No Asset Tag");
    DMI_IGNORE("None");
    DMI_IGNORE("Type1Family");
    DMI_IGNORE("123456789");

    /* ASUS socket 775 MB */
    DMI_IGNORE("Asset-1234567890");
    DMI_IGNORE("MB-1234567890");
    DMI_IGNORE("Chassis Serial Number");
    DMI_IGNORE("Chassis Version");
    DMI_IGNORE("Chassis Manufacture");

    /* Zotac version nonsense */
    p = chk;
    while (*p != 0) { *p = 'x'; p++; } /* all X */
    DMI_IGNORE(chk);
    p = chk;
    while (*p != 0) { *p = '0'; p++; } /* all 0 */
    DMI_IGNORE(chk);

    /*... more, I'm sure. */

    g_free(chk);
    return 0;
}

static const char *dmi_sysfs_root(void) {
    char *candidates[] = {
        "/sys/devices/virtual/dmi",
        "/sys/class/dmi",
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

int dmi_str_status(const char *id_str) {
    gchar *str = dmi_get_str_abs(id_str);
    int ret = 1;

    if (!str)
        ret = 0;

    if ( ignore_placeholder_strings(&str) > 0 )
        ret = -1;

    g_free(str);
    return ret;
}

char *dmi_get_str(const char *id_str) {
    gchar *ret = dmi_get_str_abs(id_str);
    /* return NULL on empty */
    if (ret && *ret == 0) {
        g_free(ret);
        ret = NULL;
    }
    ignore_placeholder_strings(&ret);
    return ret;
}

char *dmi_get_str_abs(const char *id_str) {
    static struct {
        char *id;
        char *path;
    } tab_dmi_sysfs[] = {
        /* dmidecode -> sysfs */
        { "bios-release-date", "id/bios_date" },
        { "bios-vendor", "id/bios_vendor" },
        { "bios-version", "id/bios_version" },
        { "baseboard-product-name", "id/board_name" },
        { "baseboard-manufacturer", "id/board_vendor" },
        { "baseboard-version", "id/board_version" },
        { "baseboard-serial-number", "id/board_serial" },
        { "baseboard-asset-tag", "id/board_asset_tag" },
        { "system-product-name", "id/product_name" },
        { "system-manufacturer", "id/sys_vendor" },
        { "system-serial-number", "id/product_serial" },
        { "system-product-family", "id/product_family" },
        { "system-version", "id/product_version" },
        { "system-uuid", "product_uuid" },
        { "system-sku", "id/product_sku" },  /*dmidecode doesn't actually support this one*/
        { "chassis-type", "id/chassis_type" },
        { "chassis-serial-number", "id/chassis_serial" },
        { "chassis-manufacturer", "id/chassis_vendor" },
        { "chassis-version", "id/chassis_version" },
        { "chassis-asset-tag", "id/chassis_asset_tag" },
        { NULL, NULL }
    };
    const gchar *dmi_root = dmi_sysfs_root();
    gchar *ret = NULL;
    gchar full_path[PATH_MAX];
    gboolean spawned;
    gchar *out, *err;

    int i = 0;

    /* try sysfs first */
    if (dmi_root) {
        while (tab_dmi_sysfs[i].id != NULL) {
            if (strcmp(id_str, tab_dmi_sysfs[i].id) == 0) {
                snprintf(full_path, PATH_MAX, "%s/%s", dmi_root, tab_dmi_sysfs[i].path);
                if (g_file_get_contents(full_path, &ret, NULL, NULL) )
                    goto dmi_str_done;
            }
            i++;
        }
    }

    /* try dmidecode, but may require root */
    snprintf(full_path, PATH_MAX, "dmidecode -s %s", id_str);
    spawned = hardinfo_spawn_command_line_sync(full_path,
            &out, &err, &i, NULL);
    if (spawned) {
        if (i == 0)
            ret = out;
        else
            g_free(out);

        g_free(err);
    }

dmi_str_done:
    if (ret != NULL) {
        ret = strend(ret, '\n');
        ret = g_strstrip(ret);
    }
    return ret;
}

char *dmi_chassis_type_str(int chassis_type, gboolean with_val) {
    static const char *types[] = {
        N_("Invalid chassis type (0)"),
        N_("Unknown chassis type"), /* 1 is "Other", but not helpful in HardInfo */
        N_("Unknown chassis type"),
        N_("Desktop"),
        N_("Low-profile Desktop"),
        N_("Pizza Box"),
        N_("Mini Tower"),
        N_("Tower"),
        N_("Portable"),
        N_("Laptop"),
        N_("Notebook"),
        N_("Handheld"),
        N_("Docking Station"),
        N_("All-in-one"),
        N_("Subnotebook"),
        N_("Space-saving"),
        N_("Lunch Box"),
        N_("Main Server Chassis"),
        N_("Expansion Chassis"),
        N_("Sub Chassis"),
        N_("Bus Expansion Chassis"),
        N_("Peripheral Chassis"),
        N_("RAID Chassis"),
        N_("Rack Mount Chassis"),
        N_("Sealed-case PC"),
        N_("Multi-system"),
        N_("CompactPCI"),
        N_("AdvancedTCA"),
        N_("Blade"),
        N_("Blade Enclosing"),
        N_("Tablet"),
        N_("Convertible"),
        N_("Detachable"),
        N_("IoT Gateway"),
        N_("Embedded PC"),
        N_("Mini PC"),
        N_("Stick PC"),
    };

    if (chassis_type <= 0) {
        gchar *chassis = dmi_get_str("chassis-type");
        if (chassis) {
            chassis_type = atoi(chassis);
            g_free(chassis);
        } else
            chassis_type = -1;
    }

    if (chassis_type >= 0 && chassis_type < (int)G_N_ELEMENTS(types)) {
        if (with_val)
            return g_strdup_printf("[%d] %s", chassis_type, _(types[chassis_type]));

        return g_strdup(_(types[chassis_type]));
    }
    return NULL;
}

/* TODO: something better maybe */
static char *dd_cache[128] = {};
void dmidecode_cache_free()
{ int i; for(i = 0; i < 128; i++) g_free(dd_cache[i]); }

char *dmidecode_read(const dmi_type *type) {
    gchar *ret = NULL;
    gchar full_path[PATH_MAX];
    gboolean spawned;
    gchar *out, *err;

    int i = 0;

    if (type) {
        if (dd_cache[*type])
            return g_strdup(dd_cache[*type]);
        snprintf(full_path, PATH_MAX, "dmidecode -t %"PRId32, *type);
    } else {
        if (dd_cache[127])
            return g_strdup(dd_cache[127]);
        snprintf(full_path, PATH_MAX, "dmidecode");
    }

    spawned = hardinfo_spawn_command_line_sync(full_path,
            &out, &err, &i, NULL);
    if (spawned) {
        if (i == 0)
            ret = out;
        else
            g_free(out);

        g_free(err);
    }

    if (ret) {
        if (type)
            dd_cache[*type] = g_strdup(ret);
        else
            dd_cache[127] = g_strdup(ret);
    }

    return ret;
}

dmi_handle_list *dmi_handle_list_add(dmi_handle_list *hl, dmi_handle_ext new_handle_ext) {
    if (new_handle_ext.type < G_N_ELEMENTS(dmi_type_strings) )
        new_handle_ext.type_str = dmi_type_strings[new_handle_ext.type];
    if (!hl) {
        hl = malloc(sizeof(dmi_handle_list));
        hl->count = 1;
        hl->handles = malloc(sizeof(dmi_handle) * hl->count);
        hl->handles_ext = malloc(sizeof(dmi_handle_ext) * hl->count);
    } else {
        hl->count++;
        hl->handles = realloc(hl->handles, sizeof(dmi_handle) * hl->count);
        hl->handles_ext = realloc(hl->handles_ext, sizeof(dmi_handle_ext) * hl->count);
    }
    hl->handles_ext[hl->count - 1] = new_handle_ext;
    hl->handles[hl->count - 1] = new_handle_ext.id;

    return hl;
}

dmi_handle_list *dmidecode_handles(const dmi_type *type) {
    gchar *full = NULL, *p = NULL, *next_nl = NULL;
    dmi_handle_list *hl = NULL;

    // Handle 0x003B, DMI type 9, 17 bytes

    full = dmidecode_read(type);
    if (full) {
        p = full;
        while(next_nl = strchr(p, '\n')) {
            unsigned int ch = 0, ct = 0, cb = 0;
            strend(p, '\n');
            if (sscanf(p, "Handle 0x%X, DMI type %u, %u bytes", &ch, &ct, &cb) > 0) {
                if (type && !ct) ct = *type;
                hl = dmi_handle_list_add(hl, (dmi_handle_ext){.id = ch, .type = ct, .size = cb});
            }
            p = next_nl + 1;
        }
        free(full);
    }
    return hl;
}

void dmi_handle_list_free(dmi_handle_list *hl) {
    if (hl) {
        free(hl->handles);
        free(hl->handles_ext);
    }
    free(hl);
}

char *dmidecode_match(const char *name, const dmi_type *type, const dmi_handle *handle) {
    gchar *ret = NULL, *full = NULL, *p = NULL, *next_nl = NULL;
    unsigned int ch = 0;
    int ln = 0;

    if (!name) return NULL;
    ln = strlen(name);

    full = dmidecode_read(type);
    if (full) {
        p = full;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            if (!(sscanf(p, "Handle 0x%X", &ch) > 0) ) {
                if (!handle || *handle == ch) {
                    while(*p == '\t') p++;
                    if (strncmp(p, name, ln) == 0) {
                        if (*(p + ln) == ':') {
                            p = p + ln + 1;
                            while(*p == ' ') p++;
                            ret = strdup(p);
                            break;
                        }
                    }
                }
            }
            p = next_nl + 1;
        }
        free(full);
    }

    return ret;
}

dmi_handle_list *dmidecode_match_value(const char *name, const char *value, const dmi_type *type) {
    dmi_handle_list *hl = NULL;
    gchar *full = NULL, *p = NULL, *next_nl = NULL;
    unsigned int ch = 0, ct = 0, cb = 0;
    int ln = 0, lnv = 0;

    if (!name) return NULL;
    ln = strlen(name);
    lnv = (value) ? strlen(value) : 0;

    full = dmidecode_read(type);
    if (full) {
        p = full;
        while(next_nl = strchr(p, '\n')) {
            strend(p, '\n');
            if (!(sscanf(p, "Handle 0x%X, DMI type %u, %u bytes", &ch, &ct, &cb) > 0)) {
                while(*p == '\t') p++;
                if (strncmp(p, name, ln) == 0) {
                    if (*(p + ln) == ':') {
                        p = p + ln + 1;
                        while(*p == ' ') p++;
                        if (!value || strncmp(p, value, lnv) == 0)
                            hl = dmi_handle_list_add(hl, (dmi_handle_ext){.id = ch, .type = ct, .size = cb});
                    }
                }
            }
            p = next_nl + 1;
        }
        free(full);
    }

    return hl;
}
