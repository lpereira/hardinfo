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

static gchar *storage_icons = NULL;

static gboolean
remove_scsi_devices(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "SCSI");
}

/* SCSI support by Pascal F.Martin <pascalmartin@earthlink.net> */
void
__scan_scsi_devices(void)
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
    g_hash_table_foreach_remove(moreinfo, remove_scsi_devices, NULL);

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

                for (i = 0; type2icon[i].type != NULL; i++)
                    if (strstr(buf + 8, type2icon[i].type))
                        break;

                type = type2icon[i].label;
                icon = type2icon[i].icon;
	    }
	    
	    gchar *devid = g_strdup_printf("SCSI%d", n);
	    storage_list = h_strdup_cprintf("$%s$%s=\n", storage_list, devid, model);
	    storage_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n", storage_icons, devid, model, icon);
	    
	    gchar *strhash = g_strdup_printf("[Device Information]\n"
					     "Model=%s\n",model);
	    
	    const gchar *url = vendor_get_url(model);
	    if (url) {
	      strhash = h_strdup_cprintf("Vendor=%s (%s)\n",
                                         strhash,
                                         vendor_get_name(model),
                                         url);
	    } else {
	      strhash = h_strdup_cprintf("Vendor=%s\n",
                                         strhash,
                                         vendor_get_name(model));
	    }

            strhash = h_strdup_cprintf("Type=%s\n"
	    			       "Revision=%s\n"
				       "[SCSI Controller]\n"
                                       "Controller=scsi%d\n"
                                       "Channel=%d\n"
                                       "ID=%d\n" "LUN=%d\n",
                                       strhash,
                                       type,
                                       revision,
                                       scsi_controller,
                                       scsi_channel,
                                       scsi_id,
                                       scsi_lun);
	    g_hash_table_insert(moreinfo, devid, strhash);

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
    return g_str_has_prefix(key, "IDE");
}

void
__scan_ide_devices(void)
{
    FILE *proc_ide;
    gchar *device, iface, *model, *media, *pgeometry = NULL, *lgeometry =
	NULL;
    gint n = 0, i = 0, cache, nn = 0;
    gchar *capab = NULL, *speed = NULL, *driver = NULL;

    /* remove old devices from global device table */
    g_hash_table_foreach_remove(moreinfo, remove_ide_devices, NULL);

    storage_list = g_strconcat(storage_list, "\n[IDE Disks]\n", NULL);

    iface = 'a';
    for (i = 0; i <= 16; i++) {
	device = g_strdup_printf("/proc/ide/hd%c/model", iface);
	if (g_file_test(device, G_FILE_TEST_EXISTS)) {
	    gchar buf[128];

	    cache = 0;

	    proc_ide = fopen(device, "r");
	    fgets(buf, 128, proc_ide);
	    fclose(proc_ide);

	    buf[strlen(buf) - 1] = 0;

	    model = g_strdup(buf);

	    g_free(device);

	    device = g_strdup_printf("/proc/ide/hd%c/media", iface);
	    proc_ide = fopen(device, "r");
	    fgets(buf, 128, proc_ide);
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

  	            while (fgets(buf, 128, prcap) && g_timer_elapsed(timer, NULL) < 0.5) {
  	               if (g_str_has_prefix(buf, "  Does")) {
  	                   if (g_str_has_suffix(buf, "media\n") && !strstr(buf, "speed")) {
      	                       gchar *media_type = g_strstrip(strstr(buf, "Does "));
      	                       gchar **ttmp = g_strsplit(media_type, " ", 0);
  	                   
      	                       capab = h_strdup_cprintf("\nCan %s#%d=%s\n",
  	                                               capab,
  	                                               ttmp[1], ++nn, ttmp[2]);
  	                                           
                               g_strfreev(ttmp);
                           } else if (strstr(buf, "Buffer-Underrun-Free")) {
                               capab = h_strdup_cprintf("\nSupports BurnProof=%s\n",
                                                       capab,
                                                       strstr(buf, "Does not") ? "No" : "Yes");
                           } else if (strstr(buf, "multi-session")) {
                               capab = h_strdup_cprintf("\nCan read multi-session CDs=%s\n",
                                                       capab,
                                                       strstr(buf, "Does not") ? "No" : "Yes");
                           } else if (strstr(buf, "audio CDs")) {
                               capab = h_strdup_cprintf("\nCan play audio CDs=%s\n",
                                                       capab,
                                                       strstr(buf, "Does not") ? "No" : "Yes");
                           } else if (strstr(buf, "PREVENT/ALLOW")) {
                               capab = h_strdup_cprintf("\nCan lock media=%s\n",
                                                       capab,
                                                       strstr(buf, "Does not") ? "No" : "Yes");
                           }
  	               } else if ((strstr(buf, "read") || strstr(buf, "write")) && strstr(buf, "kB/s")) {
  	                   speed = g_strconcat(speed ? speed : "",
  	                                       strreplace(g_strstrip(buf), ":", '='),
  	                                       "\n", NULL);
  	               } else if (strstr(buf, "Device seems to be")) {
  	                   driver = g_strdup_printf("Driver=%s\n", strchr(buf, ':') + 1);
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

	    storage_list = h_strdup_cprintf("$%s$%s=\n", storage_list,
					 devid, model);
	    storage_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n", storage_icons, devid,
	                                  model, g_str_equal(media, "cdrom") ? \
	                                         "cdrom" : "hdd");
	    
	    gchar *strhash = g_strdup_printf("[Device Information]\n"
		                             "Model=%s\n",
					     model);
	    
	    const gchar *url = vendor_get_url(model);
	    
	    if (url) {
	      strhash = h_strdup_cprintf("Vendor=%s (%s)\n",
                                         strhash,
                                         vendor_get_name(model),
                                         url);
	    } else {
	      strhash = h_strdup_cprintf("Vendor=%s\n",
                                         strhash,
                                         vendor_get_name(model));
	    }
	    
            strhash = h_strdup_cprintf("Device Name=hd%c\n"
                                       "Media=%s\n"
                                       "Cache=%dkb\n",
                                       strhash,
                                       iface,
                                       media,
                                       cache);
            if (driver) {
                strhash = h_strdup_cprintf("%s\n", strhash, driver);
                
                g_free(driver);
                driver = NULL;
            }
            
	    if (pgeometry && lgeometry) {
		strhash = h_strdup_cprintf("[Geometry]\n"
					  "Physical=%s\n"
					  "Logical=%s\n",
					  strhash, pgeometry, lgeometry);

                g_free(pgeometry);
                pgeometry = NULL;
                g_free(lgeometry);
                lgeometry = NULL;
            }
            
            if (capab) {
                strhash = h_strdup_cprintf("[Capabilities]\n%s", strhash, capab);
                
                g_free(capab);
                capab = NULL;
            }
            
            if (speed) {
                strhash = h_strdup_cprintf("[Speeds]\n%s", strhash, speed);
                
                g_free(speed);
                speed = NULL;
            }
            
	    g_hash_table_insert(moreinfo, devid, strhash);

	    g_free(model);
	    model = g_strdup("");
	} else
	    g_free(device);

	iface++;
    }
}
