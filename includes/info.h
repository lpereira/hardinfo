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

#pragma once

#include <stdarg.h>
#include <glib.h>

enum InfoGroupSort {
    INFO_GROUP_SORT_NONE,
    INFO_GROUP_SORT_NAME_ASCENDING,
    INFO_GROUP_SORT_NAME_DESCENDING,
    INFO_GROUP_SORT_VALUE_ASCENDING,
    INFO_GROUP_SORT_VALUE_DESCENDING,
    INFO_GROUP_SORT_TAG_ASCENDING,
    INFO_GROUP_SORT_TAG_DESCENDING,
    INFO_GROUP_SORT_MAX,
};

struct Info {
    GArray *groups;

    const gchar *column_titles[5];

    ShellViewType view_type;

    int reload_interval;

    gboolean column_headers_visible;
    gboolean zebra_visible;
    gboolean normalize_percentage;
};

struct InfoGroup {
    const gchar *name;
    enum InfoGroupSort sort;

    GArray *fields;

     /* scaffolding fields */
    const gchar *computed;
};

struct InfoField {
          gchar *name;
    const gchar *name_const;
          gchar *value;
    const gchar *value_const;
          gchar *icon;
    const gchar *icon_const;
          gchar *tag;
    const gchar *tag_const;

    int update_interval;
    gboolean highlight;      /* select in GUI, highlight in report (flag:*) */
    gboolean report_details; /* show moreinfo() in report (flag:!) */
};

struct Info *info_new(void);

struct InfoGroup *info_add_group(struct Info *info, const gchar *group_name, ...);
void info_add_computed_group(struct Info *info, const gchar *name, const gchar *value);

void info_group_add_fields(struct InfoGroup *group, ...);
void info_group_add_fieldsv(struct InfoGroup *group, va_list ap);

struct InfoField info_field_printf(const gchar *name, const gchar *format, ...)
    __attribute__((format(printf, 2, 3)));

#define info_field_full(...)                                                   \
    (struct InfoField) { __VA_ARGS__ }

#define info_field(n, v, ...)                                                  \
    info_field_full(.name = (n), .value = (v), __VA_ARGS__)

#define info_field_update(n, ui, ...)                                          \
    info_field_full(.name = (n), .value_const = "...", .update_interval = (ui),      \
                    __VA_ARGS__)

#define info_field_last()                                                      \
    (struct InfoField) {}

#define info_field_get_name(fp) ((fp)->name ? (fp)->name : (fp)->name_const)
#define info_field_get_value(fp) ((fp)->value ? (fp)->value : (fp)->value_const)
#define info_field_get_tag(fp) ((fp)->tag ? (fp)->tag : (fp)->tag_const)
#define info_field_get_icon(fp) ((fp)->icon ? (fp)->icon : (fp)->icon_const)

void info_set_column_title(struct Info *info, const gchar *column, const gchar *title);
void info_set_column_headers_visible(struct Info *info, gboolean setting);
void info_set_zebra_visible(struct Info *info, gboolean setting);
void info_set_normalize_percentage(struct Info *info, gboolean setting);
void info_set_view_type(struct Info *info, ShellViewType setting);
void info_set_reload_interval(struct Info *info, int setting);

gchar *info_flatten(struct Info *info);
