/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 L. A. F. Pereira <l@tia.mat.br>
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

#include <pwd.h>
#include "hardinfo.h"
#include "computer.h"

gchar *users = NULL;

void
scan_users_do(void)
{
    struct passwd *passwd_;
    passwd_ = getpwent();
    if (!passwd_)
        return;

    if (users) {
        g_free(users);
        moreinfo_del_with_prefix("COMP:USER");
    }

    users = g_strdup("");

    while (passwd_) {
        gchar *key = g_strdup_printf("USER%s", passwd_->pw_name);
        gchar *val = g_strdup_printf("[%s]\n"
                "%s=%d\n"
                "%s=%d\n"
                "%s=%s\n"
                "%s=%s\n",
                _("User Information"),
                _("User ID"), (gint) passwd_->pw_uid,
                _("Group ID"), (gint) passwd_->pw_gid,
                _("Home Directory"), passwd_->pw_dir,
                _("Default Shell"), passwd_->pw_shell);
        moreinfo_add_with_prefix("COMP", key, val);

        strend(passwd_->pw_gecos, ',');
        users = h_strdup_cprintf("$%s$%s=%s\n", users, key, passwd_->pw_name, passwd_->pw_gecos);
        passwd_ = getpwent();
        g_free(key);
    }

    endpwent();
}
