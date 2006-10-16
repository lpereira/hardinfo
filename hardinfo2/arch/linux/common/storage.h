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

static gchar *storage_icons = "";

static gboolean
remove_scsi_devices(gpointer key, gpointer value, gpointer data)
{
    if (!strncmp((gchar *) key, "SCSI", 4)) {
	g_free((gchar *) key);
	g_free((GtkTreeIter *) value);
	return TRUE;
    }
    return FALSE;
}

/* SCSI support by Pascal F.Martin <pascalmartin@earthlink.net> */
void
scan_scsi(void)
{
    FILE *proc_scsi;
    gchar buffer[256], *buf;
    gint n = 0;
    gint scsi_controller;
    gint scsi_channel;
    gint scsi_id;
    gint scsi_lun;
    gchar *vendor = NULL, *revision = NULL, *model = NULL;

    /* remove old devices from global device table */
    g_hash_table_foreach_remove(devices, remove_scsi_devices, NULL);

    if (!g_file_test("/proc/scsi/scsi", G_FILE_TEST_EXISTS))
	return;

    storage_list = g_strconcat(storage_list, "\n[SCSI Disks]\n", NULL);

    proc_scsi = fopen("/proc/scsi/scsi", "r");
    while (fgets(buffer, 256, proc_scsi)) {
	buf = g_strstrip(buffer);
	if (!strncmp(buf, "Host: scsi", 10)) {
	    sscanf(buf,
		   "Host: scsi%d Channel: %d Id: %d Lun: %d",
		   &scsi_controller, &scsi_channel, &scsi_id, &scsi_lun);

	    n++;
	} else if (!strncmp(buf, "Vendor: ", 8)) {
	    char *p;
	    char *rev = strstr(buf, "Rev: ");

	    model = strstr(buf, "Model: ");

	    if (model == NULL) {
		model = buf + strlen(buf);
	    }
	    p = model;
	    while (*(--p) == ' ');
	    *(++p) = 0;
	    vendor = g_strdup(buf + 8);

	    if (rev != NULL) {
		revision = g_strdup(rev + 5);
	    } else {
		rev = model + strlen(model);
	    }
	    p = rev;
	    while (*(--p) == ' ');
	    *(++p) = 0;
	    model = g_strdup_printf("%s %s", vendor, model + 7);

	} else if (!strncmp(buf, "Type:   ", 8)) {
	    char *p = strstr(buf, "ANSI SCSI revi");
	    gchar *type = NULL, *icon = NULL;

	    if (p != NULL) {
		while (*(--p) == ' ');
		*(++p) = 0;

                static struct {
                    char *type;
                    char *label;
                    char *icon;
                } type2icon[] = {
                    { "Direct-Access", "Disk", "hdd"},
                    { "Sequential-Access", "Tape", "tape"},
                    { "Printer", "Printer", "lpr"},
                    { "WORM", "CD-ROM", "cd"},
                    { "CD-ROM", "CD-ROM", "cd"},
                    { "Scanner", "Scanner", "scanner"},
                    { NULL, "Generic", "scsi"} 
                };
                int i;

                for (i = 0; type2icon[i].type != NULL; i++)
                    if (g_str_equal(buf + 8, type2icon[i].type))
                        break;

                type = type2icon[i].label;
                icon = type2icon[i].icon;
	    }
	    
	    gchar *devid = g_strdup_printf("SCSI%d", n);
	    storage_list = g_strdup_printf("%s$%s$%s=\n", storage_list, devid, model);
	    storage_icons = g_strdup_printf("%sIcon$%s$%s=%s.png\n", storage_icons, devid, model, icon);

	    gchar *strhash = g_strdup_printf("[Device Information]\n"
					     "Model=%s\n"
					     "Vendor=%s (%s)\n"
					     "Type=%s\n"
					     "Revision=%s\n"
					     "[SCSI Controller]\n"
					     "Controller=scsi%d\n"
					     "Channel=%d\n"
					     "ID=%d\n" "LUN=%d\n",
					     model,
					     vendor_get_name(model),
					     vendor_get_url(model),
					     type,
					     revision,
					     scsi_controller,
					     scsi_channel,
					     scsi_id,
					     scsi_lun);
	    g_hash_table_insert(devices, devid, strhash);

	    g_free(model);
	    g_free(revision);
	    g_free(vendor);
	}
    }
    fclose(proc_scsi);
}

static gboolean
remove_ide_devices(gpointer key, gpointer value, gpointer data)
{
    if (!strncmp((gchar *) key, "IDE", 3)) {
	g_free((gchar *) key);
	g_free((gchar *) value);

	return TRUE;
    }
    return FALSE;
}


void
scan_ide(void)
{
    FILE *proc_ide;
    gchar *device, iface, *model, *media, *pgeometry = NULL, *lgeometry =
	NULL;
    gint n = 0, i = 0, cache;

    /* remove old devices from global device table */
    g_hash_table_foreach_remove(devices, remove_ide_devices, NULL);

    storage_list = g_strdup_printf("%s\n[IDE Disks]\n", storage_list);

    iface = 'a';
    for (i = 0; i <= 16; i++) {
	device = g_strdup_printf("/proc/ide/hd%c/model", iface);
	if (g_file_test(device, G_FILE_TEST_EXISTS)) {
	    gchar buf[64];

	    cache = 0;

	    proc_ide = fopen(device, "r");
	    fgets(buf, 64, proc_ide);
	    fclose(proc_ide);

	    buf[strlen(buf) - 1] = 0;

	    model = g_strdup(buf);

	    g_free(device);

	    device = g_strdup_printf("/proc/ide/hd%c/media", iface);
	    proc_ide = fopen(device, "r");
	    fgets(buf, 64, proc_ide);
	    fclose(proc_ide);
	    buf[strlen(buf) - 1] = 0;

	    media = g_strdup(buf);

	    g_free(device);

	    device = g_strdup_printf("/proc/ide/hd%c/cache", iface);
	    if (g_file_test(device, G_FILE_TEST_EXISTS)) {
		proc_ide = fopen(device, "r");
		fscanf(proc_ide, "%d", &cache);
		fclose(proc_ide);
	    }
	    g_free(device);

	    device = g_strdup_printf("/proc/ide/hd%c/geometry", iface);
	    if (g_file_test(device, G_FILE_TEST_EXISTS)) {
		gchar *tmp;

		proc_ide = fopen(device, "r");

		fgets(buf, 64, proc_ide);
		for (tmp = buf; *tmp; tmp++) {
		    if (*tmp >= '0' && *tmp <= '9')
			break;
		}

		pgeometry = g_strdup(g_strstrip(tmp));

		fgets(buf, 64, proc_ide);
		for (tmp = buf; *tmp; tmp++) {
		    if (*tmp >= '0' && *tmp <= '9')
			break;
		}
		lgeometry = g_strdup(g_strstrip(tmp));

		fclose(proc_ide);
	    }
	    g_free(device);

	    n++;

	    gchar *devid = g_strdup_printf("IDE%d", n);

	    storage_list = g_strdup_printf("%s$%s$%s=\n", storage_list,
					 devid, model);
	    storage_icons = g_strdup_printf("%sIcon$%s$%s=%s.png\n", storage_icons, devid,
	                                  model, g_str_equal(media, "cdrom") ? \
	                                         "cdrom" : "hdd");

	    gchar *strhash = g_strdup_printf("[Device Information]\n"
					     "Model=%s\n"
					     "Vendor=%s (%s)\n"
					     "Device Name=hd%c\n"
					     "Media=%s\n" "Cache=%dkb\n",
					     model,
					     vendor_get_name(model),
					     vendor_get_url(model),
					     iface,
					     media,
					     cache);
	    if (pgeometry && lgeometry)
		strhash = g_strdup_printf("%s[Geometry]\n"
					  "Physical=%s\n"
					  "Logical=%s\n",
					  strhash, pgeometry, lgeometry);
            
	    g_hash_table_insert(devices, devid, strhash);

	    g_free(model);
	    model = "";

	    g_free(pgeometry);
	    pgeometry = NULL;
	    g_free(lgeometry);
	    lgeometry = NULL;
	} else
	    g_free(device);

	iface++;
    }
}
