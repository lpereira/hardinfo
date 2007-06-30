/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

#include <time.h>

static void
__scan_battery_acpi(void)
{
    gchar *acpi_path;

    gchar *present = NULL;
    gchar *capacity = NULL;
    gchar *technology = NULL;
    gchar *voltage = NULL;
    gchar *model = NULL, *serial = NULL, *type = NULL;
    gchar *state = NULL, *rate = NULL;
    gchar *remaining = NULL;
    
    if (battery_list) {
      g_free(battery_list);
    }
    battery_list = g_strdup("");
    
    acpi_path = g_strdup("/proc/acpi/battery");
    if (g_file_test(acpi_path, G_FILE_TEST_EXISTS)) {
      GDir *acpi;
      
      if ((acpi = g_dir_open(acpi_path, 0, NULL))) {
        const gchar *entry;
        
        while ((entry = g_dir_read_name(acpi))) {
          gchar *path = g_strdup_printf("%s/%s/info", acpi_path, entry);
          FILE *f;
          gchar buffer[256];
          gdouble charge_rate = 1.0;
          
          f = fopen(path, "r");
          g_free(path);
          
          if (!f)
            goto cleanup;
          
          while (fgets(buffer, 256, f)) {
            gchar **tmp = g_strsplit(buffer, ":", 2);
            
            GET_STR("present", present);
            GET_STR("design capacity", capacity);
            GET_STR("battery technology", technology);
            GET_STR("design voltage", voltage);
            GET_STR("model number", model);
            GET_STR("serial number", serial);
            GET_STR("battery type", type);
            
            g_strfreev(tmp);
          }          
          fclose(f);
          
          path = g_strdup_printf("%s/%s/state", acpi_path, entry);
          f = fopen(path, "r");
          g_free(path);
          
          if (!f)
            goto cleanup;
          
          while (fgets(buffer, 256, f)) {
            gchar **tmp = g_strsplit(buffer, ":", 2);
            
            GET_STR("charging state", state);
            GET_STR("present rate", rate);
            GET_STR("remaining capacity", remaining);
          
            g_strfreev(tmp);
          }
          
          fclose(f);
          
          if (g_str_equal(present, "yes")) {
            charge_rate = atof(remaining) / atof(capacity);
          
            battery_list = g_strdup_printf("%s\n[Battery: %s]\n"
                                           "State=%s (load: %s)\n"
                                           "Capacity=%s / %s (%.2f%%)\n"
                                           "Battery Technology=%s (%s)\n"
                                           "Model Number=%s\n"
                                           "Serial Number=%s\n",
                                           battery_list,
                                           entry,
                                           state, rate,
                                           remaining, capacity, charge_rate * 100.0,
                                           technology, type,
                                           model,
                                           serial);
          }
          
         cleanup:
          g_free(present);
          g_free(capacity);
          g_free(technology);
          g_free(type);
          g_free(model);
          g_free(serial);
          g_free(state);
          g_free(remaining);
          g_free(rate);

          present = capacity = technology = type = \
                model = serial = state = remaining = rate = NULL;
        }
      
        g_dir_close(acpi);
      }
    }
    
    g_free(acpi_path);
}

static void
__scan_battery_apm(void)
{
    FILE                *procapm;
    static char         *sremaining = NULL, *stotal = NULL;
    static unsigned int  last_time = 0;
    static int           percentage = 0;
    const  char         *ac_status[] = { "Battery",
                                         "AC Power",
                                         "Charging" };
    int                  ac_bat;
    char                 apm_bios_ver[16], apm_drv_ver[16];
    char                 trash[10];
    
    if ((procapm = fopen("/proc/apm", "r"))) {
        int old_percentage = percentage;
        
        fscanf(procapm, "%s %s %s 0x%x %s %s %d%%",
               apm_drv_ver, apm_bios_ver, trash,
               &ac_bat, trash, trash, &percentage);
        fclose(procapm);
        
        if (last_time == 0) {
            last_time = time(NULL);
            sremaining = stotal = NULL;
        }

        if (old_percentage - percentage > 0) {
            if (sremaining && stotal) {
                g_free(sremaining);
                g_free(stotal);
            }
                        
            int secs_remaining = (time(NULL) - last_time) * percentage /
                                 (old_percentage - percentage);
            sremaining = seconds_to_string(secs_remaining);
            stotal = seconds_to_string((secs_remaining * 100) / percentage);
            
            last_time = time(NULL);
        }
    } else {
        return;
    }

    if (stotal && sremaining) {
        battery_list = g_strdup_printf("%s\n[Battery (APM)]\n"
                                       "Charge=%d%%\n"
                                       "Remaining Charge=%s of %s\n"
                                       "Using=%s\n"
                                       "APM driver version=%s\n"
                                       "APM BIOS version=%s\n",
                                       battery_list,
                                       percentage,
                                       sremaining, stotal,
                                       ac_status[ac_bat],
                                       apm_drv_ver, apm_bios_ver);
    } else {
        battery_list = g_strdup_printf("%s\n[Battery (APM)]\n"
                                       "Charge=%d%%\n"
                                       "Using=%s\n"
                                       "APM driver version=%s\n"
                                       "APM BIOS version=%s\n",
                                       battery_list,
                                       percentage,
                                       ac_status[ac_bat],
                                       apm_drv_ver, apm_bios_ver);
    
    }
}

static void
__scan_battery(void)
{
    if (battery_list) {
      g_free(battery_list);
    }
    battery_list = g_strdup("");

    __scan_battery_acpi();
    __scan_battery_apm();
    
    if (*battery_list == '\0') {
        g_free(battery_list);
        
        battery_list = g_strdup("[No batteries]\n"
                                "No batteries found on this system=\n");
    }
}
