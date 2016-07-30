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

#include "hardinfo.h"
#include "devices.h"

gchar *storage_icons = NULL;

/* SCSI support by Pascal F.Martin <pascalmartin@earthlink.net> */
void
__scan_scsi_devices(void)
{
    FILE *proc_scsi;
    gchar buffer[256], *buf;
    gint n = 0;
    gint scsi_controller = 0;
    gint scsi_channel = 0;
    gint scsi_id = 0 ;
    gint scsi_lun = 0;
    gchar *vendor = NULL, *revision = NULL, *model = NULL;
    gchar *scsi_storage_list;

    /* remove old devices from global device table */
    moreinfo_del_with_prefix("DEV:SCSI");

    if (!g_file_test("/proc/scsi/scsi", G_FILE_TEST_EXISTS))
	return;

    scsi_storage_list = g_strdup(_("\n[SCSI Disks]\n"));

    if ((proc_scsi = fopen("/proc/scsi/scsi", "r"))) {
        while (fgets(buffer, 256, proc_scsi)) {
            buf = g_strstrip(buffer);
            if (!strncmp(buf, "Host: scsi", 10)) {
                sscanf(buf,
                       "Host: scsi%d Channel: %d Id: %d Lun: %d",
                       &scsi_controller, &scsi_channel, &scsi_id, &scsi_lun);

                n++;
            } else if (!strncmp(buf, "Vendor: ", 8)) {
                buf[17] = '\0';
                buf[41] = '\0';
                buf[53] = '\0';

                vendor   = g_strdup(g_strstrip(buf + 8));
                model    = g_strdup_printf("%s %s", vendor, g_strstrip(buf + 24));
                revision = g_strdup(g_strstrip(buf + 46));
            } else if (!strncmp(buf, "Type:   ", 8)) {
                char *p;
                gchar *type = NULL, *icon = NULL;

                if (!(p = strstr(buf, "ANSI SCSI revision"))) {
                    p = strstr(buf, "ANSI  SCSI revision");
                }

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
                        { "WORM", "CD-ROM", "cdrom"},
                        { "CD-ROM", "CD-ROM", "cdrom"},
                        { "Scanner", "Scanner", "scanner"},
                        { "Flash Disk", "USB Flash Disk", "usbfldisk" },
                        { NULL, "Generic", "scsi"}
                    };
                    int i;

                    if (model && strstr(model, "Flash Disk")) {
                      type = "Flash Disk";
                      icon = "usbfldisk";
                    } else {
                      for (i = 0; type2icon[i].type != NULL; i++)
                          if (g_str_equal(buf + 8, type2icon[i].type))
                              break;

                      type = type2icon[i].label;
                      icon = type2icon[i].icon;
                    }
                }

                gchar *devid = g_strdup_printf("SCSI%d", n);
                scsi_storage_list = h_strdup_cprintf("$%s$%s=\n", scsi_storage_list, devid, model);
                storage_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n", storage_icons, devid, model, icon);

                gchar *strhash = g_strdup_printf(_("[Device Information]\n"
                                                 "Model=%s\n"), model);

                const gchar *url = vendor_get_url(model);
                if (url) {
                  strhash = h_strdup_cprintf(_("Vendor=%s (%s)\n"),
                                             strhash,
                                             vendor_get_name(model),
                                             url);
                } else {
                  strhash = h_strdup_cprintf(_("Vendor=%s\n"),
                                             strhash,
                                             vendor_get_name(model));
                }

                strhash = h_strdup_cprintf(_("Type=%s\n"
                                           "Revision=%s\n"
                                           "[SCSI Controller]\n"
                                           "Controller=scsi%d\n"
                                           "Channel=%d\n"
                                           "ID=%d\n" "LUN=%d\n"),
                                           strhash,
                                           type,
                                           revision,
                                           scsi_controller,
                                           scsi_channel,
                                           scsi_id,
                                           scsi_lun);
                moreinfo_add_with_prefix("DEV", devid, strhash);
                g_free(devid);

                g_free(model);
                g_free(revision);
                g_free(vendor);

                scsi_controller = scsi_channel = scsi_id = scsi_lun = 0;
            }
        }
        fclose(proc_scsi);
    }

    if (n) {
      storage_list = h_strconcat(storage_list, scsi_storage_list, NULL);
      g_free(scsi_storage_list);
    }
}

void __scan_ide_devices(void)
{
    FILE *proc_ide;
    gchar *device, iface, *model, *media, *pgeometry = NULL, *lgeometry = NULL;
    gint n = 0, i = 0, cache, nn = 0;
    gchar *capab = NULL, *speed = NULL, *driver = NULL, *ide_storage_list;

    /* remove old devices from global device table */
    moreinfo_del_with_prefix("DEV:IDE");

    ide_storage_list = g_strdup(_("\n[IDE Disks]\n"));

    iface = 'a';
    for (i = 0; i <= 16; i++) {
	device = g_strdup_printf("/proc/ide/hd%c/model", iface);
	if (g_file_test(device, G_FILE_TEST_EXISTS)) {
	    gchar buf[128];

	    cache = 0;

	    proc_ide = fopen(device, "r");
	    if (!proc_ide)
	        continue;

	    (void) fgets(buf, 128, proc_ide);
	    fclose(proc_ide);

	    buf[strlen(buf) - 1] = 0;

	    model = g_strdup(buf);

	    g_free(device);

	    device = g_strdup_printf("/proc/ide/hd%c/media", iface);
	    proc_ide = fopen(device, "r");
	    if (!proc_ide) {
	        free(model);
	        continue;
            }

	    (void) fgets(buf, 128, proc_ide);
	    fclose(proc_ide);
	    buf[strlen(buf) - 1] = 0;

	    media = g_strdup(buf);
	    if (g_str_equal(media, "cdrom")) {
		/* obtain cd-rom drive information from cdrecord */
		GTimer *timer;
		gchar *tmp = g_strdup_printf("cdrecord dev=/dev/hd%c -prcap 2>/dev/stdout", iface);
		FILE *prcap;

		if ((prcap = popen(tmp, "r"))) {
		    /* we need a timeout so cdrecord does not try to get information on cd drives
		       with inserted media, which is not possible currently. half second should be
		       enough. */
		    timer = g_timer_new();
		    g_timer_start(timer);

		    while (fgets(buf, 128, prcap)
			   && g_timer_elapsed(timer, NULL) < 0.5) {
			if (g_str_has_prefix(buf, "  Does")) {
			    if (g_str_has_suffix(buf, "media\n")
				&& !strstr(buf, "speed")) {
				gchar *media_type = g_strstrip(strstr(buf, "Does "));
				gchar **ttmp = g_strsplit(media_type, " ", 0);

				capab = h_strdup_cprintf("\nCan %s#%d=%s\n", capab, ttmp[1], ++nn, ttmp[2]);

				g_strfreev(ttmp);
			    } else if (strstr(buf, "Buffer-Underrun-Free")) {
				capab =
				    h_strdup_cprintf
				    ("\nSupports BurnProof=%s\n", capab, strstr(buf, "Does not") ? "No" : "Yes");
			    } else if (strstr(buf, "multi-session")) {
				capab =
				    h_strdup_cprintf
				    ("\nCan read multi-session CDs=%s\n",
				     capab, strstr(buf, "Does not") ? "No" : "Yes");
			    } else if (strstr(buf, "audio CDs")) {
				capab =
				    h_strdup_cprintf
				    ("\nCan play audio CDs=%s\n", capab, strstr(buf, "Does not") ? "No" : "Yes");
			    } else if (strstr(buf, "PREVENT/ALLOW")) {
				capab =
				    h_strdup_cprintf
				    ("\nCan lock media=%s\n", capab, strstr(buf, "Does not") ? "No" : "Yes");
			    }
			} else if ((strstr(buf, "read")
				    || strstr(buf, "write"))
				   && strstr(buf, "kB/s")) {
			    speed =
				g_strconcat(speed ? speed : "", strreplacechr(g_strstrip(buf), ":", '='), "\n", NULL);
			} else if (strstr(buf, "Device seems to be")) {
			    driver = g_strdup_printf(_("Driver=%s\n"), strchr(buf, ':') + 1);
			}
		    }

		    pclose(prcap);
		    g_timer_destroy(timer);
		}

		g_free(tmp);
	    }
	    g_free(device);

	    device = g_strdup_printf("/proc/ide/hd%c/cache", iface);
	    if (g_file_test(device, G_FILE_TEST_EXISTS)) {
		proc_ide = fopen(device, "r");
		if (proc_ide) {
                    (void) fscanf(proc_ide, "%d", &cache);
                    fclose(proc_ide);
                } else {
                    cache = 0;
                }
	    }
	    g_free(device);

	    device = g_strdup_printf("/proc/ide/hd%c/geometry", iface);
	    if (g_file_test(device, G_FILE_TEST_EXISTS)) {
		gchar *tmp;

		proc_ide = fopen(device, "r");
		if (proc_ide) {
                    (void) fgets(buf, 64, proc_ide);
                    for (tmp = buf; *tmp; tmp++) {
                        if (*tmp >= '0' && *tmp <= '9')
                            break;
                    }

                    pgeometry = g_strdup(g_strstrip(tmp));

                    (void) fgets(buf, 64, proc_ide);
                    for (tmp = buf; *tmp; tmp++) {
                        if (*tmp >= '0' && *tmp <= '9')
                            break;
                    }
                    lgeometry = g_strdup(g_strstrip(tmp));

                    fclose(proc_ide);
                } else {
                    pgeometry = g_strdup("Unknown");
                    lgeometry = g_strdup("Unknown");
                }

	    }
	    g_free(device);

	    n++;

	    gchar *devid = g_strdup_printf("IDE%d", n);

	    ide_storage_list = h_strdup_cprintf("$%s$%s=\n", ide_storage_list, devid, model);
	    storage_icons =
		h_strdup_cprintf("Icon$%s$%s=%s.png\n", storage_icons,
				 devid, model, g_str_equal(media, "cdrom") ? "cdrom" : "hdd");

	    gchar *strhash = g_strdup_printf(_("[Device Information]\n" "Model=%s\n"),
					     model);

	    const gchar *url = vendor_get_url(model);

	    if (url) {
		strhash = h_strdup_cprintf(_("Vendor=%s (%s)\n"), strhash, vendor_get_name(model), url);
	    } else {
		strhash = h_strdup_cprintf(_("Vendor=%s\n"), strhash, vendor_get_name(model));
	    }

	    strhash = h_strdup_cprintf(_("Device Name=hd%c\n"
					 "Media=%s\n" "Cache=%dkb\n"), strhash, iface, media, cache);
	    if (driver) {
		strhash = h_strdup_cprintf("%s\n", strhash, driver);

		g_free(driver);
		driver = NULL;
	    }

	    if (pgeometry && lgeometry) {
		strhash = h_strdup_cprintf(_("[Geometry]\n"
					     "Physical=%s\n" "Logical=%s\n"), strhash, pgeometry, lgeometry);

		g_free(pgeometry);
		pgeometry = NULL;
		g_free(lgeometry);
		lgeometry = NULL;
	    }

	    if (capab) {
		strhash = h_strdup_cprintf(_("[Capabilities]\n%s"), strhash, capab);

		g_free(capab);
		capab = NULL;
	    }

	    if (speed) {
		strhash = h_strdup_cprintf(_("[Speeds]\n%s"), strhash, speed);

		g_free(speed);
		speed = NULL;
	    }

	    moreinfo_add_with_prefix("DEV", devid, strhash);
	    g_free(devid);
	    g_free(model);
	} else {
	    g_free(device);
        }

	iface++;
    }

    if (n) {
	storage_list = h_strconcat(storage_list, ide_storage_list, NULL);
	g_free(ide_storage_list);
    }
}
