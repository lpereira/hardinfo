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

#define GET_STR(field_name,ptr)      					\
  if (!ptr && strstr(tmp[0], field_name)) {				\
    ptr = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));	\
    g_strfreev(tmp);                 					\
    continue;                        					\
  }

static gboolean
remove_module_devices(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "MOD");
}

static void
scan_modules_do(void)
{
    FILE *lsmod;
    gchar buffer[1024];

    if (module_list) {
        g_free(module_list);
    }
    
    module_list = NULL;
    g_hash_table_foreach_remove(moreinfo, remove_module_devices, NULL);

    lsmod = popen("/sbin/lsmod", "r");
    if (!lsmod)
	return;

    fgets(buffer, 1024, lsmod);	/* Discards the first line */

    while (fgets(buffer, 1024, lsmod)) {
	gchar *buf, *strmodule, *hashkey;
	gchar *author = NULL,
	    *description = NULL,
	    *license = NULL,
	    *deps = NULL, *vermagic = NULL, *filename = NULL, modname[64];
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
	}

	/* append this module to the list of modules */
	module_list = h_strdup_cprintf("$%s$%s=%s\n",
				      module_list,
				      hashkey,
				      modname,
				      description ? description : "");

#define NONE_IF_NULL(var) (var) ? (var) : "N/A"

	/* create the module information string */
	strmodule = g_strdup_printf("[Module Information]\n"
				    "Path=%s\n"
				    "Used Memory=%.2fKiB\n"
				    "[Description]\n"
				    "Name=%s\n"
				    "Description=%s\n"
				    "Version Magic=%s\n"
				    "[Copyright]\n"
				    "Author=%s\n"
				    "License=%s\n",
				    NONE_IF_NULL(filename),
				    memory / 1024.0,
				    modname,
				    NONE_IF_NULL(description),
				    NONE_IF_NULL(vermagic),
				    NONE_IF_NULL(author),
				    NONE_IF_NULL(license));

	/* if there are dependencies, append them to that string */
	if (deps && strlen(deps)) {
	    gchar **tmp = g_strsplit(deps, ",", 0);

	    strmodule = h_strconcat(strmodule,
                                    "\n[Dependencies]\n",
                                    g_strjoinv("=\n", tmp),
                                    "=\n", NULL);
	    g_strfreev(tmp);
	    g_free(deps);
	}

	g_hash_table_insert(moreinfo, hashkey, strmodule);

	g_free(license);
	g_free(description);
	g_free(author);
	g_free(vermagic);
	g_free(filename);
    }
    pclose(lsmod);
}
