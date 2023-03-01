/*
 * spd-decode.c, spd-vendors.c
 * Copyright (c) 2010 L. A. F. Pereira
 * modified by Ondrej Čerman (2019)
 * modified by Burt P. (2019)
 *
 * Based on decode-dimms.pl
 * Copyright 1998, 1999 Philip Edelbrock <phil@netroedge.com>
 * modified by Christian Zuckschwerdt <zany@triq.net>
 * modified by Burkart Lingner <burkart@bollchen.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#include "devices.h"
#include "hardinfo.h"

gboolean spd_no_driver = FALSE;
gboolean spd_no_support = FALSE;
gboolean spd_ddr4_partial_data = FALSE;
int spd_ram_types = 0; /* bits using enum RamType */

typedef enum {
    UNKNOWN           = 0,
    DIRECT_RAMBUS     = 1,
    RAMBUS            = 2,
    FPM_DRAM          = 3,
    EDO               = 4,
    PIPELINED_NIBBLE  = 5,
    SDR_SDRAM         = 6,
    MULTIPLEXED_ROM   = 7,
    DDR_SGRAM         = 8,
    DDR_SDRAM         = 9,
    DDR2_SDRAM        = 10,
    DDR3_SDRAM        = 11,
    DDR4_SDRAM        = 12,
    N_RAM_TYPES       = 13
} RamType;

static const char *ram_types[] = {"Unknown",   "Direct Rambus",    "Rambus",     "FPM DRAM",
                                  "EDO",       "Pipelined Nibble", "SDR SDRAM",  "Multiplexed ROM",
                                  "DDR SGRAM", "DDR SDRAM",        "DDR2 SDRAM", "DDR3 SDRAM",
                                  "DDR4 SDRAM"};
#define GET_RAM_TYPE_STR(rt) (ram_types[(rt < N_RAM_TYPES) ? rt : 0])

#include "spd-vendors.c"

struct dmi_mem_socket;
typedef struct {
    unsigned char bytes[512];
    char dev[32];  /* %1d-%04d\0 */
    const char *spd_driver;
    int spd_size;

    RamType type;

    int vendor_bank;
    int vendor_index;
    const char *vendor_str;
    const Vendor *vendor;

    int dram_vendor_bank;
    int dram_vendor_index;
    const char *dram_vendor_str;
    const Vendor *dram_vendor;

    char partno[32];
    const char *form_factor;
    char type_detail[256];

    dmi_mem_size size_MiB;

    int spd_rev_major; // bytes[1] >> 4
    int spd_rev_minor; // bytes[1] & 0xf

    int week, year;

    gboolean ddr4_no_ee1004;

    struct dmi_mem_socket *dmi_socket;
    int match_score;
} spd_data;

#define spd_data_new() g_new0(spd_data, 1)
void spd_data_free(spd_data *s) { g_free(s); }

/*
 * We consider that no data was written to this area of the SPD EEPROM if
 * all bytes read 0x00 or all bytes read 0xff
 */
static int spd_written(unsigned char *bytes, int len) {
    do {
        if (*bytes == 0x00 || *bytes == 0xFF) return 1;
    } while (--len && bytes++);

    return 0;
}

static int parity(int value) {
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value &= 0xf;

    return (0x6996 >> value) & 1;
}

static void decode_sdr_module_size(unsigned char *bytes, dmi_mem_size *size) {
    int i, k = 0;

    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    if (bytes[5] <= 8 && bytes[17] <= 8) { k = bytes[5] * bytes[17]; }

    if (i > 0 && i <= 12 && k > 0) {
        if (size) { *size = (1 << i) * k; }
    } else {
        if (size) { *size = -1; }
    }
}

static void decode_sdr_module_timings(unsigned char *bytes, float *tcl, float *trcd, float *trp,
                                      float *tras) {
    float cas[3], ctime;
    int i, j;

    for (i = 0, j = 0; j < 7; j++) {
        if (bytes[18] & 1 << j) { cas[i++] = j + 1; }
    }

    ctime = ((bytes[9] >> 4) + (bytes[9] & 0xf)) * 0.1;

    if (trcd) { *trcd = ceil(bytes[29] / ctime); }
    if (trp) { *trp = ceil(bytes[27] / ctime); }
    if (tras) { *tras = ceil(bytes[30] / ctime); }
    if (tcl) { *tcl = cas[i]; }
}

static void decode_sdr_module_row_address_bits(unsigned char *bytes, char **bits) {
    char *temp;

    switch (bytes[3]) {
    case 0: temp = "Undefined"; break;
    case 1: temp = "1/16"; break;
    case 2: temp = "2/27"; break;
    case 3: temp = "3/18"; break;
    default:
        /* printf("%d\n", bytes[3]); */
        temp = NULL;
    }

    if (bits) { *bits = temp; }
}

static void decode_sdr_module_col_address_bits(unsigned char *bytes, char **bits) {
    char *temp;

    switch (bytes[4]) {
    case 0: temp = "Undefined"; break;
    case 1: temp = "1/16"; break;
    case 2: temp = "2/17"; break;
    case 3: temp = "3/18"; break;
    default:
        /*printf("%d\n", bytes[4]); */
        temp = NULL;
    }

    if (bits) { *bits = temp; }
}

static void decode_sdr_module_number_of_rows(unsigned char *bytes, int *rows) {
    if (rows) { *rows = bytes[5]; }
}

static void decode_sdr_module_data_with(unsigned char *bytes, int *width) {
    if (width) {
        if (bytes[7] > 1) {
            *width = 0;
        } else {
            *width = (bytes[7] * 0xff) + bytes[6];
        }
    }
}

static void decode_sdr_module_interface_signal_levels(unsigned char *bytes, char **signal_levels) {
    char *temp;

    switch (bytes[8]) {
    case 0: temp = "5.0 Volt/TTL"; break;
    case 1: temp = "LVTTL"; break;
    case 2: temp = "HSTL 1.5"; break;
    case 3: temp = "SSTL 3.3"; break;
    case 4: temp = "SSTL 2.5"; break;
    case 255: temp = "New Table"; break;
    default: temp = NULL;
    }

    if (signal_levels) { *signal_levels = temp; }
}

static void decode_sdr_module_configuration_type(unsigned char *bytes, char **module_config_type) {
    char *temp;

    switch (bytes[11]) {
    case 0: temp = "No parity"; break;
    case 1: temp = "Parity"; break;
    case 2: temp = "ECC"; break;
    default: temp = NULL;
    }

    if (module_config_type) { *module_config_type = temp; }
}

static void decode_sdr_module_refresh_type(unsigned char *bytes, char **refresh_type) {
    char *temp;

    if (bytes[12] & 0x80) {
        temp = "Self refreshing";
    } else {
        temp = "Not self refreshing";
    }

    if (refresh_type) { *refresh_type = temp; }
}

static void decode_sdr_module_refresh_rate(unsigned char *bytes, char **refresh_rate) {
    char *temp;

    switch (bytes[12] & 0x7f) {
    case 0: temp = "Normal (15.625us)"; break;
    case 1: temp = "Reduced (3.9us)"; break;
    case 2: temp = "Reduced (7.8us)"; break;
    case 3: temp = "Extended (31.3us)"; break;
    case 4: temp = "Extended (62.5us)"; break;
    case 5: temp = "Extended (125us)"; break;
    default: temp = NULL;
    }

    if (refresh_rate) { *refresh_rate = temp; }
}

static void decode_sdr_module_detail(unsigned char *bytes, char *type_detail) {
    bytes = bytes; /* silence unused warning */
    if (type_detail) {
        snprintf(type_detail, 255, "SDR");
    }
}

static gchar *decode_sdr_sdram_extra(unsigned char *bytes) {
    int rows, data_width;
    float tcl, trcd, trp, tras;
    char *row_address_bits, *col_address_bits, *signal_level;
    char *module_config_type, *refresh_type, *refresh_rate;

    decode_sdr_module_timings(bytes, &tcl, &trcd, &trp, &tras);
    decode_sdr_module_row_address_bits(bytes, &row_address_bits);
    decode_sdr_module_col_address_bits(bytes, &col_address_bits);
    decode_sdr_module_number_of_rows(bytes, &rows);
    decode_sdr_module_data_with(bytes, &data_width);
    decode_sdr_module_interface_signal_levels(bytes, &signal_level);
    decode_sdr_module_configuration_type(bytes, &module_config_type);
    decode_sdr_module_refresh_type(bytes, &refresh_type);
    decode_sdr_module_refresh_rate(bytes, &refresh_rate);

    /* TODO:
       - RAS to CAS delay
       - Supported CAS latencies
       - Supported CS latencies
       - Supported WE latencies
       - Cycle Time / Access time
       - SDRAM module attributes
       - SDRAM device attributes
       - Row densities
       - Other misc stuff
     */

    /* expected to continue an [SPD] section */
    return g_strdup_printf("%s=%s\n"
                           "%s=%s\n"
                           "%s=%d\n"
                           "%s=%d bits\n"
                           "%s=%s\n"
                           "%s=%s\n"
                           "%s=%s (%s)\n"
                           "[%s]\n"
                           "tCL=%.2f\n"
                           "tRCD=%.2f\n"
                           "tRP=%.2f\n"
                           "tRAS=%.2f\n",
                           _("Row address bits"), row_address_bits ? row_address_bits : _("(Unknown)"),
                           _("Column address bits"), col_address_bits ? col_address_bits : _("(Unknown)"),
                           _("Number of rows"), rows,
                           _("Data width"), data_width,
                           _("Interface signal levels"), signal_level ? signal_level : _("(Unknown)"),
                           _("Configuration type"), module_config_type ? module_config_type : _("(Unknown)"),
                           _("Refresh"), refresh_type, refresh_rate ? refresh_rate : _("Unknown"),
                           _("Timings"), tcl, trcd, trp, tras);
}

static void decode_ddr_module_speed(unsigned char *bytes, float *ddrclk, int *pcclk) {
    float temp, clk;
    int tbits, pc;

    temp = (bytes[9] >> 4) + (bytes[9] & 0xf) * 0.1;
    clk = 2 * (1000 / temp);
    tbits = (bytes[7] * 256) + bytes[6];

    if (bytes[11] == 2 || bytes[11] == 1) { tbits -= 8; }

    pc = clk * tbits / 8;
    if (pc % 100 > 50) { pc += 100; }
    pc -= pc % 100;

    if (ddrclk) *ddrclk = (int)clk;

    if (pcclk) *pcclk = pc;
}

static void decode_ddr_module_size(unsigned char *bytes, dmi_mem_size *size) {
    int i, k;

    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    k = (bytes[5] <= 8 && bytes[17] <= 8) ? bytes[5] * bytes[17] : 0;

    if (i > 0 && i <= 12 && k > 0) {
        if (size) { *size = (1 << i) * k; }
    } else {
        if (size) { *size = -1; }
    }
}

static void decode_ddr_module_timings(unsigned char *bytes, float *tcl, float *trcd, float *trp,
                                      float *tras) {
    float ctime;
    float highest_cas = 0;
    int i;

    for (i = 0; i < 7; i++) {
        if (bytes[18] & (1 << i)) { highest_cas = 1 + i * 0.5f; }
    }

    ctime = (bytes[9] >> 4) + (bytes[9] & 0xf) * 0.1;

    if (trcd) {
        *trcd = (bytes[29] >> 2) + ((bytes[29] & 3) * 0.25);
        *trcd = ceil(*trcd / ctime);
    }

    if (trp) {
        *trp = (bytes[27] >> 2) + ((bytes[27] & 3) * 0.25);
        *trp = ceil(*trp / ctime);
    }

    if (tras) {
        *tras = bytes[30];
        *tras = ceil(*tras / ctime);
    }

    if (tcl) { *tcl = highest_cas; }
}

static void decode_ddr_module_detail(unsigned char *bytes, char *type_detail) {
    float ddr_clock;
    int pc_speed;
    if (type_detail) {
        decode_ddr_module_speed(bytes, &ddr_clock, &pc_speed);
        snprintf(type_detail, 255, "DDR-%.0f (PC-%d)", ddr_clock, pc_speed);
    }
}

static gchar *decode_ddr_sdram_extra(unsigned char *bytes) {
    float tcl, trcd, trp, tras;

    decode_ddr_module_timings(bytes, &tcl, &trcd, &trp, &tras);

    return g_strdup_printf("[%s]\n"
                           "tCL=%.2f\n"
                           "tRCD=%.2f\n"
                           "tRP=%.2f\n"
                           "tRAS=%.2f\n",
                           _("Timings"), tcl, trcd, trp, tras);
}

static float decode_ddr2_module_ctime(unsigned char byte) {
    float ctime;

    ctime = (byte >> 4);
    byte &= 0xf;

    if (byte <= 9) {
        ctime += byte * 0.1;
    } else if (byte == 10) {
        ctime += 0.25;
    } else if (byte == 11) {
        ctime += 0.33;
    } else if (byte == 12) {
        ctime += 0.66;
    } else if (byte == 13) {
        ctime += 0.75;
    }

    return ctime;
}

static void decode_ddr2_module_speed(unsigned char *bytes, float *ddr_clock, int *pc2_speed) {
    float ctime;
    float ddrclk;
    int tbits, pcclk;
    ctime = decode_ddr2_module_ctime(bytes[9]);
    ddrclk = 2 * (1000 / ctime);

    tbits = (bytes[7] * 256) + bytes[6];
    if (bytes[11] & 0x03) { tbits -= 8; }

    pcclk = ddrclk * tbits / 8;
    pcclk -= pcclk % 100;

    if (ddr_clock) { *ddr_clock = (int)ddrclk; }
    if (pc2_speed) { *pc2_speed = pcclk; }
}

static void decode_ddr2_module_size(unsigned char *bytes, dmi_mem_size *size) {
    int i, k;

    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    k = ((bytes[5] & 0x7) + 1) * bytes[17];

    if (i > 0 && i <= 12 && k > 0) {
        if (size) { *size = ((1 << i) * k); }
    } else {
        if (size) { *size = 0; }
    }
}

static void decode_ddr2_module_type(unsigned char *bytes, const char **type) {
    switch (bytes[20]) {
        case 0x01: *type = "RDIMM (Registered DIMM)"; break;
        case 0x02: *type = "UDIMM (Unbuffered DIMM)"; break;
        case 0x04: *type = "SO-DIMM (Small Outline DIMM)"; break;
        case 0x06: *type = "72b-SO-CDIMM (Small Outline Clocked DIMM, 72-bit data bus)"; break;
        case 0x07: *type = "72b-SO-RDIMM (Small Outline Registered DIMM, 72-bit data bus)"; break;
        case 0x08: *type = "Micro-DIMM"; break;
        case 0x10: *type = "Mini-RDIMM (Mini Registered DIMM)"; break;
        case 0x20: *type = "Mini-UDIMM (Mini Unbuffered DIMM)"; break;
        default: *type = NULL;
    }
}

static void decode_ddr2_module_timings(float ctime, unsigned char *bytes, float *trcd, float *trp, float *tras) {

    if (trcd) { *trcd = ceil(((bytes[29] >> 2) + ((bytes[29] & 3) * 0.25)) / ctime); }

    if (trp) { *trp = ceil(((bytes[27] >> 2) + ((bytes[27] & 3) * 0.25)) / ctime); }

    if (tras) { *tras = ceil(bytes[30] / ctime); }
}

static gboolean decode_ddr2_module_ctime_for_casx(int casx_minus, unsigned char *bytes, float *ctime, float *tcl){
    int highest_cas, i, bytei;
    float ctimev = 0;

    switch (casx_minus){
        case 0:
            bytei = 9;
            break;
        case 1:
            bytei = 23;
            break;
        case 2:
            bytei = 25;
            break;
        default:
            return FALSE;
    }

    for (i = 0; i < 7; i++) {
        if (bytes[18] & (1 << i)) { highest_cas = i; }
    }

    if ((bytes[18] & (1 << (highest_cas-casx_minus))) == 0)
        return FALSE;

    ctimev = decode_ddr2_module_ctime(bytes[bytei]);
    if (ctimev == 0)
        return FALSE;

    if (tcl) { *tcl = highest_cas-casx_minus; }
    if (ctime) { *ctime = ctimev; }

    return TRUE;
}

static void decode_ddr2_module_detail(unsigned char *bytes, char *type_detail) {
    float ddr_clock;
    int pc2_speed;
    if (type_detail) {
        decode_ddr2_module_speed(bytes, &ddr_clock, &pc2_speed);
        snprintf(type_detail, 255, "DDR2-%.0f (PC2-%d)", ddr_clock, pc2_speed);
    }
}

static gchar *decode_ddr2_sdram_extra(unsigned char *bytes) {
    float trcd, trp, tras, ctime, tcl;
    const char* voltage;
    gchar *out;
    int i;

    switch(bytes[8]){
        case 0x0:
            voltage = "TTL/5 V tolerant";
            break;
        case 0x1:
            voltage = "LVTTL";
            break;
        case 0x2:
            voltage = "HSTL 1.5 V";
            break;
        case 0x3:
            voltage = "SSTL 3.3 V";
            break;
        case 0x4:
            voltage = "SSTL 2.5 V";
            break;
        case 0x5:
            voltage = "SSTL 1.8 V";
            break;
        default:
            voltage = _("(Unknown)");
    }

    /* expected to continue an [SPD] section */
    out = g_strdup_printf("%s=%s\n"
                          "[%s]\n",
                          _("Voltage"), voltage,
                          _("JEDEC Timings"));

    for (i = 0; i <= 2; i++) {
        if (!decode_ddr2_module_ctime_for_casx(i, bytes, &ctime, &tcl))
            break;
        decode_ddr2_module_timings(ctime, bytes,  &trcd, &trp, &tras);
        out = h_strdup_cprintf("DDR2-%d=%.0f-%.0f-%.0f-%.0f\n",
                            out,
                           (int)(2 * (1000 / ctime)), tcl, trcd, trp, tras);
    }

    return out;
}

static void decode_ddr3_module_speed(unsigned char *bytes, float *ddr_clock, int *pc3_speed) {
    float ctime;
    float ddrclk;
    int tbits, pcclk;
    float mtb = 0.125;

    if (bytes[10] == 1 && bytes[11] == 8) mtb = 0.125;
    if (bytes[10] == 1 && bytes[11] == 15) mtb = 0.0625;
    ctime = mtb * bytes[12];

    ddrclk = 2 * (1000 / ctime);

    tbits = 64;
    switch (bytes[8]) {
    case 1: tbits = 16; break;
    case 4: tbits = 32; break;
    case 3:
    case 0xb: tbits = 64; break;
    }

    pcclk = ddrclk * tbits / 8;
    pcclk -= pcclk % 100;

    if (ddr_clock) { *ddr_clock = (int)ddrclk; }
    if (pc3_speed) { *pc3_speed = pcclk; }
}

static void decode_ddr3_module_size(unsigned char *bytes, dmi_mem_size *size) {
    unsigned int sdr_capacity = 256 << (bytes[4] & 0xF);
    unsigned int sdr_width = 4 << (bytes[7] & 0x7);
    unsigned int bus_width = 8 << (bytes[8] & 0x7);
    unsigned int ranks = 1 + ((bytes[7] >> 3) & 0x7);

    *size = sdr_capacity / 8 * bus_width / sdr_width * ranks;
}

static void decode_ddr3_module_timings(unsigned char *bytes, float *trcd, float *trp, float *tras,
                                       float *tcl) {
    float ctime;
    float mtb = 0.125;
    float taa;

    if (bytes[10] == 1 && bytes[11] == 8) mtb = 0.125;
    if (bytes[10] == 1 && bytes[11] == 15) mtb = 0.0625;
    ctime = mtb * bytes[12];
    taa = bytes[16] * mtb;

    if (trcd) { *trcd = bytes[18] * mtb; }

    if (trp) { *trp = bytes[20] * mtb; }

    if (tras) { *tras = (bytes[22] + (bytes[21] & 0xf)) * mtb; }

    if (tcl) { *tcl = ceil(taa/ctime); }
}

static void decode_ddr3_module_type(unsigned char *bytes, const char **type) {
    switch (bytes[3]) {
    case 0x01: *type = "RDIMM (Registered Long DIMM)"; break;
    case 0x02: *type = "UDIMM (Unbuffered Long DIMM)"; break;
    case 0x03: *type = "SODIMM (Small Outline DIMM)"; break;
    default: *type = NULL;
    }
}

static void decode_ddr3_module_detail(unsigned char *bytes, char *type_detail) {
    float ddr_clock;
    int pc3_speed;
    if (type_detail) {
        decode_ddr3_module_speed(bytes, &ddr_clock, &pc3_speed);
        snprintf(type_detail, 255, "DDR3-%.0f (PC3-%d)", ddr_clock, pc3_speed);
    }
}

static gchar *decode_ddr3_sdram_extra(unsigned char *bytes) {
    float trcd, trp, tras, tcl;

    decode_ddr3_module_timings(bytes, &trcd, &trp, &tras, &tcl);

    int ranks = 1 + ((bytes[7] >> 3) & 0x7);
    int pins = 4 << (bytes[7] & 0x7);
    int die_count = (bytes[33] >> 4) & 0x7;
    int ts = !!(bytes[32] & 0x80);

    /* expected to continue an [SPD] section */
    return g_strdup_printf("%s=%d\n"
                           "%s=%d\n"
                           "%s=%d %s\n"
                           "%s=[%02x] %s\n"
                           "%s=%s%s%s\n"
                           "%s="
                              "%s%s%s%s%s%s%s%s"
                              "%s%s%s%s%s%s%s\n"
                           "[%s]\n"
                           "tCL=%.0f\n"
                           "tRCD=%.3fns\n"
                           "tRP=%.3fns\n"
                           "tRAS=%.3fns\n",
                           _("Ranks"), ranks,
                           _("IO Pins per Chip"), pins,
                           _("Die count"), die_count, die_count ? "" : _("(Unspecified)"),
                           _("Thermal Sensor"), bytes[32], ts ? _("Present") : _("Not present"),
                           _("Supported Voltages"),
                                (bytes[6] & 4) ? "1.25V " : "",
                                (bytes[6] & 2) ? "1.35V " : "",
                                (bytes[6] & 1) ? "" : "1.5V",
                           _("Supported CAS Latencies"),
                                (bytes[15] & 0x40) ? "18 " : "",
                                (bytes[15] & 0x20) ? "17 " : "",
                                (bytes[15] & 0x10) ? "16 " : "",
                                (bytes[15] & 0x08) ? "15 " : "",
                                (bytes[15] & 0x04) ? "14 " : "",
                                (bytes[15] & 0x02) ? "13 " : "",
                                (bytes[15] & 0x01) ? "12 " : "",
                                (bytes[14] & 0x80) ? "11 " : "",
                                (bytes[14] & 0x40) ? "10 " : "",
                                (bytes[14] & 0x20) ? "9 " : "",
                                (bytes[14] & 0x10) ? "8 " : "",
                                (bytes[14] & 0x08) ? "7 " : "",
                                (bytes[14] & 0x04) ? "6 " : "",
                                (bytes[14] & 0x02) ? "5 " : "",
                                (bytes[14] & 0x01) ? "4" : "",
                           _("Timings"), tcl, trcd, trp, tras
                           );
}

static void decode_ddr3_part_number(unsigned char *bytes, char *part_number) {
    int i;
    if (part_number) {
        for (i = 128; i <= 145; i++) *part_number++ = bytes[i];
        *part_number = '\0';
    }
}

static void decode_ddr34_manufacturer(unsigned char count, unsigned char code, char **manufacturer, int *bank, int *index) {
    if (!manufacturer) return;

    if (code == 0x00 || code == 0xFF) {
        *manufacturer = NULL;
        return;
    }

    if (parity(count) != 1 || parity(code) != 1) {
        *manufacturer = _("Invalid");
        return;
    }

    *bank = count & 0x7f;
    *index = code & 0x7f;
    if (*bank >= VENDORS_BANKS) {
        *manufacturer = NULL;
        return;
    }

    *manufacturer = (char *)JEDEC_MFG_STR(*bank, *index - 1);
}

static void decode_ddr3_manufacturer(unsigned char *bytes, char **manufacturer, int *bank, int *index) {
    decode_ddr34_manufacturer(bytes[117], bytes[118], (char **) manufacturer, bank, index);
}

static void decode_module_manufacturer(unsigned char *bytes, char **manufacturer) {
    char *out = "Unknown";
    unsigned char first;
    int ai = 0;
    int len = 8;

    if (!spd_written(bytes, 8)) {
        out = "Undefined";
        goto end;
    }

    do { ai++; } while ((--len && (*bytes++ == 0x7f)));
    first = *--bytes;

    if (ai == 0) {
        out = "Invalid";
        goto end;
    }

    if (parity(first) != 1) {
        out = "Invalid";
        goto end;
    }

    out = (char*)JEDEC_MFG_STR(ai - 1, (first & 0x7f) - 1);

end:
    if (manufacturer) { *manufacturer = out; }
}

static void decode_module_part_number(unsigned char *bytes, char *part_number) {
    if (part_number) {
        bytes += 8 + 64;

        while (*++bytes && *bytes >= 32 && *bytes < 127) { *part_number++ = *bytes; }
        *part_number = '\0';
    }
}

static char *print_spd_timings(int speed, float cas, float trcd, float trp, float tras,
                               float ctime) {
    return g_strdup_printf("DDR4-%d=%.0f-%.0f-%.0f-%.0f\n", speed, cas, ceil(trcd / ctime - 0.025),
                           ceil(trp / ctime - 0.025), ceil(tras / ctime - 0.025));
}

static void decode_ddr4_module_type(unsigned char *bytes, const char **type) {
    switch (bytes[3]) {
    case 0x01: *type = "RDIMM (Registered DIMM)"; break;
    case 0x02: *type = "UDIMM (Unbuffered DIMM)"; break;
    case 0x03: *type = "SODIMM (Small Outline Unbuffered DIMM)"; break;
    case 0x04: *type = "LRDIMM (Load-Reduced DIMM)"; break;
    case 0x05: *type = "Mini-RDIMM (Mini Registered DIMM)"; break;
    case 0x06: *type = "Mini-UDIMM (Mini Unbuffered DIMM)"; break;
    case 0x08: *type = "72b-SO-RDIMM (Small Outline Registered DIMM, 72-bit data bus)"; break;
    case 0x09: *type = "72b-SO-UDIMM (Small Outline Unbuffered DIMM, 72-bit data bus)"; break;
    case 0x0c: *type = "16b-SO-UDIMM (Small Outline Unbuffered DIMM, 16-bit data bus)"; break;
    case 0x0d: *type = "32b-SO-UDIMM (Small Outline Unbuffered DIMM, 32-bit data bus)"; break;
    default: *type = NULL;
    }
}

static float ddr4_mtb_ftb_calc(unsigned char b1, signed char b2) {
    float mtb = 0.125;
    float ftb = 0.001;
    return b1 * mtb + b2 * ftb;
}

static void decode_ddr4_module_speed(unsigned char *bytes, float *ddr_clock, int *pc4_speed) {
    float ctime;
    float ddrclk;
    int tbits, pcclk;

    ctime = ddr4_mtb_ftb_calc(bytes[18], bytes[125]);
    ddrclk = 2 * (1000 / ctime);
    tbits = 8 << (bytes[13] & 7);

    pcclk = ddrclk * tbits / 8;
    pcclk -= pcclk % 100;

    if (ddr_clock) { *ddr_clock = (int)ddrclk; }
    if (pc4_speed) { *pc4_speed = pcclk; }
}

static void decode_ddr4_module_spd_timings(unsigned char *bytes, float speed, char **str) {
    float ctime, ctime_max, pctime, taa, trcd, trp, tras;
    int pcas, best_cas, base_cas, ci, i, j;
    unsigned char cas_support[] = {bytes[20], bytes[21], bytes[22], bytes[23] & 0x1f};
    float possible_ctimes[] = {15 / 24.0, 15 / 22.0, 15 / 20.0, 15 / 18.0,
                               15 / 16.0, 15 / 14.0, 15 / 12.0};

    base_cas = bytes[23] & 0x80 ? 23 : 7;

    ctime = ddr4_mtb_ftb_calc(bytes[18], bytes[125]);
    ctime_max = ddr4_mtb_ftb_calc(bytes[19], bytes[124]);

    taa = ddr4_mtb_ftb_calc(bytes[24], bytes[123]);
    trcd = ddr4_mtb_ftb_calc(bytes[25], bytes[122]);
    trp = ddr4_mtb_ftb_calc(bytes[26], bytes[121]);
    tras = (((bytes[27] & 0x0f) << 8) + bytes[28]) * 0.125;

    *str = print_spd_timings((int)speed, ceil(taa / ctime - 0.025), trcd, trp, tras, ctime);

    for (ci = 0; ci < 7; ci++) {
        best_cas = 0;
        pctime = possible_ctimes[ci];

        for (i = 3; i >= 0; i--) {
            for (j = 7; j >= 0; j--) {
                if ((cas_support[i] & (1 << j)) != 0) {
                    pcas = base_cas + 8 * i + j;
                    if (ceil(taa / pctime) - 0.025 <= pcas) { best_cas = pcas; }
                }
            }
        }

        if (best_cas > 0 && pctime <= ctime_max && pctime >= ctime) {
            *str = h_strdup_cprintf(
                "%s\n", *str,
                print_spd_timings((int)(2000.0 / pctime), best_cas, trcd, trp, tras, pctime));
        }
    }
}

static void decode_ddr4_module_size(unsigned char *bytes, dmi_mem_size *size) {
    int sdrcap = 256 << (bytes[4] & 15);
    int buswidth = 8 << (bytes[13] & 7);
    int sdrwidth = 4 << (bytes[12] & 7);
    int signal_loading = bytes[6] & 3;
    int lranks_per_dimm = ((bytes[12] >> 3) & 7) + 1;

    if (signal_loading == 2) lranks_per_dimm *= ((bytes[6] >> 4) & 7) + 1;

    *size = sdrcap / 8 * buswidth / sdrwidth * lranks_per_dimm;
}


static void decode_ddr234_module_date(unsigned char weekb, unsigned char yearb, int *week, int *year) {
    if (yearb == 0x0 || yearb == 0xff ||
        weekb == 0x0 || weekb == 0xff) {
        return;
    }
    *week = (weekb>>4) & 0xf;
    *week *= 10;
    *week += weekb & 0xf;
    *year = (yearb>>4) & 0xf;
    *year *= 10;
    *year += yearb & 0xf;
    *year += 2000;
}

static void decode_ddr2_module_date(unsigned char *bytes, int *week, int *year) {
    decode_ddr234_module_date(bytes[94], bytes[93], week, year);
}

static void decode_ddr3_module_date(unsigned char *bytes, int *week, int *year) {
    decode_ddr234_module_date(bytes[121], bytes[120], week, year);
}

static void decode_ddr4_module_date(unsigned char *bytes, int spd_size, int *week, int *year) {
    if (spd_size < 324)
        return;
    decode_ddr234_module_date(bytes[324], bytes[323], week, year);
}

static void decode_ddr3_dram_manufacturer(unsigned char *bytes,
                                            char **manufacturer, int *bank, int *index) {
    decode_ddr34_manufacturer(bytes[94], bytes[95], (char **) manufacturer, bank, index);
}

static void decode_ddr4_dram_manufacturer(unsigned char *bytes, int spd_size,
                                            char **manufacturer, int *bank, int *index) {
    if (spd_size < 351) {
        *manufacturer = NULL;
        return;
    }

    decode_ddr34_manufacturer(bytes[350], bytes[351], (char **) manufacturer, bank, index);
}

static void detect_ddr4_xmp(unsigned char *bytes, int spd_size, int *majv, int *minv) {
    if (spd_size < 387)
        return;

    *majv = 0; *minv = 0;

    if (bytes[384] != 0x0c || bytes[385] != 0x4a) // XMP magic number
        return;

    if (majv)
        *majv = bytes[387] >> 4;
    if (minv)
        *minv = bytes[387] & 0xf;
}

static void decode_ddr4_xmp(unsigned char *bytes, int spd_size, char **str) {
    float ctime;
    float ddrclk, taa, trcd, trp, tras;

    if (spd_size < 405)
        return;

    ctime = ddr4_mtb_ftb_calc(bytes[396], bytes[431]);
    ddrclk = 2 * (1000 / ctime);
    taa = ddr4_mtb_ftb_calc(bytes[401], bytes[430]);
    trcd = ddr4_mtb_ftb_calc(bytes[402], bytes[429]);
    trp  = ddr4_mtb_ftb_calc(bytes[403], bytes[428]);
    tras = (((bytes[404] & 0x0f) << 8) + bytes[405]) * 0.125;

    *str = g_strdup_printf("[%s]\n"
               "%s=DDR4 %d MHz\n"
               "%s=%d.%d V\n"
               "[%s]\n"
               "%s",
               _("XMP Profile"),
               _("Speed"), (int) ddrclk,
               _("Voltage"), bytes[393] >> 7, bytes[393] & 0x7f,
               _("XMP Timings"),
               print_spd_timings((int) ddrclk, ceil(taa/ctime - 0.025), trcd, trp, tras, ctime));
}

static void decode_ddr4_module_detail(unsigned char *bytes, char *type_detail) {
    float ddr_clock;
    int pc4_speed;
    if (type_detail) {
        decode_ddr4_module_speed(bytes, &ddr_clock, &pc4_speed);
        snprintf(type_detail, 255, "DDR4-%.0f (PC4-%d)", ddr_clock, pc4_speed);
    }
}

static gchar *decode_ddr4_sdram_extra(unsigned char *bytes, int spd_size) {
    float ddr_clock;
    int pc4_speed, xmp_majv = -1, xmp_minv = -1;
    char *speed_timings = NULL, *xmp_profile = NULL, *xmp = NULL, *manf_date = NULL;
    static gchar *out;

    decode_ddr4_module_speed(bytes, &ddr_clock, &pc4_speed);
    decode_ddr4_module_spd_timings(bytes, ddr_clock, &speed_timings);
    detect_ddr4_xmp(bytes, spd_size, &xmp_majv, &xmp_minv);

    if (xmp_majv == -1 && xmp_minv == -1) {
        xmp = NULL;
    }
    else if (xmp_majv <= 0 && xmp_minv <= 0) {
        xmp = g_strdup(_("No"));
    }
    else {
        xmp = g_strdup_printf("%s (revision %d.%d)", _("Yes"), xmp_majv, xmp_minv);
        if (xmp_majv == 2 && xmp_minv == 0)
            decode_ddr4_xmp(bytes, spd_size, &xmp_profile);
    }

    /* expected to continue an [SPD] section */
    out = g_strdup_printf("%s=%s\n"
                          "%s=%s\n"
                          "[%s]\n"
                          "%s\n"
                          "%s",
                          _("Voltage"), bytes[11] & 0x01 ? "1.2 V": _("(Unknown)"),
                          _("XMP"), xmp ? xmp : _("(Unknown)"),
                          _("JEDEC Timings"), speed_timings,
                          xmp_profile ? xmp_profile: "");

    g_free(speed_timings);
    g_free(manf_date);
    g_free(xmp);
    g_free(xmp_profile);

    return out;
}

static void decode_ddr4_part_number(unsigned char *bytes, int spd_size, char *part_number) {
    int i;
    if (!part_number) return;

    if (spd_size < 348) {
        *part_number++ = '\0';
        return;
    }

    for (i = 329; i <= 348; i++)
        *part_number++ = bytes[i];
    *part_number = '\0';
}

static void decode_ddr4_module_manufacturer(unsigned char *bytes, int spd_size,
                                            char **manufacturer, int *bank, int *index) {
    if (spd_size < 321) {
        *manufacturer = NULL;
        return;
    }

    decode_ddr34_manufacturer(bytes[320], bytes[321], manufacturer, bank, index);
}

static int decode_ram_type(unsigned char *bytes) {
    if (bytes[0] < 4) {
        switch (bytes[2]) {
        case 1: return DIRECT_RAMBUS;
        case 17: return RAMBUS;
        }
    } else {
        switch (bytes[2]) {
        case 1: return FPM_DRAM;
        case 2: return EDO;
        case 3: return PIPELINED_NIBBLE;
        case 4: return SDR_SDRAM;
        case 5: return MULTIPLEXED_ROM;
        case 6: return DDR_SGRAM;
        case 7: return DDR_SDRAM;
        case 8: return DDR2_SDRAM;
        case 11: return DDR3_SDRAM;
        case 12: return DDR4_SDRAM;
        }
    }

    return UNKNOWN;
}

static int read_spd(char *spd_path, int offset, size_t size, int use_sysfs,
                    unsigned char *bytes_out) {
    int data_size = 0;
    if (use_sysfs) {
        FILE *spd;
        gchar *temp_path;

        temp_path = g_strdup_printf("%s/eeprom", spd_path);
        if ((spd = fopen(temp_path, "rb"))) {
            fseek(spd, offset, SEEK_SET);
            data_size = fread(bytes_out, 1, size, spd);
            fclose(spd);
        }

        g_free(temp_path);
    } else {
        int i;

        for (i = 0; i <= 3; i++) {
            FILE *spd;
            char *temp_path;

            temp_path = g_strdup_printf("%s/%02x", spd_path, offset + i * 16);
            if ((spd = fopen(temp_path, "rb"))) {
                data_size += fread(bytes_out + i * 16, 1, 16, spd);
                fclose(spd);
            }

            g_free(temp_path);
        }
    }

    return data_size;
}

static GSList *decode_dimms2(GSList *eeprom_list, const gchar *driver, gboolean use_sysfs, int max_size) {
    GSList *eeprom, *dimm_list = NULL;
    int count = 0;
    int spd_size = 0;
    unsigned char bytes[512] = {0};
    spd_data *s = NULL;

    for (eeprom = eeprom_list; eeprom; eeprom = eeprom->next, count++) {
        gchar *spd_path = (gchar*)eeprom->data;
        s = NULL;

        RamType ram_type;

        memset(bytes, 0, 512); /* clear */
        spd_size = read_spd(spd_path, 0, max_size, use_sysfs, bytes);
        ram_type = decode_ram_type(bytes);

        switch (ram_type) {
        case SDR_SDRAM:
            s = spd_data_new();
            memcpy(s->bytes, bytes, 512);
            decode_module_part_number(bytes, s->partno);
            decode_module_manufacturer(bytes + 64, (char**)&s->vendor_str);
            decode_sdr_module_size(bytes, &s->size_MiB);
            decode_sdr_module_detail(bytes, s->type_detail);
            break;
        case DDR_SDRAM:
            s = spd_data_new();
            memcpy(s->bytes, bytes, 512);
            decode_module_part_number(bytes, s->partno);
            decode_module_manufacturer(bytes + 64, (char**)&s->vendor_str);
            decode_ddr_module_size(bytes, &s->size_MiB);
            decode_ddr_module_detail(bytes, s->type_detail);
            break;
        case DDR2_SDRAM:
            s = spd_data_new();
            memcpy(s->bytes, bytes, 512);
            decode_module_part_number(bytes, s->partno);
            decode_module_manufacturer(bytes + 64, (char**)&s->vendor_str);
            decode_ddr2_module_size(bytes, &s->size_MiB);
            decode_ddr2_module_detail(bytes, s->type_detail);
            decode_ddr2_module_type(bytes, &s->form_factor);
            decode_ddr2_module_date(bytes, &s->week, &s->year);
            break;
        case DDR3_SDRAM:
            s = spd_data_new();
            memcpy(s->bytes, bytes, 512);
            decode_ddr3_part_number(bytes, s->partno);
            decode_ddr3_manufacturer(bytes, (char**)&s->vendor_str,
                &s->vendor_bank, &s->vendor_index);
            decode_ddr3_dram_manufacturer(bytes, (char**)&s->dram_vendor_str,
                &s->dram_vendor_bank, &s->dram_vendor_index);
            decode_ddr3_module_size(bytes, &s->size_MiB);
            decode_ddr3_module_type(bytes, &s->form_factor);
            decode_ddr3_module_detail(bytes, s->type_detail);
            decode_ddr3_module_date(bytes, &s->week, &s->year);
            break;
        case DDR4_SDRAM:
            s = spd_data_new();
            memcpy(s->bytes, bytes, 512);
            decode_ddr4_part_number(bytes, spd_size, s->partno);
            decode_ddr4_module_manufacturer(bytes, spd_size, (char**)&s->vendor_str,
                &s->vendor_bank, &s->vendor_index);
            decode_ddr4_dram_manufacturer(bytes, spd_size, (char**)&s->dram_vendor_str,
                &s->dram_vendor_bank, &s->dram_vendor_index);
            decode_ddr4_module_size(bytes, &s->size_MiB);
            decode_ddr4_module_type(bytes, &s->form_factor);
            decode_ddr4_module_detail(bytes, s->type_detail);
            decode_ddr4_module_date(bytes, spd_size, &s->week, &s->year);
            s->ddr4_no_ee1004 = s->ddr4_no_ee1004 || (spd_size < 512);
            spd_ddr4_partial_data = spd_ddr4_partial_data || s->ddr4_no_ee1004;
            break;
        case UNKNOWN:
            break;
        default:
            DEBUG("Unsupported EEPROM type: %s for %s\n", GET_RAM_TYPE_STR(ram_type), spd_path); continue;
            if (ram_type) {
                /* some kind of RAM that we can't decode, but exists */
                s = spd_data_new();
                memcpy(s->bytes, bytes, 512);
            }
        }

        if (s) {
            strncpy(s->dev, g_basename(spd_path), 31);
            s->spd_driver = driver;
            s->spd_size = spd_size;
            s->type = ram_type;
            spd_ram_types |= (1 << (ram_type-1));
            switch (ram_type) {
            case SDR_SDRAM:
            case DDR_SDRAM:
            case DDR2_SDRAM:
                s->spd_rev_major = bytes[62] >> 4;
                s->spd_rev_minor = bytes[62] & 0xf;
                break;
            case DDR3_SDRAM:
            case DDR4_SDRAM:
                s->spd_rev_major = bytes[1] >> 4;
                s->spd_rev_minor = bytes[1] & 0xf;
                break;
            }
            s->vendor = vendor_match(s->vendor_str, NULL);
            s->dram_vendor = vendor_match(s->dram_vendor_str, NULL);
            dimm_list = g_slist_append(dimm_list, s);
        }
    }

    return dimm_list;
}

struct SpdDriver {
    const char *driver;
    const char *dir_path;
    const gint max_size;
    const gboolean use_sysfs;
    const char *spd_name;
};

static const struct SpdDriver spd_drivers[] = {
    { "ee1004",      "/sys/bus/i2c/drivers/ee1004", 512, TRUE, "ee1004"},
    { "at24",        "/sys/bus/i2c/drivers/at24"  , 256, TRUE, "spd"},
    { "eeprom",      "/sys/bus/i2c/drivers/eeprom", 256, TRUE, "eeprom"},
    { "eeprom-proc", "/proc/sys/dev/sensors"      , 256, FALSE, NULL},
    { NULL }
};

GSList *spd_scan() {
    GDir *dir = NULL;
    GSList *eeprom_list = NULL, *dimm_list = NULL;
    gchar *dimm_list_entry, *dir_entry, *name_file, *name;
    const struct SpdDriver *driver;
    gboolean driver_found = FALSE;
    gboolean is_spd = FALSE;

    spd_ddr4_partial_data = FALSE;
    spd_no_driver = FALSE;
    spd_no_support = FALSE;

    for (driver = spd_drivers; driver->dir_path; driver++) {
        if (g_file_test(driver->dir_path, G_FILE_TEST_EXISTS)) {
            dir = g_dir_open(driver->dir_path, 0, NULL);
            if (!dir) continue;

            driver_found = TRUE;
            while ((dir_entry = (char *)g_dir_read_name(dir))) {
                is_spd = FALSE;

                if (driver->use_sysfs) {
                    name_file = NULL;
                    name = NULL;

                    if (isdigit(dir_entry[0])) {
                        name_file = g_build_filename(driver->dir_path, dir_entry, "name", NULL);
                        g_file_get_contents(name_file, &name, NULL, NULL);

                        is_spd = g_strcmp0(name, driver->spd_name);

                        g_free(name_file);
                        g_free(name);
                    }
                }
                else {
                    is_spd = g_str_has_prefix(dir_entry, "eeprom-");
                }

                if (is_spd){
                    dimm_list_entry = g_strdup_printf("%s/%s", driver->dir_path, dir_entry);
                    eeprom_list = g_slist_prepend(eeprom_list, dimm_list_entry);
                }
            }

            g_dir_close(dir);

            if (eeprom_list) {
                dimm_list = decode_dimms2(eeprom_list, driver->driver, driver->use_sysfs, driver->max_size);
                g_slist_free(eeprom_list);
                eeprom_list = NULL;
            }
            if (dimm_list) break;
        }
    }

    if (!driver_found) {
        if (!g_file_test("/sys/module/eeprom", G_FILE_TEST_EXISTS) &&
            !g_file_test("/sys/module/at24", G_FILE_TEST_EXISTS)) {
            spd_no_driver = TRUE; /* trigger hinote for no eeprom driver */
        } else {
            spd_no_support = TRUE; /* trigger hinote for unsupported system */
        }
    }

    return dimm_list;
}
