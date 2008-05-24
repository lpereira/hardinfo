/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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
/*
 * DMI support based on patch by Stewart Adam <s.adam@diffingo.com>
 */
#include <unistd.h>
#include <sys/types.h>
              
typedef struct _DMIInfo		DMIInfo;

struct _DMIInfo {
  const gchar *name;
  const gchar *file;		/* for sysfs */
  const gchar *param;		/* for dmidecode */
};

DMIInfo dmi_info_table[] = {
  { "$BIOS",	NULL,					NULL },
  { "Date",	"/sys/class/dmi/id/bios_date",		"bios-release-date" },
  { "Vendor",	"/sys/class/dmi/id/bios_vendor",	"bios-vendor" },
  { "Version",	"/sys/class/dmi/id/bios_version",	"bios-version" },
  { "$Board",	NULL,					NULL },
  { "Name",	"/sys/class/dmi/id/board_name",		"baseboard-product-name" },
  { "Vendor",	"/sys/class/dmi/id/board_vendor",	"baseboard-manufacturer" },
};

static gchar *dmi_info = NULL;

gboolean dmi_get_info_dmidecode()
{
  FILE *dmi_pipe;
  gchar buffer[256];
  DMIInfo *info;
  gboolean dmi_failed = FALSE;
  gint i;

  if (dmi_info) {
    g_free(dmi_info);
    dmi_info = NULL;
  }
  
  for (i = 0; i < G_N_ELEMENTS(dmi_info_table); i++) {
    info = &dmi_info_table[i];
    
    if (*(info->name) == '$') {
      dmi_info = h_strdup_cprintf("[%s]\n", dmi_info,
                                  (info->name) + 1);
    } else {
      gchar *temp;
      
      if (!info->param)
        continue;

      temp = g_strconcat("dmidecode -s ", info->param, NULL);
      
      if ((dmi_pipe = popen(temp, "r"))) {
        g_free(temp);

        fgets(buffer, 256, dmi_pipe);
        if (pclose(dmi_pipe)) {
          dmi_failed = TRUE;
          break;
        }
        
        dmi_info = h_strdup_cprintf("%s=%s\n",
                                    dmi_info,
                                    info->name,
                                    buffer);
      } else {
        dmi_failed = TRUE;
        break;
      }
    }                                
  }
  
  if (dmi_failed) {
    g_free(dmi_info);
    dmi_info = NULL;
  }
  
  return !dmi_failed;
}

gboolean dmi_get_info_sys()
{
  FILE *dmi_file;
  gchar buffer[256];
  DMIInfo *info;
  gboolean dmi_failed = FALSE;
  gint i;
  
  if (dmi_info) {
    g_free(dmi_info);
    dmi_info = NULL;
  }
  
  for (i = 0; i < G_N_ELEMENTS(dmi_info_table); i++) {
    info = &dmi_info_table[i];
    
    if (*(info->name) == '$') {
      dmi_info = h_strdup_cprintf("[%s]\n", dmi_info,
                                  (info->name) + 1);
    } else {
      if (!info->file)
        continue;
        
      if ((dmi_file = fopen(info->file, "r"))) {
        fgets(buffer, 256, dmi_file);
        fclose(dmi_file);
        
        dmi_info = h_strdup_cprintf("%s=%s\n",
                                    dmi_info,
                                    info->name,
                                    buffer);
      } else {
        dmi_failed = TRUE;
        break;
      }
    }                                
  }
  
  if (dmi_failed) {
    g_free(dmi_info);
    dmi_info = NULL;
  }
  
  return !dmi_failed;
}

void __scan_dmi()
{
  gboolean dmi_ok;
  
  dmi_ok = dmi_get_info_sys();
  
  if (!dmi_ok) {
    dmi_ok = dmi_get_info_dmidecode();
  }
  
  if (!dmi_ok) {
    dmi_info = g_strdup("[No DMI information]\n"
                        "There was an error retrieving the information.=\n"
                        "Please try running HardInfo as root.=\n");
  }
}
