/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2019 L. A. F. Pereira <l@tia.mat.br>
 *    This file Burt P. <pburt0@gmail.com>
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

#ifndef __DISTRO_FLAVORS_H__
#define __DISTRO_FLAVORS_H__

typedef struct {
    const char *name;
    const char *icon;
    const char *url;
} DistroFlavor;

typedef struct UbuntuFlavor {
    const DistroFlavor base;
    const char *package;
} UbuntuFlavor;

/* items are const; free with g_slist_free() */
GSList *ubuntu_flavors_scan(void);

#endif
