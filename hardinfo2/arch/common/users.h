static gchar *sys_users = NULL,
             *human_users = NULL;

static gboolean
remove_users(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "USER");
}

static void
scan_users_do(void)
{
    FILE *passwd;
    char buffer[512];
    
    passwd = fopen("/etc/passwd", "r");
    if (!passwd)
      return;
    
    if (sys_users) {
      g_free(sys_users);
      g_free(human_users);

      g_hash_table_foreach_remove(moreinfo, remove_users, NULL);
    }
  
    sys_users = g_strdup("");
    human_users = g_strdup("");
    
    while (fgets(buffer, 512, passwd)) {
      gchar **tmp;
      gint uid;
      
      tmp = g_strsplit(buffer, ":", 0);
      
      gchar *key = g_strdup_printf("USER%s", tmp[0]);
      gchar *val = g_strdup_printf("[User Information]\n"
                                   "User ID=%s\n"
                                   "Group ID=%s\n"
                                   "Home directory=%s\n"
                                   "Default shell=%s\n",
                                   tmp[2], tmp[3], tmp[5], tmp[6]);
      g_hash_table_insert(moreinfo, key, val);

      uid = atoi(tmp[2]);
      strend(tmp[4], ',');
      if (uid >= 1000 && uid <= 65530) {
        human_users = h_strdup_cprintf("$%s$%s=%s\n", human_users, key, tmp[0], tmp[4]);
      } else {
        sys_users = h_strdup_cprintf("$%s$%s=%s\n", sys_users, key, tmp[0], tmp[4]);
      }      
      
      g_strfreev(tmp);
    }
    
    fclose(passwd);
}
