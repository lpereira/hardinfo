/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
 *    This file from: rpiz - https://github.com/bp0/rpiz
 *    Copyright (C) 2017  Burt P. <pburt0@gmail.com>
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

static char unk[] = "(Unknown)";

/* information table from: http://elinux.org/RPi_HardwareHistory */
typedef struct {
    char *value, *intro, *model, *pcb, *mem, *mfg, *soc;
} RaspberryPiBoard;

static const RaspberryPiBoard rpi_boardinfo[] = {
/*  Value        Introduction  Model Name             PCB rev.  Memory(spec)   Manufacturer  SOC(spec) *
 *                             Raspberry Pi %s                                                         */
  { unk,         unk,          unk,                   unk,      unk,        unk,             NULL },
  { "Beta",      "Q1 2012",    "B (Beta)",            unk,      "256 MB",    "(Beta board)",  NULL },
  { "0002",      "Q1 2012",    "B",                   "1.0",    "256 MB",    unk,             "BCM2835" },
  { "0003",      "Q3 2012",    "B (ECN0001)",         "1.0",    "256 MB",    "(Fuses mod and D14 removed)",   NULL },
  { "0004",      "Q3 2012",    "B",                   "2.0",    "256 MB",    "Sony",          NULL },
  { "0005",      "Q4 2012",    "B",                   "2.0",    "256 MB",    "Qisda",         NULL },
  { "0006",      "Q4 2012",    "B",                   "2.0",    "256 MB",    "Egoman",        NULL },
  { "0007",      "Q1 2013",    "A",                   "2.0",    "256 MB",    "Egoman",        NULL },
  { "0008",      "Q1 2013",    "A",                   "2.0",    "256 MB",    "Sony",          NULL },
  { "0009",      "Q1 2013",    "A",                   "2.0",    "256 MB",    "Qisda",         NULL },
  { "000d",      "Q4 2012",    "B",                   "2.0",    "512 MB",    "Egoman",        NULL },
  { "000e",      "Q4 2012",    "B",                   "2.0",    "512 MB",    "Sony",          NULL },
  { "000f",      "Q4 2012",    "B",                   "2.0",    "512 MB",    "Qisda",         NULL },
  { "0010",      "Q3 2014",    "B+",                  "1.0",    "512 MB",    "Sony",          NULL },
  { "0011",      "Q2 2014",    "Compute Module 1",    "1.0",    "512 MB",    "Sony",          NULL },
  { "0012",      "Q4 2014",    "A+",                  "1.1",    "256 MB",    "Sony",          NULL },
  { "0013",      "Q1 2015",    "B+",                  "1.2",    "512 MB",    unk,             NULL },
  { "0014",      "Q2 2014",    "Compute Module 1",    "1.0",    "512 MB",    "Embest",        NULL },
  { "0015",      unk,          "A+",                  "1.1",    "256 MB/512 MB",    "Embest",      NULL  },
  { "a01040",    unk,          "2 Model B",           "1.0",    "1024 MB DDR2",      "Sony",          "BCM2836" },
  { "a01041",    "Q1 2015",    "2 Model B",           "1.1",    "1024 MB DDR2",      "Sony",          "BCM2836" },
  { "a21041",    "Q1 2015",    "2 Model B",           "1.1",    "1024 MB DDR2",      "Embest",        "BCM2836" },
  { "a22042",    "Q3 2016",    "2 Model B",           "1.2",    "1024 MB DDR2",      "Embest",        "BCM2837" },  /* (with BCM2837) */
  { "900021",    "Q3 2016",    "A+",                  "1.1",    "512 MB",    "Sony",          NULL },
  { "900032",    "Q2 2016?",    "B+",                 "1.2",    "512 MB",    "Sony",          NULL },
  { "900092",    "Q4 2015",    "Zero",                "1.2",    "512 MB",    "Sony",          NULL },
  { "900093",    "Q2 2016",    "Zero",                "1.3",    "512 MB",    "Sony",          NULL },
  { "920093",    "Q4 2016?",   "Zero",                "1.3",    "512 MB",    "Embest",        NULL },
  { "9000c1",    "Q1 2017",    "Zero W",              "1.1",    "512 MB",    "Sony",          NULL },
  { "a02082",    "Q1 2016",    "3 Model B",           "1.2",    "1024 MB DDR2",      "Sony",          "BCM2837" },
  { "a020a0",    "Q1 2017",    "Compute Module 3 or CM3 Lite",  "1.0",    "1024 MB DDR2",    "Sony",          NULL },
  { "a22082",    "Q1 2016",    "3 Model B",           "1.2",    "1024 MB DDR2",      "Embest",        "BCM2837" },
  { "a32082",    "Q4 2016",    "3 Model B",           "1.2",    "1024 MB DDR2",      "Sony Japan",    NULL  },
  { "a020d3",    "Q1 2018",    "3 Model B+",          "1.3",    "1024 MB DDR2",      "Sony",          "BCM2837" },
  { "9020e0",    "Q4 2018",    "3 Model A+",          "1.0",    "512 MB DDR2",    "Sony",          "BCM2837" },

  { "a03111",    "Q2 2019",    "4 Model B",           "1.0",    "1024 MB DDR4",      "Sony",          "BCM2838" },
  { "b03111",    "Q2 2019",    "4 Model B",           "1.0",    "2048 MB DDR4",      "Sony",          "BCM2838" },
  { "c03111",    "Q2 2019",    "4 Model B",           "1.1",    "4096 MB DDR4",      "Sony",          "BCM2838" },
  { "c03114",    "Q2 2020",    "4 Model B",           "1.4",    "8192 MB DDR4",      "Sony",          "BCM2838" },

  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/* return number of chars to skip */
static int rpi_ov_check(const char *r_code) {
    /* sources differ. prefix is either 1000... or just 1... */
    //if (strncmp(r, "1000", 4) == 0)
    //    return 4;
    if (strncmp(r_code, "1", 1) == 0)
        return 1;
    return 0;
}

static int rpi_code_match(const char* code0, const char* code1) {
    int c0, c1;
    if (code0 == NULL || code1 == NULL) return 0;
    c0 = strtol(code0, NULL, 16);
    c1 = strtol(code1, NULL, 16);
    if (c0 && c1)
        return (c0 == c1) ? 1 : 0;
    else
        return (strcmp(code0, code1) == 0) ? 1 : 0;
}

static int rpi_find_board(const char *r_code) {
    int i = 0;
    char *r = (char*)r_code;
    if (r_code == NULL)
        return 0;
    /* ignore the overvolt prefix */
    r += rpi_ov_check(r_code);
    while (rpi_boardinfo[i].value != NULL) {
        if (rpi_code_match(r, rpi_boardinfo[i].value))
            return i;

        i++;
    }
    return 0;
}

/* ------------------------- */

#include "cpu_util.h" /* for PROC_CPUINFO */

static gchar *rpi_board_details(void) {
    int i = 0;
    gchar *ret = NULL;
    gchar *soc = NULL;
    gchar *serial = NULL;
    gchar *revision = NULL;
    int ov = 0;
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
        get_str("Revision", revision);
        get_str("Hardware", soc);
        get_str("Serial", serial);
    }
    fclose(cpuinfo);

    if (revision == NULL || soc == NULL) {
        g_free(soc);
        g_free(revision);
        return NULL;
    }

    ov = rpi_ov_check(revision);
    i = rpi_find_board(revision);
    ret = g_strdup_printf("[%s]\n"
                "%s=%s %s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n",
                _("Raspberry Pi"),
                _("Board Name"), _("Raspberry Pi"), rpi_boardinfo[i].model,
                _("PCB Revision"), rpi_boardinfo[i].pcb,
                _("Introduction"), rpi_boardinfo[i].intro,
                _("Manufacturer"), rpi_boardinfo[i].mfg,
                _("RCode"), (rpi_boardinfo[i].value != unk) ? rpi_boardinfo[i].value : revision,
                _("SOC (spec)"), rpi_boardinfo[i].soc,
                _("Memory (spec)"), rpi_boardinfo[i].mem,
                _("Serial Number"), serial,
                _("Permanent overvolt bit"), (ov) ? C_("rpi-ov-bit", "Set") : C_("rpi-ov-bit", "Not set") );

    g_free(soc);
    g_free(revision);

    if (rpi_boardinfo[i].mem)
        dtree_mem_str = rpi_boardinfo[i].mem;

    return ret;
}
