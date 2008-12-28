/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    List of vendors based on GtkSysInfo (c) Pissens Sebastien.
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

#include <string.h>
#include <gtk/gtk.h>
#include "vendor.h"

static const Vendor vendors[] = {
    {"ATI", "ATI Technologies, Inc.", "www.ati.com"},
    {"nVidia", "NVIDIA Corporation", "www.nvidia.com"},
    {"3Com", "3Com", "www.3com.com"},
    {"Intel", "Intel Corp.", "www.intel.com"},
    {"Cirrus Logic", "Cirrus Logic", "www.cirrus.com"},
    {"VIA Technologies", "VIA Technologies", "www.via.com.tw"},
    {"VIA", "VIA Technologies", "www.via.com.tw"},
    {"hp", "Hewlett-Packard", "www.hp.com"},
    {"NEC Corporation", "NEC Coporation", "www.nec.com"},
    {"MAXTOR", "MAXTOR", "www.maxtor.com"},
    {"SAMSUNG", "SAMSUNG", "www.samsung.com"},
    {"PIONEER", "PIONEER", "www.pioneer-eur.com"},
    {"PLEXTOR", "PLEXTOR", "www.plextor.be"},
    {"Realtek Semiconductor", "Realtek", "www.realtek.com.tw"},
    {"TOSHIBA", "TOSHIBA", "www.toshiba.com"},
    {"LITE-ON", "LITE-ON", "www.liteonit.com"},
    {"WDC", "Western Digital", "www.wdc.com"},
    {"HL-DT-ST", "LG Electronics", "www.lge.com"},
    {"ST", "SEAGATE", "www.seagate.com"},
    {"Lexmark", "Lexmark", "www.lexmark.com"},
    {"_NEC", "NEC Corporation", "www.nec.com"},
    {"Creative Labs", "Creative Labs", "www.creative.com"},
    {"Brooktree", "Conexant", "www.brooktree.com"},
    {"Atheros", "Atheros Communications", "www.atheros.com"},
    {"MATSHITA", "Panasonic", "www.panasonic.com"},
    {"Silicon Image", "Silicon Image, Inc.", "www.siliconimage.com"},
    {"Silicon Integrated Image", "Silicon Image, Inc.", "www.siliconimage.com"},
    {"KYE", "KYE Systems Corp.", "www.genius-kye.com"},
    {"Broadcom", "Broadcom Corp.", "www.broadcom.com"},
    {"Apple", "Apple Computer, Inc.", "www.apple.com"},
    {"IBM", "IBM Corp.", "www.ibm.com"},
    {"Dell", "Dell Computer Corp.", "www.dell.com"},
    {"Logitech", "Logitech International SA", "www.logitech.com"},
    {"FUJITSU", "Fujitsu", "www.fujitsu.com"},
    {"CDU", "Sony", "www.sony.com"},
    {"SanDisk", "SanDisk", "www.sandisk.com"},
    {"ExcelStor", "ExcelStor Technology", "www.excelstor.com"},
    {"D-Link", "D-Link", "www.dlink.com.tw"},
    {"Giga-byte", "Gigabyte", "www.gigabyte.com.tw"},
    {"C-Media", "C-Media Electronics", "www.cmedia.com.tw"},
    {"Avermedia", "AVerMedia Technologies", "www.aver.com"},
    {"Philips", "Philips", "www.philips.com"},
    {"RaLink", "Ralink Technology", "www.ralinktech.com"},
    {"Siemens", "Siemens AG", "www.siemens.com"},
    {"HP", "Hewlett-Packard", "www.hp.com"},
    {"Hewlett-Packard", "Hewlett-Packard", "www.hp.com"},
    {"TEAC", "TEAC America Inc.", "www.teac.com"},
    {"Microsoft", "Microsoft Corp.", "www.microsoft.com"},
    {"Memorex", "Memorex Products, Inc.", "www.memorex.com"},
    {"eMPIA", "eMPIA Technology, Inc.", "www.empiatech.com.tw"},
    {"Canon", "Canon Inc.", "www.canon.com"},
    {"A4Tech", "A4tech Co., Ltd.", "www.a4tech.com"},
    {NULL, NULL, NULL},
};

const gchar *vendor_get_name(const gchar * id)
{
    int i;

    for (i = 0; vendors[i].id; i++) {
	if (strstr(id, vendors[i].id))
	    return vendors[i].name;
    }

    return id;
}

const gchar *vendor_get_url(const gchar * id)
{
    int i;

    for (i = 0; vendors[i].id; i++) {
	if (strstr(id, vendors[i].id))
	    return vendors[i].url;
    }

    return NULL;
}
