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

#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <gtk/gtk.h>

void cb_about();
void cb_about_module(GtkAction *action);
void cb_generate_report();
void cb_save_graphic();
void cb_quit();
void cb_refresh();
void cb_copy_to_clipboard();
void cb_side_pane();
void cb_toolbar();
void cb_open_web_page();
void cb_open_online_docs();
void cb_sync_manager();
void cb_report_bug();
void cb_donate();
void cb_connect_to();
void cb_manage_hosts();

#endif	/* __CALLBACKS_H__ */
