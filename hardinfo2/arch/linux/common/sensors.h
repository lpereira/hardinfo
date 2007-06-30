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

static gchar *sensors = NULL;
static GHashTable *sensor_labels = NULL;
static GHashTable *sensor_compute = NULL;

static void read_sensor_labels(gchar * driver)
{
    FILE *conf;
    gchar buf[256], *line, *p;
    gboolean lock = FALSE;
    gint i;

    sensor_labels = g_hash_table_new_full(g_str_hash, g_str_equal,
					  g_free, g_free);
    sensor_compute = g_hash_table_new(g_str_hash, g_str_equal);

    conf = fopen("/etc/sensors.conf", "r");
    if (!conf)
	return;

    while (fgets(buf, 256, conf)) {
	line = buf;

	remove_linefeed(line);
	strend(line, '#');

	if (*line == '\0') {
	    continue;
	} else if (lock && strstr(line, "label")) {	/* label lines */
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
	} else if (lock && strstr(line, "ignore")) {	/* ignore lines */
	    p = strstr(line, "ignore") + 6;
	    if (!strchr(p, ' '))
		continue;

	    while (*p == ' ')
		p++;
	    g_hash_table_insert(sensor_labels, g_strdup(p), "ignore");
	} else if (lock && strstr(line, "compute")) {	/* compute lines */
	    gchar **formulas =
		g_strsplit(strstr(line, "compute") + 7, " ", 0);
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
	} else if (g_str_has_prefix(line, "chip")) {	/* chip lines (delimiter) */
	    if (lock == FALSE) {
		gchar **chips = g_strsplit(line, " ", 0);

		for (i = 1; chips[i]; i++) {
		    strend(chips[i], '*');

		    if (g_str_has_prefix(driver, chips[i] + 1)) {
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

static gchar *get_sensor_label(gchar * sensor)
{
    gchar *ret;

    ret = g_hash_table_lookup(sensor_labels, sensor);
    if (!ret)
	ret = g_strdup(sensor);
    else
	ret = g_strdup(ret);

    return ret;
}

static float adjust_sensor(gchar * name, float value)
{
    GSList *postfix;

    postfix = g_hash_table_lookup(sensor_compute, name);
    if (!postfix)
	return value;

    return math_postfix_eval(postfix, value);
}


static void read_sensors_hwmon(void)
{
    int hwmon, count;
    gchar *path_hwmon, *path_sensor, *tmp, *driver, *name, *mon;
    hwmon = 0;

    path_hwmon =
	g_strdup_printf("/sys/class/hwmon/hwmon%d/device/", hwmon);
    while (g_file_test(path_hwmon, G_FILE_TEST_EXISTS)) {
	tmp = g_strdup_printf("%sdriver", path_hwmon);
	driver = g_file_read_link(tmp, NULL);
	g_free(tmp);

	tmp = g_path_get_basename(driver);
	g_free(driver);
	driver = tmp;

	if (!sensor_labels) {
	    read_sensor_labels(driver);
	}

	sensors = g_strconcat(sensors, "[Cooling Fans]\n", NULL);
	for (count = 1;; count++) {
	    path_sensor =
		g_strdup_printf("%sfan%d_input", path_hwmon, count);
	    if (!g_file_get_contents(path_sensor, &tmp, NULL, NULL)) {
		g_free(path_sensor);
		break;
	    }

	    mon = g_strdup_printf("fan%d", count);
	    name = get_sensor_label(mon);
	    if (!g_str_equal(name, "ignore")) {
		sensors = g_strdup_printf("%s%s=%.0fRPM\n",
					  sensors, name,
					  adjust_sensor(mon, atof(tmp)));
	    }

	    g_free(name);
	    g_free(mon);
	    g_free(tmp);
	    g_free(path_sensor);
	}

	sensors = g_strconcat(sensors, "[Temperatures]\n", NULL);
	for (count = 1;; count++) {
	    path_sensor =
		g_strdup_printf("%stemp%d_input", path_hwmon, count);
	    if (!g_file_get_contents(path_sensor, &tmp, NULL, NULL)) {
		g_free(path_sensor);
		break;
	    }

	    mon = g_strdup_printf("temp%d", count);
	    name = get_sensor_label(mon);
	    if (!g_str_equal(name, "ignore")) {
		sensors = g_strdup_printf("%s%s=%.2f\302\260C\n",
					  sensors, name,
					  adjust_sensor(mon,
							atof(tmp) /
							1000.0));
	    }

	    g_free(tmp);
	    g_free(name);
	    g_free(path_sensor);
	    g_free(mon);
	}

	sensors = g_strconcat(sensors, "[Voltage Values]\n", NULL);
	for (count = 0;; count++) {
	    path_sensor =
		g_strdup_printf("%sin%d_input", path_hwmon, count);
	    if (!g_file_get_contents(path_sensor, &tmp, NULL, NULL)) {
		g_free(path_sensor);
		break;
	    }


	    mon = g_strdup_printf("in%d", count);
	    name = get_sensor_label(mon);
	    if (!g_str_equal(name, "ignore")) {
		sensors = g_strdup_printf("%s%s=%.3fV\n",
					  sensors, name,
					  adjust_sensor(mon,
							atof(tmp) /
							1000.0));
	    }

	    g_free(tmp);
	    g_free(mon);
	    g_free(name);
	    g_free(path_sensor);
	}

	g_free(path_hwmon);
	g_free(driver);
	path_hwmon =
	    g_strdup_printf("/sys/class/hwmon/hwmon%d/device/", ++hwmon);
    }

    g_free(path_hwmon);

}

static void read_sensors_acpi(void)
{
    const gchar *path_tz = "/proc/acpi/thermal_zone";

    if (g_file_test(path_tz, G_FILE_TEST_EXISTS)) {
	GDir *tz;

	if ((tz = g_dir_open(path_tz, 0, NULL))) {
	    const gchar *entry;
	    gchar *temp = "";


	    while ((entry = g_dir_read_name(tz))) {
		gchar *path =
		    g_strdup_printf("%s/%s/temperature", path_tz, entry);
		gchar *contents;

		if (g_file_get_contents(path, &contents, NULL, NULL)) {
		    int temperature;

		    sscanf(contents, "temperature: %d C", &temperature);

		    temp = g_strdup_printf("%s\n%s=%d\302\260C\n",
					      temp, entry, temperature);

		    g_free(contents);
		}
	    }

	    if (*temp != '\0')
    	        sensors =
	    	    g_strdup_printf("%s\n[ACPI Thermal Zone]\n%s",
	    	                    sensors, temp);

	    g_dir_close(tz);
	}
    }

}

static void read_sensors_omnibook(void)
{
    const gchar *path_ob = "/proc/omnibook/temperature";
    gchar *contents;

    if (g_file_get_contents(path_ob, &contents, NULL, NULL)) {
        int temperature;

        sscanf(contents, "CPU temperature: %d C", &temperature);

        sensors = g_strdup_printf("%s\n[Omnibook]\n"
                                  "CPU temperature=%d\302\260C\n",
                                  sensors, temperature);

        g_free(contents);
    }
}

static void read_sensors_hddtemp(void)
{
    Socket *s;
    static gchar *old = NULL;
    gchar buffer[1024];
    gint len = 0;

    if ((s = sock_connect("127.0.0.1", 7634))) {
	while (!len)
	    len = sock_read(s, buffer, sizeof(buffer));
	sock_close(s);

	if (len > 2 && buffer[0] == '|' && buffer[1] == '/') {
	    gchar **disks;
	    int i;

	    if (old)
		g_free(old);

	    old = g_strdup("[Hard Disk Temperature]\n");

	    disks = g_strsplit(buffer, "\n", 0);
	    for (i = 0; disks[i]; i++) {
		gchar **fields = g_strsplit(disks[i] + 1, "|", 5);

		/*
		 * 0 -> /dev/hda
		 * 1 -> FUJITSU MHV2080AH
		 * 2 -> 41
		 * 3 -> C
		 */
		old = g_strdup_printf("%s\n"
				      "%s (%s)=%s\302\260%s\n",
				      old,
				      fields[1], fields[0],
				      fields[2], fields[3]);

		g_strfreev(fields);
	    }

	    g_strfreev(disks);
	}
    } else {
	g_free(old);
	old = NULL;
    }

    if (old) {
	sensors = g_strconcat(sensors, "\n", old, NULL);
    }
}

static void __scan_sensors(void)
{
    if (sensors)
	g_free(sensors);

    sensors = g_strdup("");

    read_sensors_hwmon();
    read_sensors_acpi();
    read_sensors_omnibook();
    read_sensors_hddtemp();

    /* FIXME: Add support for  ibm acpi and more sensors */
}
