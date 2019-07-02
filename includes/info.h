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

enum {
    INFO_NONE           = 0,
    INFO_FIELD_SELECTED = 1,     /* '*' */
    INFO_FIELD_REPORT_DETAILS
                        = 1<<1,  /* '!' */
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

    GArray *fields;

     /* scaffolding fields */
    const gchar *computed;
};

struct InfoField {
    const gchar *name;
    const gchar *value;
    const gchar *icon;
          gchar *tag; /* moreinfo() lookup tag */

    int update_interval;
    unsigned int flags;

    gboolean free_value_on_flatten;
};

struct Info *info_new(void);

struct InfoGroup *info_add_group(struct Info *info, const gchar *group_name, ...);
void info_add_computed_group(struct Info *info, const gchar *name, const gchar *value);

void info_group_add_fields(struct InfoGroup *group, ...);
void info_group_add_fieldsv(struct InfoGroup *group, va_list ap);

struct InfoField info_field(const gchar *name, const gchar *value);
struct InfoField info_field_printf(const gchar *name, const gchar *format, ...)
    __attribute__((format(printf, 2, 3)));
struct InfoField info_field_update(const gchar *name, int update_interval);
struct InfoField info_field_last(void);

static inline struct InfoField info_field_with_icon(struct InfoField field, const gchar *icon)
{
    field.icon = icon;
    return field;
}
static inline struct InfoField info_field_set_flags(struct InfoField field, int flags)
{
    field.flags |= flags;
    return field;
}
#define info_field_selected(field, INFO_FIELD_SELECTED);
#define info_field_report_details(field, INFO_FIELD_REPORT_DETAILS);
static inline struct InfoField info_field_with_tag(struct InfoField field, const gchar *tag)
{
    field.tag = g_strdup(tag);
    return field;
}

void info_set_column_title(struct Info *info, const gchar *column, const gchar *title);
void info_set_column_headers_visible(struct Info *info, gboolean setting);
void info_set_zebra_visible(struct Info *info, gboolean setting);
void info_set_normalize_percentage(struct Info *info, gboolean setting);
void info_set_view_type(struct Info *info, ShellViewType setting);
void info_set_reload_interval(struct Info *info, int setting);

gchar *info_flatten(struct Info *info);
