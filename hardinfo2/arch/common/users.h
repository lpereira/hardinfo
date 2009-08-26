/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

static gchar *users = NULL;

static gboolean
remove_users(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "USER");
}

static void
scan_users_do(void)
{
    struct passwd *passwd_;
    passwd_ = getpwent();
    if (!passwd_)
        return;

    if (users) {
        g_free(users);
        g_hash_table_foreach_remove(moreinfo, remove_users, NULL);
    }

    users = g_strdup("");

    while (passwd_) {
        gchar *key = g_strdup_printf("USER%s", passwd_->pw_name);
        gchar *val = g_strdup_printf("[User Information]\n"
                "User ID=%d\n"
                "Group ID=%d\n"
                "Home directory=%s\n"
                "Default shell=%s\n",
                (gint) passwd_->pw_uid,
                (gint) passwd_->pw_gid,
                passwd_->pw_dir,
                passwd_->pw_shell);
        g_hash_table_insert(moreinfo, key, val);

        strend(passwd_->pw_gecos, ',');
        users = h_strdup_cprintf("$%s$%s=%s\n", users, key, passwd_->pw_name, passwd_->pw_gecos);
        passwd_ = getpwent();
    }
}
