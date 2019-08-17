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

gboolean no_monitors = FALSE;

gchar *edid_ids_file = NULL;

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

typedef struct {
    gchar *drm_connection;
    uint8_t *edid_bin;
    gsize edid_len;
    struct edid id;
    gchar *_vstr; /* use monitor_vendor_str() */
} monitor;
#define monitor_new() g_new0(monitor, 1)

monitor *monitor_new_from_sysfs(const gchar *sysfs_edid_file) {
    monitor *m = monitor_new();
    g_file_get_contents(sysfs_edid_file, (gchar**)&(m->edid_bin), &m->edid_len, NULL);
    if (m->edid_len) {
        edid_fill(&m->id, (void*)(m->edid_bin), m->edid_len);
    }

    gchar *pd = g_path_get_dirname(sysfs_edid_file);
    m->drm_connection = g_path_get_basename(pd);
    g_free(pd);

    return m;
}

void monitor_free(monitor *m) {
    if (m) {
        g_free(m->_vstr);
        g_free(m->drm_connection);
        g_free(m->edid_bin);
        g_free(m);
    }
}

const gchar *monitor_vendor_str(monitor *m) {
    if (m->_vstr)
        return m->_vstr;

    ids_query_result result = {};

    if (!edid_ids_file)
        find_edid_ids_file();

    scan_ids_file(edid_ids_file, m->id.ven, &result, -1);
    if (result.results[0]) {
        m->_vstr = g_strdup(result.results[0]);
        return m->_vstr;
    }
    return g_strdup(m->id.ven);
}

gchar *monitor_name(monitor *m, gboolean include_vendor) {
    if (!m) return NULL;
    gchar *desc = NULL;

    if (include_vendor) {
        if (*m->id.ven)
            desc = appfsp(desc, "%s", vendor_get_shortest_name(monitor_vendor_str(m)));
        else
            desc = appfsp(desc, "%s", "Unknown");
    }

    if (m->id.diag_in) {
        gchar *din = util_strchomp_float(g_strdup_printf("%0.1f", m->id.diag_in));
        desc = appfsp(desc, "%s\"", din);
        g_free(din);
    }

    if (m->id.name)
        desc = appfsp(desc, "%s", m->id.name);
    else
        desc = appfsp(desc, "%s %s", m->id.a_or_d ? "Digital" : "Analog", "Display");

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
    if (m->edid_len) {
        const gchar *vstr = monitor_vendor_str(m);

        gchar *dom = NULL;
        if (m->id.week && m->id.year)
            dom = g_strdup_printf(_("Week %d of %d"), m->id.week, m->id.year);
        else if (m->id.year)
            dom = g_strdup_printf("%d", m->id.year);

        gchar *bpcc = NULL;
        if (m->id.bpc)
            bpcc = g_strdup_printf("%d", m->id.bpc);

        gchar *scr_size = NULL;
        if (m->id.horiz_cm && m->id.vert_cm)
            scr_size = g_strdup_printf("%d cm × %d cm", m->id.horiz_cm, m->id.vert_cm);

        int aok = m->id.checksum_ok;
        if (m->id.ext_blocks_fail) aok = 0;
        gchar *csum = aok ? _("Ok") : _("Fail");

        gchar *ret = g_strdup_printf("[%s]\n"
            "%s=%d.%d\n" /* version */
            "%s=%d %s\n" /* size */
            "%s=%d\n" /* ext block */
            "%s=%s %s\n" /* checksum */
            "[%s]\n"
            "%s=%s\n" /* vendor */
            "%s=%s\n" /* name */
            "%s=%s\n" /* dom */
            "%s=%s\n" /* size */
            "%s=%s\n" /* sig type */
            "%s=%s\n" /* bpcc */
            ,
            _("EDID Meta"),
            _("Version"), (int)m->id.ver_major, (int)m->id.ver_minor,
            _("Data Size"), m->id.size, _("bytes"),
            _("Extension Blocks"), m->id.ext_blocks,
            _("Checksum"), csum, aok ? "" : problem_marker(),
            _("EDID Device"),
            _("Vendor"), vstr,
            _("Name"), m->id.name,
            _("Manufacture Date"), UNKIFNULL2(dom),
            _("Screen Size"), UNKIFNULL2(scr_size),
            _("Signal Type"), m->id.a_or_d ? _("Digital") : _("Analog"),
            _("Bits per Color Channel"), UNKIFNULL2(bpcc)
            );
        g_free(scr_size);
        g_free(bpcc);
        g_free(dom);
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
    //gchar **edid_files = get_output_lines("find /home/pburt/github/verbose-spork/junk/testing/.testing/edid/ -name edid.*");
    int i, found = 0;
    for(i = 0; edid_files[i]; i++) {
        monitor *m = monitor_new_from_sysfs(edid_files[i]);
        if (m) {
            found++;
            if (m->id.checksum_ok && m->drm_connection) {
                gchar *tag = g_strdup_printf("%d-%s", found, m->drm_connection);
                tag_make_safe_inplace(tag);
                gchar *desc = monitor_name(m, TRUE);
                gchar *edid_section = make_edid_section(m);
                gchar *details = g_strdup_printf("[%s]\n"
                                "%s=%s\n"
                                "%s\n",
                                _("Connection"),
                                _("DRM"), m->drm_connection,
                                edid_section
                                );
                moreinfo_add_with_prefix(tag_prefix, tag, details); /* moreinfo now owns *details */
                ret = h_strdup_cprintf("$!%s$%s=%s|%s\n",
                        ret,
                        tag, m->drm_connection, desc
                        );
                icons = h_strdup_cprintf("Icon$%s$=%s\n", icons, tag, monitor_icon);
                g_free(desc);
                g_free(edid_section);
            }
            monitor_free(m);
        }
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
