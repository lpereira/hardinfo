#include "udisks2_util.h"
#include "storage_util.h"
#include "util_ids.h"
#include "hardinfo.h"

gchar *sdcard_ids_file = NULL;
gchar *oui_ids_file = NULL;

// moved from udisks2_util.h
void find_sdcard_ids_file() {
    if (sdcard_ids_file) return;
    char *file_search_order[] = {
        g_build_filename(g_get_user_config_dir(), "hardinfo", "sdcard.ids", NULL),
        g_build_filename(params.path_data, "sdcard.ids", NULL),
        NULL
    };
    int n;
    for(n = 0; file_search_order[n]; n++) {
        if (!sdcard_ids_file && !access(file_search_order[n], R_OK))
            sdcard_ids_file = file_search_order[n];
        else
            g_free(file_search_order[n]);
    }
}

void find_oui_ids_file() {
    if (oui_ids_file) return;
    char *file_search_order[] = {
        g_build_filename(g_get_user_config_dir(), "hardinfo", "ieee_oui.ids", NULL),
        g_build_filename(params.path_data, "ieee_oui.ids", NULL),
        NULL
    };
    int n;
    for(n = 0; file_search_order[n]; n++) {
        if (!oui_ids_file && !access(file_search_order[n], R_OK))
            oui_ids_file = file_search_order[n];
        else
            g_free(file_search_order[n]);
    }
}

gchar* get_oui_from_wwid(gchar* wwid){
    gchar* ret = NULL;

    if (g_str_has_prefix(wwid, "nna.")) {
        if (strlen(wwid)*4 < 48)
            return NULL;

        switch (wwid[4]){
            case '1':
            case '2':
                ret = g_strndup(wwid + 8, 6);
                break;
            case '5':
            case '6':
                ret = g_strndup(wwid + 5, 6);
                break;
        }
    }
    else if(g_str_has_prefix(wwid, "eui.")) {
        if (strlen(wwid)*4 < 48)
            return NULL;
        ret = g_strndup(wwid+4, 6);
    }

    return ret;
}

gchar* get_oui_company(gchar* oui){
    ids_query_result result = {};

    if (!oui_ids_file)
        find_oui_ids_file();

    scan_ids_file(oui_ids_file, oui, &result, -1);
    if (result.results[0])
        return g_strdup(result.results[0]);

    return NULL;
}

// moved from udisks2_util.h
void check_sdcard_vendor(u2driveext *e) {
    if (!e || !e->d) return;
    if (!e->d->media) return;
    if (! (g_str_has_prefix(e->d->media, "flash_sd")
            || g_str_has_prefix(e->d->media, "flash_mmc") )) return;
    if (e->d->vendor && e->d->vendor[0]) return;
    if (!e->d->block_dev) return;

    if (!sdcard_ids_file)
        find_sdcard_ids_file();

    gchar *qpath = NULL;
    ids_query_result result = {};

    gchar *oemid_path = g_strdup_printf("/sys/block/%s/device/oemid", e->d->block_dev);
    gchar *manfid_path = g_strdup_printf("/sys/block/%s/device/manfid", e->d->block_dev);
    gchar *oemid = NULL, *manfid = NULL;
    g_file_get_contents(oemid_path, &oemid, NULL, NULL);
    g_file_get_contents(manfid_path, &manfid, NULL, NULL);

    unsigned int id = oemid?strtol(oemid, NULL, 16):0;
    char c2 = id & 0xff, c1 = (id >> 8) & 0xff;

    qpath = g_strdup_printf("OEMID %02x%02x", (unsigned int)c1, (unsigned int)c2);
    scan_ids_file(sdcard_ids_file, qpath, &result, -1);
    g_free(oemid);
    if (result.results[0])
        oemid = g_strdup(result.results[0]);
    else
        oemid = g_strdup_printf("OEM %02x%02x \"%c%c\"",
            (unsigned int)c1, (unsigned int)c2,
            isprint(c1) ? c1 : '.', isprint(c2) ? c2 : '.');
    g_free(qpath);

    id = manfid?strtol(manfid, NULL, 16):0;
    qpath = g_strdup_printf("MANFID %06x", id);
    scan_ids_file(sdcard_ids_file, qpath, &result, -1);
    g_free(manfid);
    if (result.results[0])
        manfid = g_strdup(result.results[0]);
    else
        manfid = g_strdup_printf("MANF %06x", id);
    g_free(qpath);

    vendor_list vl = NULL;
    const Vendor *v = NULL;
    v = vendor_match(oemid, NULL);
    if (v) vl = vendor_list_append(vl, v);
    v = vendor_match(manfid, NULL);
    if (v) vl = vendor_list_append(vl, v);
    vl = vendor_list_remove_duplicates_deep(vl);
    e->vendors = vendor_list_concat(e->vendors, vl);

    g_free(e->d->vendor);
    if (g_strcmp0(oemid, manfid) == 0)
        e->d->vendor = g_strdup(oemid);
    else
        e->d->vendor = g_strdup_printf("%s / %s", oemid, manfid);

    g_free(oemid);
    g_free(manfid);
    g_free(oemid_path);
    g_free(manfid_path);

    if (e->d->revision && e->d->revision[0]) return;

    /* bonus: revision */
    gchar *fwrev_path = g_strdup_printf("/sys/block/%s/device/fwrev", e->d->block_dev);
    gchar *hwrev_path = g_strdup_printf("/sys/block/%s/device/hwrev", e->d->block_dev);
    gchar *fwrev = NULL, *hwrev = NULL;
    g_file_get_contents(fwrev_path, &fwrev, NULL, NULL);
    g_file_get_contents(hwrev_path, &hwrev, NULL, NULL);

    unsigned int fw = fwrev ? strtol(fwrev, NULL, 16) : 0;
    unsigned int hw = hwrev ? strtol(hwrev, NULL, 16) : 0;
    g_free(e->d->revision);
    e->d->revision = g_strdup_printf("%02x.%02x", hw, fw);

    g_free(fwrev);
    g_free(hwrev);
    g_free(fwrev_path);
    g_free(hwrev_path);

}

void set_nvme_controller_info(u2driveext *e){
    gchar *path = g_strdup_printf("/sys/block/%s/device/device", e->d->block_dev);
    gchar *systarget = g_file_read_link(path, NULL);
    gchar *target = systarget ? g_filename_to_utf8(systarget, -1, NULL, NULL, NULL) : NULL;
    gchar *pci_addy = target ? g_path_get_basename(target) : NULL;
    e->nvme_controller = pci_addy ? pci_get_device_str(pci_addy) : NULL;
    g_free(path);
    g_free(systarget);
    g_free(target);
    g_free(pci_addy);
    if (e->nvme_controller) {
        e->vendors = vendor_list_append(e->vendors,
            vendor_match(e->nvme_controller->vendor_id_str, NULL));
        e->vendors = vendor_list_append(e->vendors,
            vendor_match(e->nvme_controller->sub_vendor_id_str, NULL));
    }
}

GSList* get_udisks2_drives_ext(void){
    GSList *node, *list;
    u2driveext* extdrive;

    list = get_udisks2_all_drives_info();

    for (node = list; node != NULL; node = node->next) {
        extdrive = u2drive_ext((udiskd *)node->data);
        node->data = extdrive;

        if (!extdrive->d->vendor || !extdrive->d->vendor[0]) {
            // sometimes vendors adds their name to the model field
            const Vendor *v = vendor_match(extdrive->d->model, NULL);
            if (v)
                extdrive->d->vendor = g_strdup(v->name);
        }

        check_sdcard_vendor(extdrive);

        extdrive->vendors = vendor_list_append(extdrive->vendors, vendor_match(extdrive->d->vendor, NULL));

        // get OUI from WWID
        if (extdrive->d->wwid) {
            extdrive->wwid_oui.oui = get_oui_from_wwid(extdrive->d->wwid);
            if (extdrive->wwid_oui.oui) {
                extdrive->wwid_oui.vendor = get_oui_company(extdrive->wwid_oui.oui);
            }
            if (extdrive->wwid_oui.vendor){
                extdrive->vendors = vendor_list_append(extdrive->vendors, vendor_match(extdrive->wwid_oui.vendor, NULL));
            }
        }

        // NVMe PCI device
        if (strstr(extdrive->d->block_dev, "nvme")) {
            set_nvme_controller_info(extdrive);
        }

        extdrive->vendors = gg_slist_remove_null(extdrive->vendors);
        extdrive->vendors = vendor_list_remove_duplicates_deep(extdrive->vendors);

    }
    return list;
}


u2driveext* u2drive_ext(udiskd * udisks_drive_data) {
    u2driveext* data = g_new0(u2driveext, 1);
    data->d = udisks_drive_data;
    return data;
}

void u2driveext_free(u2driveext *u) {
    if (u) {
        udiskd_free(u->d);
        g_free(u->wwid_oui.oui);
        g_free(u->wwid_oui.vendor);
        pcid_free(u->nvme_controller);
        g_free(u);
    }
}

void storage_shutdown(){
    g_free(sdcard_ids_file);
    g_free(oui_ids_file);
}
