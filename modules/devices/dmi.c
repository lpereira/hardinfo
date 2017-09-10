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
#include "dmi_util.h"

typedef struct _DMIInfo		DMIInfo;

struct _DMIInfo {
  const gchar *name;
  const gchar *id_str;
  int group;
};

DMIInfo dmi_info_table[] = {
  { N_("Product"), NULL, 1 },
  { N_("Name"), "system-product-name", 0 },
  { N_("Family"), "system-product-family", 0 },
  { N_("Vendor"), "system-manufacturer", 0 },
  { N_("Version"), "system-version", 0 },
  { N_("BIOS"), NULL, 1 },
  { N_("Date"), "bios-release-date", 0 },
  { N_("Vendor"), "bios-vendor", 0 },
  { N_("Version"), "bios-version", 0 },
  { N_("Board"), NULL, 1 },
  { N_("Name"), "baseboard-product-name", 0 },
  { N_("Vendor"), "baseboard-manufacturer", 0 },
  { N_("Version"), "baseboard-version", 0 },
  { N_("Serial Number"), "baseboard-serial-number", 0 },
  { N_("Asset Tag"), "baseboard-asset-tag", 0 },
  { N_("Chassis"), NULL, 1 },
  { N_("Vendor"), "chassis-manufacturer", 0 },
  { N_("Type"), "chassis-type", 0 },
  { N_("Version"), "chassis-version", 0 },
  { N_("Serial Number"), "chassis-serial-number", 0 },
  { N_("Asset Tag"), "chassis-asset-tag", 0 },
};

gchar *dmi_info = NULL;

static void add_to_moreinfo(const char *group, const char *key, char *value)
{
  char *new_key = g_strconcat("DMI:", group, ":", key, NULL);
  moreinfo_add_with_prefix("DEV", new_key, g_strdup(g_strstrip(value)));
}

gboolean dmi_get_info()
{
  const gchar *group = NULL;
  DMIInfo *info;
  gboolean dmi_succeeded = FALSE;
  gint i;
  gchar *value;

  if (dmi_info) {
    g_free(dmi_info);
    dmi_info = NULL;
  }

  for (i = 0; i < G_N_ELEMENTS(dmi_info_table); i++) {
    info = &dmi_info_table[i];

    if (info->group) {
      group = info->name;
      dmi_info = h_strdup_cprintf("[%s]\n", dmi_info, _(info->name) );
    } else if (group && info->id_str) {
      if (strcmp(info->id_str, "chassis-type") == 0)
        value = dmi_chassis_type_str(-1, 1);
      else
        value = dmi_get_str(info->id_str);

      if (value != NULL) {
        add_to_moreinfo(group, info->name, value);

        const gchar *url = vendor_get_url(value);
        if (url) {
          const gchar *vendor = vendor_get_name(value);
          dmi_info = h_strdup_cprintf("%s=%s (%s, %s)\n",
                                      dmi_info,
                                      _(info->name),
                                      value,
                                      vendor, url);
        } else {
          dmi_info = h_strdup_cprintf("%s=%s\n",
                                      dmi_info,
                                      _(info->name),
                                      value);
        }
        dmi_succeeded = TRUE;
      } else {
        dmi_info = h_strdup_cprintf("%s=%s\n",
                                    dmi_info,
                                    _(info->name),
                                    (getuid() == 0)
                                      ? _("(Not available)")
                                      : _("(Not available; Perhaps try running HardInfo as root.)") );
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

  dmi_ok = dmi_get_info();

  if (!dmi_ok) {
    dmi_info = g_strdup("[No DMI information]\n"
                        "There was an error retrieving the information.=\n"
                        "Please try running HardInfo as root.=\n");
  }
}
