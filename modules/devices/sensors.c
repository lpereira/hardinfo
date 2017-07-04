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

gchar *sensors = NULL;
GHashTable *sensor_compute = NULL;
GHashTable *sensor_labels = NULL;

static void read_sensor_labels(gchar *driver) {
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
            gchar *name = NULL, *value = NULL;

            for (i = 0; names[i]; i++) {
                if (names[i][0] == '\0')
                    continue;

                if (!name)
                    name = g_strdup(names[i]);
                else if (!value)
                    value = g_strdup(names[i]);
                else
                    value = g_strconcat(value, " ", names[i], NULL);
            }

            remove_quotes(value);
            g_hash_table_insert(sensor_labels, name, value);

            g_strfreev(names);
        } else if (lock && strstr(line, "ignore")) { /* ignore lines */
            p = strstr(line, "ignore") + 6;
            if (!strchr(p, ' '))
                continue;

            while (*p == ' ')
                p++;
            g_hash_table_insert(sensor_labels, g_strdup(p), "ignore");
        } else if (lock && strstr(line, "compute")) { /* compute lines */
            gchar **formulas = g_strsplit(strstr(line, "compute") + 7, " ", 0);
            gchar *name = NULL, *formula = NULL;

            for (i = 0; formulas[i]; i++) {
                if (formulas[i][0] == '\0')
                    continue;
                if (formulas[i][0] == ',')
                    break;

                if (!name)
                    name = g_strdup(formulas[i]);
                else if (!formula)
                    formula = g_strdup(formulas[i]);
                else
                    formula = g_strconcat(formula, formulas[i], NULL);
            }

            g_strfreev(formulas);
            g_hash_table_insert(sensor_compute, name,
                                math_string_to_postfix(formula));
        } else if (g_str_has_prefix(line,
                                    "chip")) { /* chip lines (delimiter) */
            if (lock == FALSE) {
                gchar **chips = g_strsplit(line, " ", 0);

                for (i = 1; chips[i]; i++) {
                    strend(chips[i], '*');

                    if (g_str_has_prefix(chips[i] + 1, driver)) {
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
                       const char *driver,
                       double value,
                       const char *unit) {
    char key[64];

    sensors = h_strdup_cprintf("%s/%s=%.2f%s|%s\n", sensors,
        driver, sensor, value, unit, type);

    snprintf(key, sizeof(key), "%s/%s", driver, sensor);
    moreinfo_add_with_prefix("DEV", key, g_strdup_printf("%.2f%s", value, unit));

    lginterval = h_strdup_cprintf("UpdateInterval$%s=1000\n", lginterval, key);
}

static gchar *get_sensor_label(gchar *sensor) {
    gchar *ret;

    ret = g_hash_table_lookup(sensor_labels, sensor);
    if (!ret)
        ret = g_strdup(sensor);
    else
        ret = g_strdup(ret);

    return ret;
}

static float adjust_sensor(gchar *name, float value) {
    GSList *postfix;

    postfix = g_hash_table_lookup(sensor_compute, name);
    if (!postfix)
        return value;

    return math_postfix_eval(postfix, value);
}

static char *get_sensor_path(int number, const char *prefix) {
    return g_strdup_printf("/sys/class/hwmon/hwmon%d/%s", number, prefix);
}

static char *determine_driver_for_hwmon_path(char *path) {
    char *tmp, *driver;

    tmp = g_strdup_printf("%s/device/driver", path);
    driver = g_file_read_link(tmp, NULL);
    g_free(tmp);

    if (driver) {
        tmp = g_path_get_basename(driver);
        g_free(driver);
        driver = tmp;
    } else {
        tmp = g_strdup_printf("%s/device", path);
        driver = g_file_read_link(tmp, NULL);
        g_free(tmp);
    }

    if (!driver) {
        tmp = g_strdup_printf("%s/name", path);
        if (!g_file_get_contents(tmp, &driver, NULL, NULL)) {
            driver = g_strdup("unknown");
        } else {
            driver = g_strstrip(driver);
        }
        g_free(tmp);
    }

    return driver;
}

struct HwmonSensor {
    const char *friendly_name;
    const char *path_format;
    const char *key_format;
    const char *unit;
    const float adjust_ratio;
    const int begin_at;
};

static const struct HwmonSensor hwmon_sensors[] = {
    {
        "Fan",
        "%s/fan%d_input",
        "fan%d",
        "RPM",
        1.0,
        1
    },
    {
        "Temperature",
        "%s/temp%d_input",
        "temp%d",
        "\302\260C",
        1000.0,
        1
    },
    {
        "Voltage",
        "%s/in%d_input",
        "in%d",
        "V",
        1000.0,
        0
    },
    { }
};

static const char *hwmon_prefix[] = {"device", "", NULL};

static void read_sensors_hwmon(void) {
    int hwmon, count;
    gchar *path_hwmon, *path_sensor, *tmp, *driver, *name, *mon;
    const char **prefix;

    for (prefix = hwmon_prefix; *prefix; prefix++) {
        hwmon = 0;
        path_hwmon = get_sensor_path(hwmon, *prefix);
        while (path_hwmon && g_file_test(path_hwmon, G_FILE_TEST_EXISTS)) {
            const struct HwmonSensor *sensor;

            driver = determine_driver_for_hwmon_path(path_hwmon);
            DEBUG("hwmon%d has driver=%s", hwmon, driver);

            if (!sensor_labels) {
                read_sensor_labels(driver);
            }

            for (sensor = hwmon_sensors; sensor->friendly_name; sensor++) {
                DEBUG("current sensor type=%s", sensor->friendly_name);

                for (count = sensor->begin_at;; count++) {
                    path_sensor =
                        g_strdup_printf(sensor->path_format, path_hwmon, count);
                    DEBUG("should be reading from %s", path_sensor);
                    if (!g_file_get_contents(path_sensor, &tmp, NULL, NULL)) {
                        g_free(path_sensor);
                        if (count < 256)
                            continue; // brute-force find all
                        else
                            break;
                    }

                    mon = g_strdup_printf(sensor->key_format, count);
                    name = get_sensor_label(mon);
                    if (!g_str_equal(name, "ignore")) {
                        float adjusted = adjust_sensor(mon,
                            atof(tmp) / sensor->adjust_ratio);

                        add_sensor(sensor->friendly_name,
                                   name,
                                   driver,
                                   adjusted,
                                   sensor->unit);
                    }

                    g_free(tmp);
                    g_free(mon);
                    g_free(name);
                    g_free(path_sensor);
                }
            }

            g_free(path_hwmon);
            g_free(driver);

            path_hwmon = get_sensor_path(++hwmon, *prefix);
        }

        g_free(path_hwmon);
    }
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
                               "\302\260C");
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
                               "\302\260C");

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
                   "\302\260C\n");

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
            add_sensor("Hard Drive",
                       fields[1],
                       "hddtemp",
                       atoi(fields[2]),
                       unit);

            g_strfreev(fields);
        }

        g_strfreev(disks);
    }
}

void scan_sensors_do(void) {
    g_free(sensors);
    sensors = g_strdup("");

    g_free(lginterval);
    lginterval = g_strdup("");

    read_sensors_hwmon();
    read_sensors_acpi();
    read_sensors_sys_thermal();
    read_sensors_omnibook();
    read_sensors_hddtemp();
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
