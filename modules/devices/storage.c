/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    modified by Ondrej Čerman (2019)
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
#include "udisks2_util.h"

gchar *storage_icons = NULL;

gboolean __scan_udisks2_devices(void) {
    GSList *node, *drives;
    udiskd *disk;
    udiskp *part;
    udisksa *attrib;
    gchar *udisks2_storage_list = NULL, *features = NULL, *moreinfo = NULL;
    gchar *devid, *label, *size, *tmp = NULL, *media_comp = NULL;
    const gchar *url, *vendor_str, *media_label, *alabel, *icon, *media_curr = NULL;
    int n = 0, i, j;

    // http://storaged.org/doc/udisks2-api/latest/gdbus-org.freedesktop.UDisks2.Drive.html#gdbus-property-org-freedesktop-UDisks2-Drive.MediaCompatibility
    static struct {
        char *media;
        char *label;
        char *icon;
    } media_info[] = {
        { "thumb",                  "Thumb-drive",        "usbfldisk" },
        { "flash",                  "Flash Card",         "usbfldisk" },
        { "flash_cf",               "CompactFlash",       "usbfldisk" },
        { "flash_ms",               "MemoryStick",        "usbfldisk" },
        { "flash_sm",               "SmartMedia",         "usbfldisk" },
        { "flash_sd",               "SD",                 "usbfldisk" },
        { "flash_sdhc",             "SDHC",               "usbfldisk" },
        { "flash_sdxc",             "SDXC",               "usbfldisk" },
        { "flash_mmc",              "MMC",                "usbfldisk" },
        { "floppy",                 "Floppy Disk",        "media-floppy" },
        { "floppy_zip",             "Zip Disk",           "media-floppy" },
        { "floppy_jaz",             "Jaz Disk",           "media-floppy" },
        { "optical",                "Optical Disc",       "cdrom" },
        { "optical_cd",             "CD-ROM",             "cdrom" },
        { "optical_cd_r",           "CD-R",               "cdrom" },
        { "optical_cd_rw",          "CD-RW",              "cdrom" },
        { "optical_dvd",            "DVD-ROM",            "cdrom" },
        { "optical_dvd_r",          "DVD-R",              "cdrom" },
        { "optical_dvd_rw",         "DVD-RW",             "cdrom" },
        { "optical_dvd_ram",        "DVD-RAM",            "cdrom" },
        { "optical_dvd_plus_r",     "DVD+R" ,             "cdrom" },
        { "optical_dvd_plus_rw",    "DVD+RW" ,            "cdrom" },
        { "optical_dvd_plus_r_dl",  "DVD+R DL",           "cdrom" },
        { "optical_dvd_plus_rw_dl", "DVD+RW DL",          "cdrom" },
        { "optical_bd",             "BD-ROM",             "cdrom" },
        { "optical_bd_r",           "BD-R",               "cdrom" },
        { "optical_bd_re",          "BD-RE",              "cdrom" },
        { "optical_hddvd",          "HD DVD-ROM",         "cdrom" },
        { "optical_hddvd_r",        "HD DVD-R",           "cdrom" },
        { "optical_hddvd_rw",       "HD DVD-RW",          "cdrom" },
        { "optical_mo",             "MO Disc",            "cdrom" },
        { "optical_mrw",            "MRW Media",          "cdrom" },
        { "optical_mrw_w",          "MRW Media (write)",  "cdrom" },
        { NULL, NULL }
    };

    struct {
        char *identifier;
        char *label;
    } smart_attrib_info[] = {
        { "raw-read-error-rate",          _("Read Error Rate" ) },
        { "throughput-performance",       _("Throughput Performance") },
        { "spin-up-time",                 _("Spin-Up Time") },
        { "start-stop-count",             _("Start/Stop Count") },
        { "reallocated-sector-count",     _("Reallocated Sector Count") },
        { "read-channel-margin",          _("Read Channel Margin") },
        { "seek-error-rate",              _("Seek Error Rate") },
        { "seek-time-performance",        _("Seek Timer Performance") },
        { "power-on-hours",               _("Power-On Hours") },
        { "spin-retry-count",             _("Spin Retry Count") },
        { "calibration-retry-count",      _("Calibration Retry Count") },
        { "power-cycle-count",            _("Power Cycle Count") },
        { "read-soft-error-rate",         _("Soft Read Error Rate") },
        { "runtime-bad-block-total",      _("Runtime Bad Block") },
        { "end-to-end-error",             _("End-to-End error") },
        { "reported-uncorrect",           _("Reported Uncorrectable Errors") },
        { "command-timeout",              _("Command Timeout") },
        { "high-fly-writes",              _("High Fly Writes") },
        { "airflow-temperature-celsius",  _("Airflow Temperature") },
        { "g-sense-error-rate",           _("G-sense Error Rate") },
        { "power-off-retract-count",      _("Power-off Retract Count") },
        { "load-cycle-count",             _("Load Cycle Count") },
        { "temperature-celsius-2",        _("Temperature") },
        { "hardware-ecc-recovered",       _("Hardware ECC Recovered") },
        { "reallocated-event-count",      _("Reallocation Event Count") },
        { "current-pending-sector",       _("Current Pending Sector Count") },
        { "offline-uncorrectable",        _("Uncorrectable Sector Count") },
        { "udma-crc-error-count",         _("UltraDMA CRC Error Count") },
        { "multi-zone-error-rate",        _("Multi-Zone Error Rate") },
        { "soft-read-error-rate",         _("Soft Read Error Rate") },
        { "run-out-cancel",               _("Run Out Cancel") },
        { "flying-height",                _("Flying Height") },
        { "spin-high-current",            _("Spin High Current") },
        { "spin-buzz",                    _("Spin Buzz") },
        { "offline-seek-performance",     _("Offline Seek Performance") },
        { "disk-shift",                   _("Disk Shift") },
        { "g-sense-error-rate-2",         _("G-Sense Error Rate") },
        { "loaded-hours",                 _("Loaded Hours") },
        { "load-retry-count",             _("Load/Unload Retry Count") },
        { "load-friction",                _("Load Friction") },
        { "load-cycle-count-2",           _("Load/Unload Cycle Count") },
        { "load-in-time",                 _("Load-in time") },
        { "torq-amp-count",               _("Torque Amplification Count") },
        { "power-off-retract-count-2",    _("Power-Off Retract Count") },
        { "head-amplitude",               _("GMR Head Amplitude") },
        { "temperature-celsius",          _("Temperature") },
        { "endurance-remaining",          _("Endurance Remaining") },
        { "power-on-seconds-2",           _("Power-On Hours") },
        { "good-block-rate",              _("Good Block Rate") },
        { "head-flying-hours",            _("Head Flying Hours") },
        { "read-error-retry-rate",        _("Read Error Retry Rate") },
        { "total-lbas-written",           _("Total LBAs Written") },
        { "total-lbas-read",              _("Total LBAs Read") },
        { NULL, NULL }
    };

    moreinfo_del_with_prefix("DEV:UDISKS");
    udisks2_storage_list = g_strdup(_("\n[UDisks2]\n"));

    drives = get_udisks2_all_drives_info();
    for (node = drives; node != NULL; node = node->next) {
        disk = (udiskd *)node->data;
        devid = g_strdup_printf("UDISKS%d", n++);

        if (disk->vendor && strlen(disk->vendor) > 0) {
            label = g_strdup_printf("%s %s", disk->vendor, disk->model);
            vendor_str = disk->vendor;
        }
        else{
            label = g_strdup(disk->model);
            vendor_str = disk->model;
        }

        icon = NULL;

        media_curr = disk->media;
        if (disk->media){
            for (j = 0; media_info[j].media != NULL; j++) {
                if (g_strcmp0(disk->media, media_info[j].media) == 0) {
                    media_curr = media_info[j].label;
                    break;
                }
            }
        }

        if (disk->media_compatibility){
            for (i = 0; disk->media_compatibility[i] != NULL; i++){
                media_label = disk->media_compatibility[i];

                for (j = 0; media_info[j].media != NULL; j++) {
                    if (g_strcmp0(disk->media_compatibility[i], media_info[j].media) == 0) {
                        media_label = media_info[j].label;
                        if (icon == NULL)
                            icon = media_info[j].icon;
                        break;
                    }
                }

                if (media_comp == NULL){
                    media_comp = g_strdup(media_label);
                }
                else{
                    media_comp = h_strdup_cprintf(", %s", media_comp, media_label);
                }
            }
        }
        if (icon == NULL && disk->ejectable && g_strcmp0(disk->connection_bus, "usb") == 0) {
            icon = "usbfldisk";
        }
        if (icon == NULL){
            icon = "hdd";
        }

        udisks2_storage_list = h_strdup_cprintf("$%s$%s=\n", udisks2_storage_list, devid, label);
        storage_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n", storage_icons, devid, label, icon);
        features = h_strdup_cprintf("%s", features, disk->removable ? _("Removable"): _("Fixed"));

        if (disk->ejectable) {
            features = h_strdup_cprintf(", %s", features, _("Ejectable"));
        }
        if (disk->smart_supported) {
            features = h_strdup_cprintf(", %s", features, _("Self-monitoring (S.M.A.R.T.)"));
        }
        if (disk->pm_supported) {
            features = h_strdup_cprintf(", %s", features, _("Power Management"));
        }
        if (disk->apm_supported) {
            features = h_strdup_cprintf(", %s", features, _("Advanced Power Management"));
        }
        if (disk->aam_supported) {
            features = h_strdup_cprintf(", %s", features, _("Automatic Acoustic Management"));
        }

        moreinfo = g_strdup_printf(_("[Drive Information]\n"
                                   "Model=%s\n"),
                                   label);

        moreinfo = h_strdup_cprintf(_("Vendor=%s\n"),
                                     moreinfo,
                                     idle_free(vendor_get_link(vendor_str)));

        size = size_human_readable((gfloat) disk->size);
        moreinfo = h_strdup_cprintf(_("Revision=%s\n"
                                    "Block Device=%s\n"
                                    "Serial=%s\n"
                                    "Size=%s\n"
                                    "Features=%s\n"),
                                    moreinfo,
                                    disk->revision,
                                    disk->block_dev,
                                    disk->serial,
                                    size,
                                    features);
        g_free(size);

        if (disk->rotation_rate > 0) {
            moreinfo = h_strdup_cprintf(_("Rotation Rate=%d RPM\n"), moreinfo, disk->rotation_rate);
        }
        if (media_comp || media_curr) {
            moreinfo = h_strdup_cprintf(_("Media=%s\n"
                                        "Media compatibility=%s\n"),
                                        moreinfo,
                                        media_curr ? media_curr : _("(None)"),
                                        media_comp ? media_comp : _("(Unknown)"));
        }
        if (disk->connection_bus && strlen(disk->connection_bus) > 0) {
            moreinfo = h_strdup_cprintf(_("Connection bus=%s\n"), moreinfo, disk->connection_bus);
        }
        if (disk->smart_enabled) {
            moreinfo = h_strdup_cprintf(_("[Self-monitoring (S.M.A.R.T.)]\n"
                                        "Status=%s\n"
                                        "Bad Sectors=%ld\n"
                                        "Power on time=%d days %d hours\n"
                                        "Temperature=%d°C\n"),
                                        moreinfo,
                                        disk->smart_failing ? _("Failing"): _("OK"),
                                        disk->smart_bad_sectors,
                                        disk->smart_poweron/(60*60*24), (disk->smart_poweron/60/60) % 24,
                                        disk->smart_temperature);

            if (disk->smart_attributes != NULL) {
                moreinfo = h_strdup_cprintf(_("[S.M.A.R.T. Attributes]\n"
                                            "Attribute=Normalized Value / Worst / Threshold\n"),
                                            moreinfo);

                attrib = disk->smart_attributes;

                while (attrib != NULL){
                    tmp = g_strdup("");
                    if (attrib->value != -1)
                        tmp = h_strdup_cprintf("%3d", tmp, attrib->value);
                    else
                        tmp = h_strdup_cprintf("???", tmp);

                    if (attrib->worst != -1)
                        tmp = h_strdup_cprintf(" / %3d", tmp, attrib->worst);
                    else
                        tmp = h_strdup_cprintf(" / ???", tmp);

                    if (attrib->threshold != -1)
                        tmp = h_strdup_cprintf(" / %3d", tmp, attrib->threshold);
                    else
                        tmp = h_strdup_cprintf(" / ???", tmp);


                    alabel = attrib->identifier;
                    for (i = 0; smart_attrib_info[i].identifier != NULL; i++) {
                        if (g_strcmp0(attrib->identifier, smart_attrib_info[i].identifier) == 0) {
                            alabel = smart_attrib_info[i].label;
                            break;
                        }
                    }

                    moreinfo = h_strdup_cprintf(_("(%d) %s=%s\n"),
                                            moreinfo,
                                            attrib->id, alabel, tmp);
                    g_free(tmp);
                    attrib = attrib->next;
                }
            }
        }
        if (disk->partition_table || disk->partitions) {
            moreinfo = h_strdup_cprintf(_("[Partition table]\n"
                                        "Type=%s\n"),
                                        moreinfo,
                                        disk->partition_table ? disk->partition_table : _("(Unknown)"));

            if (disk->partitions != NULL) {
                part = disk->partitions;
                while (part != NULL){

                    tmp = size_human_readable((gfloat) part->size);
                    if (part->label) {
                        tmp = h_strdup_cprintf(" - %s", tmp, part->label);
                    }
                    if (part->type && part->version) {
                        tmp = h_strdup_cprintf(" (%s %s)", tmp, part->type, part->version);
                    }
                    else if (part->type) {
                        tmp = h_strdup_cprintf(" (%s)", tmp, part->type);
                    }
                    moreinfo = h_strdup_cprintf(_("Partition %s=%s\n"),
                                                moreinfo,
                                                part->block, tmp);
                    g_free(tmp);
                    part = part->next;
                }
            }
        }

        moreinfo_add_with_prefix("DEV", devid, moreinfo);
        g_free(devid);
        g_free(features);
        g_free(label);
        g_free(media_comp);
        media_comp = NULL;

        features = NULL;
        moreinfo = NULL;
        devid = NULL;

        udiskd_free(disk);
    }
    g_slist_free(drives);

    if (n) {
        storage_list = h_strconcat(storage_list, udisks2_storage_list, NULL);
        g_free(udisks2_storage_list);
        return TRUE;
    }

    g_free(udisks2_storage_list);
    return FALSE;
}

/* SCSI support by Pascal F.Martin <pascalmartin@earthlink.net> */
void __scan_scsi_devices(void)
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

    scsi_storage_list = g_strdup(_("\n[SCSI Disks]\n"));

    int otype = 0;
    if (proc_scsi = fopen("/proc/scsi/scsi", "r")) {
        otype = 1;
    } else if (proc_scsi = popen("lsscsi -c", "r")) {
        otype = 2;
    }

    if (otype) {
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

                strhash = h_strdup_cprintf(_("Vendor=%s\n"),
                                           strhash,
                                           idle_free(vendor_get_link(model)));

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
        if (otype == 1)
            fclose(proc_scsi);
        else if (otype == 2)
            pclose(proc_scsi);
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

            strhash = h_strdup_cprintf(_("Vendor=%s\n"), strhash, idle_free(vendor_get_link(model)));

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
