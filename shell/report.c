/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 L. A. F. Pereira <l@tia.mat.br>
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

#include <report.h>
#include <stdio.h>
#include <string.h>
#include <shell.h>
#include <iconcache.h>
#include <hardinfo.h>
#include <config.h>

static ReportDialog *report_dialog_new(GtkTreeModel * model,
				       GtkWidget * parent);
static void set_all_active(ReportDialog * rd, gboolean setting);

static FileTypes file_types[] = {
    {"HTML (*.html)", "text/html", ".html", report_context_html_new},
    {"Plain Text (*.txt)", "text/plain", ".txt", report_context_text_new},
    {"Shell Dump (*.txt)", "text/plain", ".txt", report_context_shell_new},
    {NULL, NULL, NULL, NULL}
};

/* virtual functions */
void report_header(ReportContext * ctx)
{ ctx->header(ctx); }

void report_footer(ReportContext * ctx)
{ ctx->footer(ctx); }

void report_title(ReportContext * ctx, gchar * text)
{ ctx->title(ctx, text); }

void report_subtitle(ReportContext * ctx, gchar * text)
{ ctx->subtitle(ctx, text); }

void report_subsubtitle(ReportContext * ctx, gchar * text)
{ ctx->subsubtitle(ctx, text); }

void report_key_value(ReportContext * ctx, gchar *key, gchar *value, gsize longest_key)
{ ctx->keyvalue(ctx, key, value, longest_key); }

void report_details_start(ReportContext *ctx, gchar *key, gchar *value, gsize longest_key)
{ ctx->details_start(ctx, key, value, longest_key); }

void report_details_section(ReportContext *ctx, gchar *label)
{ ctx->details_section(ctx, label); }

void report_details_keyvalue(ReportContext *ctx, gchar *key, gchar *value, gsize longest_key)
{ ctx->details_keyvalue(ctx, key, value, longest_key); }

void report_details_end(ReportContext *ctx)
{ ctx->details_end(ctx); }
/* end virtual functions */

gint report_get_visible_columns(ReportContext *ctx)
{
    gint columns;

    /* Column count starts at two, since we always have at least
       two columns visible. */
    columns = 2;

    /* Either the Progress column or the Value column is available at
       the same time. So we don't count them. */

    if (ctx->columns & REPORT_COL_EXTRA1)
      columns++;

    if (ctx->columns & REPORT_COL_EXTRA2)
      columns++;

    return columns;
}

gchar *icon_name_css_id(const gchar *file) {
    gchar *safe = g_strdup_printf("icon-%s", file);
    gchar *p = safe;
    while(*p) {
        if (!isalnum(*p))
            *p = '-';
        p++;
    }
    return safe;
}

gchar *make_icon_css(const gchar *file) {
    if (!file || *file == 0)
        return g_strdup("");
    gchar *ret = NULL;
    gchar *path = g_build_filename(params.path_data, "pixmaps", file, NULL);
    gchar *contents = NULL;
    gsize length = 0;
    if ( g_file_get_contents(path, &contents, &length, NULL) ) {
        gchar *css_class = icon_name_css_id(file);
        const char *ctype = "image/png";
        if (g_str_has_suffix(file, ".svg") )
            ctype = "image/svg+xml";
        gchar *b64data = g_base64_encode(contents, length);
        ret = g_strdup_printf(
            ".%s\n"
            "{ background: url(data:%s;base64,%s) no-repeat;\n"
            "  background-size: cover; }\n",
            css_class ? css_class : "",
            ctype, b64data );
        g_free(b64data);
        g_free(css_class);
    }
    g_free(contents);
    g_free(path);
    return ret ? ret : g_strdup("");
}

void cache_icon(ReportContext *ctx, const gchar *file) {
    if (!ctx->icon_data) return;
    if (!g_hash_table_lookup(ctx->icon_data, file) )
        g_hash_table_insert(ctx->icon_data, g_strdup(file), make_icon_css(file));
}

void report_context_configure(ReportContext * ctx, GKeyFile * keyfile)
{
    gchar **keys;
    const gchar *group = "$ShellParam$";

    if (ctx->icon_refs) {
        g_hash_table_remove_all(ctx->icon_refs);
        ctx->icon_refs = NULL;
    }
    ctx->icon_refs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    keys = g_key_file_get_keys(keyfile, group, NULL, NULL);
    if (keys) {
      gint i = 0;

      for (; keys[i]; i++) {
        gchar *key = keys[i];

        if (g_str_equal(key, "ShowColumnHeaders")) {
          ctx->show_column_headers = g_key_file_get_boolean(keyfile, group, key, NULL);
        } else if (g_str_has_prefix(key, "ColumnTitle")) {
          gchar *value, *title = strchr(key, '$');

          if (!title) {
                  DEBUG("couldn't find column title");
                  break;
          }
          title++;
          if (!*title) {
                  DEBUG("title is empty");
                  break;
          }

          value = g_key_file_get_value(keyfile, group, key, NULL);
          if (g_str_equal(title, "Extra1")) {
                  ctx->columns |= REPORT_COL_EXTRA1;
          } else if (g_str_equal(title, "Extra2")) {
                  ctx->columns |= REPORT_COL_EXTRA2;
          } else if (g_str_equal(title, "Value")) {
                  ctx->columns |= REPORT_COL_VALUE;
          } else if (g_str_equal(title, "TextValue")) {
                  ctx->columns |= REPORT_COL_TEXTVALUE;
          } else if (g_str_equal(title, "Progress")) {
                  ctx->columns |= REPORT_COL_PROGRESS;
          }

          g_hash_table_replace(ctx->column_titles,
                               g_strdup(title), g_strdup(value));
        } else if (g_str_equal(key, "ViewType")) {
          if (g_key_file_get_integer(keyfile, group, "ViewType", NULL) == SHELL_VIEW_PROGRESS) {
            ctx->columns &= ~REPORT_COL_VALUE;
            ctx->columns |= REPORT_COL_PROGRESS;
          }
        } else if (g_str_has_prefix(key, "Icon$")) {
            gchar *ikey = g_utf8_strchr(key, -1, '$');
            gchar *tag = key_mi_tag(ikey);
            gchar *icon = g_key_file_get_value(keyfile, group, key, NULL);
            cache_icon(ctx, icon);
            g_hash_table_insert(ctx->icon_refs, tag, icon);
        }
      }

      g_strfreev(keys);
    }

}

static void report_html_details_start(ReportContext *ctx, gchar *key, gchar *value, gsize longest_key) {
    guint cols = report_get_visible_columns(ctx);
    report_key_value(ctx, key, value, longest_key);
    ctx->parent_columns = ctx->columns;
    ctx->columns = REPORT_COL_VALUE;
    ctx->output = h_strdup_cprintf("<tr><td colspan=\"%d\"><table class=\"details\">\n", ctx->output, cols);
}

static void report_html_details_end(ReportContext *ctx) {
    ctx->output = h_strdup_cprintf("</table></td></tr>\n", ctx->output);
    ctx->columns = ctx->parent_columns;
    ctx->parent_columns = 0;
}

void report_details(ReportContext *ctx, gchar *key, gchar *value, gchar *details, gsize longest_key)
{
    GKeyFile *key_file = g_key_file_new();
    gchar **groups;
    gint i;

    report_details_start(ctx, key, value, longest_key);
    ctx->in_details = TRUE;

    g_key_file_load_from_data(key_file, details, strlen(details), 0, NULL);
    groups = g_key_file_get_groups(key_file, NULL);

    for (i = 0; groups[i]; i++) {
        gchar *group, *tmpgroup;
        gchar **keys;
        gint j;

        if (groups[i][0] == '$') {
            continue;
        }

        group = groups[i];

        tmpgroup = g_strdup(group);
        strend(group, '#');

        report_subsubtitle(ctx, group);

        keys = g_key_file_get_keys(key_file, tmpgroup, NULL, NULL);

        gsize longest_key = 0;
        for (j = 0; keys[j]; j++) {
            gchar *lbl;
            key_get_components(keys[j], NULL, NULL, NULL, &lbl, NULL, TRUE);
            longest_key = MAX(longest_key, strlen(lbl));
            g_free(lbl);
        }

        for (j = 0; keys[j]; j++) {
            gchar *key = keys[j];
            gchar *raw_value, *value;

            raw_value = g_key_file_get_value(key_file, tmpgroup, key, NULL);
            value = g_strcompress(raw_value); /* un-escape \n, \t, etc */

            if (g_utf8_validate(key, -1, NULL) && g_utf8_validate(value, -1, NULL)) {
                strend(key, '#');

                if (g_str_equal(value, "...")) {
                    g_free(value);
                    if (!(value = ctx->entry->fieldfunc(key))) {
                        value = g_strdup("...");
                    }
                }

                report_key_value(ctx, key, value, longest_key);

            }

            g_free(value);
            g_free(raw_value);
        }

        g_strfreev(keys);
        g_free(tmpgroup);
    }

    g_strfreev(groups);
    g_key_file_free(key_file);

    ctx->in_details = FALSE;
    report_details_end(ctx);
}

static void report_table_shell_dump(ReportContext *ctx, gchar *key_file_str, int level)
{
    gchar *text, *p, *next_nl, *eq, *indent;
    gchar *key, *value;

    indent = g_strnfill(level * 4, ' ');

    if (key_file_str) {
        p = text = g_strdup(key_file_str);
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            eq = strchr(p, '=');
            if (*p != '[' && eq) {
                *eq = 0;
                key = p; value = eq + 1;

                ctx->output = h_strdup_cprintf("%s%s=%s\n", ctx->output, indent, key, value);
                if (key_wants_details(key) || params.force_all_details) {
                    gchar *mi_tag = key_mi_tag(key);
                    gchar *mi_data = ctx->entry->morefunc(mi_tag); /*const*/

                    if (mi_data)
                        report_table_shell_dump(ctx, mi_data, level + 1);

                    g_free(mi_tag);
                }

            } else
                ctx->output = h_strdup_cprintf("%s%s\n", ctx->output, indent, p);
            p = next_nl + 1;
        }
    }
    g_free(text);
    g_free(indent);
    return;
}

void report_table(ReportContext * ctx, gchar * text)
{
    GKeyFile *key_file = NULL;
    gchar **groups;
    gint i;

    if (ctx->format == REPORT_FORMAT_SHELL) {
        report_table_shell_dump(ctx, text, 0);
        return;
    }

    key_file = g_key_file_new();

    /* make only "Value" column visible ("Key" column is always visible) */
    ctx->columns = REPORT_COL_VALUE;
    ctx->show_column_headers = FALSE;

    /**/
    g_key_file_load_from_data(key_file, text, strlen(text), 0, NULL);
    groups = g_key_file_get_groups(key_file, NULL);

    for (i = 0; groups[i]; i++) {
        if (groups[i][0] == '$') {
            report_context_configure(ctx, key_file);
            break;
        }
    }

    for (i = 0; groups[i]; i++) {
        gchar *group, *tmpgroup;
        gchar **keys;
        gint j;

        if (groups[i][0] == '$') {
            continue;
        }

        group = groups[i];

        tmpgroup = g_strdup(group);
        strend(group, '#');

        report_subsubtitle(ctx, group);

        keys = g_key_file_get_keys(key_file, tmpgroup, NULL, NULL);

        gsize longest_key = 0;
        for (j = 0; keys[j]; j++) {
            gchar *lbl;
            key_get_components(keys[j], NULL, NULL, NULL, &lbl, NULL, TRUE);
            longest_key = MAX(longest_key, strlen(lbl));
            g_free(lbl);
        }

        for (j = 0; keys[j]; j++) {
            gchar *key = keys[j];
            gchar *raw_value, *value;

            raw_value = g_key_file_get_value(key_file, tmpgroup, key, NULL);
            value = g_strcompress(raw_value); /* un-escape \n, \t, etc */

            if (g_utf8_validate(key, -1, NULL) && g_utf8_validate(value, -1, NULL)) {
                strend(key, '#');

                if (g_str_equal(value, "...")) {
                    g_free(value);
                    if (!(value = ctx->entry->fieldfunc(key))) {
                        value = g_strdup("...");
                    }
                }

                if ( key_is_flagged(key) ) {
                    gchar *mi_tag = key_mi_tag(key);
                    gchar *mi_data = NULL; /*const*/

                    if (key_wants_details(key) || params.force_all_details)
                        mi_data = ctx->entry->morefunc(mi_tag);

                    if (mi_data)
                        report_details(ctx, key, value, mi_data, longest_key);
                    else
                        report_key_value(ctx, key, value, longest_key);

                    g_free(mi_tag);
                } else {
                    report_key_value(ctx, key, value, longest_key);
                }

            }

            g_free(value);
            g_free(raw_value);
        }

        g_strfreev(keys);

        g_free(tmpgroup);
    }

    g_strfreev(groups);
    g_key_file_free(key_file);
}

static void report_html_header(ReportContext * ctx)
{
    g_free(ctx->output);

    ctx->output =
	g_strdup_printf
	("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Final//EN\">\n"
	 "<html><head>\n" "<title>HardInfo (%s) System Report</title>\n"
	 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
	 "<style>\n" "    body    { background: #fff }\n"
	 "    .title  { font: bold 130%% serif; color: #0066FF; padding: 30px 0 10px 0 }\n"
	 "    .stitle { font: bold 100%% sans-serif; color: #0044DD; padding: 0 0 0 0; }\n"
	 "    .sstitle{ font: bold 80%% serif; color: #000000; background: #efefef }\n"
	 "    .field  { font: 80%% sans-serif; color: #000000; padding: 2px; }\n"
	 "    .value  { font: 80%% sans-serif; color: #505050 }\n"
	 "    .hilight  { font: bold 110%% sans-serif; color: #000000; background: #ffff66 }\n"
	 "    table.details { margin-left: 50px; }\n"
	 "    td.icon { width: 1.2em; padding-left: 1.2em; }\n"
	 "    td.icon img  { width: 1.2em; }\n"
	 "    td.icon div { display: block; box-sizing: border-box; -moz-box-sizing: border-box;\n"
	 "        width: 1.2em; height: 1.2em; background-position: right; }\n"
	 "    td.icon_subtitle div { display: block; box-sizing: border-box; -moz-box-sizing: border-box;\n"
	 "        width: 1.8em; height: 1.8em; background-position: right; }\n"
	 "</style>\n" "</head><body>\n",
	 VERSION);
}

static void report_html_footer(ReportContext * ctx)
{
    ctx->output = h_strconcat(ctx->output, "</table>", NULL);
    ctx->output = h_strconcat(ctx->output, "<style>\n", NULL);
    GList *l = NULL, *keys = g_hash_table_get_keys(ctx->icon_data);
    for(l = keys; l; l = l->next) {
        gchar *data = g_hash_table_lookup(ctx->icon_data, (gchar*)l->data);
        if (data)
            ctx->output = h_strconcat(ctx->output, data, NULL);
    }
    g_list_free(keys);
    ctx->output = h_strconcat(ctx->output, "</style>\n", NULL);
    ctx->output = h_strconcat(ctx->output, "</html>", NULL);
}

static void report_html_title(ReportContext * ctx, gchar * text)
{
    if (!ctx->first_table) {
      ctx->output = h_strdup_cprintf("</table>", ctx->output);
    }

    ctx->output = h_strdup_cprintf("<h1 class=\"title\">%s</h1>", ctx->output, text);
}

static void report_html_subtitle(ReportContext * ctx, gchar * text)
{
    gint columns = report_get_visible_columns(ctx);

    if (!ctx->first_table) {
      ctx->output = h_strdup_cprintf("</table>", ctx->output);
    } else {
      ctx->first_table = FALSE;
    }

    gchar *icon = NULL;
    if (ctx->entry->icon_file) {
        gchar *icon_class = icon_name_css_id(ctx->entry->icon_file);
        icon = g_strdup_printf("<div class=\"%s\"></div>", icon_class);
        g_free(icon_class);
    } else {
        icon = g_strdup("");
    }

    ctx->output = h_strdup_cprintf("<table><tr><td class=\"icon_subtitle\">%s</td><td colspan=\"%d\" class=\"stit"
				  "le\">%s</td></tr>\n",
				  ctx->output,
                  icon,
				  columns,
				  text);
    g_free(icon);
}

static void report_html_subsubtitle(ReportContext * ctx, gchar * text)
{
    gint columns = report_get_visible_columns(ctx);

    ctx->output = h_strdup_cprintf("<tr><td colspan=\"%d\" class=\"ssti"
				  "tle\">%s</td></tr>\n",
				  ctx->output,
				  columns+1,
				  text);
}

static void
report_html_key_value(ReportContext * ctx, gchar *key, gchar *value, gsize longest_key)
{
    gint columns = report_get_visible_columns(ctx);
    gchar **values;
    gint i, mc;

    gboolean highlight = key_is_highlighted(key);
    gchar *tag = key_mi_tag(key);
    gchar *icon = tag ? (gchar*)g_hash_table_lookup(ctx->icon_refs, tag) : NULL;
    g_free(tag);
    /* icon from the table is const, so can be re-used without free */
    if (icon) {
        gchar *icon_class = icon_name_css_id(icon);
        icon = g_strdup_printf("<div class=\"%s\"></div>", icon_class);
        g_free(icon_class);
    } else
        icon = g_strdup("");

    gchar *name = (gchar*)key_get_name(key);

    if (columns == 2) {
      ctx->output = h_strdup_cprintf("<tr%s><td class=\"icon\">%s</td><td class=\"field\">%s</td>"
                                    "<td class=\"value\">%s</td></tr>\n",
                                    ctx->output,
                                    highlight ? " class=\"hilight\"" : "",
                                    icon, name, value);
    } else {
      values = g_strsplit(value, "|", columns);
      mc = g_strv_length(values) - 1;

      ctx->output = h_strdup_cprintf("\n<tr%s>\n<td class=\"icon\">%s</td><td class=\"field\">%s</td>", ctx->output, highlight ? " class=\"hilight\"" : "", icon, name);

      for (i = mc; i >= 0; i--) {
        ctx->output = h_strdup_cprintf("<td class=\"value\">%s</td>",
                                       ctx->output,
                                       values[i]);
      }

      ctx->output = h_strdup_cprintf("</tr>\n", ctx->output);

      g_strfreev(values);
    }
    g_free(icon);
}

static void report_text_header(ReportContext * ctx)
{
    g_free(ctx->output);

    ctx->output = g_strdup("");
}

static void report_text_footer(ReportContext * ctx)
{
}

static void report_text_title(ReportContext * ctx, gchar * text)
{
    gchar *str = (gchar *) ctx->output;
    int i = strlen(text);

    str = h_strdup_cprintf("\n%s\n", str, text);
    for (; i; i--)
	str = h_strconcat(str, "*", NULL);

    str = h_strconcat(str, "\n\n", NULL);
    ctx->output = str;
}

static void report_text_subtitle(ReportContext * ctx, gchar * text)
{
    gchar *str = ctx->output;
    int i = strlen(text);

    str = h_strdup_cprintf("\n%s\n", str, text);
    for (; i; i--)
	str = h_strconcat(str, "-", NULL);

    str = h_strconcat(str, "\n\n", NULL);
    ctx->output = str;
}

static void report_text_subsubtitle(ReportContext * ctx, gchar * text)
{
    gchar indent[10] = "   ";
    if (!ctx->in_details)
        indent[0] = 0;
    ctx->output = h_strdup_cprintf("%s-%s-\n", ctx->output, indent, text);
}

static void
report_text_key_value(ReportContext * ctx, gchar *key, gchar *value, gsize longest_key)
{
    gint columns = report_get_visible_columns(ctx);
    gchar **values;
    gint i, mc, field_width = MAX(10, longest_key);
    gchar indent[10] = "      ";
    if (!ctx->in_details)
        indent[0] = 0;
    gchar field_spacer[51];
    for(i = 0; i < 49; i++)
        field_spacer[i] = ' ';
    field_width = MIN(50, field_width);
    field_spacer[field_width] = 0;

    gboolean highlight = key_is_highlighted(key);
    gboolean multiline = (value && strlen(value) && strchr(value, '\n'));
    gchar *name = (gchar*)key_get_name(key);
    gchar *pf = g_strdup_printf("%s%s", indent, highlight ? "* " : "  ");
    gchar *rjname = g_strdup(field_spacer);
    if (strlen(name) > strlen(rjname))
        name[strlen(rjname)] = 0;
    strcpy(rjname + strlen(rjname) - strlen(name), name);

    if (columns == 2 || ctx->in_details) {
      if (strlen(value)) {
          if (multiline) {
              gchar **lines = g_strsplit(value, "\n", 0);
              for(i=0; lines[i]; i++) {
                  if (i == 0)
                      ctx->output = h_strdup_cprintf("%s%s : %s\n", ctx->output, pf, rjname, lines[i]);
                  else
                      ctx->output = h_strdup_cprintf("%s%s   %s\n", ctx->output, pf, field_spacer, lines[i]);
              }
              g_strfreev(lines);
          } else {
              ctx->output = h_strdup_cprintf("%s%s : %s\n", ctx->output, pf, rjname, value);
          }
      } else
          ctx->output = h_strdup_cprintf("%s%s\n", ctx->output, pf, rjname);
    } else {
      values = g_strsplit(value, "|", columns);
      mc = g_strv_length(values) - 1;

      ctx->output = h_strdup_cprintf("%s%s", ctx->output, pf, rjname);

      for (i = mc; i >= 0; i--) {
        ctx->output = h_strdup_cprintf("\t%s",
                                       ctx->output,
                                       values[i]);
      }

      ctx->output = h_strdup_cprintf("\n", ctx->output);

      g_strfreev(values);
    }
    g_free(pf);
}

static GSList *report_create_module_list_from_dialog(ReportDialog * rd)
{
    ShellModule *module;
    GSList *modules = NULL;
    GtkTreeModel *model = rd->model;
    GtkTreeIter iter;

    gtk_tree_model_get_iter_first(model, &iter);
    do {
	gboolean selected;
	gchar *name;

	gtk_tree_model_get(model, &iter, TREE_COL_SEL, &selected, -1);
	if (!selected)
	    continue;

	module = g_new0(ShellModule, 1);

	gtk_tree_model_get(model, &iter, TREE_COL_NAME, &name, -1);
	module->name = name;
	module->entries = NULL;

	if (gtk_tree_model_iter_has_child(model, &iter)) {
	    ShellModuleEntry *entry;

	    gint children = gtk_tree_model_iter_n_children(model, &iter);
	    gint i;

	    for (i = 0; i < children; i++) {
		GtkTreeIter child;

		gtk_tree_model_iter_nth_child(model, &child, &iter, i);

		gtk_tree_model_get(model, &child, TREE_COL_SEL, &selected,
				   -1);
		if (!selected)
		    continue;

		gtk_tree_model_get(model, &child, TREE_COL_MODULE_ENTRY, &entry,
				   -1);
		module->entries = g_slist_append(module->entries, entry);
	    }
	}

	modules = g_slist_append(modules, module);
    } while (gtk_tree_model_iter_next(rd->model, &iter));

    return modules;
}

static void
report_create_inner_from_module_list(ReportContext * ctx, GSList * modules)
{
    for (; modules; modules = modules->next) {
	ShellModule *module = (ShellModule *) modules->data;
	GSList *entries;

	if (!params.gui_running && !params.quiet)
	    fprintf(stderr, "\033[40;32m%s\033[0m\n", module->name);

	report_title(ctx, module->name);

	for (entries = module->entries; entries; entries = entries->next) {
	    ShellModuleEntry *entry = (ShellModuleEntry *) entries->data;
        if (entry->flags & MODULE_FLAG_HIDE) continue;

	    if (!params.gui_running && !params.quiet)
		fprintf(stderr, "\033[2K\033[40;32;1m %s\033[0m\n",
			entry->name);

        if (entry->icon_file)
            cache_icon(ctx, entry->icon_file);

	    ctx->entry = entry;
	    report_subtitle(ctx, entry->name);
	    module_entry_scan(entry);
	    report_table(ctx, module_entry_function(entry));
	}
    }
}

void report_module_list_free(GSList * modules)
{
    GSList *m;

    for (m = modules; m; m = m->next) {
	ShellModule *module = (ShellModule *) m->data;

	g_slist_free(module->entries);
    }

    g_slist_free(modules);
}

static gchar *report_get_filename(void)
{
    GtkWidget *dialog;
    gchar *filename = NULL;

#if GTK_CHECK_VERSION(3, 0, 0)
    dialog = gtk_file_chooser_dialog_new(_("Save File"),
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 _("_Cancel"),
					 GTK_RESPONSE_CANCEL,
					 _("_Save"),
					 GTK_RESPONSE_ACCEPT, NULL);
#else
    dialog = gtk_file_chooser_dialog_new(_("Save File"),
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_SAVE,
					 GTK_RESPONSE_ACCEPT, NULL);
#endif

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
				      "hardinfo_report");

    file_chooser_add_filters(dialog, file_types);
    file_chooser_open_expander(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	gchar *ext = file_chooser_get_extension(dialog, file_types);
	filename = file_chooser_build_filename(dialog, ext);
    }
    gtk_widget_destroy(dialog);
    return filename;
}

ReportContext *report_context_html_new()
{
    ReportContext *ctx;

    ctx = g_new0(ReportContext, 1);
    ctx->header = report_html_header;
    ctx->footer = report_html_footer;
    ctx->title = report_html_title;
    ctx->subtitle = report_html_subtitle;
    ctx->subsubtitle = report_html_subsubtitle;
    ctx->keyvalue = report_html_key_value;

    ctx->details_start = report_html_details_start;
    ctx->details_section = report_html_subsubtitle;
    ctx->details_keyvalue = report_html_key_value;
    ctx->details_end = report_html_details_end;

    ctx->output = g_strdup("");
    ctx->format = REPORT_FORMAT_HTML;

    ctx->column_titles = g_hash_table_new_full(g_str_hash, g_str_equal,
                                               g_free, g_free);
    ctx->first_table = TRUE;

    ctx->icon_data = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    return ctx;
}

ReportContext *report_context_text_new()
{
    ReportContext *ctx;

    ctx = g_new0(ReportContext, 1);
    ctx->header = report_text_header;
    ctx->footer = report_text_footer;
    ctx->title = report_text_title;
    ctx->subtitle = report_text_subtitle;
    ctx->subsubtitle = report_text_subsubtitle;
    ctx->keyvalue = report_text_key_value;

    ctx->details_start = report_text_key_value;
    ctx->details_section = report_text_subsubtitle;
    ctx->details_keyvalue = report_text_key_value;
    ctx->details_end = report_text_footer; /* nothing */

    ctx->output = g_strdup("");
    ctx->format = REPORT_FORMAT_TEXT;

    ctx->column_titles = g_hash_table_new_full(g_str_hash, g_str_equal,
                                               g_free, g_free);
    ctx->first_table = TRUE;

    return ctx;
}

ReportContext *report_context_shell_new()
{
    ReportContext *ctx;

    ctx = g_new0(ReportContext, 1);
    ctx->header = report_text_header;
    ctx->footer = report_text_footer;

    ctx->title = report_text_title;
    ctx->subtitle = report_text_subtitle;
    /* special format handled in report_table(),
     * doesn't need the others. */

    ctx->output = g_strdup("");
    ctx->format = REPORT_FORMAT_SHELL;

    ctx->column_titles = g_hash_table_new_full(g_str_hash, g_str_equal,
                                               g_free, g_free);
    ctx->first_table = TRUE;

    return ctx;
}

void report_context_free(ReportContext * ctx)
{
    g_hash_table_destroy(ctx->column_titles);
    if(ctx->icon_refs)
        g_hash_table_destroy(ctx->icon_refs);
    if(ctx->icon_data)
        g_hash_table_destroy(ctx->icon_data);
    g_free(ctx->output);
    g_free(ctx);
}

void report_create_from_module_list(ReportContext * ctx, GSList * modules)
{
    if (ctx->format == REPORT_FORMAT_HTML)
        params.fmt_opts = FMT_OPT_HTML;

    report_header(ctx);

    report_create_inner_from_module_list(ctx, modules);
    report_module_list_free(modules);

    report_footer(ctx);
}

gchar *report_create_from_module_list_format(GSList * modules,
					     ReportFormat format)
{
    ReportContext *(*create_context) ();
    ReportContext *ctx;
    gchar *retval;

    if (format >= N_REPORT_FORMAT)
	return NULL;

    create_context = file_types[format].data;
    if (!create_context)
	return NULL;

    ctx = create_context();

    report_create_from_module_list(ctx, modules);
    retval = g_strdup(ctx->output);

    report_context_free(ctx);

    return retval;
}

static gboolean report_generate(ReportDialog * rd)
{
    GSList *modules;
    ReportContext *ctx;
    ReportContext *(*create_context) ();
    gchar *file;
    FILE *stream;

    int old_fmt_opts = params.fmt_opts;
    params.fmt_opts = FMT_OPT_NONE;

    if (!(file = report_get_filename()))
	return FALSE;

    if (!(stream = fopen(file, "w+"))) {
	g_free(file);
	return FALSE;
    }

    create_context = file_types_get_data_by_name(file_types, file);

    if (!create_context) {
	g_warning(_("Cannot create ReportContext. Programming bug?"));
	g_free(file);
	fclose(stream);
    params.fmt_opts = old_fmt_opts;
	return FALSE;
    }

    ctx = create_context();
    modules = report_create_module_list_from_dialog(rd);

    report_create_from_module_list(ctx, modules);
    fputs(ctx->output, stream);
    fclose(stream);

    if (ctx->format == REPORT_FORMAT_HTML) {
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_NONE,
					_("Open the report with your web browser?"));
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			       _("_No"), GTK_RESPONSE_REJECT,
			       _("_Open"), GTK_RESPONSE_ACCEPT, NULL);
#else
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			       GTK_STOCK_NO, GTK_RESPONSE_REJECT,
			       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
#endif
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	    gchar *temp;

	    temp = g_strdup_printf("file://%s", file);
	    uri_open(temp);

	    g_free(temp);
        }

	gtk_widget_destroy(dialog);
    }

    report_context_free(ctx);
    g_free(file);

    params.fmt_opts = old_fmt_opts;
    return TRUE;
}

void report_dialog_show(GtkTreeModel * model, GtkWidget * parent)
{
    gboolean success;
    ReportDialog *rd = report_dialog_new(model, parent);

    if (gtk_dialog_run(GTK_DIALOG(rd->dialog)) == GTK_RESPONSE_ACCEPT) {
	shell_status_update(_("Generating report..."));
	gtk_widget_hide(rd->dialog);
	shell_view_set_enabled(FALSE);
	shell_status_set_enabled(TRUE);

	success = report_generate(rd);

	shell_status_set_enabled(FALSE);

	if (success)
	    shell_status_update(_("Report saved."));
	else
	    shell_status_update(_("Error while creating the report."));
    }

    set_all_active(rd, FALSE);
    gtk_widget_destroy(rd->dialog);
    g_free(rd);
}

static void
set_children_active(GtkTreeModel * model, GtkTreeIter * iter,
		    gboolean setting)
{
    if (gtk_tree_model_iter_has_child(model, iter)) {
	gint children = gtk_tree_model_iter_n_children(model, iter);

	gtk_tree_store_set(GTK_TREE_STORE(model), iter, TREE_COL_SEL,
			   setting, -1);

	for (children--; children >= 0; children--) {
	    GtkTreeIter child;

	    gtk_tree_model_iter_nth_child(model, &child, iter, children);
	    gtk_tree_store_set(GTK_TREE_STORE(model), &child, TREE_COL_SEL,
			       setting, -1);
	}
    }
}

static void set_all_active(ReportDialog * rd, gboolean setting)
{
    GtkTreeIter iter;
    GtkTreeModel *model = rd->model;

    gtk_tree_model_get_iter_first(model, &iter);

    do {
	set_children_active(model, &iter, setting);
    } while (gtk_tree_model_iter_next(model, &iter));
}

static void report_dialog_sel_none(GtkWidget * widget, ReportDialog * rd)
{
    set_all_active(rd, FALSE);
}

static void report_dialog_sel_all(GtkWidget * widget, ReportDialog * rd)
{
    set_all_active(rd, TRUE);
}

static void
report_dialog_sel_toggle(GtkCellRendererToggle * cellrenderertoggle,
			 gchar * path_str, ReportDialog * rd)
{
    GtkTreeModel *model = rd->model;
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean active;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, TREE_COL_SEL, &active, -1);

    active = !active;
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, TREE_COL_SEL, active,
		       -1);
    set_children_active(model, &iter, active);

    if (active) {
        GtkTreeIter parent;

        if (gtk_tree_model_iter_parent(model, &parent, &iter)) {
            gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
                               TREE_COL_SEL, active, -1);
        }
    }

    gtk_tree_path_free(path);
}

static ReportDialog
    * report_dialog_new(GtkTreeModel * model, GtkWidget * parent)
{
    ReportDialog *rd;
    GtkWidget *dialog;
    GtkWidget *dialog1_vbox;
    GtkWidget *scrolledwindow2;
    GtkWidget *treeview2;
    GtkWidget *vbuttonbox3;
    GtkWidget *button3;
    GtkWidget *button6;
    GtkWidget *dialog1_action_area;
    GtkWidget *button8;
    GtkWidget *button7;
    GtkWidget *label;
    GtkWidget *hbox;

    GtkTreeViewColumn *column;
    GtkCellRenderer *cr_text, *cr_pbuf, *cr_toggle;

    rd = g_new0(ReportDialog, 1);

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), _("Generate Report"));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog),
			    GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2, 14, 0)
    dialog1_vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
    dialog1_vbox = GTK_DIALOG(dialog)->vbox;
#endif
    gtk_box_set_spacing(GTK_BOX(dialog1_vbox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(dialog1_vbox), 4);
    gtk_widget_show(dialog1_vbox);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
    hbox = gtk_hbox_new(FALSE, 5);
#endif
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("<big><b>Generate Report</b></big>\n"
			  "Please choose the information that you wish "
			  "to view in your report:"));
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
#endif

    gtk_box_pack_start(GTK_BOX(hbox),
		       icon_cache_get_image("report-large.png"),
		       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_widget_show_all(hbox);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
    hbox = gtk_hbox_new(FALSE, 5);
#endif
    gtk_box_pack_start(GTK_BOX(dialog1_vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow2);
    gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow2, TRUE, TRUE,
		       0);
    gtk_widget_set_size_request(scrolledwindow2, -1, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					(scrolledwindow2), GTK_SHADOW_IN);

    treeview2 = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview2), FALSE);
    gtk_widget_show(treeview2);
    gtk_container_add(GTK_CONTAINER(scrolledwindow2), treeview2);

    column = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), column);

    cr_toggle = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(column, cr_toggle, FALSE);
    g_signal_connect(cr_toggle, "toggled",
		     G_CALLBACK(report_dialog_sel_toggle), rd);
    gtk_tree_view_column_add_attribute(column, cr_toggle, "active",
				       TREE_COL_SEL);

    cr_pbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, cr_pbuf, FALSE);
    gtk_tree_view_column_add_attribute(column, cr_pbuf, "pixbuf",
				       TREE_COL_PBUF);

    cr_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, cr_text, TRUE);
    gtk_tree_view_column_add_attribute(column, cr_text, "markup",
				       TREE_COL_NAME);

#if GTK_CHECK_VERSION(3, 0, 0)
    vbuttonbox3 = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
#else
    vbuttonbox3 = gtk_vbutton_box_new();
#endif
    gtk_widget_show(vbuttonbox3);
    gtk_box_pack_start(GTK_BOX(hbox), vbuttonbox3, FALSE, TRUE, 0);
    gtk_box_set_spacing(GTK_BOX(vbuttonbox3), 5);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox3),
			      GTK_BUTTONBOX_START);

    button3 = gtk_button_new_with_mnemonic(_("Select _None"));
    gtk_widget_show(button3);
    gtk_container_add(GTK_CONTAINER(vbuttonbox3), button3);
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_default(button3, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(button3, GTK_CAN_DEFAULT);
#endif
    g_signal_connect(button3, "clicked",
		     G_CALLBACK(report_dialog_sel_none), rd);

    button6 = gtk_button_new_with_mnemonic(_("Select _All"));
    gtk_widget_show(button6);
    gtk_container_add(GTK_CONTAINER(vbuttonbox3), button6);
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_default(button6, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(button6, GTK_CAN_DEFAULT);
#endif
    g_signal_connect(button6, "clicked", G_CALLBACK(report_dialog_sel_all),
		     rd);

#if GTK_CHECK_VERSION(2, 14, 0)
/* TODO:GTK3
 * [https://developer.gnome.org/gtk3/stable/GtkDialog.html#gtk-dialog-get-action-area]
 * gtk_dialog_get_action_area has been deprecated since version 3.12 and should not be used in newly-written code.
 * Direct access to the action area is discouraged; use gtk_dialog_add_button(), etc.
 */
    dialog1_action_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
    dialog1_action_area = GTK_DIALOG(dialog)->action_area;
#endif
    gtk_widget_show(dialog1_action_area);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog1_action_area),
			      GTK_BUTTONBOX_END);

    button8 = gtk_button_new_with_mnemonic(_("_Cancel"));
    gtk_widget_show(button8);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button8,
				 GTK_RESPONSE_CANCEL);
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_default(button8, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(button8, GTK_CAN_DEFAULT);
#endif

    button7 = gtk_button_new_with_mnemonic(_("_Generate"));
    gtk_widget_show(button7);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button7,
				 GTK_RESPONSE_ACCEPT);
#if GTK_CHECK_VERSION(2, 18, 0)
    gtk_widget_set_can_default(button7, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(button7, GTK_CAN_DEFAULT);
#endif

    rd->dialog = dialog;
    rd->btn_cancel = button8;
    rd->btn_generate = button7;
    rd->btn_sel_all = button6;
    rd->btn_sel_none = button3;
    rd->treeview = treeview2;
    rd->model = model;

    gtk_tree_view_collapse_all(GTK_TREE_VIEW(treeview2));
    set_all_active(rd, TRUE);

    return rd;
}
