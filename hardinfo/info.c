/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2017 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include "hardinfo.h"

static const gchar *info_column_titles[] = {
    "TextValue", "Value", "Progress", "Extra1", "Extra2"
};

struct Info *info_new(void)
{
    struct Info *info = g_new0(struct Info, 1);

    info->groups = g_array_new(FALSE, FALSE, sizeof(struct InfoGroup));
    info->view_type = SHELL_VIEW_NORMAL;
    info->column_headers_visible = FALSE;
    info->zebra_visible = FALSE;
    info->normalize_percentage = TRUE;

    return info;
}

void info_group_add_fieldsv(struct InfoGroup *group, va_list ap)
{
    while (1) {
        struct InfoField field = va_arg(ap, struct InfoField);

        if (field.magic == INFO_LAST_MARKER)
            break;
        g_array_append_val(group->fields, field);
    }
}

void info_group_add_fields(struct InfoGroup *group, ...)
{
    va_list ap;

    va_start(ap, group);
    info_group_add_fieldsv(group, ap);
    va_end(ap);
}

struct InfoGroup *info_add_group(struct Info *info, const gchar *group_name, ...)
{
    struct InfoGroup group = {
        .name = group_name,
        .fields = g_array_new(FALSE, FALSE, sizeof(struct InfoField))
    };
    va_list ap;

    va_start(ap, group_name);
    info_group_add_fieldsv(&group, ap);
    va_end(ap);

    g_array_append_val(info->groups, group);

    return &g_array_index(info->groups, struct InfoGroup, info->groups->len - 1);
}

struct InfoField info_field_printf(const gchar *name, const gchar *format, ...)
{
    gchar *value;
    va_list ap;

    va_start(ap, format);
    value = g_strdup_vprintf(format, ap);
    va_end(ap);

    return (struct InfoField) {
        .name = name,
        .value = value,
        .free_value_on_flatten = TRUE,
    };
}

void info_add_computed_group(struct Info *info, const gchar *name, const gchar *value)
{
    /* This is a scaffolding method: HardInfo should move away from pre-computing
     * the strings in favor of storing InfoGroups instead; while modules are not
     * fully converted, use this instead. */
    struct InfoGroup group = {
        .name = name,
        .computed = value,
    };

    g_array_append_val(info->groups, group);
}

void info_set_column_title(struct Info *info, const gchar *column, const gchar *title)
{
    int i;

    for (i = 0; i < G_N_ELEMENTS(info_column_titles); i++) {
        if (g_str_equal(info_column_titles[i], column)) {
            info->column_titles[i] = title;
            return;
        }
    }
}

void info_set_column_headers_visible(struct Info *info, gboolean setting)
{
    info->column_headers_visible = setting;
}

void info_set_zebra_visible(struct Info *info, gboolean setting)
{
    info->zebra_visible = setting;
}

void info_set_normalize_percentage(struct Info *info, gboolean setting)
{
    info->normalize_percentage = setting;
}

void info_set_view_type(struct Info *info, ShellViewType setting)
{
    info->view_type = setting;
}

void info_set_reload_interval(struct Info *info, int setting)
{
    info->reload_interval = setting;
}

static void flatten_group(GString *output, const struct InfoGroup *group, guint group_count)
{
    guint i;

    if (group->name != NULL)
        g_string_append_printf(output, "[%s]\n", group->name);

    if (group->fields) {
        for (i = 0; i < group->fields->len; i++) {
            struct InfoField field;
            gchar tag[256] = "";

            field = g_array_index(group->fields, struct InfoField, i);

            if (field.tag)
                strncpy(tag, field.tag, 255);
            else
                snprintf(tag, 255, "ITEM%d-%d", group_count, i);

            if (*tag != 0 || field.flags)
                g_string_append_printf(output, "$%s%s%s$",
                    field.flags & INFO_FIELD_SELECTED ? "*" : "",
                    field.flags & INFO_FIELD_SELECTED ? "!" : "",
                    tag);

            g_string_append_printf(output, "%s=%s\n", field.name, field.value);

            if (field.free_value_on_flatten)
                g_free((gchar *)field.value);

            g_free(field.tag);
        }
    } else if (group->computed) {
        g_string_append_printf(output, "%s\n", group->computed);
    }
}

static void flatten_shell_param(GString *output, const struct InfoGroup *group, guint group_count)
{
    guint i;

    if (!group->fields)
        return;

    for (i = 0; i < group->fields->len; i++) {
        struct InfoField field;

        field = g_array_index(group->fields, struct InfoField, i);

        if (field.update_interval) {
            g_string_append_printf(output, "UpdateInterval$%s=%d\n",
                field.name, field.update_interval);
        }

        if (field.icon) {
            g_string_append_printf(output, "Icon$ITEM%d-%d$=%s\n",
                group_count, i, field.icon);
        }
    }
}

static void flatten_shell_param_global(GString *output, const struct Info *info)
{
    int i;

    g_string_append_printf(output, "ViewType=%d\n", info->view_type);
    g_string_append_printf(output, "ShowColumnHeaders=%s\n",
        info->column_headers_visible ? "true" : "false");

    if (info->zebra_visible)
        g_string_append(output, "Zebra=1\n");

    if (info->reload_interval)
        g_string_append_printf(output, "ReloadInterval=%d\n", info->reload_interval);

    if (!info->normalize_percentage)
        g_string_append(output, "NormalizePercentage=false\n");

    for (i = 0; i < G_N_ELEMENTS(info_column_titles); i++) {
        if (!info->column_titles[i])
            continue;

        g_string_append_printf(output, "ColumnTitle$%s=%s\n",
            info_column_titles[i], info->column_titles[i]);
    }
}

gchar *info_flatten(struct Info *info)
{
    /* This is a scaffolding method: eventually the HardInfo shell should
     * understand a struct Info instead of parsing these strings, which are
     * brittle and unnecessarily complicates things.  Being a temporary
     * method, no attention is paid to improve the memory allocation
     * strategy. */
    GString *values;
    GString *shell_param;
    guint i;

    values = g_string_new(NULL);
    shell_param = g_string_new(NULL);

    if (info->groups) {
        for (i = 0; i < info->groups->len; i++) {
            struct InfoGroup group =
                g_array_index(info->groups, struct InfoGroup, i);

            flatten_group(values, &group, i);
            flatten_shell_param(shell_param, &group, i);

            if (group.fields)
                g_array_free(group.fields, TRUE);
        }

        g_array_free(info->groups, TRUE);
    }

    flatten_shell_param_global(shell_param, info);
    g_string_append_printf(values, "[$ShellParam$]\n%s", shell_param->str);

    g_string_free(shell_param, TRUE);
    g_free(info);

    return g_string_free(values, FALSE);
}
