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

void info_group_add_field(struct InfoGroup *group, struct InfoField field)
{
    if (!group)
        return;

    /* info_field_last() */
    if (!field.name)
        return;

    g_array_append_val(group->fields, field);
}

void info_group_add_fieldsv(struct InfoGroup *group, va_list ap)
{
    while (1) {
        struct InfoField field = va_arg(ap, struct InfoField);

        /* info_field_last() */
        if (!field.name)
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

static int info_field_cmp_name_ascending(const void *a, const void *b)
{
    const struct InfoField *aa = a, *bb = b;
    return g_strcmp0(aa->name, bb->name);
}

static int info_field_cmp_name_descending(const void *a, const void *b)
{
    const struct InfoField *aa = a, *bb = b;
    return g_strcmp0(bb->name, aa->name);
}

static int info_field_cmp_value_ascending(const void *a, const void *b)
{
    const struct InfoField *aa = a, *bb = b;
    return g_strcmp0(aa->value, bb->value);
}

static int info_field_cmp_value_descending(const void *a, const void *b)
{
    const struct InfoField *aa = a, *bb = b;
    return g_strcmp0(bb->value, aa->value);
}

static int info_field_cmp_tag_ascending(const void *a, const void *b)
{
    const struct InfoField *aa = a, *bb = b;
    return g_strcmp0(aa->tag, bb->tag);
}

static int info_field_cmp_tag_descending(const void *a, const void *b)
{
    const struct InfoField *aa = a, *bb = b;
    return g_strcmp0(bb->tag, aa->tag);
}

static const GCompareFunc sort_functions[INFO_GROUP_SORT_MAX] = {
    [INFO_GROUP_SORT_NONE] = NULL,
    [INFO_GROUP_SORT_NAME_ASCENDING] = info_field_cmp_name_ascending,
    [INFO_GROUP_SORT_NAME_DESCENDING] = info_field_cmp_name_descending,
    [INFO_GROUP_SORT_VALUE_ASCENDING] = info_field_cmp_value_ascending,
    [INFO_GROUP_SORT_VALUE_DESCENDING] = info_field_cmp_value_descending,
    [INFO_GROUP_SORT_TAG_ASCENDING] = info_field_cmp_tag_ascending,
    [INFO_GROUP_SORT_TAG_DESCENDING] = info_field_cmp_tag_descending,
};

static void _field_free_strings(struct InfoField *field)
{
    if (field) {
        if (field->free_value_on_flatten)
            g_free((gchar *)field->value);
        if (field->free_name_on_flatten)
            g_free((gchar *)field->name);
        g_free(field->tag);
    }
}

static void _group_free_field_strings(struct InfoGroup *group)
{
    guint i;
    if (group && group->fields) {
        for (i = 0; i < group->fields->len; i++) {
            struct InfoField field;
            field = g_array_index(group->fields, struct InfoField, i);
            _field_free_strings(&field);
        }
    }
}

static void flatten_group(GString *output, const struct InfoGroup *group, guint group_count)
{
    guint i;

    if (group->name != NULL)
        g_string_append_printf(output, "[%s]\n", group->name);

    if (group->sort != INFO_GROUP_SORT_NONE) {
        g_array_sort(group->fields, sort_functions[group->sort]);
    }

    if (group->fields) {
        for (i = 0; i < group->fields->len; i++) {
            struct InfoField field;
            gchar tmp_tag[256] = ""; /* for generated tag */

            field = g_array_index(group->fields, struct InfoField, i);
            const gchar *tp = field.tag;
            gboolean tagged = !!tp;
            gboolean flagged = field.highlight || field.report_details;
            if (!tp) {
                snprintf(tmp_tag, 255, "ITEM%d-%d", group_count, i);
                tp = tmp_tag;
            }

            if (tagged || flagged || field.icon)
                g_string_append_printf(output, "$%s%s%s$",
                    field.highlight ? "*" : "",
                    field.report_details ? "!" : "",
                    tp);

            g_string_append_printf(output, "%s=%s\n", field.name, field.value);
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
        gchar tmp_tag[256] = ""; /* for generated tag */

        field = g_array_index(group->fields, struct InfoField, i);
        const gchar *tp = field.tag;
        gboolean tagged = !!tp;
        if (!tp) {
            snprintf(tmp_tag, 255, "ITEM%d-%d", group_count, i);
            tp = tmp_tag;
        }

        if (field.update_interval) {
            g_string_append_printf(output, "UpdateInterval$%s%s%s=%d\n",
                tagged ? tp : "", tagged ? "$" : "", /* tag and close or nothing */
                field.name,
                field.update_interval);
        }

        if (field.icon) {
            g_string_append_printf(output, "Icon$%s$=%s\n",
                tp, field.icon);
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

            _group_free_field_strings(&group);

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
