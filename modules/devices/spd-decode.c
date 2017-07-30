/*
 * spd-decode.c
 * Copyright (c) 2010 Leandro A. F. Pereira
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

#include "hardinfo.h"
#include "devices.h"

typedef enum {
    UNKNOWN,
    DIRECT_RAMBUS,
    RAMBUS,
    FPM_DRAM,
    EDO,
    PIPELINED_NIBBLE,
    SDR_SDRAM,
    MULTIPLEXED_ROM,
    DDR_SGRAM,
    DDR_SDRAM,
    DDR2_SDRAM,
    DDR3_SDRAM
} RamType;

char *spd_info = NULL;

static const char *ram_types[] = {
    "Unknown",
    "Direct Rambus",
    "Rambus",
    "FPM DRAM",
    "EDO",
    "Pipelined Nibble",
    "SDR SDRAM",
    "Multiplexed ROM",
    "DDR SGRAM",
    "DDR SDRAM",
    "DDR2 SDRAM",
    "DDR3 SDRAM"
};

static const char *vendors1[] = { "AMD", "AMI", "Fairchild", "Fujitsu",
    "GTE", "Harris", "Hitachi", "Inmos",
    "Intel", "I.T.T.", "Intersil",
    "Monolithic Memories",
    "Mostek",
    "Freescale (former Motorola)",
    "National", "NEC",
    "RCA", "Raytheon",
    "Conexant (Rockwell)", "Seeq",
    "NXP (former Signetics, Philips Semi.)",
    "Synertek",
    "Texas Instruments", "Toshiba",
    "Xicor", "Zilog", "Eurotechnique",
    "Mitsubishi",
    "Lucent (AT&T)", "Exel", "Atmel",
    "SGS/Thomson",
    "Lattice Semi.", "NCR",
    "Wafer Scale Integration", "IBM",
    "Tristar", "Visic",
    "Intl. CMOS Technology", "SSSI",
    "MicrochipTechnology", "Ricoh Ltd.",
    "VLSI", "Micron Technology",
    "Hyundai Electronics",
    "OKI Semiconductor", "ACTEL",
    "Sharp",
    "Catalyst", "Panasonic", "IDT",
    "Cypress",
    "DEC", "LSI Logic",
    "Zarlink (former Plessey)", "UTMC",
    "Thinking Machine", "Thomson CSF",
    "Integrated CMOS (Vertex)",
    "Honeywell",
    "Tektronix", "Sun Microsystems",
    "SST", "ProMos/Mosel Vitelic",
    "Infineon (former Siemens)",
    "Macronix", "Xerox", "Plus Logic",
    "SunDisk", "Elan Circuit Tech.",
    "European Silicon Str.",
    "Apple Computer",
    "Xilinx", "Compaq",
    "Protocol Engines", "SCI",
    "Seiko Instruments", "Samsung",
    "I3 Design System", "Klic",
    "Crosspoint Solutions",
    "Alliance Semiconductor", "Tandem",
    "Hewlett-Packard",
    "Intg. Silicon Solutions",
    "Brooktree", "New Media",
    "MHS Electronic",
    "Performance Semi.",
    "Winbond Electronic",
    "Kawasaki Steel",
    "Bright Micro",
    "TECMAR", "Exar", "PCMCIA",
    "LG Semi (former Goldstar)",
    "Northern Telecom", "Sanyo",
    "Array Microsystems",
    "Crystal Semiconductor",
    "Analog Devices", "PMC-Sierra",
    "Asparix", "Convex Computer",
    "Quality Semiconductor",
    "Nimbus Technology", "Transwitch",
    "Micronas (ITT Intermetall)",
    "Cannon", "Altera", "NEXCOM",
    "QUALCOMM",
    "Sony", "Cray Research",
    "AMS(Austria Micro)", "Vitesse",
    "Aster Electronics",
    "Bay Networks (Synoptic)",
    "Zentrum or ZMD",
    "TRW",
    "Thesys", "Solbourne Computer",
    "Allied-Signal", "Dialog",
    "Media Vision",
    "Level One Communication"
};

static const char *vendors2[] = { "Cirrus Logic",
    "National Instruments",
    "ILC Data Device",
    "Alcatel Mietec",
    "Micro Linear",
    "Univ. of NC",
    "JTAG Technologies",
    "BAE Systems",
    "Nchip",
    "Galileo Tech",
    "Bestlink Systems",
    "Graychip",
    "GENNUM",
    "VideoLogic",
    "Robert Bosch",
    "Chip Express",
    "DATARAM",
    "United Microelec Corp.",
    "TCSI",
    "Smart Modular",
    "Hughes Aircraft",
    "Lanstar Semiconductor",
    "Qlogic", "Kingston",
    "Music Semi",
    "Ericsson Components",
    "SpaSE",
    "Eon Silicon Devices",
    "Programmable Micro Corp",
    "DoD",
    "Integ. Memories Tech.",
    "Corollary Inc.",
    "Dallas Semiconductor",
    "Omnivision",
    "EIV(Switzerland)",
    "Novatel Wireless",
    "Zarlink (former Mitel)",
    "Clearpoint",
    "Cabletron",
    "STEC (former Silicon Technology)",
    "Vanguard",
    "Hagiwara Sys-Com",
    "Vantis", "Celestica",
    "Century",
    "Hal Computers",
    "Rohm Company Ltd.",
    "Juniper Networks",
    "Libit Signal Processing",
    "Mushkin Enhanced Memory",
    "Tundra Semiconductor",
    "Adaptec Inc.",
    "LightSpeed Semi.",
    "ZSP Corp.",
    "AMIC Technology",
    "Adobe Systems",
    "Dynachip",
    "PNY Electronics",
    "Newport Digital",
    "MMC Networks",
    "T Square",
    "Seiko Epson",
    "Broadcom",
    "Viking Components",
    "V3 Semiconductor",
    "Flextronics (former Orbit)",
    "Suwa Electronics",
    "Transmeta",
    "Micron CMS",
    "American Computer & Digital Components Inc",
    "Enhance 3000 Inc",
    "Tower Semiconductor",
    "CPU Design",
    "Price Point",
    "Maxim Integrated Product",
    "Tellabs",
    "Centaur Technology",
    "Unigen Corporation",
    "Transcend Information",
    "Memory Card Technology",
    "CKD Corporation Ltd.",
    "Capital Instruments, Inc.",
    "Aica Kogyo, Ltd.",
    "Linvex Technology",
    "MSC Vertriebs GmbH",
    "AKM Company, Ltd.",
    "Dynamem, Inc.",
    "NERA ASA",
    "GSI Technology",
    "Dane-Elec (C Memory)",
    "Acorn Computers",
    "Lara Technology",
    "Oak Technology, Inc.",
    "Itec Memory",
    "Tanisys Technology",
    "Truevision",
    "Wintec Industries",
    "Super PC Memory",
    "MGV Memory",
    "Galvantech",
    "Gadzoox Nteworks",
    "Multi Dimensional Cons.",
    "GateField",
    "Integrated Memory System",
    "Triscend", "XaQti",
    "Goldenram",
    "Clear Logic",
    "Cimaron Communications",
    "Nippon Steel Semi. Corp.",
    "Advantage Memory",
    "AMCC",
    "LeCroy",
    "Yamaha Corporation",
    "Digital Microwave",
    "NetLogic Microsystems",
    "MIMOS Semiconductor",
    "Advanced Fibre",
    "BF Goodrich Data.",
    "Epigram",
    "Acbel Polytech Inc.",
    "Apacer Technology",
    "Admor Memory",
    "FOXCONN",
    "Quadratics Superconductor",
    "3COM",
};

static const char *vendors3[] = { "Camintonn Corporation", "ISOA Incorporated",
    "Agate Semiconductor", "ADMtek Incorporated",
    "HYPERTEC", "Adhoc Technologies", "MOSAID Technologies",
    "Ardent Technologies",
    "Switchcore", "Cisco Systems, Inc.", "Allayer Technologies",
    "WorkX AG",
    "Oasis Semiconductor", "Novanet Semiconductor", "E-M Solutions",
    "Power General",
    "Advanced Hardware Arch.", "Inova Semiconductors GmbH", "Telocity",
    "Delkin Devices",
    "Symagery Microsystems", "C-Port Corporation",
    "SiberCore Technologies", "Southland Microsystems",
    "Malleable Technologies", "Kendin Communications",
    "Great Technology Microcomputer", "Sanmina Corporation",
    "HADCO Corporation", "Corsair", "Actrans System Inc.",
    "ALPHA Technologies",
    "Silicon Laboratories, Inc. (Cygnal)", "Artesyn Technologies",
    "Align Manufacturing", "Peregrine Semiconductor",
    "Chameleon Systems", "Aplus Flash Technology", "MIPS Technologies",
    "Chrysalis ITS",
    "ADTEC Corporation", "Kentron Technologies", "Win Technologies",
    "Tachyon Semiconductor (former ASIC Designs Inc.)",
    "Extreme Packet Devices", "RF Micro Devices", "Siemens AG",
    "Sarnoff Corporation",
    "Itautec Philco SA", "Radiata Inc.", "Benchmark Elect. (AVEX)",
    "Legend",
    "SpecTek Incorporated", "Hi/fn", "Enikia Incorporated",
    "SwitchOn Networks",
    "AANetcom Incorporated", "Micro Memory Bank", "ESS Technology",
    "Virata Corporation",
    "Excess Bandwidth", "West Bay Semiconductor", "DSP Group",
    "Newport Communications",
    "Chip2Chip Incorporated", "Phobos Corporation",
    "Intellitech Corporation", "Nordic VLSI ASA",
    "Ishoni Networks", "Silicon Spice", "Alchemy Semiconductor",
    "Agilent Technologies",
    "Centillium Communications", "W.L. Gore", "HanBit Electronics",
    "GlobeSpan",
    "Element 14", "Pycon", "Saifun Semiconductors", "Sibyte, Incorporated",
    "MetaLink Technologies", "Feiya Technology", "I & C Technology",
    "Shikatronics",
    "Elektrobit", "Megic", "Com-Tier", "Malaysia Micro Solutions",
    "Hyperchip", "Gemstone Communications", "Anadigm (former Anadyne)",
    "3ParData",
    "Mellanox Technologies", "Tenx Technologies", "Helix AG", "Domosys",
    "Skyup Technology", "HiNT Corporation", "Chiaro",
    "MDT Technologies GmbH (former MCI Computer GMBH)",
    "Exbit Technology A/S", "Integrated Technology Express", "AVED Memory",
    "Legerity",
    "Jasmine Networks", "Caspian Networks", "nCUBE",
    "Silicon Access Networks",
    "FDK Corporation", "High Bandwidth Access", "MultiLink Technology",
    "BRECIS",
    "World Wide Packets", "APW", "Chicory Systems", "Xstream Logic",
    "Fast-Chip", "Zucotto Wireless", "Realchip", "Galaxy Power",
    "eSilicon", "Morphics Technology", "Accelerant Networks",
    "Silicon Wave",
    "SandCraft", "Elpida"
};

static const char *vendors4[] = { "Solectron", "Optosys Technologies",
    "Buffalo (former Melco)",
    "TriMedia Technologies",
    "Cyan Technologies", "Global Locate",
    "Optillion",
    "Terago Communications",
    "Ikanos Communications",
    "Princeton Technology",
    "Nanya Technology",
    "Elite Flash Storage",
    "Mysticom", "LightSand Communications",
    "ATI Technologies",
    "Agere Systems",
    "NeoMagic", "AuroraNetics", "Golden Empire",
    "Mushkin",
    "Tioga Technologies", "Netlist", "TeraLogic",
    "Cicada Semiconductor",
    "Centon Electronics", "Tyco Electronics",
    "Magis Works", "Zettacom",
    "Cogency Semiconductor", "Chipcon AS",
    "Aspex Technology",
    "F5 Networks",
    "Programmable Silicon Solutions",
    "ChipWrights",
    "Acorn Networks",
    "Quicklogic",
    "Kingmax Semiconductor", "BOPS", "Flasys",
    "BitBlitz Communications",
    "eMemory Technology", "Procket Networks",
    "Purple Ray",
    "Trebia Networks",
    "Delta Electronics", "Onex Communications",
    "Ample Communications",
    "Memory Experts Intl",
    "Astute Networks", "Azanda Network Devices",
    "Dibcom", "Tekmos",
    "API NetWorks", "Bay Microsystems",
    "Firecron Ltd",
    "Resonext Communications",
    "Tachys Technologies", "Equator Technology",
    "Concept Computer",
    "SILCOM",
    "3Dlabs", "c't Magazine", "Sanera Systems",
    "Silicon Packets",
    "Viasystems Group", "Simtek",
    "Semicon Devices Singapore",
    "Satron Handelsges",
    "Improv Systems", "INDUSYS GmbH", "Corrent",
    "Infrant Technologies",
    "Ritek Corp", "empowerTel Networks",
    "Hypertec",
    "Cavium Networks",
    "PLX Technology", "Massana Design",
    "Intrinsity",
    "Valence Semiconductor",
    "Terawave Communications",
    "IceFyre Semiconductor", "Primarion",
    "Picochip Designs Ltd",
    "Silverback Systems",
    "Jade Star Technologies",
    "Pijnenburg Securealink",
    "TakeMS International AG",
    "Cambridge Silicon Radio",
    "Swissbit", "Nazomi Communications",
    "eWave System",
    "Rockwell Collins", "Picocel Co., Ltd.",
    "Alphamosaic Ltd",
    "Sandburst",
    "SiCon Video", "NanoAmp Solutions",
    "Ericsson Technology",
    "PrairieComm",
    "Mitac International", "Layer N Networks",
    "MtekVision",
    "Allegro Networks",
    "Marvell Semiconductors",
    "Netergy Microelectronic", "NVIDIA",
    "Internet Machines",
    "Peak Electronics",
    "Litchfield Communication",
    "Accton Technology",
    "Teradiant Networks",
    "Europe Technologies", "Cortina Systems",
    "RAM Components",
    "Raqia Networks",
    "ClearSpeed", "Matsushita Battery",
    "Xelerated",
    "SimpleTech",
    "Utron Technology", "Astec International",
    "AVM gmbH",
    "Redux Communications",
    "Dot Hill Systems", "TeraChip"
};

static const char *vendors5[] = { "T-RAM Incorporated",
    "Innovics Wireless", "Teknovus", "KeyEye Communications",
    "Runcom Technologies", "RedSwitch", "Dotcast",
    "Silicon Mountain Memory",
    "Signia Technologies", "Pixim", "Galazar Networks",
    "White Electronic Designs",
    "Patriot Scientific", "Neoaxiom Corporation", "3Y Power Technology",
    "Europe Technologies",
    "Potentia Power Systems", "C-guys Incorporated",
    "Digital Communications Technology Incorporated",
    "Silicon-Based Technology",
    "Fulcrum Microsystems", "Positivo Informatica Ltd",
    "XIOtech Corporation", "PortalPlayer",
    "Zhiying Software", "Direct2Data", "Phonex Broadband",
    "Skyworks Solutions",
    "Entropic Communications", "Pacific Force Technology", "Zensys A/S",
    "Legend Silicon Corp.",
    "sci-worx GmbH", "SMSC (former Oasis Silicon Systems)",
    "Renesas Technology", "Raza Microelectronics",
    "Phyworks", "MediaTek", "Non-cents Productions", "US Modular",
    "Wintegra Ltd", "Mathstar", "StarCore", "Oplus Technologies",
    "Mindspeed", "Just Young Computer", "Radia Communications", "OCZ",
    "Emuzed", "LOGIC Devices", "Inphi Corporation", "Quake Technologies",
    "Vixel", "SolusTek", "Kongsberg Maritime", "Faraday Technology",
    "Altium Ltd.", "Insyte", "ARM Ltd.", "DigiVision",
    "Vativ Technologies", "Endicott Interconnect Technologies", "Pericom",
    "Bandspeed",
    "LeWiz Communications", "CPU Technology", "Ramaxel Technology",
    "DSP Group",
    "Axis Communications", "Legacy Electronics", "Chrontel",
    "Powerchip Semiconductor",
    "MobilEye Technologies", "Excel Semiconductor", "A-DATA Technology",
    "VirtualDigm",
    "G Skill Intl", "Quanta Computer", "Yield Microelectronics",
    "Afa Technologies",
    "KINGBOX Technology Co. Ltd.", "Ceva", "iStor Networks",
    "Advance Modules",
    "Microsoft", "Open-Silicon", "Goal Semiconductor",
    "ARC International",
    "Simmtec", "Metanoia", "Key Stream", "Lowrance Electronics",
    "Adimos", "SiGe Semiconductor", "Fodus Communications",
    "Credence Systems Corp.",
    "Genesis Microchip Inc.", "Vihana, Inc.", "WIS Technologies",
    "GateChange Technologies",
    "High Density Devices AS", "Synopsys", "Gigaram",
    "Enigma Semiconductor Inc.",
    "Century Micro Inc.", "Icera Semiconductor",
    "Mediaworks Integrated Systems", "O'Neil Product Development",
    "Supreme Top Technology Ltd.", "MicroDisplay Corporation",
    "Team Group Inc.", "Sinett Corporation",
    "Toshiba Corporation", "Tensilica", "SiRF Technology", "Bacoc Inc.",
    "SMaL Camera Technologies", "Thomson SC", "Airgo Networks",
    "Wisair Ltd.",
    "SigmaTel", "Arkados", "Compete IT gmbH Co. KG",
    "Eudar Technology Inc.",
    "Focus Enhancements", "Xyratex"
};

static const char *vendors6[] = { "Specular Networks",
    "Patriot Memory",
    "U-Chip Technology Corp.",
    "Silicon Optix",
    "Greenfield Networks",
    "CompuRAM GmbH", "Stargen, Inc.",
    "NetCell Corporation",
    "Excalibrus Technologies Ltd",
    "SCM Microsystems",
    "Xsigo Systems, Inc.",
    "CHIPS & Systems Inc",
    "Tier 1 Multichip Solutions",
    "CWRL Labs", "Teradici",
    "Gigaram, Inc.",
    "g2 Microsystems",
    "PowerFlash Semiconductor",
    "P.A. Semi, Inc.",
    "NovaTech Solutions, S.A.",
    "c2 Microsystems, Inc.",
    "Level5 Networks",
    "COS Memory AG",
    "Innovasic Semiconductor",
    "02IC Co. Ltd", "Tabula, Inc.",
    "Crucial Technology",
    "Chelsio Communications",
    "Solarflare Communications",
    "Xambala Inc.", "EADS Astrium",
    "ATO Semicon Co. Ltd.",
    "Imaging Works, Inc.",
    "Astute Networks, Inc.", "Tzero",
    "Emulex",
    "Power-One", "Pulse~LINK Inc.",
    "Hon Hai Precision Industry",
    "White Rock Networks Inc.",
    "Telegent Systems USA, Inc.",
    "Atrua Technologies, Inc.",
    "Acbel Polytech Inc.",
    "eRide Inc.",
    "ULi Electronics Inc.",
    "Magnum Semiconductor Inc.",
    "neoOne Technology, Inc.",
    "Connex Technology, Inc.",
    "Stream Processors, Inc.",
    "Focus Enhancements",
    "Telecis Wireless, Inc.",
    "uNav Microelectronics",
    "Tarari, Inc.", "Ambric, Inc.",
    "Newport Media, Inc.", "VMTS",
    "Enuclia Semiconductor, Inc.",
    "Virtium Technology Inc.",
    "Solid State System Co., Ltd.",
    "Kian Tech LLC",
    "Artimi",
    "Power Quotient International",
    "Avago Technologies",
    "ADTechnology", "Sigma Designs",
    "SiCortex, Inc.",
    "Ventura Technology Group",
    "eASIC", "M.H.S. SAS",
    "Micro Star International",
    "Rapport Inc.",
    "Makway International",
    "Broad Reach Engineering Co.",
    "Semiconductor Mfg Intl Corp",
    "SiConnect", "FCI USA Inc.",
    "Validity Sensors",
    "Coney Technology Co. Ltd.",
    "Spans Logic", "Neterion Inc.",
    "Qimonda",
    "New Japan Radio Co. Ltd.",
    "Velogix", "Montalvo Systems",
    "iVivity Inc.",
    "Walton Chaintech",
    "AENEON",
    "Lorom Industrial Co. Ltd.",
    "Radiospire Networks",
    "Sensio Technologies, Inc.",
    "Nethra Imaging",
    "Hexon Technology Pte Ltd",
    "CompuStocx (CSX)",
    "Methode Electronics, Inc.",
    "Connect One Ltd.",
    "Opulan Technologies",
    "Septentrio NV",
    "Goldenmars Technology Inc.",
    "Kreton Corporation",
    "Cochlear Ltd.",
    "Altair Semiconductor",
    "NetEffect, Inc.",
    "Spansion, Inc.",
    "Taiwan Semiconductor Mfg",
    "Emphany Systems Inc.",
    "ApaceWave Technologies",
    "Mobilygen Corporation", "Tego",
    "Cswitch Corporation",
    "Haier (Beijing) IC Design Co.",
    "MetaRAM",
    "Axel Electronics Co. Ltd.",
    "Tilera Corporation",
    "Aquantia",
    "Vivace Semiconductor",
    "Redpine Signals", "Octalica",
    "InterDigital Communications",
    "Avant Technology",
    "Asrock, Inc.", "Availink",
    "Quartics, Inc.",
    "Element CXI",
    "Innovaciones Microelectronicas",
    "VeriSilicon Microelectronics",
    "W5 Networks"
};

static const char *vendors7[] = { "MOVEKING", "Mavrix Technology, Inc.",
    "CellGuide Ltd.", "Faraday Technology",
    "Diablo Technologies, Inc.", "Jennic", "Octasic",
    "Molex Incorporated",
    "3Leaf Networks",
    "Bright Micron Technology", "Netxen", "NextWave Broadband Inc.",
    "DisplayLink", "ZMOS Technology",
    "Tec-Hill", "Multigig, Inc.", "Amimon", "Euphonic Technologies, Inc.",
    "BRN Phoenix",
    "InSilica", "Ember Corporation", "Avexir Technologies Corporation",
    "Echelon Corporation",
    "Edgewater Computer Systems", "XMOS Semiconductor Ltd.",
    "GENUSION, Inc.", "Memory Corp NV",
    "SiliconBlue Technologies", "Rambus Inc."
};

static const char **vendors[7] = { vendors1, vendors2, vendors3, vendors4, vendors5, vendors6,
    vendors7
};

/*
 * We consider that no data was written to this area of the SPD EEPROM if
 * all bytes read 0x00 or all bytes read 0xff
 */
static int spd_written(unsigned char *bytes, int len)
{
    do {
	if (*bytes == 0x00 || *bytes == 0xFF)
	    return 1;
    } while (--len && bytes++);

    return 0;
}

static int parity(int value)
{
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value &= 0xf;

    return (0x6996 >> value) & 1;
}

static void decode_sdr_module_size(unsigned char *bytes, int *size)
{
    int i, k = 0;

    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    if (bytes[5] <= 8 && bytes[17] <= 8) {
	k = bytes[5] * bytes[17];
    }

    if (i > 0 && i <= 12 && k > 0) {
	if (size) {
	    *size = (1 << i) * k;
	}
    } else {
	if (size) {
	    *size = -1;
	}
    }
}

static void decode_sdr_module_timings(unsigned char *bytes, float *tcl, float *trcd, float *trp, float *tras)
{
    float cas[3], ctime;
    int i, j;

    for (i = 0, j = 0; j < 7; j++) {
	if (bytes[18] & 1 << j) {
	    cas[i++] = j + 1;
	}
    }

    ctime = (bytes[9] >> 4 + bytes[9] & 0xf) * 0.1;

    if (trcd) {
	*trcd = ceil(bytes[29] / ctime);
    }
    if (trp) {
	*trp = ceil(bytes[27] / ctime);
    }
    if (tras) {
	*tras = ceil(bytes[30] / ctime);
    }
    if (tcl) {
	*tcl = cas[i];
    }
}

static void decode_sdr_module_row_address_bits(unsigned char *bytes, char **bits)
{
    char *temp;

    switch (bytes[3]) {
    case 0:
	temp = "Undefined";
	break;
    case 1:
	temp = "1/16";
	break;
    case 2:
	temp = "2/27";
	break;
    case 3:
	temp = "3/18";
	break;
    default:
	/* printf("%d\n", bytes[3]); */
	temp = "Unknown";
    }

    if (bits) {
	*bits = temp;
    }
}

static void decode_sdr_module_col_address_bits(unsigned char *bytes, char **bits)
{
    char *temp;

    switch (bytes[4]) {
    case 0:
	temp = "Undefined";
	break;
    case 1:
	temp = "1/16";
	break;
    case 2:
	temp = "2/17";
	break;
    case 3:
	temp = "3/18";
	break;
    default:
	/*printf("%d\n", bytes[4]); */
	temp = "Unknown";
    }

    if (bits) {
	*bits = temp;
    }
}

static void decode_sdr_module_number_of_rows(unsigned char *bytes, int *rows)
{
    if (rows) {
	*rows = bytes[5];
    }
}

static void decode_sdr_module_data_with(unsigned char *bytes, int *width)
{
    if (width) {
	if (bytes[7] > 1) {
	    *width = 0;
	} else {
	    *width = (bytes[7] * 0xff) + bytes[6];
	}
    }
}

static void decode_sdr_module_interface_signal_levels(unsigned char *bytes, char **signal_levels)
{
    char *temp;

    switch (bytes[8]) {
    case 0:
	temp = "5.0 Volt/TTL";
	break;
    case 1:
	temp = "LVTTL";
	break;
    case 2:
	temp = "HSTL 1.5";
	break;
    case 3:
	temp = "SSTL 3.3";
	break;
    case 4:
	temp = "SSTL 2.5";
	break;
    case 255:
	temp = "New Table";
	break;
    default:
	temp = "Undefined";
    }

    if (signal_levels) {
	*signal_levels = temp;
    }
}

static void decode_sdr_module_configuration_type(unsigned char *bytes, char **module_config_type)
{
    char *temp;

    switch (bytes[11]) {
    case 0:
	temp = "No parity";
	break;
    case 1:
	temp = "Parity";
	break;
    case 2:
	temp = "ECC";
	break;
    default:
	temp = "Undefined";
    }

    if (module_config_type) {
	*module_config_type = temp;
    }
}

static void decode_sdr_module_refresh_type(unsigned char *bytes, char **refresh_type)
{
    char *temp;

    if (bytes[12] & 0x80) {
	temp = "Self refreshing";
    } else {
	temp = "Not self refreshing";
    }

    if (refresh_type) {
	*refresh_type = temp;
    }
}

static void decode_sdr_module_refresh_rate(unsigned char *bytes, char **refresh_rate)
{
    char *temp;

    switch (bytes[12] & 0x7f) {
    case 0:
	temp = "Normal (15.625us)";
	break;
    case 1:
	temp = "Reduced (3.9us)";
	break;
    case 2:
	temp = "Reduced (7.8us)";
	break;
    case 3:
	temp = "Extended (31.3us)";
	break;
    case 4:
	temp = "Extended (62.5us)";
	break;
    case 5:
	temp = "Extended (125us)";
	break;
    default:
	temp = "Undefined";
    }

    if (refresh_rate) {
	*refresh_rate = temp;
    }
}

static gchar *decode_sdr_sdram(unsigned char *bytes, int *size)
{
    int rows, data_width;
    float tcl, trcd, trp, tras;
    char *row_address_bits, *col_address_bits, *signal_level;
    char *module_config_type, *refresh_type, *refresh_rate;

    decode_sdr_module_size(bytes, size);
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

    return g_strdup_printf("[Module Information]\n"
			   "Module type=SDR\n"
			   "SPD revision=%d\n"
			   "Row address bits=%s\n"
			   "Column address bits=%s\n"
			   "Number of rows=%d\n"
			   "Data width=%d bits\n"
			   "Interface signal levels=%s\n"
			   "Configuration type=%s\n"
			   "Refresh=%s (%s)\n"
			   "[Timings]\n"
			   "tCL=%.2f\n"
			   "tRCD=%.2f\n"
			   "tRP=%.2f\n"
			   "tRAS=%.2f\n",
			   bytes[62],
			   row_address_bits, col_address_bits, rows,
			   data_width, signal_level, module_config_type,
			   refresh_type, refresh_rate, tcl, trcd, trp, tras);
}

static void decode_ddr_module_speed(unsigned char *bytes, float *ddrclk, int *pcclk)
{
    float temp, clk;
    int tbits, pc;

    temp = (bytes[9] >> 4) + (bytes[9] & 0xf) * 0.1;
    clk = 2 * (1000 / temp);
    tbits = (bytes[7] * 256) + bytes[6];

    if (bytes[11] == 2 || bytes[11] == 1) {
	tbits -= 8;
    }

    pc = clk * tbits / 8;
    if (pc % 100 > 50) {
	pc += 100;
    }
    pc -= pc % 100;

    if (ddrclk)
	*ddrclk = (int) clk;

    if (pcclk)
	*pcclk = pc;
}

static void decode_ddr_module_size(unsigned char *bytes, int *size)
{
    int i, k;

    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    k = (bytes[5] <= 8 && bytes[17] <= 8) ? bytes[5] * bytes[17] : 0;

    if (i > 0 && i <= 12 && k > 0) {
	if (size) {
	    *size = (1 << i) * k;
	}
    } else {
	if (size) {
	    *size = -1;
	}
    }
}

static void *decode_ddr_module_timings(unsigned char *bytes, float *tcl, float *trcd, float *trp, float *tras)
{
    float ctime;
    float highest_cas = 0;
    int i;

    for (i = 0; i < 7; i++) {
	if (bytes[18] & (1 << i)) {
	    highest_cas = 1 + i * 0.5f;
	}
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

    if (tcl) {
	*tcl = highest_cas;
    }
}

static gchar *decode_ddr_sdram(unsigned char *bytes, int *size)
{
    float ddr_clock;
    float tcl, trcd, trp, tras;
    int pc_speed;

    decode_ddr_module_speed(bytes, &ddr_clock, &pc_speed);
    decode_ddr_module_size(bytes, size);
    decode_ddr_module_timings(bytes, &tcl, &trcd, &trp, &tras);

    return g_strdup_printf("[Module Information]\n"
			   "Module type=DDR %.2fMHz (PC%d)\n"
			   "SPD revision=%d.%d\n"
			   "[Timings]\n"
			   "tCL=%.2f\n"
			   "tRCD=%.2f\n"
			   "tRP=%.2f\n"
			   "tRAS=%.2f\n", ddr_clock, pc_speed, bytes[62] >> 4, bytes[62] & 0xf, tcl, trcd, trp, tras);
}

static float decode_ddr2_module_ctime(unsigned char byte)
{
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

static void decode_ddr2_module_speed(unsigned char *bytes, float *ddr_clock, int *pc2_speed)
{
    float ctime;
    float ddrclk;
    int tbits, pcclk;

    ctime = decode_ddr2_module_ctime(bytes[9]);
    ddrclk = 2 * (1000 / ctime);

    tbits = (bytes[7] * 256) + bytes[6];
    if (bytes[11] & 0x03) {
	tbits -= 8;
    }

    pcclk = ddrclk * tbits / 8;
    pcclk -= pcclk % 100;

    if (ddr_clock) {
	*ddr_clock = (int) ddrclk;
    }
    if (pc2_speed) {
	*pc2_speed = pcclk;
    }
}

static void decode_ddr2_module_size(unsigned char *bytes, int *size)
{
    int i, k;

    i = (bytes[3] & 0x0f) + (bytes[4] & 0x0f) - 17;
    k = ((bytes[5] & 0x7) + 1) * bytes[17];

    if (i > 0 && i <= 12 && k > 0) {
	if (*size) {
	    *size = ((1 << i) * k);
	}
    } else {
	if (*size) {
	    *size = 0;
	}
    }
}

static void decode_ddr2_module_timings(unsigned char *bytes, float *trcd, float *trp, float *tras, float *tcl)
{
    float ctime;
    float highest_cas = 0;
    int i;

    for (i = 0; i < 7; i++) {
	if (bytes[18] & (1 << i)) {
	    highest_cas = i;
	}
    }

    ctime = decode_ddr2_module_ctime(bytes[9]);

    if (trcd) {
	*trcd = ceil(((bytes[29] >> 2) + ((bytes[29] & 3) * 0.25)) / ctime);
    }

    if (trp) {
	*trp = ceil(((bytes[27] >> 2) + ((bytes[27] & 3) * 0.25)) / ctime);
    }

    if (tras) {
	*tras = ceil(bytes[30] / ctime);
    }

    if (tcl) {
	*tcl = highest_cas;
    }
}

static gchar *decode_ddr2_sdram(unsigned char *bytes, int *size)
{
    float ddr_clock;
    float trcd, trp, tras, tcl;
    int pc2_speed;

    decode_ddr2_module_speed(bytes, &ddr_clock, &pc2_speed);
    decode_ddr2_module_size(bytes, size);
    decode_ddr2_module_timings(bytes, &trcd, &trp, &tras, &tcl);

    return g_strdup_printf("[Module Information]\n"
			   "Module type=DDR2 %.2f MHz (PC2-%d)\n"
			   "SPD revision=%d.%d\n"
			   "[Timings]\n"
			   "tCL=%.2f\n"
			   "tRCD=%.2f\n"
			   "tRP=%.2f\n"
			   "tRAS=%.2f\n", ddr_clock, pc2_speed, bytes[62] >> 4, bytes[62] & 0xf, tcl, trcd, trp, tras);
}

static void decode_ddr3_module_speed(unsigned char *bytes, float *ddr_clock, int *pc3_speed)
{
    float ctime;
    float ddrclk;
    int tbits, pcclk;
    float mtb = 0.125;

    if (bytes[10] == 1 && bytes[11] == 8)
	mtb = 0.125;
    if (bytes[10] == 1 && bytes[11] == 15)
	mtb = 0.0625;
    ctime = mtb * bytes[12];

    ddrclk = 2 * (1000 / ctime);

    tbits = 64;
    switch (bytes[8]) {
    case 1:
	tbits = 16;
	break;
    case 4:
	tbits = 32;
	break;
    case 3:
    case 0xb:
	tbits = 64;
	break;
    }

    pcclk = ddrclk * tbits / 8;
    pcclk -= pcclk % 100;

    if (ddr_clock) {
	*ddr_clock = (int) ddrclk;
    }
    if (pc3_speed) {
	*pc3_speed = pcclk;
    }
}

static void decode_ddr3_module_size(unsigned char *bytes, int *size)
{
    *size = 512 << bytes[4];
}

static void decode_ddr3_module_timings(unsigned char *bytes, float *trcd, float *trp, float *tras, float *tcl)
{
    float ctime;
    float highest_cas = 0;
    int i;
    float mtb = 0.125;

    if (bytes[10] == 1 && bytes[11] == 8)
	mtb = 0.125;
    if (bytes[10] == 1 && bytes[11] == 15)
	mtb = 0.0625;
    ctime = mtb * bytes[12];

    switch (bytes[14]) {
    case 6:
	highest_cas = 5;
	break;
    case 4:
	highest_cas = 6;
	break;
    case 0xc:
	highest_cas = 7;
	break;
    case 0x1e:
	highest_cas = 8;
	break;
    }
    if (trcd) {
	*trcd = bytes[18] * mtb;
    }

    if (trp) {
	*trp = bytes[20] * mtb;
    }

    if (tras) {
	*tras = (bytes[22] + bytes[21] & 0xf) * mtb;
    }

    if (tcl) {
	*tcl = highest_cas;
    }
}

static void decode_ddr3_module_type(unsigned char *bytes, const char **type)
{
    switch (bytes[3]) {
    case 0x00:
        *type = "Undefined";
        break;
    case 0x01:
        *type = "RDIMM (Registered Long DIMM)";
        break;
    case 0x02:
        *type = "UDIMM (Unbuffered Long DIMM)";
        break;
    case 0x03:
        *type = "SODIMM (Small Outline DIMM)";
        break;
    default:
        *type = "Unknown";
    }
}

static gchar *decode_ddr3_sdram(unsigned char *bytes, int *size)
{
    float ddr_clock;
    float trcd, trp, tras, tcl;
    int pc3_speed;
    const char *type;

    decode_ddr3_module_speed(bytes, &ddr_clock, &pc3_speed);
    decode_ddr3_module_size(bytes, size);
    decode_ddr3_module_timings(bytes, &trcd, &trp, &tras, &tcl);
    decode_ddr3_module_type(bytes, &type);

    return g_strdup_printf("[Module Information]\n"
			   "Module type=DDR3 %.2f MHz (PC3-%d)\n"
			   "SPD revision=%d.%d\n"
			   "Type=%s\n"
			   "[Timings]\n"
			   "tCL=%.2f\n"
			   "tRCD=%.3fns\n"
			   "tRP=%.3fns\n"
			   "tRAS=%.3fns\n",
			   ddr_clock, pc3_speed,
			   bytes[1] >> 4, bytes[1] & 0xf,
			   type,
			   tcl,
			   trcd,
			   trp,
			   tras);
}

static void decode_ddr3_part_number(unsigned char *bytes, char *part_number)
{
    int i;
    if (part_number) {
	for (i = 128; i <= 145; i++)
	    *part_number++ = bytes[i];
	*part_number = '\0';
    }
}

static void decode_ddr3_manufacturer(unsigned char *bytes, char **manufacturer)
{
    char *out = "Unknown";

  end:
    if (manufacturer) {
	*manufacturer = out;
    }
}

static void decode_module_manufacturer(unsigned char *bytes, char **manufacturer)
{
    char *out = "Unknown";
    unsigned char first;
    int ai = 0;
    int len = 8;
    unsigned char *initial = bytes;

    if (!spd_written(bytes, 8)) {
	out = "Undefined";
	goto end;
    }

    do {
	ai++;
    } while ((--len && (*bytes++ == 0x7f)));
    first = *--bytes;

    if (ai == 0) {
	out = "Invalid";
	goto end;
    }

    if (parity(first) != 1) {
	out = "Invalid";
	goto end;
    }

    out = (char *) vendors[ai - 1][(first & 0x7f) - 1];

  end:
    if (manufacturer) {
	*manufacturer = out;
    }
}

static void decode_module_part_number(unsigned char *bytes, char *part_number)
{
    if (part_number) {
	bytes += 8 + 64;

	while (*bytes++ && *bytes >= 32 && *bytes < 127) {
	    *part_number++ = *bytes;
	}
	*part_number = '\0';
    }
}

static int decode_ram_type(unsigned char *bytes)
{
    if (bytes[0] < 4) {
	switch (bytes[2]) {
	case 1:
	    return DIRECT_RAMBUS;
	case 17:
	    return RAMBUS;
	}
    } else {
	switch (bytes[2]) {
	case 1:
	    return FPM_DRAM;
	case 2:
	    return EDO;
	case 3:
	    return PIPELINED_NIBBLE;
	case 4:
	    return SDR_SDRAM;
	case 5:
	    return MULTIPLEXED_ROM;
	case 6:
	    return DDR_SGRAM;
	case 7:
	    return DDR_SDRAM;
	case 8:
	    return DDR2_SDRAM;
	case 11:
	    return DDR3_SDRAM;
	}
    }

    return UNKNOWN;
}

static void read_spd(char *spd_path, int offset, size_t size, int use_sysfs, unsigned char *bytes_out)
{
    if (use_sysfs) {
	FILE *spd;
	gchar *temp_path;

	temp_path = g_strdup_printf("%s/eeprom", spd_path);
	if ((spd = fopen(temp_path, "rb"))) {
	    fseek(spd, offset, SEEK_SET);
	    fread(bytes_out, 1, size, spd);
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
		fread(bytes_out + i * 16, 1, 16, spd);
		fclose(spd);
	    }

	    g_free(temp_path);
	}
    }
}

static gchar *decode_dimms(GSList * dimm_list, gboolean use_sysfs)
{
    GSList *dimm;
    GString *output;
    gint count = 0;

    output = g_string_new("");

    for (dimm = dimm_list; dimm; dimm = dimm->next, count++) {
	gchar *spd_path = (gchar *) dimm->data;
	gchar *manufacturer;
	gchar *detailed_info;
	gchar *moreinfo_key;
	gchar part_number[32];
	unsigned char bytes[256];
	int module_size;
	RamType ram_type;

	shell_status_pulse();

	read_spd(spd_path, 0, 256, use_sysfs, bytes);
	ram_type = decode_ram_type(bytes);

	switch (ram_type) {
	case DDR2_SDRAM:
	    detailed_info = decode_ddr2_sdram(bytes, &module_size);
	    decode_module_part_number(bytes, part_number);
	    decode_module_manufacturer(bytes + 64, &manufacturer);
	    break;
	case DDR3_SDRAM:
	    detailed_info = decode_ddr3_sdram(bytes, &module_size);
	    decode_ddr3_part_number(bytes, part_number);
	    decode_ddr3_manufacturer(bytes, &manufacturer);
	    break;
	case DDR_SDRAM:
	    detailed_info = decode_ddr_sdram(bytes, &module_size);
	    decode_module_part_number(bytes, part_number);
	    decode_module_manufacturer(bytes + 64, &manufacturer);
	    break;
	case SDR_SDRAM:
	    detailed_info = decode_sdr_sdram(bytes, &module_size);
	    decode_module_part_number(bytes, part_number);
	    decode_module_manufacturer(bytes + 64, &manufacturer);
	    break;
	default:
	    DEBUG("Unsupported EEPROM type: %s\n", ram_types[ram_type]);
	    continue;
	}



	gchar *key = g_strdup_printf("MEM%d", count);
	moreinfo_add_with_prefix("DEV", key, g_strdup(detailed_info));
	g_free(key);
	g_string_append_printf(output, "$MEM%d$%d=%s|%d MB|%s\n", count, count, part_number, module_size, manufacturer);

	g_free(spd_path);
	g_free(detailed_info);
    }

    return g_string_free(output, FALSE);
}

void scan_spd_do(void)
{
    GDir *dir = NULL;
    GSList *dimm_list = NULL;
    gboolean use_sysfs = FALSE;
    gchar *dir_entry;
    gchar *list;

    if (g_file_test("/sys/bus/i2c/drivers/eeprom", G_FILE_TEST_EXISTS)) {
	dir = g_dir_open("/sys/bus/i2c/drivers/eeprom", 0, NULL);
	use_sysfs = TRUE;
    } else if (g_file_test("/proc/sys/dev/sensors", G_FILE_TEST_EXISTS)) {
	dir = g_dir_open("/proc/sys/dev/sensors", 0, NULL);
    }

    if (!dir) {
	g_free(spd_info);
	if (!g_file_test("/sys/module/eeprom", G_FILE_TEST_EXISTS)) {
	    spd_info = g_strdup(_("[SPD]\n"
                "Please load the eeprom module to obtain information about memory SPD=\n"
                "[$ShellParam$]\n"
                "ReloadInterval=500\n"));
	} else {
	    spd_info = g_strdup(_("[SPD]\n" "Reading memory SPD not supported on this system=\n"));
	}

	return;
    }

    while ((dir_entry = (char *) g_dir_read_name(dir))) {
	if (use_sysfs && isdigit(dir_entry[0])) {
	    dimm_list = g_slist_prepend(dimm_list, g_strdup_printf("/sys/bus/i2c/drivers/eeprom/%s", dir_entry));
	} else if (g_str_has_prefix(dir_entry, "eeprom-")) {
	    dimm_list = g_slist_prepend(dimm_list, g_strdup_printf("/proc/sys/dev/sensors/%s", dir_entry));
	}
    }

    g_dir_close(dir);

    list = decode_dimms(dimm_list, use_sysfs);
    g_slist_free(dimm_list);

    g_free(spd_info);
    spd_info = g_strdup_printf("[%s]\n"
                   "%s\n"
                   "[$ShellParam$]\n"
                   "ViewType=1\n"
                   "ColumnTitle$TextValue=%s\n" /* Bank */
                   "ColumnTitle$Extra1=%s\n" /* Size */
                   "ColumnTitle$Extra2=%s\n" /* Manufacturer */
                   "ColumnTitle$Value=%s\n" /* Model */
                   "ShowColumnHeaders=true\n",
                   _("SPD"), list,
                   _("Bank"), _("Size"), _("Manufacturer"), _("Model") );
    g_free(list);
}
