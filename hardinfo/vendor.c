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

/* { match_string, name, url } */
static Vendor vendors[] = {
    { "ASUS", 0, "ASUS", "www.asus.com" }, /* "ASUSTek" is common */
    { "ATI", 1, "ATI Technologies", "www.ati.com" },
    { "nVidia", 0, "nVidia", "www.nvidia.com" },
    { "3Com", 0, "3Com", "www.3com.com" },
    { "Intel", 0, "Intel", "www.intel.com" },
    { "Cirrus Logic", 0, "Cirrus Logic", "www.cirrus.com" },
    { "VIA", 1, "VIA Technologies", "www.via.com.tw" },
    { "NEC", 1, "NEC Corporation", "www.nec.com"},
    { "Realtek", 0, "Realtek", "www.realtek.com.tw"},
    { "Toshiba", 0, "Toshiba", "www.toshiba.com"},
    { "LITE-ON", 1, "LITE-ON", "www.liteonit.com"},
    { "Maxtor", 0, "Maxtor", "www.maxtor.com"},
    { "Samsung", 0, "Samsung", "www.samsung.com"},
    { "Pioneer", 0, "Pioneer", "www.pioneer-eur.com"},
    { "Plextor", 0, "Plextor", "www.plextor.be"},
    { "WDC", 1, "Western Digital", "www.wdc.com"},
    { "HL-DT-ST", 1, "LG Electronics", "www.lge.com"},
    { "Lexmark", 0, "Lexmark", "www.lexmark.com"},
    { "Creative Labs", 0, "Creative Labs", "www.creative.com"},
    { "Brooktree", 0, "Conexant", "www.brooktree.com"},
    { "Atheros", 0, "Atheros Communications", "www.atheros.com"},
    { "MATSHITA", 0, "Panasonic", "www.panasonic.com"},
    { "Silicon Image", 0, "Silicon Image", "www.siliconimage.com"},
    { "Silicon Integrated Image", 0, "Silicon Image", "www.siliconimage.com"},
    { "Broadcom", 0, "Broadcom", "www.broadcom.com"},
    { "Apple", 0, "Apple", "www.apple.com"},
    { "IBM", 1, "IBM", "www.ibm.com"},
    { "Dell", 0, "Dell Computer", "www.dell.com"},
    { "Logitech", 0, "Logitech International", "www.logitech.com"},
    { "FUJITSU", 0, "Fujitsu", "www.fujitsu.com"},
    { "CDU", 1, "Sony", "www.sony.com"},
    { "SanDisk", 0, "SanDisk", "www.sandisk.com"},
    { "ExcelStor", 0, "ExcelStor Technology", "www.excelstor.com"},
    { "D-Link", 0, "D-Link", "www.dlink.com.tw"},
    { "Giga-byte", 0, "Gigabyte Technology", "www.gigabyte.com.tw"},
    { "Gigabyte", 0, "Gigabyte Technology", "www.gigabyte.com.tw"},
    { "C-Media", 0, "C-Media Electronics", "www.cmedia.com.tw"},
    { "Avermedia", 0, "AVerMedia Technologies", "www.aver.com"},
    { "Philips", 0, "Philips", "www.philips.com"},
    { "RaLink", 0, "Ralink Technology", "www.ralinktech.com"},
    { "Siemens", 0, "Siemens AG", "www.siemens.com"},
    { "Hewlett-Packard", 0, "Hewlett-Packard", "www.hp.com"},
    { "HP", 1, "Hewlett-Packard", "www.hp.com"},
    { "TEAC", 1, "TEAC America", "www.teac.com"},
    { "Microsoft", 0, "Microsoft", "www.microsoft.com"},
    {" Memorex", 0, "Memorex Products", "www.memorex.com"},
    { "eMPIA", 1, "eMPIA Technology", "www.empiatech.com.tw"},
    { "Canon", 0, "Canon", "www.canon.com"},
    { "A4Tech", 0, "A4tech", "www.a4tech.com"},
    { "ALCOR", 0, "Alcor", "www.alcor.org"},
    { "Vimicro", 0, "Vimicro", "www.vimicro.com"},
    { "OTi", 1, "Ours Technology", "www.oti.com.tw"},
    { "BENQ", 0, "BenQ", "www.benq.com"},
    { "Acer", 0, "Acer", "www.acer.com"},
    { "QUANTUM", 0, "Quantum", "www.quantum.com"},
    { "Kingston", 0, "Kingston", "www.kingston.com"},
    { "Chicony", 0, "Chicony", "www.chicony.com.tw"},
    { "Genius", 0, "Genius", "www.genius.ru"},
    { "KYE", 1, "KYE Systems", "www.genius-kye.com"},
    { "ST", 1, "SEAGATE", "www.seagate.com"},

    /* BIOS manufacturers */
    { "American Megatrends", 0, "American Megatrends", "www.ami.com"},
    { "Award", 0, "Award Software International", "www.award-bios.com"},
    { "Phoenix", 0, "Phoenix Technologies", "www.phoenix.com"},
    /* x86 vendor strings */
    { "AMDisbetter!", 0, "Advanced Micro Devices", "www.amd.com" },
    { "AuthenticAMD", 0, "Advanced Micro Devices", "www.amd.com" },
    { "CentaurHauls", 0, "VIA (formerly Centaur Technology)", "www.via.tw" },
    { "CyrixInstead", 0, "Cyrix", "" },
    { "GenuineIntel", 0, "Intel", "www.intel.com" },
    { "TransmetaCPU", 0, "Transmeta", "" },
    { "GenuineTMx86", 0, "Transmeta", "" },
    { "Geode by NSC", 0, "National Semiconductor", "" },
    { "NexGenDriven", 0, "NexGen", "" },
    { "RiseRiseRise", 0, "Rise Technology", "" },
    { "SiS SiS SiS", 0, "Silicon Integrated Systems", "" },
    { "UMC UMC UMC", 0, "United Microelectronics Corporation", "" },
    { "VIA VIA VIA", 0, "VIA", "www.via.tw" },
    { "Vortex86 SoC", 0, "DMP Electronics", "" },
    /* x86 VM vendor strings */
    { "KVMKVMKVM", 0, "KVM", "" },
    { "Microsoft Hv", 0, "Microsoft Hyper-V", "www.microsoft.com" },
    { "lrpepyh vr", 0, "Parallels", "" },
    { "VMwareVMware", 0, "VMware", "" },
    { "XenVMMXenVMM", 0, "Xen HVM", "" },
};

static GSList *vendor_list = NULL;

/* sort the vendor list by length of match_string,
 * LONGEST first */
gint vendor_sort (gconstpointer a, gconstpointer b) {
    int la = 0, lb = 0;
    if (a) la = strlen(a);
    if (b) lb = strlen(b);
    if (a == b) return 0;
    if (a > b) return -1;
    return 1;
}

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

      DEBUG("loading %s", path);

      vendors = g_key_file_new();
      if (g_key_file_load_from_file(vendors, path, 0, NULL)) {
        num_vendors = g_key_file_get_integer(vendors, "vendors", "number", NULL);

        for (i = num_vendors - 1; i >= 0; i--) {
          Vendor *v = g_new0(Vendor, 1);

          tmp = g_strdup_printf("vendor%d", i);

          v->match_string = g_key_file_get_string(vendors, tmp, "match_string", NULL);
          if (v->match_string == NULL) {
              /* try old name */
              v->match_string = g_key_file_get_string(vendors, tmp, "id", NULL);
          }
          v->match_case = g_key_file_get_integer(vendors, tmp, "match_case", NULL);
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

    /* sort the vendor list by length of match string so that short strings are
     * less likely to incorrectly match.
     * example: ST matches ASUSTeK but SEAGATE is not ASUS */
    vendor_list = g_slist_sort(vendor_list, &vendor_sort);

    g_free(path);
}

const gchar *vendor_get_name(const gchar * id_str)
{
    GSList *vendor;

    if (!id_str)
        return NULL;

    for (vendor = vendor_list; vendor; vendor = vendor->next) {
        Vendor *v = (Vendor *)vendor->data;

        if (v)
            if (v->match_case) {
                if (v->match_string && strstr(id_str, v->match_string))
                    return v->name;
            } else {
                if (v->match_string && strcasestr(id_str, v->match_string))
                    return v->name;
            }

    }

    return id_str; /* What about const? */
}

const gchar *vendor_get_url(const gchar * id_str)
{
    GSList *vendor;

    if (!id_str) {
      return NULL;
    }

    for (vendor = vendor_list; vendor; vendor = vendor->next) {
        Vendor *v = (Vendor *)vendor->data;

        if (v)
            if (v->match_case) {
                if (v->match_string && strstr(id_str, v->match_string))
                    return v->url;
            } else {
                if (v->match_string && strcasestr(id_str, v->match_string))
                    return v->url;
            }

    }

    return NULL;
}
