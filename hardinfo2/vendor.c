/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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
#include "syncmanager.h"
#include "config.h"
#include "hardinfo.h"

static const Vendor vendors[] = {
    {"ATI", "ATI Technologies", "www.ati.com"},
    {"nVidia", "NVIDIA", "www.nvidia.com"},
    {"3Com", "3Com", "www.3com.com"},
    {"Intel", "Intel", "www.intel.com"},
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
    {"Silicon Image", "Silicon Image", "www.siliconimage.com"},
    {"Silicon Integrated Image", "Silicon Image", "www.siliconimage.com"},
    {"KYE", "KYE Systems", "www.genius-kye.com"},
    {"Broadcom", "Broadcom", "www.broadcom.com"},
    {"Apple", "Apple", "www.apple.com"},
    {"IBM", "IBM", "www.ibm.com"},
    {"Dell", "Dell Computer", "www.dell.com"},
    {"Logitech", "Logitech International", "www.logitech.com"},
    {"FUJITSU", "Fujitsu", "www.fujitsu.com"},
    {"CDU", "Sony", "www.sony.com"},
    {"SanDisk", "SanDisk", "www.sandisk.com"},
    {"ExcelStor", "ExcelStor Technology", "www.excelstor.com"},
    {"D-Link", "D-Link", "www.dlink.com.tw"},
    {"Giga-byte", "Gigabyte Technology", "www.gigabyte.com.tw"},
    {"Gigabyte", "Gigabyte Technology", "www.gigabyte.com.tw"},
    {"C-Media", "C-Media Electronics", "www.cmedia.com.tw"},
    {"Avermedia", "AVerMedia Technologies", "www.aver.com"},
    {"Philips", "Philips", "www.philips.com"},
    {"RaLink", "Ralink Technology", "www.ralinktech.com"},
    {"Siemens", "Siemens AG", "www.siemens.com"},
    {"HP", "Hewlett-Packard", "www.hp.com"},
    {"Hewlett-Packard", "Hewlett-Packard", "www.hp.com"},
    {"TEAC", "TEAC America", "www.teac.com"},
    {"Microsoft", "Microsoft", "www.microsoft.com"},
    {"Memorex", "Memorex Products", "www.memorex.com"},
    {"eMPIA", "eMPIA Technology", "www.empiatech.com.tw"},
    {"Canon", "Canon", "www.canon.com"},
    {"A4Tech", "A4tech", "www.a4tech.com"},
    {"ALCOR", "Alcor", "www.alcor.org"},
    {"Vimicro", "Vimicro", "www.vimicro.com"},
    {"OTi", "Ours Technology", "www.oti.com.tw"},
    {"BENQ", "BenQ", "www.benq.com"},
    {"Acer", "Acer", "www.acer.com"},
    /* BIOS manufacturers */
    {"American Megatrends", "American Megatrends", "www.ami.com"},
    {"Award", "Award Software International", "www.award-bios.com"},
    {"Phoenix", "Phoenix Technologies", "www.phoenix.com"},
};

static GSList *vendor_list = NULL;

void vendor_init(void)
{
    gint i;
    gchar *path;
    static SyncEntry se = {
       .fancy_name = "Update vendor list",
       .name = "RecvVendorList",
       .save_to = "vendor.conf",
       .get_data = NULL
    };

    DEBUG("initializing vendor list");
    sync_manager_add_entry(&se);
    
    path = g_build_filename(g_get_home_dir(), ".hardinfo", "vendor.conf", NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
      DEBUG("local vendor.conf not found, trying system-wise");
      g_free(path);
      path = g_build_filename(params.path_data, "vendor.conf", NULL);
    }
    
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
      GKeyFile *vendors;
      gchar *tmp;
      gint num_vendors;
      
      DEBUG("loading vendor.conf");
      
      vendors = g_key_file_new();
      if (g_key_file_load_from_file(vendors, path, 0, NULL)) {
        num_vendors = g_key_file_get_integer(vendors, "vendors", "number", NULL);
        
        for (i = num_vendors--; i >= 0; i--) {
          Vendor *v = g_new0(Vendor, 1);
          
          tmp = g_strdup_printf("vendor%d", i);
          
          v->id   = g_key_file_get_string(vendors, tmp, "id", NULL);
          v->name = g_key_file_get_string(vendors, tmp, "name", NULL);
          v->url  = g_key_file_get_string(vendors, tmp, "url", NULL);
          
          vendor_list = g_slist_prepend(vendor_list, v);
          
          g_free(tmp);
        }
      }
      
      g_key_file_free(vendors);
    } else {
      DEBUG("system-wise vendor.conf not found, using internal database");
      
      for (i = G_N_ELEMENTS(vendors) - 1; i >= 0; i--) {
        vendor_list = g_slist_prepend(vendor_list, (gpointer) &vendors[i]);
      }
    } 

    g_free(path);
}

const gchar *vendor_get_name(const gchar * id)
{
    GSList *vendor;
    int i;
    
    if (!id) {
      return NULL;
    }
    
    for (vendor = vendor_list; vendor; vendor = vendor->next) {
      Vendor *v = (Vendor *)vendor->data;
      
      if (v && v->id && strstr(id, v->id)) {
        return g_strdup(v->name);
      }
    }

    return id;
}

const gchar *vendor_get_url(const gchar * id)
{
    GSList *vendor;
    int i;

    if (!id) {
      return NULL;
    }
    
    for (vendor = vendor_list; vendor; vendor = vendor->next) {
      Vendor *v = (Vendor *)vendor->data;
      
      if (v && v->id && strstr(id, v->id)) {
        return g_strdup(v->url);
      }
    }

    return NULL;
}
