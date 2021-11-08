/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "devices.h"

typedef struct _CUPSDest	CUPSDest;
typedef struct _CUPSOption	CUPSOption;

struct _CUPSOption {
    char *name, *value;
};

struct _CUPSDest {
    char *name, *instance;
    int is_default;
    int num_options;
    CUPSOption *options;
};

static int (*cups_dests_get) (CUPSDest **dests) = NULL;
static int (*cups_dests_free) (int num_dests, CUPSDest *dests) = NULL;
static gboolean cups_init = FALSE;

GModule *cups;

void
init_cups(void)
{
    const char *libcups[] = { "libcups", "libcups.so", "libcups.so.1", "libcups.so.2", NULL };

    if (!(cups_dests_get && cups_dests_free)) {
        int i;

        for (i = 0; libcups[i] != NULL; i++) {
            cups = g_module_open(libcups[i], G_MODULE_BIND_LAZY);
            if (cups)
                break;
        }

        if (!cups) {
	    cups_init = FALSE;
	    return;
	}

	if (!g_module_symbol(cups, "cupsGetDests", (gpointer) & cups_dests_get)
	    || !g_module_symbol(cups, "cupsFreeDests", (gpointer) & cups_dests_free)) {
            g_module_close(cups);
	    cups_init = FALSE;
	}
    }

    cups_init = TRUE;
}

gchar *__cups_callback_ptype(gchar *strvalue)
{
  if (strvalue) {
    unsigned value = atoi(strvalue);
    gchar *output = g_strdup("\n");

    if (value & 0x0004)
      output = h_strdup_cprintf(_("\342\232\254 Can do black and white printing=\n"), output);
    if (value & 0x0008)
      output = h_strdup_cprintf(_("\342\232\254 Can do color printing=\n"), output);
    if (value & 0x0010)
      output = h_strdup_cprintf(_("\342\232\254 Can do duplexing=\n"), output);
    if (value & 0x0020)
      output = h_strdup_cprintf(_("\342\232\254 Can do staple output=\n"), output);
    if (value & 0x0040)
      output = h_strdup_cprintf(_("\342\232\254 Can do copies=\n"), output);
    if (value & 0x0080)
      output = h_strdup_cprintf(_("\342\232\254 Can collate copies=\n"), output);
    if (value & 0x80000)
      output = h_strdup_cprintf(_("\342\232\254 Printer is rejecting jobs=\n"), output);
    if (value & 0x1000000)
      output = h_strdup_cprintf(_("\342\232\254 Printer was automatically discovered and added=\n"), output);

    return output;
  } else {
    return g_strdup(_("Unknown"));
  }
}

gchar *__cups_callback_state(gchar *value)
{
  if (!value) {
    return g_strdup(_("Unknown"));
  }

  if (g_str_equal(value, "3")) {
    return g_strdup(_("Idle"));
  } else if (g_str_equal(value, "4")) {
    return g_strdup(_("Printing a Job"));
  } else if (g_str_equal(value, "5")) {
    return g_strdup(_("Stopped"));
  } else {
    return g_strdup(_("Unknown"));
  }
}

gchar *__cups_callback_state_change_time(gchar *value)
{
  struct tm tm;
  char buf[255];

  if (value) {
    strptime(value, "%s", &tm);
    strftime(buf, sizeof(buf), "%c", &tm);

    return g_strdup(buf);
  } else {
    return g_strdup(_("Unknown"));
  }
}

gchar *__cups_callback_boolean(gchar *value)
{
  if (value) {
    return g_strdup(g_str_equal(value, "1") ? _("Yes") : _("No"));
  } else {
    return g_strdup(_("Unknown"));
  }
}

const struct {
  char *key, *name;
  gchar *(*callback)(gchar *value);
  gboolean maybe_vendor;
} cups_fields[] = {
  { "Printer Information", NULL, NULL },
  { "printer-info", "Destination Name", NULL },
  { "printer-make-and-model", "Make and Model", NULL, TRUE },

  { "Capabilities", NULL, NULL },
  { "printer-type", "#", __cups_callback_ptype },

  { "Printer State", NULL, NULL },
  { "printer-state", "State", __cups_callback_state },
  { "printer-state-change-time", "Change Time", __cups_callback_state_change_time },
  { "printer-state-reasons", "State Reasons" },

  { "Sharing Information", NULL, NULL },
  { "printer-is-shared", "Shared?", __cups_callback_boolean },
  { "printer-location", "Physical Location" },
  { "auth-info-required", "Authentication Required", __cups_callback_boolean },

  { "Jobs", NULL, NULL },
  { "job-hold-until", "Hold Until", NULL },
  { "job-priority", "Priority", NULL },
  { "printer-is-accepting-jobs", "Accepting Jobs", __cups_callback_boolean },

  { "Media", NULL, NULL },
  { "media", "Media", NULL },
  { "finishings", "Finishings", NULL },
  { "copies", "Copies", NULL },
};

void
scan_printers_do(void)
{
    int num_dests, i, j;
    CUPSDest *dests;
    gchar *prn_id, *prn_moreinfo;

    g_free(printer_list);
    g_free(printer_icons);

    if (!cups_init) {
        init_cups();

        printer_icons = g_strdup("");
        printer_list = g_strdup(_("[Printers]\n"
                                "No suitable CUPS library found="));
        return;
    }

    /* remove old devices from global device table */
    moreinfo_del_with_prefix("DEV:PRN");

    num_dests = cups_dests_get(&dests);
    if (num_dests > 0) {
	printer_list = g_strdup_printf(_("[Printers (CUPS)]\n"));
        printer_icons = g_strdup("");
	for (i = 0; i < num_dests; i++) {
	    GHashTable *options;

	    options = g_hash_table_new(g_str_hash, g_str_equal);

	    for (j = 0; j < dests[i].num_options; j++) {
	      g_hash_table_insert(options,
	                          g_strdup(dests[i].options[j].name),
	                          g_strdup(dests[i].options[j].value));
	    }

            prn_id = g_strdup_printf("PRN%d", i);

	    printer_list = h_strdup_cprintf("\n$%s$%s=%s\n",
					    printer_list,
					    prn_id,
					    dests[i].name,
					    dests[i].is_default ? ((params.markup_ok) ? "<i>Default</i>" : "(Default)") : "");
            printer_icons = h_strdup_cprintf("\nIcon$%s$%s=printer.png",
                                             printer_icons,
                                             prn_id,
                                             dests[i].name);

            prn_moreinfo = g_strdup("");
            for (j = 0; j < G_N_ELEMENTS(cups_fields); j++) {
              if (!cups_fields[j].name) {
                prn_moreinfo = h_strdup_cprintf("[%s]\n",
                                                prn_moreinfo,
                                                cups_fields[j].key);
              } else {
                gchar *temp;

                temp = g_hash_table_lookup(options, cups_fields[j].key);

                if (cups_fields[j].callback) {
                  temp = cups_fields[j].callback(temp);
                } else {
                  if (temp) {
                    /* FIXME Do proper escaping */
                    temp = g_strdup(strreplacechr(temp, "&=", ' '));
                  } else {
                    temp = g_strdup(_("Unknown"));
                  }
                }

                prn_moreinfo = h_strdup_cprintf("%s%s=%s\n",
                                                prn_moreinfo,
                                                cups_fields[j].maybe_vendor ? "$^$" : "",
                                                cups_fields[j].name,
                                                temp);

                g_free(temp);
              }
            }

            moreinfo_add_with_prefix("DEV", prn_id, prn_moreinfo);
            g_free(prn_id);
            g_hash_table_destroy(options);
	}

	cups_dests_free(num_dests, dests);
    } else {
	printer_list = g_strdup(_("[Printers]\n"
	                        "No printers found=\n"));
    }
}
