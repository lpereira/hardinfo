/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#ifndef __VENDOR_H__
#define __VENDOR_H__

typedef struct _Vendor	Vendor;
struct _Vendor {
  char *match_string;
  int match_case; /* 0 = ignore case, 1 = match case */
  char *name;
  char *name_short;
  char *url;
  char *url_support;
};

void vendor_init(void);
void vendor_cleanup(void);
const Vendor *vendor_match(const gchar *id_str, ...); /* end list of strings with NULL */
const gchar *vendor_get_name(const gchar *id_str);
const gchar *vendor_get_shortest_name(const gchar *id_str);
const gchar *vendor_get_url(const gchar *id_str);
gchar *vendor_get_link(const gchar *id_str);
gchar *vendor_get_link_from_vendor(const Vendor *v);
void vendor_free(Vendor *v);

#endif	/* __VENDOR_H__ */
