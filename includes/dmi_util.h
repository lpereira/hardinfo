/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
 *    This file
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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

#ifndef __DMI_UTIL_H__
#define __DMI_UTIL_H__

#include <inttypes.h>

typedef uint32_t dmi_handle;
typedef uint32_t dmi_type;

/* -1 = yes, but will be ignored
 *  0 = no
 *  1 = yes */
int dmi_str_status(const char *id_str);
char *dmi_get_str(const char *id_str);     /* ignore nonsense */
char *dmi_get_str_abs(const char *id_str); /* include nonsense */

/* if chassis_type is <=0 it will be fetched from DMI.
 * with_val = true, will return a string like "[3] Desktop" instead of just
 * "Desktop". */
char *dmi_chassis_type_str(int chassis_type, gboolean with_val);

typedef struct {
    dmi_handle id;
    uint32_t size;
    dmi_type type;
    const char *type_str; /* untranslated, use _() for translation */
} dmi_handle_ext;

typedef struct {
    uint32_t count;
    dmi_handle *handles;
    dmi_handle_ext *handles_ext;
} dmi_handle_list;

/* get a list of handles that match a dmi_type */
dmi_handle_list *dmidecode_handles(const dmi_type *type);
void dmi_handle_list_free(dmi_handle_list *hl);

/* get a list of handles which have a name, and/or optional value, and/or limit to an optional dmi_type
 * dmidecode_match_value("Name", NULL, NULL) : all dmi handles with an item called "Name"
 * dmidecode_match_value("Name", "Value", NULL) : all dmi_handles with an item called "Name" that has a value of "Value"
 * dmidecode_match_value("Name", "Value", &dt) : all dmi_handles with "Name: Value" that are of type stored in dt */
dmi_handle_list *dmidecode_match_value(const char *name, const char *value, const dmi_type *type);

/* get the first value for name, limiting to optional dmi_type and/or optional handle */
char *dmidecode_match(const char *name, const dmi_type *type, const dmi_handle *handle);

void dmidecode_cache_free();

#endif
