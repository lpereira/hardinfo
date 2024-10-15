/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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
#include <time.h>

#include "hardinfo.h"
#include "devices.h"

const struct {
  gchar *key, *name;
} ups_fields[] = {
  { "UPS Status", NULL },
  { "STATUS", "Status" },
  { "TIMELEFT", "Time Left" },
  { "LINEV", "Line Voltage" },
  { "LOADPCT", "Load Percent" },

  { "UPS Battery Information", NULL },
  { "BATTV", "Battery Voltage" },
  { "BCHARGE", "Battery Charge" },
  { "BATTDATE", "Battery Date" },

  { "UPS Information", NULL },
  { "APCMODEL", "Model" },
  { "FIRMWARE", "Firmware Version" },
  { "SERIALNO", "Serial Number" },
  { "UPSMODE", "UPS Mode" },
  { "CABLE", "Cable" },
  { "UPSNAME", "UPS Name" },

  { "UPS Nominal Values", NULL },
  { "NOMINV", "Voltage" },
  { "NOMBATTV", "Battery Voltage" },
  { "NOMPOWER", "Power" }
};


static void
__scan_battery_apcupsd(void)
{
    GHashTable  *ups_data;
    FILE	*apcaccess;
    char	buffer[512], *apcaccess_path;
    guint		i;

    apcaccess_path = find_program("apcaccess");
    if (apcaccess_path && (apcaccess = popen(apcaccess_path, "r"))) {
      /* first line isn't important */
      if (fgets(buffer, 512, apcaccess)) {
        /* allocate the key, value hash table */
        ups_data = g_hash_table_new(g_str_hash, g_str_equal);

        /* read up all the apcaccess' output, saving it in the key, value hash table */
        while (fgets(buffer, 512, apcaccess)) {
          buffer[9] = '\0';

          g_hash_table_insert(ups_data,
                              g_strdup(g_strstrip(buffer)),
                              g_strdup(g_strstrip(buffer + 10)));
        }

        /* builds the ups info string, respecting the field order as found in ups_fields */
        for (i = 0; i < G_N_ELEMENTS(ups_fields); i++) {
          if (!ups_fields[i].name) {
            /* there's no name: make a group with the key as its name */
            battery_list = h_strdup_cprintf("[%s]\n", battery_list, ups_fields[i].key);
          } else {
            /* there's a name: adds a line */
            const gchar *name = g_hash_table_lookup(ups_data, ups_fields[i].key);
            battery_list = h_strdup_cprintf("%s=%s\n", battery_list,
                                            ups_fields[i].name, name);
          }
        }

        g_hash_table_destroy(ups_data);
      }

      pclose(apcaccess);
    }
    
    g_free(apcaccess_path);
}

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
    gchar *manufacturer = NULL;
    
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
	    GET_STR("OEM info", manufacturer);
            
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

         gchar *tmp = vendor_get_link(manufacturer);
         g_free(manufacturer);
         manufacturer = tmp;
          
          if (g_str_equal(present, "yes")) {
            if (remaining && capacity)
               charge_rate = atof(remaining) / atof(capacity);
            else
               charge_rate = 0;

            battery_list = h_strdup_cprintf(_("\n[Battery: %s]\n"
                                           "State=%s (load: %s)\n"
                                           "Capacity=%s / %s (%.2f%%)\n"
                                           "Battery Technology=%s (%s)\n"
					   "Manufacturer=%s\n"
                                           "Model Number=%s\n"
                                           "Serial Number=%s\n"),
                                           battery_list,
                                           entry,
                                           state, rate,
                                           remaining, capacity, charge_rate * 100.0,
                                           technology, type,
					   manufacturer,
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
	  g_free(manufacturer);

          present = capacity = technology = type = \
                model = serial = state = remaining = rate = manufacturer = NULL;
        }
      
        g_dir_close(acpi);
      }
    }
    
    g_free(acpi_path);
}

static gchar *
read_contents(const gchar *base, const gchar *key)
{
    gchar *value;
    gchar *path;

    path = g_strdup_printf("%s/%s", base, key);
    if (!path)
        return NULL;

    if (!g_file_get_contents(path, &value, NULL, NULL)) {
        g_free(path);
        return NULL;
    }

    g_free(path);
    return g_strchomp(value);
}

static void
__scan_battery_sysfs_add_battery(const gchar *name)
{
    gchar *path = g_strdup_printf("/sys/class/power_supply/%s", name);
    gchar *status, *capacity, *capacity_level, *technology, *manufacturer;
    gchar *model_name, *serial_number, *charge_full_design=NULL, *charge_full=NULL;
    gchar *voltage_min_design=NULL,*energy_full_design=NULL;
    float full_design=-1.0,full_current=-1.0,voltage=-1.0;
    unsigned long l;

    if (!path)
        return;

    if(name[0]=='A'){//AC Supply
        status=read_contents(path, "online");
	if(status==NULL) status=g_strdup("1");
	if(!strcmp(status,"1")) {
	    g_free(status);
	    status=g_strdup("Attached");
	}else{
	    g_free(powerstate);powerstate=g_strdup("BAT");
	    g_free(status);status=g_strdup("Not attached");
	}
        battery_list = h_strdup_cprintf(_("\n[AC Power Supply: %s]\n"
            "Online=%s\n"
            "AC Power Type=%s\n"
	    ),
            battery_list,
            name,
            status,
	    read_contents(path, "type")
            );
	g_free(status);
    }

    if((name[0]=='B') || strstr(name,"CMB")){//Battery

    status = read_contents(path, "status");
    capacity = read_contents(path, "capacity");
    capacity_level = read_contents(path, "capacity_level");
    technology = read_contents(path, "technology");
    manufacturer = read_contents(path, "manufacturer");
    model_name = read_contents(path, "model_name");
    serial_number = read_contents(path, "serial_number");
    energy_full_design = read_contents(path, "energy_full_design");
    charge_full_design = read_contents(path, "charge_full_design");
    charge_full = read_contents(path, "charge_full");
    voltage_min_design = read_contents(path, "voltage_min_design");

    if(voltage_min_design) if(sscanf(voltage_min_design, "%lu", &l)==1) voltage=(float)l/1000000.0;//uV->V
    if(!charge_full_design && energy_full_design) if(sscanf(energy_full_design, "%lu", &l)==1) full_design=(float)l/(voltage>0?voltage*1000000.0:-1.0);//uWh->Ah
    if(charge_full_design) if(sscanf(charge_full_design, "%lu", &l)==1) full_design=(float)l/1000000.0;//uAh->Ah
    if(charge_full) if(sscanf(charge_full, "%lu", &l)==1) full_current=(float)l/1000000.0;//uAh->Ah

    battery_list = h_strdup_cprintf(_("\n[Battery: %s]\n"
        "State=%s\n"
        "Capacity=%s / %s\n"
        "Battery Health=%.0f %%\n"
        "Design Full Energy=%.3f Wh\n"
        "Current Full Energy=%.3f Wh\n"
        "Design Full Capacity=%.3f Ah\n"
        "Current Full Capacity=%.3f Ah\n"
        "Voltage Design=%.3f V\n"
        "Battery Technology=%s\n"
        "Manufacturer=%s\n"
        "Model Number=%s\n"
        "Serial Number=%s\n"),
        battery_list,
        name,
        status,
        capacity, capacity_level,
	full_design>0?(full_current*100.0)/full_design:-1,
	voltage>0?full_design*voltage:-1,
        voltage>0?full_current*voltage:-1,
	full_design,
        full_current,
	voltage,
        technology,
        manufacturer,
        model_name,
        serial_number);

    if(!strcmp(status,"Discharging")) {
        g_free(powerstate);powerstate=g_strdup("BAT");
    }

    free(voltage_min_design);
    free(energy_full_design);
    free(charge_full_design);
    free(charge_full);
    free(status);
    free(capacity);
    free(capacity_level);
    free(technology);
    free(manufacturer);
    free(model_name);
    free(serial_number);
    }
}

static void
__scan_battery_sysfs(void)
{
    GDir *dir;
    const gchar *entry;

    dir = g_dir_open("/sys/class/power_supply", 0, NULL);
    if (!dir)
        return;

    while ((entry = g_dir_read_name(dir))) {
        __scan_battery_sysfs_add_battery(entry);
    }

    g_dir_close(dir);
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
        
        int c=fscanf(procapm, "%s %s %s 0x%x %s %s %d%%",
               apm_drv_ver, apm_bios_ver, trash,
               &ac_bat, trash, trash, &percentage);
        fclose(procapm);
	if(c!=7) return;
        
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
        battery_list = h_strdup_cprintf(_("\n[Battery (APM)]\n"
                                       "Charge=%d%%\n"
                                       "Remaining Charge=%s of %s\n"
                                       "Using=%s\n"
                                       "APM driver version=%s\n"
                                       "APM BIOS version=%s\n"),
                                       battery_list,
                                       percentage,
                                       sremaining, stotal,
                                       ac_status[ac_bat],
                                       apm_drv_ver, apm_bios_ver);
    } else {
        battery_list = h_strdup_cprintf(_("\n[Battery (APM)]\n"
                                       "Charge=%d%%\n"
                                       "Using=%s\n"
                                       "APM driver version=%s\n"
                                       "APM BIOS version=%s\n"),
                                       battery_list,
                                       percentage,
                                       ac_status[ac_bat],
                                       apm_drv_ver, apm_bios_ver);
    
    }
}

void
scan_battery_do(void)
{
    g_free(powerstate);powerstate=g_strdup("AC");
    g_free(battery_list);
    battery_list = g_strdup("");

    __scan_battery_sysfs();
    __scan_battery_acpi();
    __scan_battery_apm();
    __scan_battery_apcupsd();

    if (*battery_list == '\0') {
        g_free(battery_list);
        
        battery_list = g_strdup_printf("[Power Status]\n Power State=%s\n%s",powerstate,
				       _("[No batteries]\nNo batteries found on this system=\n"));
    }else{
        gchar *t=battery_list;
        battery_list = g_strdup_printf("[Power Status]\n Power State=%s\n%s",powerstate,t);
	g_free(t);
    }
}
