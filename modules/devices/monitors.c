/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2019 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    Copyright (C) 2019 Burt P. <pburt0@gmail.com>
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

#include "devices.h"
#include "util_sysobj.h"
#include "util_edid.h"
#include "util_ids.h"

static const char monitor_icon[] = "monitor.png";

#define UNKIFNULL2(f) ((f) ? f : _("(Unknown)"))
#define UNKIFEMPTY2(f) ((*f) ? f : _("(Unknown)"))
#define UNSPECIFNULL2(f) ((f) ? f : _("(Unspecified)"))

gboolean no_monitors = FALSE;

gchar *edid_ids_file = NULL;
gchar *ieee_oui_ids_file = NULL;

void find_edid_ids_file() {
    if (edid_ids_file) return;
    char *file_search_order[] = {
        g_build_filename(g_get_user_config_dir(), "hardinfo", "edid.ids", NULL),
        g_build_filename(params.path_data, "edid.ids", NULL),
        NULL
    };
    int n;
    for(n = 0; file_search_order[n]; n++) {
        if (!edid_ids_file && !access(file_search_order[n], R_OK))
            edid_ids_file = file_search_order[n];
        else
            g_free(file_search_order[n]);
    }
    auto_free(edid_ids_file);
}

void find_ieee_oui_ids_file() {
    if (ieee_oui_ids_file) return;
    char *file_search_order[] = {
        g_build_filename(g_get_user_config_dir(), "hardinfo", "ieee_oui.ids", NULL),
        g_build_filename(params.path_data, "ieee_oui.ids", NULL),
        NULL
    };
    int n;
    for(n = 0; file_search_order[n]; n++) {
        if (!ieee_oui_ids_file && !access(file_search_order[n], R_OK))
            ieee_oui_ids_file = file_search_order[n];
        else
            g_free(file_search_order[n]);
    }
    auto_free(ieee_oui_ids_file);
}

typedef struct {
    gchar *drm_path;
    gchar *drm_connection;
    gchar *drm_status;
    gchar *drm_enabled;
    edid *e;
    gchar *_vstr; /* use monitor_vendor_str() */
} monitor;
#define monitor_new() g_new0(monitor, 1)

monitor *monitor_new_from_sysfs(const gchar *sysfs_edid_file) {
    gchar *edid_bin = NULL;
    gsize edid_len = 0;
    if (!sysfs_edid_file || !*sysfs_edid_file) return NULL;
    monitor *m = monitor_new();
    m->drm_path = g_path_get_dirname(sysfs_edid_file);
    m->drm_connection = g_path_get_basename(m->drm_path);
    gchar *drm_enabled_file = g_strdup_printf("%s/%s", m->drm_path, "enabled");
    gchar *drm_status_file = g_strdup_printf("%s/%s", m->drm_path, "status");
    g_file_get_contents(drm_enabled_file, &m->drm_enabled, NULL, NULL);
    if (m->drm_enabled) g_strstrip(m->drm_enabled);
    g_file_get_contents(drm_status_file, &m->drm_status, NULL, NULL);
    if (m->drm_status) g_strstrip(m->drm_status);
    g_file_get_contents(sysfs_edid_file, &edid_bin, &edid_len, NULL);
    if (edid_len)
        m->e = edid_new(edid_bin, edid_len);
    g_free(drm_enabled_file);
    g_free(drm_status_file);
    return m;
}

void monitor_free(monitor *m) {
    if (m) {
        g_free(m->_vstr);
        g_free(m->drm_connection);
        edid_free(m->e);
        g_free(m);
    }
}

gchar *monitor_vendor_str(monitor *m, gboolean include_value, gboolean link_name) {
    if (!m || !m->e) return NULL;
    edid_ven ven = m->e->ven;
    gchar v[20] = "", t[4] = "";
    ids_query_result result = {};

    if (ven.type == VEN_TYPE_PNP) {
        strcpy(v, ven.pnp);
        strcpy(t, "PNP");
    } else if (ven.type == VEN_TYPE_OUI) {
        sprintf(v, "%02x%02x%02x",
            /* file lists as big endian */
            (ven.oui >> 16) & 0xff,
            (ven.oui >> 8) & 0xff,
             ven.oui & 0xff );
        strcpy(t, "OUI");
    }

    if (!m->_vstr) {
        if (ven.type == VEN_TYPE_PNP) {
            if (!edid_ids_file)
                find_edid_ids_file();
            scan_ids_file(edid_ids_file, v, &result, -1);
            if (result.results[0])
                m->_vstr = g_strdup(result.results[0]);
        } else if (ven.type == VEN_TYPE_OUI) {
            if (!ieee_oui_ids_file)
                find_ieee_oui_ids_file();
            scan_ids_file(ieee_oui_ids_file, v, &result, -1);
            if (result.results[0])
                m->_vstr = g_strdup(result.results[0]);
        }
    }

    gchar *ret = NULL;
    if (include_value)
        ret = g_strdup_printf("[%s:%s]", t, v);
    if (m->_vstr) {
        if (link_name) {
            gchar *lv = vendor_get_link(m->_vstr);
            ret = appfsp(ret, "%s", lv);
            g_free(lv);
        } else
            ret = appfsp(ret, "%s", m->_vstr);
    } else if (!include_value && ven.type == VEN_TYPE_PNP) {
        ret = appfsp(ret, "%s", ven.pnp);
    } else
        ret = appfsp(ret, "%s", _("(Unknown)"));
    return ret;
}

gchar *monitor_name(monitor *m, gboolean include_vendor) {
    if (!m) return NULL;
    gchar *desc = NULL;
    edid *e = m->e;
    if (!e)
        return g_strdup(_("(Unknown)"));

    if (include_vendor) {
        if (e->ven.type != VEN_TYPE_INVALID) {
            gchar *vstr = monitor_vendor_str(m, FALSE, FALSE);
            desc = appfsp(desc, "%s", vendor_get_shortest_name(vstr));
            g_free(vstr);
        } else
            desc = appfsp(desc, "%s", "Unknown");
    }

    if (e->img_max.diag_in)
        desc = appfsp(desc, "%s", e->img_max.class_inch);

    if (e->name)
        desc = appfsp(desc, "%s", e->name);
    else
        desc = appfsp(desc, "%s %s", e->a_or_d ? "Digital" : "Analog", "Display");

    return desc;
}

gchar **get_output_lines(const char *cmd_line) {
    gboolean spawned;
    gchar *out, *err;
    gchar **ret = NULL;

    spawned = g_spawn_command_line_sync(cmd_line,
            &out, &err, NULL, NULL);
    if (spawned) {
        ret = g_strsplit(out, "\n", -1);
        g_free(out);
        g_free(err);
    }
    return ret;
}

static gchar *tag_make_safe_inplace(gchar *tag) {
    if (!tag)
        return tag;
    if (!g_utf8_validate(tag, -1, NULL))
        return tag; //TODO: reconsider
    gchar *p = tag, *pd = tag;
    while(*p) {
        gchar *np = g_utf8_next_char(p);
        gunichar c = g_utf8_get_char_validated(p, -1);
        int l = g_unichar_to_utf8(c, NULL);
        if (l == 1 && g_unichar_isalnum(c)) {
            g_unichar_to_utf8(c, pd);
        } else {
            *pd = '_';
        }
        p = np;
        pd++;
    }
    return tag;
}

static gchar *make_edid_section(monitor *m) {
    int i;
    edid *e = m->e;
    if (e->len) {
        gchar *vstr = monitor_vendor_str(m, TRUE, TRUE);

        gchar *dom = NULL;
        if (!e->dom.is_model_year && e->dom.week && e->dom.year)
            dom = g_strdup_printf(_("Week %d of %d"), e->dom.week, e->dom.year);
        else if (e->dom.year)
            dom = g_strdup_printf("%d", e->dom.year);

        gchar *bpcc = NULL;
        if (e->bpc)
            bpcc = g_strdup_printf("%d", e->bpc);

        int aok = e->checksum_ok;
        if (e->ext_blocks_fail) aok = 0;
        gchar *csum = aok ? _("Ok") : _("Fail");

        gchar *iface;
        if (e->interface && e->di.exists) {
            gchar *tmp = g_strdup_printf("[%x] %s\n[DI-EXT:%x] %s",
                e->interface, _(edid_interface(e->interface)),
                e->di.interface, _(edid_di_interface(e->di.interface)) );
            iface = gg_key_file_parse_string_as_value(tmp, '|');
            g_free(tmp);
        } else if (e->di.exists) {
            iface = g_strdup_printf("[DI-EXT:%x] %s",
                e->di.interface, _(edid_di_interface(e->di.interface)) );
        } else {
            iface = g_strdup_printf("[%x] %s",
                e->interface,
                e->interface ? _(edid_interface(e->interface)) : _("(Unspecified)") );
        }

        gchar *d_list, *ext_list, *dtd_list, *cea_list,
            *etb_list, *std_list, *svd_list, *sad_list,
            *didt_list, *did_string_list;

        etb_list = NULL;
        for(i = 0; i < e->etb_count; i++) {
            char *desc = edid_output_describe(&e->etbs[i]);
            etb_list = appfnl(etb_list, "etb%d=%s", i, desc);
            g_free(desc);
        }
        if (!etb_list) etb_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        std_list = NULL;
        for(i = 0; i < e->std_count; i++) {
            char *desc = edid_output_describe(&e->stds[i].out);
            std_list = appfnl(std_list, "std%d=%s", i, desc);
            g_free(desc);
        }
        if (!std_list) std_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        d_list = NULL;
        for(i = 0; i < 4; i++) {
            char *desc = edid_base_descriptor_describe(&e->d[i]);
            d_list = appfnl(d_list, "descriptor%d=%s", i, desc);
            g_free(desc);
        }
        if (!d_list) d_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        ext_list = NULL;
        for(i = 0; i < e->ext_blocks; i++) {
            int type = e->u8[(i+1)*128];
            int version = e->u8[(i+1)*128 + 1];
            ext_list = appfnl(ext_list, "ext%d = ([%02x:v%02x] %s) %s", i,
                type, version, _(edid_ext_block_type(type)),
                e->ext_ok[i] ? "ok" : "fail"
            );
        }
        if (!ext_list) ext_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        dtd_list = NULL;
        for(i = 0; i < e->dtd_count; i++) {
            char *desc = edid_dtd_describe(&e->dtds[i], 0);
            dtd_list = appfnl(dtd_list, "dtd%d = %s", i, desc);
            free(desc);
        }
        if (!dtd_list) dtd_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        cea_list = NULL;
        for(i = 0; i < e->cea_block_count; i++) {
            char *desc = edid_cea_block_describe(&e->cea_blocks[i]);
            cea_list = appfnl(cea_list, "cea_block%d = %s", i, desc);
        }
        if (!cea_list) cea_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        svd_list = NULL;
        for(i = 0; i < e->svd_count; i++) {
            char *desc = edid_output_describe(&e->svds[i].out);
            svd_list = appfnl(svd_list, "svd%d=%s", i, desc);
            g_free(desc);
        }
        if (!svd_list) svd_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        sad_list = NULL;
        for(i = 0; i < e->sad_count; i++) {
            char *desc = edid_cea_audio_describe(&e->sads[i]);
            sad_list = appfnl(sad_list, "sad%d=%s", i, desc);
            g_free(desc);
        }
        if (!sad_list) sad_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        didt_list = NULL;
        for(i = 0; i < e->didt_count; i++) {
            char *desc = edid_output_describe(&e->didts[i]);
            didt_list = appfnl(didt_list, "didt%d=%s", i, desc);
            g_free(desc);
        }
        if (!didt_list) didt_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        did_string_list = NULL;
        for(i = 0; i < e->did_string_count; i++) {
            did_string_list = appfnl(did_string_list, "did_string%d=%s", i, e->did_strings[i].str);
        }
        if (!did_string_list) did_string_list = g_strdup_printf("%s=\n", _("(Empty List)"));

        gchar *speakers = NULL;
        if (e->speaker_alloc_bits) {
            gchar *spk_tmp = edid_cea_speaker_allocation_describe(e->speaker_alloc_bits, 0);
            speakers = gg_key_file_parse_string_as_value(spk_tmp, '|');
            g_free(spk_tmp);
        } else
            speakers = g_strdup(_("(Unspecified)"));

        gchar *hex = edid_dump_hex(e, 0, 1);
        gchar *hex_esc = gg_key_file_parse_string_as_value(hex, '|');
        g_free(hex);
        if (params.markup_ok)
            hex = g_strdup_printf("<tt>%s</tt>", hex_esc);
        else
            hex = g_strdup(hex_esc);
        g_free(hex_esc);

        gchar *ret = g_strdup_printf(
            /* extending "Connection" section */
            "%s=%s\n" /* sig type */
            "%s=%s\n" /* interface */
            "%s=%s\n" /* bpcc */
            "%s=%s\n" /* speakers */
            "[%s]\n"
            "%s=%s\n" /* base out */
            "%s=%s\n" /* ext out */
            "[%s]\n"
            "%s=%s\n" /* vendor */
            "%s=%s\n" /* name */
            "%s=[%04x-%08x] %u-%u\n" /* model, n_serial */
            "%s=%s\n" /* serial */
            "%s=%s\n" /* dom */
            "[%s]\n"
            "%s=%d %s\n" /* size */
            "%s=%d.%d\n" /* version */
            "%s=%d\n" /* ext block */
            "%s=%s\n" /* ext to */
            "%s=%s %s\n" /* checksum */
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s\n"
            "[%s]\n%s=%s\n"
            ,
            _("Signal Type"), e->a_or_d ? _("Digital") : _("Analog"),
            _("Interface"), iface,
            _("Bits per Color Channel"), UNSPECIFNULL2(bpcc),
            _("Speaker Allocation"), speakers,
            _("Output (Max)"),
            edid_output_src(e->img.src), edid_output_describe(&e->img),
            edid_output_src(e->img_max.src), edid_output_describe(&e->img_max),
            _("EDID Device"),
            _("Vendor"), vstr,
            _("Name"), e->name,
            _("Model"), e->product, e->n_serial, e->product, e->n_serial,
            _("Serial"), UNKIFNULL2(e->serial),
            _("Manufacture Date"), UNKIFNULL2(dom),
            _("EDID Meta"),
            _("Data Size"), e->len, _("bytes"),
            _("Version"), (int)e->ver_major, (int)e->ver_minor,
            _("Extension Blocks"), e->ext_blocks,
            _("Extended to"), e->std ? _(edid_standard(e->std)) : _("(None)"),
            _("Checksum"), csum, aok ? "" : problem_marker(),
            _("EDID Descriptors"), d_list,
            _("Detailed Timing Descriptors (DTD)"), dtd_list,
            _("Established Timings Bitmap (ETB)"), etb_list,
            _("Standard Timings (STD)"), std_list,
            _("E-EDID Extension Blocks"), ext_list,
            _("EIA/CEA-861 Data Blocks"), cea_list,
            _("EIA/CEA-861 Short Audio Descriptors"), sad_list,
            _("EIA/CEA-861 Short Video Descriptors"), svd_list,
            _("DisplayID Timings"), didt_list,
            _("DisplayID Strings"), did_string_list,
            _("Hex Dump"), _("Data"), hex
            );
        g_free(bpcc);
        g_free(dom);

        g_free(d_list);
        g_free(ext_list);
        g_free(etb_list);
        g_free(std_list);
        g_free(dtd_list);
        g_free(cea_list);
        g_free(sad_list);
        g_free(svd_list);
        g_free(didt_list);
        g_free(did_string_list);
        g_free(iface);
        g_free(vstr);
        g_free(hex);
        //printf("ret: %s\n", ret);
        return ret;
    } else
        return g_strdup("");
}

gchar *monitors_get_info() {
    gchar *icons = g_strdup("");
    gchar *ret = g_strdup_printf("[%s]\n", _("Monitors"));
    gchar tag_prefix[] = "DEV";

    gchar **edid_files = get_output_lines("find /sys/devices -name edid");
    //gchar **edid_files = get_output_lines("find /home/pburt/github/verbose-spork/junk/testing/.testing/edid2/ -name edid.*");
    int i, found = 0;
    for(i = 0; edid_files[i]; i++) {
        monitor *m = monitor_new_from_sysfs(edid_files[i]);
        //if (m && m->e->std < STD_DISPLAYID) continue;
        //if (m && !m->e->interface) continue;
        //if (m && m->e->interface != 1) continue;
        if (m && !SEQ(m->drm_status, "disconnected")) {
            gchar *tag = g_strdup_printf("%d-%s", found, m->drm_connection);
            tag_make_safe_inplace(tag);
            gchar *desc = monitor_name(m, TRUE);
            gchar *edid_section = NULL;
            edid *e = m->e;
            if (e && e->checksum_ok)
                 edid_section = make_edid_section(m);

            gchar *details = g_strdup_printf("[%s]\n"
                                "%s=%s\n"
                                "%s=%s %s\n"
                                "%s\n",
                                _("Connection"),
                                _("DRM"), m->drm_connection,
                                _("Status"), m->drm_status, m->drm_enabled,
                                edid_section ? edid_section : ""
                                );
            moreinfo_add_with_prefix(tag_prefix, tag, details); /* moreinfo now owns *details */
            ret = h_strdup_cprintf("$!%s$%s=%s|%s\n",
                                    ret,
                                    tag, m->drm_connection, desc
                                    );
            icons = h_strdup_cprintf("Icon$%s$=%s\n", icons, tag, monitor_icon);
            g_free(desc);
            g_free(edid_section);
            found++;
        }
        monitor_free(m);
    }

    no_monitors = FALSE;
    if(!found) {
        no_monitors = TRUE;
        g_free(ret);
        ret = g_strdup_printf("[%s]\n%s=%s\n" "[$ShellParam$]\nViewType=0\n",
                _("Monitors"), _("Result"), _("(Empty)") );
    } else {
        ret = h_strdup_cprintf(
            "[$ShellParam$]\nViewType=1\n"
            "ColumnTitle$TextValue=%s\n" /* DRM connection */
            "ColumnTitle$Value=%s\n"     /* Name */
            "ShowColumnHeaders=true\n"
            "%s",
            ret,
            _("Connection"),
            _("Name"),
            icons
            );
    }

    return ret;
}

gboolean monitors_hinote(const char **msg) {
    PARAM_NOT_UNUSED(msg);
    return FALSE;
}
