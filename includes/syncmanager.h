/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
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

#ifndef __SYNCMANAGER_H__
#define __SYNCMANAGER_H__

#include <gtk/gtk.h>

typedef struct _SyncEntry SyncEntry;

struct _SyncEntry {
    const gchar *name;
    const gchar *file_name;

    gchar *(*generate_contents_for_upload)(gsize *size);

    gboolean selected;
    gboolean optional;
};

void sync_manager_add_entry(SyncEntry *entry);
void sync_manager_clear_entries(void);
void sync_manager_show(GtkWidget *parent);
gint sync_manager_count_entries(void);

void sync_manager_update_on_startup(int send_benchmark);

#endif /* __SYNCMANAGER_H__ */
