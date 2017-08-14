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

#include "devices.h"

typedef struct _DMIInfo		DMIInfo;

struct _DMIInfo {
  const gchar *name;
  const gchar *file;		/* for sysfs */
  const gchar *param;		/* for dmidecode */
};

DMIInfo dmi_info_table[] = {
  { "$BIOS",		NULL,					NULL },
  { "Date",		"/sys/class/dmi/id/bios_date",		"bios-release-date" },
  { "Vendor",		"/sys/class/dmi/id/bios_vendor",	"bios-vendor" },
  { "Version#0",	"/sys/class/dmi/id/bios_version",	"bios-version" },
  { "$Board",		NULL,					NULL },
  { "Name",		"/sys/class/dmi/id/board_name",		"baseboard-product-name" },
  { "Vendor",		"/sys/class/dmi/id/board_vendor",	"baseboard-manufacturer" },
  { "$Product",		NULL,					NULL },
  { "Name",		"/sys/class/dmi/id/product_name",	"system-product-name" },
  { "Family",		"/sys/class/dmi/id/product_family",     "system-product-family" },
  { "Version#1",	"/sys/class/dmi/id/product_version",    "system-product-version" },
};

gchar *dmi_info = NULL;

static void add_to_moreinfo(const char *group, const char *key, char *value)
{
  char *new_key = g_strconcat("DMI:", group, ":", key, NULL);
  moreinfo_add_with_prefix("DEV", new_key, g_strdup(g_strstrip(value)));
}

gboolean dmi_get_info_dmidecode()
{
  FILE *dmi_pipe;
  gchar buffer[256];
  const gchar *group;
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
      group = info->name + 1;
      dmi_info = h_strdup_cprintf("[%s]\n", dmi_info, group);
    } else {
      gchar *temp;

      if (!info->param)
        continue;

      temp = g_strconcat("dmidecode -s ", info->param, NULL);
      if ((dmi_pipe = popen(temp, "r"))) {
        g_free(temp);

        (void)fgets(buffer, 256, dmi_pipe);
        if (pclose(dmi_pipe)) {
          dmi_failed = TRUE;
          break;
        }

        add_to_moreinfo(group, info->name, buffer);

        const gchar *url = vendor_get_url(buffer);
        if (url) {
          const gchar *vendor = vendor_get_name(buffer);
          if (g_strstr_len(vendor, -1, g_strstrip(buffer)) ||
              g_strstr_len(g_strstrip(buffer), -1, vendor)) {
            dmi_info = h_strdup_cprintf("%s=%s (%s)\n",
                                        dmi_info,
                                        info->name,
                                        g_strstrip(buffer),
                                        url);
          } else {
            dmi_info = h_strdup_cprintf("%s=%s (%s, %s)\n",
                                        dmi_info,
                                        info->name,
                                        g_strstrip(buffer),
                                        vendor, url);
          }
        } else {
          dmi_info = h_strdup_cprintf("%s=%s\n",
                                      dmi_info,
                                      info->name,
                                      buffer);
        }
      } else {
        g_free(temp);
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
  const gchar *group = NULL;
  DMIInfo *info;
  gboolean dmi_succeeded = FALSE;
  gint i;

  if (dmi_info) {
    g_free(dmi_info);
    dmi_info = NULL;
  }

  for (i = 0; i < G_N_ELEMENTS(dmi_info_table); i++) {
    info = &dmi_info_table[i];

    if (*(info->name) == '$') {
      group = info->name + 1;
      dmi_info = h_strdup_cprintf("[%s]\n", dmi_info, group);
    } else if (group && info->file) {
      if ((dmi_file = fopen(info->file, "r"))) {
        (void)fgets(buffer, 256, dmi_file);
        fclose(dmi_file);

        add_to_moreinfo(group, info->name, buffer);

        const gchar *url = vendor_get_url(buffer);
        if (url) {
          const gchar *vendor = vendor_get_name(buffer);
          if (g_strstr_len(vendor, -1, g_strstrip(buffer)) ||
              g_strstr_len(g_strstrip(buffer), -1, vendor)) {
            dmi_info = h_strdup_cprintf("%s=%s (%s)\n",
                                        dmi_info,
                                        info->name,
                                        g_strstrip(buffer),
                                        url);
          } else {
            dmi_info = h_strdup_cprintf("%s=%s (%s, %s)\n",
                                        dmi_info,
                                        info->name,
                                        g_strstrip(buffer),
                                        vendor, url);
          }
        } else {
          dmi_info = h_strdup_cprintf("%s=%s\n",
                                      dmi_info,
                                      info->name,
                                      g_strstrip(buffer));
        }
        dmi_succeeded = TRUE;
      } else {
        dmi_info = h_strdup_cprintf("%s=%s\n",
                                    dmi_info,
                                    info->name,
                                    _("(Not available; Perhaps try running HardInfo as root.)") );
      }
    }
  }

  if (!dmi_succeeded) {
    g_free(dmi_info);
    dmi_info = NULL;
  }

  return dmi_succeeded;
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
