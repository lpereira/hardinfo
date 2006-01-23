/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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

static gchar *shares_list = NULL;
void
scan_shared_directories(void)
{
    GKeyFile *keyfile;
    GError *error = NULL;
    gchar **groups;
    gchar *smbconf;
    gsize length;

    if (shares_list) {
        g_free(shares_list);
    }
    
    keyfile = g_key_file_new();
    
    if (!g_file_get_contents("/etc/samba/smb.conf", &smbconf, &length, &error)) {
        shares_list = g_strdup("Cannot open /etc/samba/smb.conf=\n");
        g_error_free(error);
        goto cleanup;
    }
    
    gchar *_smbconf = smbconf;
    for (; *_smbconf; _smbconf++)
        if (*_smbconf == ';') *_smbconf = '\0';
    
    if (!g_key_file_load_from_data(keyfile, smbconf, length, 0, &error)) {
        shares_list = g_strdup("Cannot parse smb.conf=\n");
        g_error_free(error);
        goto cleanup;
    }

    shares_list = g_strdup("");

    groups = g_key_file_get_groups(keyfile, NULL);
    gchar **_groups = groups;
    while (*groups) {
        if (g_key_file_has_key(keyfile, *groups, "path", NULL) &&
            g_key_file_has_key(keyfile, *groups, "available", NULL)) {
            
            gchar *available = g_key_file_get_string(keyfile, *groups, "available", NULL);
        
            if (g_str_equal(available, "yes")) {
                gchar *path = g_key_file_get_string(keyfile, *groups, "path", NULL);
                shares_list = g_strconcat(shares_list, *groups, "=",
                                          path, "\n", NULL);
                g_free(path);
            }
            
            g_free(available);
        }
        
        *groups++;
    }
    
    g_strfreev(_groups);
  
  cleanup:
    g_key_file_free(keyfile);
    g_free(smbconf);
}

