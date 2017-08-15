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
  const gchar *id_str;
  int group;
};

DMIInfo dmi_info_table[] = {
  { N_("BIOS"), NULL, 1 },
  { N_("Date"), "bios-release-date", 0 },
  { N_("Vendor"), "bios-vendor", 0 },
  { N_("Version#0"), "bios-version", 0 },
  { N_("Board"), NULL, 1 },
  { N_("Name"), "baseboard-product-name", 0 },
  { N_("Vendor"), "baseboard-manufacturer", 0 },
  { N_("Product"), NULL, 1 },
  { N_("Name"), "system-product-name", 0 },
  { N_("Family"), "system-product-family", 0 },
  { N_("Version#1"), "system-product-version", 0 },
};

gchar *dmi_info = NULL;

const char *dmi_sysfs_root() {
    char *candidates[] = {
        "/sys/devices/virtual/dmi",
        "/sys/class/dmi",
        NULL
    };
    int i = 0;
    while (candidates[i] != NULL) {
        if(access(candidates[i], F_OK) != -1)
            return candidates[i];
        i++;
    }
    return NULL;
}

char *dmi_get_str(const char *id_str) {
  static struct {
    char *id;
    char *path;
  } tab_dmi_sysfs[] = {
    { "bios-release-date", "id/bios_date" },
    { "bios-vendor", "id/bios_vendor" },
    { "bios-version", "id/bios_version" },
    { "baseboard-product-name", "id/board_name" },
    { "baseboard-manufacturer", "id/board_vendor" },
    { "system-product-name", "id/product_name" },
    { "system-manufacturer", "id/sys_vendor" },
    { "system-product-family", "id/product_family" },
    { "system-product-version", "id/product_version" },
    { "chassis-type", "id/chassis_type" },
    { NULL, NULL }
  };
  const gchar *dmi_root = dmi_sysfs_root();
  gchar *path = NULL, *full_path = NULL, *ret = NULL;
  gboolean spawned;
  gchar *out, *err;

  int i = 0;
  while (tab_dmi_sysfs[i].id != NULL) {
    if (strcmp(id_str, tab_dmi_sysfs[i].id) == 0) {
      path = tab_dmi_sysfs[i].path;
      break;
    }
    i++;
  }

  /* try sysfs first */
  if (dmi_root && path) {
    full_path = g_strdup_printf("%s/%s", dmi_root, path);
    if (g_file_get_contents(full_path, &ret, NULL, NULL) )
      goto dmi_str_done;
  }

  /* try dmidecode, but may require root */
  full_path = g_strconcat("dmidecode -s ", id_str, NULL);
  spawned = g_spawn_command_line_sync(full_path,
            &out, &err, &i, NULL);
  if (spawned) {
    if (i == 0)
      ret = out;
    else
      g_free(out);
    g_free(err);
  }

dmi_str_done:
  if (ret != NULL) {
    ret = strend(ret, '\n');
    ret = g_strstrip(ret);
    if (strlen(ret) == 0) {
      g_free(ret);
      ret = NULL;
    }
  }
  g_free(full_path);
  return ret;
}

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
      value = dmi_get_str(info->id_str);

      if (value != NULL) {
        add_to_moreinfo(group, info->name, value);

        const gchar *url = vendor_get_url(value);
        if (url) {
          const gchar *vendor = vendor_get_name(value);
          dmi_info = h_strdup_cprintf("%s=%s (%s, %s)\n",
                                      dmi_info,
                                      _(info->name),
                                      g_strstrip(value),
                                      vendor, url);
        } else {
          dmi_info = h_strdup_cprintf("%s=%s\n",
                                      dmi_info,
                                      _(info->name),
                                      g_strstrip(value));
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
