/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include "devices.h"
#include "expr.h"
#include "hardinfo.h"
#include "socket.h"
#include "udisks2_util.h"

gchar *sensors = NULL;
gchar *sensor_icons = NULL;
GHashTable *sensor_compute = NULL;
GHashTable *sensor_labels = NULL;
gboolean hwmon_first_run = TRUE;

static gchar *last_group = NULL;

static void read_sensor_labels(gchar *devname) {
    FILE *conf;
    gchar buf[256], *line, *p;
    gboolean lock = FALSE;
    gint i;

    /* Try to open lm-sensors config file sensors3.conf */
    conf = fopen("/etc/sensors3.conf", "r");

    /* If it fails, try to open sensors.conf */
    if (!conf)
        conf = fopen("/etc/sensors.conf", "r");

    if (!conf) {
        /* Cannot open config file. */
        return;
    }

    while (fgets(buf, 256, conf)) {
        line = buf;

        remove_linefeed(line);
        strend(line, '#');

        if (*line == '\0') {
            continue;
        } else if (lock && strstr(line, "label")) { /* label lines */
            gchar **names = g_strsplit(strstr(line, "label") + 5, " ", 0);
            gchar *key = NULL, *value = NULL;

            for (i = 0; names[i]; i++) {
                if (names[i][0] == '\0')
                    continue;

                if (!key)
                    key = g_strdup_printf("%s/%s", devname, names[i]);
                else if (!value)
                    value = g_strdup(names[i]);
                else
                    value = g_strconcat(value, " ", names[i], NULL);
            }

            remove_quotes(value);
            g_hash_table_insert(sensor_labels, key, g_strstrip(value));

            g_strfreev(names);
        } else if (lock && strstr(line, "ignore")) { /* ignore lines */
            p = strstr(line, "ignore") + 6;
            if (!strchr(p, ' '))
                continue;

            while (*p == ' ')
                p++;
            g_hash_table_insert(sensor_labels, g_strdup_printf("%s/%s", devname, p), "ignore");
        } else if (lock && strstr(line, "compute")) { /* compute lines */
            strend(line, ',');
            gchar **formulas = g_strsplit(strstr(line, "compute") + 7, " ", 0);
            gchar *key = NULL, *formula = NULL;

            for (i = 0; formulas[i]; i++) {
                if (formulas[i][0] == '\0')
                    continue;

                if (!key)
                    key = g_strdup_printf("%s/%s", devname, formulas[i]);
                else if (!formula)
                    formula = g_strdup(formulas[i]);
                else
                    formula = g_strconcat(formula, formulas[i], NULL);
            }

            g_strfreev(formulas);
            g_hash_table_insert(sensor_compute, key,
                                math_string_to_postfix(formula));
        } else if (g_str_has_prefix(line,
                                    "chip")) { /* chip lines (delimiter) */
            if (lock == FALSE) {
                gchar **chips = g_strsplit(line, " ", 0);

                for (i = 1; chips[i]; i++) {
                    strend(chips[i], '*');

                    if (g_str_has_prefix(chips[i] + 1, devname)) {
                        lock = TRUE;
                        break;
                    }
                }

                g_strfreev(chips);
            } else {
                break;
            }
        }
    }

    fclose(conf);
}

static void add_sensor(const char *type,
                       const char *sensor,
                       const char *parent,
                       double value,
                       const char *unit,
                       const char *icon) {
    char key[64];

    snprintf(key, sizeof(key), "%s/%s", parent, sensor);

    if (SENSORS_GROUP_BY_TYPE) {
        // group by type
        if (g_strcmp0(last_group, type) != 0) {
            sensors = h_strdup_cprintf("[%s]\n", sensors, type);
            g_free(last_group);
            last_group = g_strdup(type);
        }
        sensors = h_strdup_cprintf("$%s$%s=%.2f%s|%s\n", sensors,
            key, sensor, value, unit, parent);
    }
    else {
        // group by device source / driver
        if (g_strcmp0(last_group, parent) != 0) {
            sensors = h_strdup_cprintf("[%s]\n", sensors, parent);
            g_free(last_group);
            last_group = g_strdup(parent);
        }
        sensors = h_strdup_cprintf("$%s$%s=%.2f%s|%s\n", sensors,
            key, sensor, value, unit, type);
    }

    if (icon != NULL) {
        sensor_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n", sensor_icons,
            key, sensor, icon);
    }

    moreinfo_add_with_prefix("DEV", key, g_strdup_printf("%.2f%s", value, unit));

    lginterval = h_strdup_cprintf("UpdateInterval$%s=1000\n", lginterval, key);
}

static gchar *get_sensor_label_from_conf(gchar *key) {
    gchar *ret;
    ret = g_hash_table_lookup(sensor_labels, key);

    if (ret)
        return g_strdup(ret);

    return NULL;
}

static float adjust_sensor(gchar *key, float value) {
    GSList *postfix;

    postfix = g_hash_table_lookup(sensor_compute, key);
    if (!postfix)
        return value;

    return math_postfix_eval(postfix, value);
}

static char *get_sensor_path(int number, const char *prefix) {
    return g_strdup_printf("/sys/class/hwmon/hwmon%d/%s", number, prefix);
}

static char *determine_devname_for_hwmon_path(char *path) {
    char *tmp, *devname = NULL;

    // device name
    tmp = g_strdup_printf("%s/name", path);
    g_file_get_contents(tmp, &devname, NULL, NULL);
    g_free(tmp);
    if (devname)
        return g_strstrip(devname);

    // fallback: driver name (from driver link)
    tmp = g_strdup_printf("%s/device/driver", path);
    devname = g_file_read_link(tmp, NULL);
    g_free(tmp);

    if (!devname) {
        // fallback: device folder name (from device link)
        tmp = g_strdup_printf("%s/device", path);
        devname = g_file_read_link(tmp, NULL);
        g_free(tmp);
    }

    if (devname) {
        tmp = g_path_get_basename(devname);
        g_free(devname);
        return tmp;
    }

    return g_strdup("unknown");
}

struct HwmonSensor {
    const char *friendly_name;
    const char *value_file_regex;
    const char *value_path_format;
    const char *label_path_format;
    const char *key_format;
    const char *unit;
    const float adjust_ratio;
    const char *icon;
};

static const struct HwmonSensor hwmon_sensors[] = {
    {
        "Fan Speed",
        "^fan([0-9]+)_input$",
        "%s/fan%d_input",
        "%s/fan%d_label",
        "fan%d",
        " RPM",
        1.0,
        "fan"
    },
    {
        "Temperature",
        "^temp([0-9]+)_input$",
        "%s/temp%d_input",
        "%s/temp%d_label",
        "temp%d",
        "\302\260C",
        1000.0,
        "therm"
    },
    {
        "Voltage",
        "^in([0-9]+)_input$",
        "%s/in%d_input",
        "%s/in%d_label",
        "in%d",
        "V",
        1000.0,
        "bolt"
    },
    {
        "Current",
        "^curr([0-9]+)_input$",
        "%s/curr%d_input",
        "%s/curr%d_label",
        "curr%d",
        " A",
        1000.0,
        "bolt"
    },
    {
        "Power",
        "^power([0-9]+)_input$",
        "%s/power%d_input",
        "%s/power%d_label",
        "power%d",
        " W",
        1000000.0,
        "bolt"
    },
    {
        "CPU Voltage",
        "^cpu([0-9]+)_vid$",
        "%s/cpu%d_vid",
        NULL,
        "cpu%d_vid",
        " V",
        1000.0,
        "bolt"
    },
    { }
};

static const char *hwmon_prefix[] = {"device", "", NULL};

static gboolean read_raw_hwmon_value(gchar *path_hwmon, const gchar *item_path_format, int item_id, gchar **result){
    gchar *full_path;
    gboolean file_result;

    if (item_path_format == NULL)
        return FALSE;

    full_path = g_strdup_printf(item_path_format, path_hwmon, item_id);
    file_result = g_file_get_contents(full_path, result, NULL, NULL);

    g_free(full_path);

    return file_result;
}

static void read_sensors_hwmon(void) {
    int hwmon, count, min, max;
    gchar *path_hwmon, *tmp, *devname, *name, *mon, *key;
    const char **prefix, *entry;
    GDir *dir;
    GRegex *regex;
    GMatchInfo *match_info;
    GError *err = NULL;

    for (prefix = hwmon_prefix; *prefix; prefix++) {
        hwmon = 0;
        path_hwmon = get_sensor_path(hwmon, *prefix);
        while (path_hwmon && g_file_test(path_hwmon, G_FILE_TEST_EXISTS)) {
            const struct HwmonSensor *sensor;

            devname = determine_devname_for_hwmon_path(path_hwmon);
            DEBUG("hwmon%d has device=%s", hwmon, devname);
            if (hwmon_first_run) {
                read_sensor_labels(devname);
            }

            dir = g_dir_open(path_hwmon, 0, NULL);
            if (!dir)
                continue;

            for (sensor = hwmon_sensors; sensor->friendly_name; sensor++) {
                DEBUG("current sensor type=%s", sensor->friendly_name);
                regex = g_regex_new (sensor->value_file_regex, 0, 0, &err);
                if (err != NULL){
                    g_free(err);
                    err = NULL;
                    continue;
                }

                g_dir_rewind(dir);
                min = 999;
                max = -1;

                while ((entry = g_dir_read_name(dir))) {
                    g_regex_match(regex, entry, 0, &match_info);
                    if (g_match_info_matches(match_info)) {
                       tmp = g_match_info_fetch(match_info, 1);
                       count = atoi(tmp);
                       g_free (tmp);

                       if (count < min){
                           min = count;
                       }
                       if (count > max){
                           max = count;
                       }
                    }
                    g_match_info_free(match_info);
                }
                g_regex_unref(regex);

                for (count = min; count <= max; count++) {
                    if (!read_raw_hwmon_value(path_hwmon, sensor->value_path_format, count, &tmp)) {
                        continue;
                    }

                    mon = g_strdup_printf(sensor->key_format, count);
                    key = g_strdup_printf("%s/%s", devname, mon);
                    name = get_sensor_label_from_conf(key);
                    if (name == NULL){
                        if (read_raw_hwmon_value(path_hwmon, sensor->label_path_format, count, &name)){
                            name = g_strchomp(name);
                        }
                        else{
                            name = g_strdup(mon);
                        }
                    }

                    if (!g_str_equal(name, "ignore")) {
                        float adjusted = adjust_sensor(key,
                            atof(tmp) / sensor->adjust_ratio);

                        add_sensor(sensor->friendly_name,
                                   name,
                                   devname,
                                   adjusted,
                                   sensor->unit,
                                   sensor->icon);
                    }

                    g_free(tmp);
                    g_free(mon);
                    g_free(key);
                    g_free(name);
                }
            }

            g_dir_close(dir);
            g_free(path_hwmon);
            g_free(devname);

            path_hwmon = get_sensor_path(++hwmon, *prefix);
        }
        g_free(path_hwmon);
    }
    hwmon_first_run = FALSE;
}

static void read_sensors_acpi(void) {
    const gchar *path_tz = "/proc/acpi/thermal_zone";

    if (g_file_test(path_tz, G_FILE_TEST_EXISTS)) {
        GDir *tz;

        if ((tz = g_dir_open(path_tz, 0, NULL))) {
            const gchar *entry;

            while ((entry = g_dir_read_name(tz))) {
                gchar *path =
                    g_strdup_printf("%s/%s/temperature", path_tz, entry);
                gchar *contents;

                if (g_file_get_contents(path, &contents, NULL, NULL)) {
                    int temperature;

                    sscanf(contents, "temperature: %d C", &temperature);

                    add_sensor("Temperature",
                               entry,
                               "ACPI Thermal Zone",
                               temperature,
                               "\302\260C",
                               "therm");
                }
            }

            g_dir_close(tz);
        }
    }
}

static void read_sensors_sys_thermal(void) {
    const gchar *path_tz = "/sys/class/thermal";

    if (g_file_test(path_tz, G_FILE_TEST_EXISTS)) {
        GDir *tz;

        if ((tz = g_dir_open(path_tz, 0, NULL))) {
            const gchar *entry;
            gchar *temp = g_strdup("");

            while ((entry = g_dir_read_name(tz))) {
                gchar *path = g_strdup_printf("%s/%s/temp", path_tz, entry);
                gchar *contents;

                if (g_file_get_contents(path, &contents, NULL, NULL)) {
                    int temperature;

                    sscanf(contents, "%d", &temperature);

                    add_sensor("Temperature",
                               entry,
                               "thermal",
                               temperature / 1000.0,
                               "\302\260C",
                               "therm");

                    g_free(contents);
                }
            }

            g_dir_close(tz);
        }
    }
}

static void read_sensors_omnibook(void) {
    const gchar *path_ob = "/proc/omnibook/temperature";
    gchar *contents;

    if (g_file_get_contents(path_ob, &contents, NULL, NULL)) {
        int temperature;

        sscanf(contents, "CPU temperature: %d C", &temperature);

        add_sensor("Temperature",
                   "CPU",
                   "omnibook",
                   temperature,
                   "\302\260C",
                   "therm");

        g_free(contents);
    }
}

static void read_sensors_hddtemp(void) {
    Socket *s;
    gchar buffer[1024];
    gint len = 0;

    if (!(s = sock_connect("127.0.0.1", 7634)))
        return;

    while (!len)
        len = sock_read(s, buffer, sizeof(buffer));
    sock_close(s);

    if (len > 2 && buffer[0] == '|' && buffer[1] == '/') {
        gchar **disks;
        int i;

        disks = g_strsplit(buffer, "\n", 0);
        for (i = 0; disks[i]; i++) {
            gchar **fields = g_strsplit(disks[i] + 1, "|", 5);

            /*
             * 0 -> /dev/hda
             * 1 -> FUJITSU MHV2080AH
             * 2 -> 41
             * 3 -> C
             */
            const gchar *unit = strcmp(fields[3], "C")
                ? "\302\260C" : "\302\260F";
            add_sensor("Drive Temperature",
                       fields[1],
                       "hddtemp",
                       atoi(fields[2]),
                       unit,
                       "therm");

            g_strfreev(fields);
        }

        g_strfreev(disks);
    }
}

void read_sensors_udisks2(void) {
    GSList *node;
    GSList *temps;
    udiskt *disk;

    temps = get_udisks2_temps();
    if (temps == NULL)
        return;

    for (node = temps; node != NULL; node = node->next) {
        disk = (udiskt *)node->data;
        add_sensor("Drive Temperature",
                   disk->drive,
                   "udisks2",
                   disk->temperature,
                   "\302\260C",
                   "therm");
        udiskt_free(disk);
    }
    g_slist_free(temps);
}

void scan_sensors_do(void) {
    g_free(sensors);
    g_free(sensor_icons);
    g_free(last_group);
    last_group = NULL;
    sensors = g_strdup("");
    sensor_icons = g_strdup("");

    g_free(lginterval);
    lginterval = g_strdup("");

    read_sensors_hwmon();
    read_sensors_acpi();
    read_sensors_sys_thermal();
    read_sensors_omnibook();
    read_sensors_hddtemp();
    read_sensors_udisks2();

    /* FIXME: Add support for  ibm acpi and more sensors */
}

void sensors_init(void) {
    sensor_labels =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    sensor_compute = g_hash_table_new(g_str_hash, g_str_equal);
}

void sensors_shutdown(void) {
    g_hash_table_destroy(sensor_labels);
    g_hash_table_destroy(sensor_compute);
}
