/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2012 L. A. F. Pereira <l@tia.mat.br>
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

#include <sys/types.h>
#include <grp.h>
#include "hardinfo.h"
#include "computer.h"

gint comparGroups (gpointer a, gpointer b) {return strcmp( (char*)a, (char*)b );}

gchar *groups = NULL;
void scan_groups_do(void)
{
    struct group *group_;
    GList *list=NULL, *a;

    setgrent();
    group_ = getgrent();
    if (!group_)
        return;

    g_free(groups);
    groups = g_strdup("");

    while (group_) {
        list=g_list_prepend(list,g_strdup_printf("%s=%d\n", group_->gr_name, group_->gr_gid));
        group_ = getgrent();
    }
    
    endgrent();

    //sort
    list=g_list_sort(list,(GCompareFunc)comparGroups);

    while (list) {
        groups = h_strdup_cprintf("%s", groups, (char *)list->data);

        //next and free
        a=list;
        list=list->next;
        free(a->data);
        g_list_free_1(a);
    }
}
