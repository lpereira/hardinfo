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

#include "computer.h"
#include "cpu_util.h" /* for STRIFNULL() */
#include "hardinfo.h"

#define GET_STR(field_name, ptr)                                        \
    if (!ptr && strstr(tmp[0], field_name)) {                           \
        ptr = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1])); \
        g_strfreev(tmp);                                                \
        continue;                                                       \
    }

GHashTable *_module_hash_table = NULL;

void scan_modules_do(void) {
    FILE *lsmod;
    gchar buffer[1024];
    gchar *lsmod_path;

    if (!_module_hash_table) { _module_hash_table = g_hash_table_new(g_str_hash, g_str_equal); }

    g_free(module_list);

    module_list = NULL;
    moreinfo_del_with_prefix("COMP:MOD");

    lsmod_path = find_program("lsmod");
    if (!lsmod_path) return;
    lsmod = popen(lsmod_path, "r");
    if (!lsmod) {
        g_free(lsmod_path);
        return;
    }

    (void)fgets(buffer, 1024, lsmod); /* Discards the first line */

    while (fgets(buffer, 1024, lsmod)) {
        gchar *buf, *strmodule, *hashkey;
        gchar *author = NULL, *description = NULL, *license = NULL, *deps = NULL, *vermagic = NULL,
              *filename = NULL, *srcversion = NULL, *version = NULL, *retpoline = NULL,
              *intree = NULL, modname[64];
        FILE *modi;
        glong memory;

        shell_status_pulse();

        buf = buffer;

        sscanf(buf, "%s %ld", modname, &memory);

        hashkey = g_strdup_printf("MOD%s", modname);
        buf = g_strdup_printf("/sbin/modinfo %s 2>/dev/null", modname);

        modi = popen(buf, "r");
        while (fgets(buffer, 1024, modi)) {
            gchar **tmp = g_strsplit(buffer, ":", 2);

            GET_STR("author", author);
            GET_STR("description", description);
            GET_STR("license", license);
            GET_STR("depends", deps);
            GET_STR("vermagic", vermagic);
            GET_STR("filename", filename);
            GET_STR("srcversion", srcversion); /* so "version" doesn't catch */
            GET_STR("version", version);
            GET_STR("retpoline", retpoline);
            GET_STR("intree", intree);

            g_strfreev(tmp);
        }
        pclose(modi);
        g_free(buf);

        /* old modutils includes quotes in some strings; strip them */
        /*remove_quotes(modname);
           remove_quotes(description);
           remove_quotes(vermagic);
           remove_quotes(author);
           remove_quotes(license); */

        /* old modutils displays <none> when there's no value for a
           given field; this is not desirable in the module name
           display, so change it to an empty string */
        if (description && g_str_equal(description, "&lt;none&gt;")) {
            g_free(description);
            description = g_strdup("");

            g_hash_table_insert(_module_hash_table, g_strdup(modname),
                                g_strdup_printf("Kernel module (%s)", modname));
        } else {
            g_hash_table_insert(_module_hash_table, g_strdup(modname), g_strdup(description));
        }

        /* append this module to the list of modules */
        module_list = h_strdup_cprintf("$%s$%s=%s\n", module_list, hashkey, modname,
                                       description ? description : "");

        STRIFNULL(filename, _("(Not available)"));
        STRIFNULL(description, _("(Not available)"));
        STRIFNULL(vermagic, _("(Not available)"));
        STRIFNULL(author, _("(Not available)"));
        STRIFNULL(license, _("(Not available)"));
        STRIFNULL(version, _("(Not available)"));

        gboolean ry = FALSE, ity = FALSE;
        if (retpoline && *retpoline == 'Y') ry = TRUE;
        if (intree && *intree == 'Y') ity = TRUE;

        g_free(retpoline);
        g_free(intree);

        retpoline = g_strdup(ry ? _("Yes") : _("No"));
        intree = g_strdup(ity ? _("Yes") : _("No"));

        /* create the module information string */
        strmodule = g_strdup_printf("[%s]\n"
                                    "%s=%s\n"
                                    "%s=%.2f %s\n"
                                    "[%s]\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "[%s]\n"
                                    "%s=%s\n"
                                    "%s=%s\n",
                                    _("Module Information"), _("Path"), filename, _("Used Memory"),
                                    memory / 1024.0, _("KiB"), _("Description"), _("Name"), modname,
                                    _("Description"), description, _("Version Magic"), vermagic,
                                    _("Version"), version, _("In Linus' Tree"), intree,
                                    _("Retpoline Enabled"), retpoline, _("Copyright"), _("Author"),
                                    author, _("License"), license);

        /* if there are dependencies, append them to that string */
        if (deps && strlen(deps)) {
            gchar **tmp = g_strsplit(deps, ",", 0);

            strmodule = h_strconcat(strmodule, "\n[", _("Dependencies"), "]\n",
                                    g_strjoinv("=\n", tmp), "=\n", NULL);
            g_strfreev(tmp);
            g_free(deps);
        }

        moreinfo_add_with_prefix("COMP", hashkey, strmodule);
        g_free(hashkey);

        g_free(license);
        g_free(description);
        g_free(author);
        g_free(vermagic);
        g_free(filename);
        g_free(srcversion);
        g_free(version);
        g_free(retpoline);
        g_free(intree);
    }
    pclose(lsmod);

    g_free(lsmod_path);
}
