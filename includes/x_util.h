/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 Leandro A. F. Pereira <leandro@hardinfo.org>
 *    This file
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
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

#ifndef __X_UTIL_H__
#define __X_UTIL_H__

/* wayland information (lives here in x_util for now) */
typedef struct {
    char *xdg_session_type;
    char *display_name;
} wl_info;

wl_info *get_walyand_info();
void wl_free(wl_info *);

typedef struct {
    char *glx_version;
    int direct_rendering;
    char *ogl_vendor, *ogl_renderer;

    char *ogl_core_version, *ogl_core_sl_version;
    char *ogl_version, *ogl_sl_version; /* compat */
    char *ogles_version, *ogles_sl_version;
} glx_info;

glx_info *glx_create();
gboolean fill_glx_info(glx_info *glx);
void glx_free(glx_info *glx);

typedef struct {
    int number;
    int px_width;
    int px_height;
    int min_px_width;
    int min_px_height;
    int max_px_width;
    int max_px_height;
} x_screen;

typedef struct {
    /* I guess it is kindof like gpu? */
    int reserved; /* TODO: */
} x_provider;

typedef struct {
    char name[64];
    int connected;
    int screen; /* index into xrr_info.screens[], look there for x screen number */
    int px_width;
    int px_height;
    int px_offset_x;
    int px_offset_y;
    int mm_width;
    int mm_height;
} x_output;

typedef struct {
    int reserved; /* TODO: */
} x_monitor;

typedef struct {
    char *display_name;
    int screen_count;
    x_screen *screens;
    int provider_count;
    x_provider *providers;
    int output_count;
    x_output *outputs;
    int monitor_count;
    x_monitor *monitors;
} xrr_info;

xrr_info *xrr_create();
gboolean fill_xrr_info(xrr_info *xrr);
void xrr_free(xrr_info *xrr);

typedef struct {
    int nox; /* complete failure to find X */
    char *display_name, *vendor, *version, *release_number;
    xrr_info *xrr;
    glx_info *glx;
} xinfo;

xinfo *xinfo_get_info();
void xinfo_free(xinfo *xi);

#endif
