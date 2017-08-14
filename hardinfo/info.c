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

void info_add_group(struct Info *info, const gchar *group_name, ...)
{
    struct InfoGroup group = {
        .name = group_name,
        .fields = g_array_new(FALSE, FALSE, sizeof(struct InfoField))
    };
    va_list ap;

    va_start(ap, group_name);
    while (1) {
        struct InfoField field = va_arg(ap, struct InfoField );

        if (!field.name)
            break;
        g_array_append_val(group.fields, field);
    }
    va_end(ap);

    g_array_append_val(info->groups, group);
}

struct InfoField info_field(const gchar *name, const gchar *value)
{
    return (struct InfoField) {
        .name = name,
        .value = value,
    };
}

struct InfoField info_field_update(const gchar *name, int update_interval)
{
    return (struct InfoField) {
        .name = name,
        .value = "...",
        .update_interval = update_interval,
    };
}

struct InfoField info_field_last(void)
{
    return (struct InfoField) {};
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

static gchar *flatten_group(gchar *output, const struct InfoGroup *group)
{
    guint i;

    output = h_strdup_cprintf("[%s]\n", output, group->name);

    if (group->fields) {
        for (i = 0; i < group->fields->len; i++) {
            struct InfoField field;

            field = g_array_index(group->fields, struct InfoField, i);
            output = h_strdup_cprintf("%s=%s\n", output, field.name, field.value);
        }
    } else if (group->computed) {
        output = h_strdup_cprintf("%s\n", output, group->computed);
    }

    return output;
}

static gchar *flatten_shell_param(gchar *output, const struct InfoGroup *group)
{
    guint i;

    if (!group->fields)
        return output;

    for (i = 0; i < group->fields->len; i++) {
        struct InfoField field;

        field = g_array_index(group->fields, struct InfoField, i);

        if (field.update_interval) {
            output = h_strdup_cprintf("UpdateInterval$%s=%d\n", output,
                field.name, field.update_interval);
        }
    }

    return output;
}

static gchar *flatten_shell_param_global(gchar *output, const struct Info *info)
{
    int i;

    output = h_strdup_cprintf("ViewType=%d\n", output, info->view_type);
    output = h_strdup_cprintf("ShowColumnHeaders=%s\n", output,
        info->column_headers_visible ? "true" : "false");

    if (info->zebra_visible)
        output = h_strdup_cprintf("Zebra=1\n", output);

    if (info->reload_interval)
        output = h_strdup_cprintf("ReloadInterval=%d\n", output, info->reload_interval);

    if (!info->normalize_percentage)
        output = h_strdup_cprintf("NormalizePercentage=false\n", output);

    for (i = 0; i < G_N_ELEMENTS(info_column_titles); i++) {
        if (!info->column_titles[i])
            continue;

        output = h_strdup_cprintf("ColumnTitle$%s=%s\n", output,
            info_column_titles[i], info->column_titles[i]);
    }

    return output;
}

gchar *info_flatten(struct Info *info)
{
    /* This is a scaffolding method: eventually the HardInfo shell should
     * understand a struct Info instead of parsing these strings, which are
     * brittle and unnecessarily complicates things.  Being a temporary
     * method, no attention is paid to improve the memory allocation
     * strategy. */
    gchar *values = NULL;
    gchar *shell_param = NULL;
    gchar *output;
    guint i;

    if (info->groups) {
        for (i = 0; i < info->groups->len; i++) {
            struct InfoGroup group =
                g_array_index(info->groups, struct InfoGroup, i);

            values = flatten_group(values, &group);
            shell_param = flatten_shell_param(shell_param, &group);

            if (group.fields)
                g_array_free(group.fields, TRUE);
        }

        g_array_free(info->groups, TRUE);
    }

    if (!values)
        values = g_strdup("");

    shell_param = flatten_shell_param_global(shell_param, info);
    if (shell_param) {
        output = g_strconcat(values, "[$ShellParam$]\n", shell_param, NULL);
        g_free(values);
        g_free(shell_param);
    } else {
        output = values;
    }

    g_free(info);

    return output;
}
