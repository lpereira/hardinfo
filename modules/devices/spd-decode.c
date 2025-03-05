/*
 * spd-decode.c
 * Copyright (c) 2010 L. A. F. Pereira
 * modified by Ondrej Čerman (2019)
 * modified by Burt P. (2019)
 * Copyright (c) 2024 hwspeedy
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include "devices.h"
#include "hardinfo.h"
#include "spd-decode.h"

int Crc16 (char *ptr, int count) {
     int crc, i;
     crc = 0;
     while (--count >= 0) {
         crc = crc ^ (int)*ptr++ << 8;
         for (i = 0; i < 8; ++i)
              if (crc & 0x8000) crc = crc << 1 ^ 0x1021; else crc = crc << 1;
     }
     return (crc & 0xFFFF);
}

int parity(int value) {
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value &= 0xf;
    return (0x6996 >> value) & 1;
}

typedef struct {
    const char *driver;
    const char *dir_path;
    const gint max_size;
    const gboolean use_sysfs;
    const char *spd_name;
} SpdDriver;

static const SpdDriver spd_drivers[] = {
#ifdef DEBUG
  { "",    "/.", 2048, FALSE, ""}, //scans /eeprom-FILENAME
#endif
    { "espd5216",    "/sys/bus/i2c/drivers/espd5216", 2048, TRUE, "espd5216"},//DDR5
    { "spd5118",     "/sys/bus/i2c/drivers/spd5118",  1024, TRUE, "spd5118"},//DDR5
    { "ee1004",      "/sys/bus/i2c/drivers/ee1004" ,   512, TRUE, "ee1004"},//DDR4
    { "at24",        "/sys/bus/i2c/drivers/at24"   ,   256, TRUE, "spd"},//improved detection used for DDR3--
    { "eeprom",      "/sys/bus/i2c/drivers/eeprom" ,   256, TRUE, "eeprom"},//Wildcard DDR3--
    /*{ "eeprom-proc", "/proc/sys/dev/sensors"       ,  256, FALSE, NULL},*/
    { NULL }
};

int decode_ram_type(unsigned char *bytes) {
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
        case 9: return DDR2_SDRAM;//FB-DIMM
        case 10: return DDR2_SDRAM;//FB-DIMM PROBE
        case 11: return DDR3_SDRAM;
        case 12: return DDR4_SDRAM;
        case 14: return DDR4_SDRAM;//DDR4E
        case 15: return DDR3_SDRAM;//LPDDR3
        case 16: return DDR4_SDRAM;//LPDDR4
        case 17: return DDR4_SDRAM;//LPDDR4X
        case 18: return DDR5_SDRAM;
        case 19: return DDR5_SDRAM;//LPDDR5
        case 20: return UNKNOWN;//DDR5_SDRAM;//DDR5 NVDIMM-P
        case 21: return DDR5_SDRAM;//LPDDR5X
        }
    }
    return UNKNOWN;
}

void decode_old_manufacturer(spd_data *spd) {
  if(spd->spd_size>72){
    unsigned char first;
    int ai = 0;
    int len = 8;
    unsigned char *bytes = 64 + spd -> bytes;
    do { ai++; } while ((--len && (*bytes++ == 0x7f)));
    first = *--bytes;
    if (ai == 0) spd->vendor_str = "Invalid";
    else if (parity(first) != 1) spd->vendor_str = "Invalid";
    else spd->vendor_str = (char *)JEDEC_MFG_STR(ai - 1, (first & 0x7f) - 1);
  }
}

void decode_manufacturer(spd_data *spd, int MLSB, int MMSB, int CLSB, int CMSB) {
  if((MLSB>=0) && (MMSB < spd->spd_size)&&(MLSB < spd->spd_size)){
    unsigned char McodeLSB=spd->bytes[MLSB];
    unsigned char McodeMSB=spd->bytes[MMSB];
    //Module
    if (((McodeMSB == 0) && (McodeLSB == 0)) || ((McodeMSB == 255) && (McodeLSB == 255))) {
        spd->vendor_str=_("Unspecified");
    } else if (parity(McodeMSB) != 1 || parity(McodeLSB) != 1) {
        spd->vendor_str=_("Invalid");
    } else {
        spd->vendor_bank = McodeLSB & 0x7f;
        spd->vendor_index = McodeMSB & 0x7f;
        if (spd->vendor_bank >= VENDORS_BANKS) {
            spd->vendor_str=NULL;
        } else spd->vendor_str=(char *)JEDEC_MFG_STR(spd->vendor_bank, spd->vendor_index - 1);
    }
  }
    //CHIP DRAM
  if((CLSB>=0) && (CLSB < spd->spd_size)&&(CMSB < spd->spd_size)){
    unsigned char CcodeLSB=spd->bytes[CLSB];
    unsigned char CcodeMSB=spd->bytes[CMSB];
    if (((CcodeMSB == 0) && (CcodeLSB == 0)) || ((CcodeMSB == 255) && (CcodeLSB == 255))) {
        spd->dram_vendor_str=_("Unspecified");
    } else if (parity(CcodeMSB) != 1 || parity(CcodeLSB) != 1) {
        spd->dram_vendor_str=_("Invalid");
    } else {
        spd->dram_vendor_bank = CcodeLSB & 0x7f;
        spd->dram_vendor_index = CcodeMSB & 0x7f;
        if (spd->dram_vendor_bank >= VENDORS_BANKS) {
            spd->dram_vendor_str=NULL;
        } else spd->dram_vendor_str=(char *)JEDEC_MFG_STR(spd->dram_vendor_bank, spd->dram_vendor_index - 1);
    }
  }
}

void decode_module_date(spd_data *spd, int Week, int Year) {
  if((Week < spd->spd_size) && (Year < spd->spd_size)){
    unsigned char weekb=spd->bytes[Week];
    unsigned char yearb=spd->bytes[Year];
    if ((yearb == 0x0) || (yearb == 0xff) || (weekb == 0x0) || (weekb == 0xff)) return;
    spd->week = (weekb>>4) & 0xf;
    spd->week *= 10;
    spd->week += weekb & 0xf;
    spd->year = (yearb>>4) & 0xf;
    spd->year *= 10;
    spd->year += yearb & 0xf;
    spd->year += 2000;
  }
}

void decode_module_partno(spd_data *spd, int start, int end) {
    unsigned int j=0;
    if( (spd->spd_size > end) ){
        int i=start;
        while(i<=end){
	    if((spd->bytes[i]>=' ') && (j<sizeof(spd->partno)-1)) spd->partno[j++]=(char)spd->bytes[i];
	    i++;
        }
    }
    spd->partno[j]=0;
}

void decode_module_serialno(spd_data *spd, int SN) {
    if (spd->spd_size > SN+3) {
        sprintf(spd->serialno,"0x%02x%02x%02x%02x",spd->bytes[SN],spd->bytes[SN+1],spd->bytes[SN+2],spd->bytes[SN+3]);
    }
}



/* -------------------------------------------------- */
/* ----------------------- SDR ---------------------- */
/* -------------------------------------------------- */
void decode_sdr_basic(spd_data *spd){
    sprintf(spd->type_detail, "SDR");
    decode_module_partno(spd, 73, 90);
    decode_old_manufacturer(spd);
    decode_module_serialno(spd, 95);
   //decode_sdr_module_size
    if(spd->spd_size > 17){
        unsigned char *bytes = spd->bytes;
        unsigned short i, k = 0;
        i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
        if (bytes[5] <= 8 && bytes[17] <= 8) { k = bytes[5] * bytes[17]; }
        if (i > 0 && i <= 12 && k > 0) {
            spd->size_MiB = (dmi_mem_size)k * (unsigned short)(1 << i);
        } else {
            spd->size_MiB = -1;
        }
    }
}

void decode_sdr_module_timings(unsigned char *bytes, float *tcl, float *trcd, float *trp, float *tras) {
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

void decode_sdr_module_row_address_bits(unsigned char *bytes, char **bits) {
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

void decode_sdr_module_col_address_bits(unsigned char *bytes, char **bits) {
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

void decode_sdr_module_number_of_rows(unsigned char *bytes, int *rows) {
    if (rows) { *rows = bytes[5]; }
}

void decode_sdr_module_data_with(unsigned char *bytes, int *width) {
    if (width) {
        if (bytes[7] > 1) {
            *width = 0;
        } else {
            *width = (bytes[7] * 0xff) + bytes[6];
        }
    }
}

void decode_sdr_module_interface_signal_levels(unsigned char *bytes, char **signal_levels) {
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

void decode_sdr_module_configuration_type(unsigned char *bytes, char **module_config_type) {
    char *temp;
    switch (bytes[11]) {
    case 0: temp = "No parity"; break;
    case 1: temp = "Parity"; break;
    case 2: temp = "ECC"; break;
    default: temp = NULL;
    }
    if (module_config_type) { *module_config_type = temp; }
}

void decode_sdr_module_refresh_type(unsigned char *bytes, char **refresh_type) {
    char *temp;
    if (bytes[12] & 0x80) {
        temp = "Self refreshing";
    } else {
        temp = "Not self refreshing";
    }
    if (refresh_type) { *refresh_type = temp; }
}

void decode_sdr_module_refresh_rate(unsigned char *bytes, char **refresh_rate) {
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

gchar *decode_sdr_sdram_extra(unsigned char *bytes) {
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





/* -------------------------------------------------- */
/* ----------------------- DDR ---------------------- */
/* -------------------------------------------------- */
void decode_ddr_module_speed(unsigned char *bytes, float *ddrclk, int *pcclk) {
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

void decode_ddr_basic(spd_data *spd){
    decode_old_manufacturer(spd);
    decode_module_partno(spd, 73, 90);
    decode_module_serialno(spd, 95);
    //decode_sdr_module_size
    unsigned char *bytes = spd->bytes;
    unsigned short i, k = 0;
    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    if (bytes[5] <= 8 && bytes[17] <= 8) { k = bytes[5] * bytes[17]; }

    if (i > 0 && i <= 12 && k > 0) {
       spd->size_MiB = (dmi_mem_size)k * (unsigned short)(1 << i);
    } else {
        spd->size_MiB = -1;
    }
    //decode_ddr_module_detail
    float ddr_clock;
    int pc_speed;
    decode_ddr_module_speed(bytes, &ddr_clock, &pc_speed);
    snprintf(spd->type_detail, 255, "DDR-%.0f (PC-%d)", ddr_clock, pc_speed);
}

void decode_ddr_module_timings(unsigned char *bytes, float *tcl, float *trcd, float *trp, float *tras) {
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

gchar *decode_ddr_sdram_extra(unsigned char *bytes) {
    float tcl, trcd, trp, tras;
    decode_ddr_module_timings(bytes, &tcl, &trcd, &trp, &tras);
    return g_strdup_printf("[%s]\n"
                           "tCL=%.2f\n"
                           "tRCD=%.2f\n"
                           "tRP=%.2f\n"
                           "tRAS=%.2f\n",
                           _("Timings"), tcl, trcd, trp, tras);
}





/* -------------------------------------------------- */
/* ----------------------- DDR2 --------------------- */
/* -------------------------------------------------- */
float decode_ddr2_module_ctime(unsigned char byte) {
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


void decode_ddr2_module_speed(unsigned char *bytes, float *ddr_clock, int *pc2_speed) {
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

void decode_ddr2_basic(spd_data *spd){
    decode_old_manufacturer(spd);
    decode_module_partno(spd, 73, 90);
    //decode_ddr_module_size(spd->bytes, &s->size_MiB);
    //decode_sdr_module_size
    unsigned char *bytes = spd->bytes;
    unsigned short i, k = 0;
    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    if (bytes[5] <= 8 && bytes[17] <= 8) { k = bytes[5] * bytes[17]; }

    if (i > 0 && i <= 12 && k > 0) {
        spd->size_MiB = (dmi_mem_size)k * (unsigned short)(1 << i);
    } else {
        spd->size_MiB = -1;
    }
    //decode_ddr_module_detail(spd->bytes, s->type_detail);
    float ddr_clock;
    int pc2_speed;
    decode_ddr_module_speed(bytes, &ddr_clock, &pc2_speed);
    snprintf(spd->type_detail, 255, "DDR2-%.0f (PC2-%d)", ddr_clock, pc2_speed);
}



void decode_ddr2_module_type(unsigned char *bytes, const char **type) {
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

void decode_ddr2_module_timings(float ctime, unsigned char *bytes, float *trcd, float *trp, float *tras) {
    if (trcd) { *trcd = ceil(((bytes[29] >> 2) + ((bytes[29] & 3) * 0.25)) / ctime); }
    if (trp) { *trp = ceil(((bytes[27] >> 2) + ((bytes[27] & 3) * 0.25)) / ctime); }
    if (tras) { *tras = ceil(bytes[30] / ctime); }
}

gboolean decode_ddr2_module_ctime_for_casx(int casx_minus, unsigned char *bytes, float *ctime, float *tcl){
    int highest_cas = 0, i, bytei;
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


gchar *decode_ddr2_sdram_extra(unsigned char *bytes) {
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






/* -------------------------------------------------- */
/* ----------------------- DDR3 --------------------- */
/* -------------------------------------------------- */


void decode_ddr3_module_speed(unsigned char *bytes, float *ddr_clock, int *pc3_speed) {
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

void decode_ddr3_module_size(unsigned char *bytes, dmi_mem_size *size) {
    unsigned int sdr_capacity = 256 << (bytes[4] & 0xF);
    unsigned int sdr_width = 4 << (bytes[7] & 0x7);
    unsigned int bus_width = 8 << (bytes[8] & 0x7);
    unsigned int ranks = 1 + ((bytes[7] >> 3) & 0x7);
    *size = (dmi_mem_size)sdr_capacity / 8 * bus_width / sdr_width * ranks;
}

void decode_ddr3_module_timings(unsigned char *bytes, float *trcd, float *trp, float *tras, float *tcl) {
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

void decode_ddr3_module_type(unsigned char *bytes, const char **type) {
    switch (bytes[3]) {
        case 0x01: *type = "RDIMM (Registered Long DIMM)"; break;
        case 0x02: *type = "UDIMM (Unbuffered Long DIMM)"; break;
        case 0x03: *type = "SODIMM (Small Outline DIMM)"; break;
        default: *type = NULL;
    }
}

void decode_ddr3_module_detail(unsigned char *bytes, char *type_detail) {
    float ddr_clock;
    int pc3_speed;
    if (type_detail) {
        decode_ddr3_module_speed(bytes, &ddr_clock, &pc3_speed);
        snprintf(type_detail, 255, "DDR3-%.0f (PC3-%d)", ddr_clock, pc3_speed);
    }
}

gchar *decode_ddr3_sdram_extra(unsigned char *bytes) {
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







/* -------------------------------------------------- */
/* ----------------------- DDR4 --------------------- */
/* -------------------------------------------------- */
char *ddr4_print_spd_timings(int speed, float cas, float trcd, float trp, float tras,
                               float ctime) {
    return g_strdup_printf("DDR4-%d=%.0f-%.0f-%.0f-%.0f\n", speed, cas, ceil(trcd / ctime - 0.025),
                           ceil(trp / ctime - 0.025), ceil(tras / ctime - 0.025));
}
void decode_ddr4_module_size(unsigned char *bytes, dmi_mem_size *size) {
    int sdrcap = 256 << (bytes[4] & 15);
    int buswidth = 8 << (bytes[13] & 7);
    int sdrwidth = 4 << (bytes[12] & 7);
    int signal_loading = bytes[6] & 3;
    int lranks_per_dimm = ((bytes[12] >> 3) & 7) + 1;

    if (signal_loading == 2) lranks_per_dimm *= ((bytes[6] >> 4) & 7) + 1;

    *size = (dmi_mem_size)sdrcap / 8 * buswidth / sdrwidth * lranks_per_dimm;
}

void decode_ddr4_module_type(unsigned char *bytes, const char **type) {
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

float ddr4_mtb_ftb_calc(unsigned char b1, signed char b2) {
    float mtb = 0.125;
    float ftb = 0.001;
    return b1 * mtb + b2 * ftb;
}

void decode_ddr4_module_speed(unsigned char *bytes, float *ddr_clock, int *pc4_speed) {
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

void decode_ddr4_module_spd_timings(unsigned char *bytes, float speed, char **str) {
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

    *str = ddr4_print_spd_timings((int)speed, ceil(taa / ctime - 0.025), trcd, trp, tras, ctime);

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
                ddr4_print_spd_timings((int)(2000.0 / pctime), best_cas, trcd, trp, tras, pctime));
        }
    }
}

void detect_ddr4_xmp(unsigned char *bytes, int spd_size, int *majv, int *minv) {
    if (spd_size > 387){
        *majv = 0; *minv = 0;
        if (bytes[384] == 0x0c && bytes[385] == 0x4a) { // XMP magic number
            if (majv) *majv = bytes[387] >> 4;
            if (minv) *minv = bytes[387] & 0xf;
        }
    }
}

void decode_ddr4_xmp(unsigned char *bytes, int spd_size, char **str) {
    float ctime;
    float ddrclk, taa, trcd, trp, tras;

    if (spd_size > 405){
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
               ddr4_print_spd_timings((int) ddrclk, ceil(taa/ctime - 0.025), trcd, trp, tras, ctime));
    }
}

void decode_ddr4_module_detail(unsigned char *bytes, char *type_detail) {
    float ddr_clock;
    int pc4_speed;
    if (type_detail) {
        decode_ddr4_module_speed(bytes, &ddr_clock, &pc4_speed);
        snprintf(type_detail, 255, "DDR4-%.0f (PC4-%d)", ddr_clock, pc4_speed);
    }
}

gchar *decode_ddr4_sdram_extra(unsigned char *bytes, int spd_size) {
    float ddr_clock;
    int pc4_speed, xmp_majv = -1, xmp_minv = -1;
    char *speed_timings = NULL, *xmp_profile = NULL, *xmp = NULL, *manf_date = NULL;
    gchar *out;

    decode_ddr4_module_speed(bytes, &ddr_clock, &pc4_speed);
    decode_ddr4_module_spd_timings(bytes, ddr_clock, &speed_timings);
    detect_ddr4_xmp(bytes, spd_size, &xmp_majv, &xmp_minv);

    if (xmp_majv == -1 && xmp_minv == -1) {
        xmp = NULL;
    } else if (xmp_majv <= 0 && xmp_minv <= 0) {
        xmp = g_strdup(_("No"));
    } else {
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







/* -------------------------------------------------- */
/* ----------------------- DDR5 --------------------- */
/* -------------------------------------------------- */
void decode_ddr5_module_type(unsigned char *bytes, const char **type) {
    switch (bytes[3] & 0x0f) {
    case 0x01: *type = "RDIMM (Registered DIMM)"; break;
    case 0x02: *type = "UDIMM (Unbuffered DIMM)"; break;
    case 0x03: *type = "SODIMM (Small Outline Unbuffered DIMM)"; break;
    case 0x04: *type = "LRDIMM (Load-Reduced DIMM)"; break;
    case 0x05: *type = "CUDIMM (Clocked Unbuffered DIMM)"; break;
    case 0x06: *type = "CSOUDIMM (Clocked Small Outline DIMM)"; break;
    case 0x07: *type = "MRDIMM (Multiplexed Rand DIMM)"; break;
    case 0x08: *type = "CAMM2 (Compression Attached MM)"; break;
    case 0x0a: *type = "DDIM (Differential DIMM)"; break;
    case 0x0b: *type = "Soldered (Solder Down)"; break;
    default: *type = NULL;
    }
}


char *ddr5_print_spd_timings(int speed, float cas, float trcd, float trp, float tras,
                               float ctime) {
    return g_strdup_printf("DDR5-%d=%.0f-%.0f-%.0f-%.0f\n", speed, cas, ceil(trcd / ctime - 0.025),
                           ceil(trp / ctime - 0.025), ceil(tras / ctime - 0.025));
}
void decode_ddr5_module_size(unsigned char *bytes, dmi_mem_size *size) {
    int sdrcap;
    int diePerPack;
    switch(bytes[4] & 31) {//gbit
        case 0: sdrcap=0;break;
        case 1: sdrcap=4;break;
        case 2: sdrcap=8;break;
        case 3: sdrcap=12;break;
        case 4: sdrcap=16;break;
        case 5: sdrcap=24;break;
        case 6: sdrcap=32;break;
        case 7: sdrcap=48;break;
        case 8: sdrcap=64;break;
        default: sdrcap=0;break;
    }
    switch(bytes[4]>>5) {
        case 0: diePerPack=1;break;
        case 1: diePerPack=2;break;
        case 2: diePerPack=2;break;
        case 3: diePerPack=4;break;
        case 4: diePerPack=8;break;
        case 5: diePerPack=16;break;
        default: diePerPack=1;break;
    }
    //MB - multiply by 2 due to 2 32bit channels
    *size = (dmi_mem_size)sdrcap*2*1024*diePerPack;
}

float ddr5_mtb_ftb_calc(unsigned char b1, signed char b2) {
    float mtb = 0.125;
    float ftb = 0.001;
    return b1 * mtb + b2 * ftb;
}

void decode_ddr5_module_speed(unsigned char *bytes, float *ddr_clock, int *pc5_speed) {
    float ctime;
    float ddrclk;
    int pcclk;

    ctime = (bytes[21]<<8) + bytes[20];
    ddrclk = (2000000.0 / ctime);

    pcclk = ddrclk*8;
    pcclk -= pcclk % 100;

    if (ddr_clock) { *ddr_clock = (int)ddrclk; }
    if (pc5_speed) { *pc5_speed = pcclk; }
}

void decode_ddr5_module_spd_timings(unsigned char *bytes, float speed, char **str) {
    float ctime, ctime_max, pctime, taa, trcd, trp, tras;
    int pcas, best_cas, base_cas, ci, i, j;
    unsigned char cas_support[] = {bytes[20], bytes[21], bytes[22], bytes[23] & 0x1f};
    float possible_ctimes[] = {15 / 24.0, 15 / 22.0, 15 / 20.0, 15 / 18.0,
                               15 / 16.0, 15 / 14.0, 15 / 12.0};

    base_cas = bytes[23] & 0x80 ? 23 : 7;

    ctime = (bytes[21]<<8) + bytes[20];
    ctime_max = (bytes[23]<<8) + bytes[22];

    taa  = (bytes[31]<<8) + bytes[30];
    trcd = (bytes[33]<<8) + bytes[32];
    trp  = (bytes[35]<<8) + bytes[34];
    tras = (bytes[36]<<8) + bytes[35];

    *str = ddr5_print_spd_timings((int)speed, ceil(taa / ctime - 0.025), trcd, trp, tras, ctime);

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
                ddr5_print_spd_timings((int)(2000.0 / pctime), best_cas, trcd, trp, tras, pctime));
        }
    }
}

void detect_ddr5_xmp(unsigned char *bytes, int spd_size, int *majv, int *minv) {
  /*if (spd_size < 387)
        return;

    *majv = 0; *minv = 0;

    if (bytes[384] != 0x0c || bytes[385] != 0x4a) // XMP magic number
        return;

    if (majv)
        *majv = bytes[387] >> 4;
    if (minv)
    *minv = bytes[387] & 0xf;*/
}

void decode_ddr5_xmp(unsigned char *bytes, int spd_size, char **str) {
  /*float ctime;
    float ddrclk, taa, trcd, trp, tras;

    if (spd_size < 405)
        return;

    ctime = ddr5_mtb_ftb_calc(bytes[396], bytes[431]);
    ddrclk = 2 * (1000 / ctime);
    taa = ddr5_mtb_ftb_calc(bytes[401], bytes[430]);
    trcd = ddr5_mtb_ftb_calc(bytes[402], bytes[429]);
    trp  = ddr5_mtb_ftb_calc(bytes[403], bytes[428]);
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
               ddr5_print_spd_timings((int) ddrclk, ceil(taa/ctime - 0.025), trcd, trp, tras, ctime));
  */
}

void decode_ddr5_module_detail(unsigned char *bytes, char *type_detail) {
    float ddr_clock;
    int pc5_speed;
    if (type_detail) {
        decode_ddr5_module_speed(bytes, &ddr_clock, &pc5_speed);
        snprintf(type_detail, 255, "DDR5-%.0f (PC5-%d)", ddr_clock, pc5_speed);
    }
}

gchar *decode_ddr5_sdram_extra(unsigned char *bytes, int spd_size) {
    float ddr_clock;
    int pc5_speed, xmp_majv = -1, xmp_minv = -1;
    char *speed_timings = NULL, *xmp_profile = NULL, *xmp = NULL, *manf_date = NULL;
    gchar *out;

    decode_ddr5_module_speed(bytes, &ddr_clock, &pc5_speed);
    decode_ddr5_module_spd_timings(bytes, ddr_clock, &speed_timings);
    detect_ddr5_xmp(bytes, spd_size, &xmp_majv, &xmp_minv);

    if (xmp_majv == -1 && xmp_minv == -1) {
        xmp = NULL;
    }
    else if (xmp_majv <= 0 && xmp_minv <= 0) {
        xmp = g_strdup(_("No"));
    }
    else {
        xmp = g_strdup_printf("%s (revision %d.%d)", _("Yes"), xmp_majv, xmp_minv);
        if (xmp_majv == 2 && xmp_minv == 0)
            decode_ddr5_xmp(bytes, spd_size, &xmp_profile);
    }

    /* expected to continue an [SPD] section */
    out = g_strdup_printf("%s=%s\n"
                          //"%s=%s\n"
                          "[%s]\n"
                          "%s\n"
                          "%s",
                          _("Voltage"), bytes[15]==0  ? "1.1 V": _("(Unknown)"),
                          //_("XMP"), xmp ? xmp : _("(Unknown)"),
                          _("JEDEC Timings"), speed_timings,
                          xmp_profile ? xmp_profile: "");

    g_free(speed_timings);
    g_free(manf_date);
    g_free(xmp);
    g_free(xmp_profile);

    return out;
}





/*- - - - - - - - - - - SPD EEprom handling  - - - - - - - - - -*/

int read_spd(char *spd_path, int offset, size_t size, int use_sysfs,
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
        FILE *spd;

        if ((spd = fopen(spd_path, "rb"))) {
            fseek(spd, offset, SEEK_SET);
            data_size = fread(bytes_out, 1, size, spd);
            fclose(spd);
        }
	/*OLD unsupported
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
	}*/
    }

    return data_size;
}

void spd_data_free(spd_data *s) { g_free(s->bytes);g_free(s); }

GSList *decode_dimms2(GSList *eeprom_list, const gchar *driver, gboolean use_sysfs, int max_size) {
    GSList *eeprom, *dimm_list = NULL;
    int count = 0;
    spd_data *s = NULL;

    for (eeprom = eeprom_list; eeprom; eeprom = eeprom->next, count++) {
        gchar *spd_path = (gchar*)eeprom->data;
        s = NULL;

        s = g_new0(spd_data,1);
        s->bytes=g_malloc(max_size);
        memset(s->bytes, 0, max_size); /* clear */
        s->spd_size = read_spd(spd_path, 0, max_size, use_sysfs, s->bytes);
        s->type = decode_ram_type(s->bytes);

        switch (s->type) {
        case SDR_SDRAM:
	    decode_sdr_basic(s);
            break;
        case DDR_SDRAM:
	    decode_ddr_basic(s);
            break;
        case DDR2_SDRAM:
	    decode_ddr2_basic(s);
            decode_ddr2_module_type(s->bytes, &s->form_factor);
            decode_module_date(s, 94, 93);
	    decode_module_serialno(s, 95);
            break;
        case DDR3_SDRAM:
            decode_module_partno(s, 128, 145);
            decode_manufacturer(s, 117, 118, 148, 149);
            decode_ddr3_module_size(s->bytes, &s->size_MiB);
            decode_ddr3_module_detail(s->bytes, s->type_detail);
            decode_ddr3_module_type(s->bytes, &s->form_factor);
            decode_module_date(s, 121, 120);
	    decode_module_serialno(s, 122);
            break;
        case DDR4_SDRAM:
            decode_module_partno(s, 329, 348);
            decode_manufacturer(s, 320, 321, 350, 351);
            decode_ddr4_module_size(s->bytes, &s->size_MiB);
            decode_ddr4_module_type(s->bytes, &s->form_factor);
            decode_ddr4_module_detail(s->bytes, s->type_detail);
            decode_module_date(s, 324, 323);
	    decode_module_serialno(s, 325);
            break;
        case DDR5_SDRAM:
            decode_module_partno(s, 521, 550);
            decode_manufacturer(s, 512, 513, 552, 553);
            decode_ddr5_module_size(s->bytes, &s->size_MiB);
            decode_ddr5_module_type(s->bytes, &s->form_factor);
            decode_ddr5_module_detail(s->bytes, s->type_detail);
            decode_module_date(s, 516, 515);
	    decode_module_serialno(s, 517);
            break;
        case UNKNOWN:
        default:
            DEBUG("Unsupported EEPROM type: %s for %s\n", GET_RAM_TYPE_STR(s->type), spd_path);
	    break;
        }

        if (s) {
            strncpy(s->dev, g_path_get_basename(spd_path), 31);
            s->spd_driver = driver;
            switch (s->type) {
            case SDR_SDRAM:
            case DDR_SDRAM:
            case DDR2_SDRAM:
                s->spd_rev_major = s->bytes[62] >> 4;
                s->spd_rev_minor = s->bytes[62] & 0xf;
                break;
            case DDR3_SDRAM:
            case DDR4_SDRAM:
            case DDR5_SDRAM:
                s->spd_rev_major = s->bytes[1] >> 4;
                s->spd_rev_minor = s->bytes[1] & 0xf;
                break;
            }
            s->vendor = vendor_match(s->vendor_str, NULL);
            s->dram_vendor = vendor_match(s->dram_vendor_str, NULL);
            dimm_list = g_slist_append(dimm_list, s);
        }
    }

    return dimm_list;
}

static GSList *memory_info_from_udev(void)
{
    GSList *dimm_list = NULL;
    gchar **lines;
    gchar *stdout;
    int i;

    if (!g_spawn_command_line_sync("/usr/bin/udevadm info -e", &stdout, NULL,
                                   NULL, NULL))
        return NULL;

    lines = g_strsplit(stdout, "\n", -1);
    g_free(stdout);

    long last_id = -1;
    spd_data *spd = NULL;
    const char *speed_mts = NULL;
    const char *speed_mts_configured = NULL;
    const char *mem_tech = NULL;
    const char *type_detail = NULL;

    for (i = 0; lines[i]; i++) {
        if (!g_str_has_prefix(lines[i], "E: MEMORY_DEVICE_"))
            continue;

        gchar *line = lines[i];
        char *underline;

        errno = 0;
        long id = strtol(line + strlen("E: MEMORY_DEVICE_"), &underline, 10);
        if (errno)
            continue;

        line = underline + 1;

        gchar *equal = strchr(line, '=');
        if (!equal)
            continue;

        gchar *key = line;
        gchar *value = equal + 1;

        if (last_id != id) {
            last_id = id;
            if (spd) {
                snprintf(spd->type_detail, 256,
                    "%s %s, %sMT/s @ %sMT/s",
                    type_detail ? type_detail : "",
                    mem_tech ? mem_tech : "",
                    speed_mts ? speed_mts : "?",
                    speed_mts_configured ? speed_mts_configured : "?");
                dimm_list = g_slist_append(dimm_list, spd);
            }
            spd = g_new0(spd_data, 1);
            spd->spd_driver = "udev";
            snprintf(spd->dev, 32, "stick-%ld", id);
            speed_mts = speed_mts_configured = mem_tech = type_detail = NULL;
        }

        if (g_str_has_prefix(key, "TYPE")) {
            if (g_str_has_prefix(key, "TYPE_DETAIL")) {
                type_detail = value;
            } else {
                if (g_str_equal(value, "DDR4"))
                    spd->type = DDR4_SDRAM;
                else if (g_str_equal(value, "DDR3"))
                    spd->type = DDR3_SDRAM;
                else if (g_str_equal(value, "DDR2"))
                    spd->type = DDR2_SDRAM;
                else if (g_str_equal(value, "DDR"))
                    spd->type = DDR_SDRAM;
                else if (g_str_equal(value, "SDR"))
                    spd->type = SDR_SDRAM;
                else
                    spd->type = UNKNOWN;
            }
        } else if (g_str_has_prefix(key, "MANUFACTURER")) {
            spd->vendor = vendor_match(value, NULL);
            spd->vendor_str = vendor_get_name(value);
        } else if (g_str_has_prefix(key, "FORM_FACTOR")) {
            if (g_str_equal(value, "RDIMM"))
                spd->form_factor = "RDIMM";
            else if (g_str_equal(value, "UDIMM"))
                spd->form_factor = "UDIMM";
            else if (g_str_equal(value, "SODIMM"))
                spd->form_factor = "SODIMM";
        } else if (g_str_has_prefix(key, "SPEED_MTS")) {
            speed_mts = value;
        } else if (g_str_has_prefix(key, "CONFIGURED_SPEED_MTS")) {
            speed_mts_configured = value;
        } else if (g_str_has_prefix(key, "MEMORY_TECHNOLOGY")) {
            mem_tech = value;
        } else if (g_str_has_prefix(key, "PART_NUMBER")) {
            snprintf(spd->partno, 32, "%s", value);
        } else if (g_str_has_prefix(key, "VOLATILE_SIZE")) {
            spd->size_MiB = strtoll(value, NULL, 10) / 1048576u;
        } else if (g_str_has_prefix(key, "MODULE_MANUFACTURER_ID")) {
            int bank, index;
            if (sscanf(value, "Bank %d, Hex 0x%x", &bank, &index) == 2) {
                spd->vendor_bank = bank;
                spd->vendor_index = index;
            }
        } else {
            /* FIXME: there are many more fields there! */
        }
    }

    if (spd) {
        snprintf(spd->type_detail, 256,
            "%s %s, %sMT/s @ %sMT/s",
            type_detail ? type_detail : "",
            mem_tech ? mem_tech : "",
            speed_mts ? speed_mts : "?",
            speed_mts_configured ? speed_mts_configured : "?");
        dimm_list = g_slist_append(dimm_list, spd);
    }

    g_strfreev(lines);

    return dimm_list;
}

GSList *spd_scan() {
    GDir *dir = NULL;
    GSList *eeprom_list = NULL, *dimm_list = NULL;
    gchar *dimm_list_entry, *dir_entry, *name_file, *name;
    const SpdDriver *driver;
    gboolean is_spd = FALSE;

    dimm_list = memory_info_from_udev();
    if (dimm_list)
        return dimm_list;

    for (driver = spd_drivers; driver->dir_path; driver++) {
        if (g_file_test(driver->dir_path, G_FILE_TEST_EXISTS)) {
            dir = g_dir_open(driver->dir_path, 0, NULL);
            if (!dir) continue;

            while ((dir_entry = (char *)g_dir_read_name(dir))) {
                is_spd = FALSE;

                if (driver->use_sysfs) {
                    name_file = NULL;
                    name = NULL;

                    if (isdigit(dir_entry[0])) {
                        name_file = g_build_filename(driver->dir_path, dir_entry, "name", NULL);
                        g_file_get_contents(name_file, &name, NULL, NULL);
			is_spd=g_strcmp0(name, driver->spd_name);
                        g_free(name_file);
			g_free(name);
			//check i2c controller is SMBus for eeprom (eeproms autodection is wild, so we improve)
			if(strstr(driver->spd_name,"eeprom") && is_spd){
			    name=g_strdup(dir_entry);
                            strend(name,'-');
                            name_file = g_strdup_printf("/sys/bus/i2c/devices/i2c-%s/name", name);
			    g_free(name); name=NULL;
                            g_file_get_contents(name_file, &name, NULL, NULL);
			    is_spd=0;
			    if(name){
                                is_spd=(strstr(name, "SMBus")?1:0);
                                g_free(name);
			    }
                            g_free(name_file);
		        }
                    }
                } else {
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

    return dimm_list;
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
            case DDR5_SDRAM:
                full_spd = decode_ddr5_sdram_extra(spd->bytes, spd->spd_size);
                break;
            default:
	        DEBUG("SPD unknown type: %d %s\n", spd->type, ram_types[spd->type]);
        }
        gchar *size_str = NULL;
        if (!spd->size_MiB)
            size_str = g_strdup(_("(Unknown)"));
        else if(spd->size_MiB >= 1024)
	    size_str = g_strdup_printf("%u %s", spd->size_MiB>>10, _("GiB") );
	else
	    size_str = g_strdup_printf("%u %s", spd->size_MiB, _("MiB") );

        gchar *mfg_date_str = NULL;
        if (spd->year)
	    mfg_date_str = g_strdup_printf("%d / %d", spd->year, spd->week);

        ret = g_strdup_printf("[%s - %s]\n"
                    "%s=%s (%s)%s\n"
                    "%s=%d.%d\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "$^$%s=[%02x%02x] %s\n" /* module vendor */
                    "$^$%s=[%02x%02x] %s\n" /* dram vendor */
                    "%s=%s\n" /* part */
                    "%s=%s\n" /* serialno */
                    "%s=%s\n" /* size */
                    "%s=%s\n" /* mfg date */
                    "%s",
                    _("Serial Presence Detect (SPD)"), ram_types[spd->type],
                    _("Source"), spd->dev, spd->spd_driver,
                        (spd->type == DDR4_SDRAM && strcmp(spd->spd_driver, "ee1004") != 0) ? problem_marker() : "",
                    _("SPD Revision"), spd->spd_rev_major, spd->spd_rev_minor,
                    _("Form Factor"), UNKIFNULL2(spd->form_factor),
                    _("Type"), UNKIFEMPTY2(spd->type_detail),
                    _("Module Vendor"), spd->vendor_bank, spd->vendor_index,
                        UNKIFNULL2(spd->vendor_str),
                    _("DRAM Vendor"), spd->dram_vendor_bank, spd->dram_vendor_index,
                        UNKIFNULL2(spd->dram_vendor_str),
                    _("Part Number"), UNKIFEMPTY2(spd->partno),
                    _("Serial Number"), UNKIFEMPTY2(spd->serialno),
                    _("Size"), size_str,
                    _("Manufacturing Date (Year / Week)"), UNKIFNULL2(mfg_date_str),
                    full_spd ? full_spd : ""
                    );
        g_free(full_spd);
        g_free(size_str);
        g_free(mfg_date_str);
    }
    return ret;
}

