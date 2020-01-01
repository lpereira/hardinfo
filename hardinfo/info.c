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
#include "util_sysobj.h" /* for SEQ() */

/* Using a slightly modified gg_key_file_parse_string_as_value()
 * from GLib in flatten(), to escape characters and the separator.
 * The function is not public in GLib and we don't have a GKeyFile
 * to pass it anyway. */
/* Now in hardinfo.h -- #include "gg_key_file_parse_string_as_value.c" */

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

void info_group_strip_extra(struct InfoGroup *group)
{
    guint fi;
    char *val, *oldval;
    struct InfoField *field;

    for (fi = 0; fi < group->fields->len; fi++) {
        field = &g_array_index(group->fields, struct InfoField, fi);
        if (field->value){
            val = strrchr(field->value, '|');
            if (val) {
                oldval = field->value;
                field->value = strdup(val + 1);
                g_free(oldval);
            }
        }
    }
}

void info_add_computed_group(struct Info *info, const gchar *name, const gchar *value)
{
    /* This is a scaffolding method: HardInfo should move away from pre-computing
     * the strings in favor of storing InfoGroups instead; while modules are not
     * fully converted, use this instead. */
    struct Info *tmp_info = NULL;
    struct InfoGroup donor = {};
    gchar *tmp_str = NULL;

    if (name)
        tmp_str = g_strdup_printf("[%s]\n%s", name, value);
    else
        tmp_str = g_strdup(value);

    tmp_info = info_unflatten(tmp_str);
    if (tmp_info->groups->len != 1) {
        fprintf(stderr,
            "info_add_computed_group(): expected only one group in value! (actual: %d)\n",
            tmp_info->groups->len);
    } else {
        donor = g_array_index(tmp_info->groups, struct InfoGroup, 0);
        g_array_append_val(info->groups, donor);
    }

    g_free(tmp_info); // TODO: doesn't do enough
    g_free(tmp_str);
}

void info_add_computed_group_wo_extra(struct Info *info, const gchar *name, const gchar *value)
{
    /* This is a scaffolding method: HardInfo should move away from pre-computing
     * the strings in favor of storing InfoGroups instead; while modules are not
     * fully converted, use this instead. */
    struct InfoGroup *agroup;

    info_add_computed_group(info, name, value);
    if (info->groups->len > 0) {
        agroup = &g_array_index(info->groups, struct InfoGroup, info->groups->len - 1);
        info_group_strip_extra(agroup);
    }
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

static void field_free_strings(struct InfoField *field)
{
    if (field->free_value_on_flatten)
        g_free((gchar *)field->value);
    if (field->free_name_on_flatten)
        g_free((gchar *)field->name);
    g_free(field->tag);
}

static void free_group_fields(struct InfoGroup *group)
{
    if (group && group->fields) {
        guint i;

        for (i = 0; i < group->fields->len; i++) {
            struct InfoField *field =
                &g_array_index(group->fields, struct InfoField, i);
            field_free_strings(field);
        }

        g_array_free(group->fields, TRUE);
    }
}

static void flatten_group(GString *output, const struct InfoGroup *group, guint group_count)
{
    guint i;

    if (group->name != NULL)
        g_string_append_printf(output, "[%s#%d]\n", group->name, group_count);

    if (group->sort != INFO_GROUP_SORT_NONE)
        g_array_sort(group->fields, sort_functions[group->sort]);

    if (group->fields) {
        for (i = 0; i < group->fields->len; i++) {
            struct InfoField *field = &g_array_index(group->fields, struct InfoField, i);
            gchar tmp_tag[256] = ""; /* for generated tag */

            gboolean do_escape = TRUE;
            if (field->value && strchr(field->value, '|') ) {
                /* turning off escaping for values that may have columns */
                do_escape = FALSE;
                /* TODO:/FIXME: struct InfoField should store the column values
                 * in an array instead of packing them into one value with '|'.
                 * Then each value can be escaped and joined together with '|'
                 * for flatten(). unflatten() can then split on non-escaped '|',
                 * and unescape the result values into the column value array.
                 * Another way to do this might be to check
                 * .column_headers_visible in struct Info, but that is not
                 * available here.
                 */
            }

            const gchar *tp = field->tag;
            gboolean tagged = !!tp;
            gboolean flagged = field->highlight || field->report_details || field->value_has_vendor;
            if (!tp) {
                snprintf(tmp_tag, 255, "ITEM%d-%d", group_count, i);
                tp = tmp_tag;
            }

            if (tagged || flagged || field->icon) {
                g_string_append_printf(output, "$%s%s%s%s$",
                    field->highlight ? "*" : "",
                    field->report_details ? "!" : "",
                    field->value_has_vendor ? "^" : "",
                    tp);
            }

            if (do_escape) {
                gchar *escaped_value = gg_key_file_parse_string_as_value(field->value, '|');
                g_string_append_printf(output, "%s=%s\n", field->name, escaped_value);
                g_free(escaped_value);
            } else {
                g_string_append_printf(output, "%s=%s\n", field->name, field->value);
            }
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
        struct InfoField *field = &g_array_index(group->fields, struct InfoField, i);
        gchar tmp_tag[256] = ""; /* for generated tag */

        const gchar *tp = field->tag;
        gboolean tagged = !!tp;
        if (!tp) {
            snprintf(tmp_tag, 255, "ITEM%d-%d", group_count, i);
            tp = tmp_tag;
        }

        if (field->update_interval) {
            g_string_append_printf(output, "UpdateInterval$%s%s%s=%d\n",
                tagged ? tp : "", tagged ? "$" : "", /* tag and close or nothing */
                field->name,
                field->update_interval);
        }

        if (field->icon) {
            g_string_append_printf(output, "Icon$%s$=%s\n",
                tp, field->icon);
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
            struct InfoGroup *group =
                &g_array_index(info->groups, struct InfoGroup, i);

            flatten_group(values, group, i);
            flatten_shell_param(shell_param, group, i);

            free_group_fields(group);
        }

        g_array_free(info->groups, TRUE);
    }

    flatten_shell_param_global(shell_param, info);
    g_string_append_printf(values, "[$ShellParam$]\n%s", shell_param->str);

    g_string_free(shell_param, TRUE);
    g_free(info);

    return g_string_free(values, FALSE);
}

struct InfoField *info_find_field(struct Info *info, const gchar *tag, const gchar *name) {
    struct InfoGroup *group;
    struct InfoField *field;
    int gi,fi;
    for (gi = 0; gi < info->groups->len; gi++) {
        struct InfoGroup *group = &g_array_index(info->groups, struct InfoGroup, gi);
        for (fi = 0; fi < group->fields->len; fi++) {
            struct InfoField *field = &g_array_index(group->fields, struct InfoField, fi);
            if (tag && SEQ(tag, field->tag) )
                return field;
            else if (name && SEQ(name, field->name) )
                return field;
        }
    }
    return NULL;
}

#define VAL_FALSE_OR_TRUE ((!g_strcmp0(value, "true") || !g_strcmp0(value, "1")) ? TRUE : FALSE)

struct Info *info_unflatten(const gchar *str)
{
    struct Info *info = info_new();
    GKeyFile *key_file = g_key_file_new();
    gchar **groups;
    gsize ngroups;
    int g, k, spg = -1;

    g_key_file_load_from_data(key_file, str, strlen(str), 0, NULL);
    groups = g_key_file_get_groups(key_file, &ngroups);
    for (g = 0; groups[g]; g++) {
        gchar *group_name = groups[g];
        gchar **keys = g_key_file_get_keys(key_file, group_name, NULL, NULL);

        if (*group_name == '$') {
            /* special group */
            if (SEQ(group_name, "$ShellParam$") )
                spg = g; /* handle after all groups are added */
            else {
                /* This special group is unknown and won't be handled, so
                 * the name will not be linked anywhere. */
                g_free(group_name);
                continue;
            }
        } else {
            /* normal group */
            struct InfoGroup group = {};
            group.name = group_name;
            group.fields = g_array_new(FALSE, FALSE, sizeof(struct InfoField));
            group.sort = INFO_GROUP_SORT_NONE;

            for (k = 0; keys[k]; k++) {
                struct InfoField field = {};
                gchar *flags, *tag, *name;
                key_get_components(keys[k], &flags, &tag, &name, NULL, NULL, TRUE);
                gchar *value = g_key_file_get_value(key_file, group_name, keys[k], NULL);

                field.tag = tag;
                field.name = name;
                field.value = value;
                field.free_value_on_flatten = TRUE;
                field.free_name_on_flatten = TRUE;
                if (key_wants_details(flags))
                    field.report_details = TRUE;
                if (key_is_highlighted(flags))
                    field.highlight = TRUE;
                if (key_value_has_vendor_string(flags))
                    field.value_has_vendor = TRUE;

                g_free(flags);
                g_array_append_val(group.fields, field);
            }
            g_array_append_val(info->groups, group);
        }
    }

    if (spg >= 0) {
        gchar *group_name = groups[spg];
        gchar **keys = g_key_file_get_keys(key_file, group_name, NULL, NULL);
        for (k = 0; keys[k]; k++) {
            gchar *value = g_key_file_get_value(key_file, group_name, keys[k], NULL);
            gchar *parm = NULL;
            if (SEQ(keys[k], "ViewType")) {
                info_set_view_type(info, atoi(value));
            } else if (SEQ(keys[k], "ShowColumnHeaders")) {
                info_set_column_headers_visible(info, VAL_FALSE_OR_TRUE);
            } else if (SEQ(keys[k], "Zebra")) {
                info_set_zebra_visible(info, VAL_FALSE_OR_TRUE);
            } else if (SEQ(keys[k], "ReloadInterval")) {
                info_set_reload_interval(info, atoi(value));
            } else if (SEQ(keys[k], "NormalizePercentage")) {
                info_set_normalize_percentage(info, VAL_FALSE_OR_TRUE);
            } else if (g_str_has_prefix(keys[k], "ColumnTitle$")) {
                info_set_column_title(info, strchr(keys[k], '$') + 1, value);
            } else if (g_str_has_prefix(keys[k], "Icon$")) {
                const gchar *chk_name = NULL;
                gchar *chk_tag = NULL;
                parm = strchr(keys[k], '$');
                if (key_is_flagged(parm))
                    chk_tag = key_mi_tag(parm);
                struct InfoField *field = info_find_field(info, chk_tag, NULL);
                if (field)
                    field->icon = value;
                g_free(chk_tag);
            } else if (g_str_has_prefix(keys[k], "UpdateInterval$")) {
                const gchar *chk_name = NULL;
                gchar *chk_tag = NULL;
                parm = strchr(keys[k], '$');
                if (key_is_flagged(parm)) {
                    chk_tag = key_mi_tag(parm);
                    chk_name = key_get_name(parm);
                } else
                    chk_name = key_get_name(parm+1);
                struct InfoField *field = info_find_field(info, chk_tag, chk_name);
                if (field)
                    field->update_interval = atoi(value);
                g_free(chk_tag);
            }
        }
        g_free(group_name);
        g_strfreev(keys);
    }

    return info;
}
