static gchar *nfs_shares_list = NULL;
void
scan_nfs_shared_directories(void)
{
    FILE *exports;
    gchar buf[512];
    
    if (nfs_shares_list) {
        g_free(nfs_shares_list);
    }

    nfs_shares_list = g_strdup("");
    
    exports = fopen("/etc/exports", "r");
    if (!exports)
        return;
        
    while (fgets(buf, 512, exports)) {
        if (buf[0] != '/')
            continue;
        
        strend(buf, ' ');
        strend(buf, '\t');

        nfs_shares_list = g_strconcat(nfs_shares_list, buf, "=\n", NULL);
    }
    fclose(exports);
}

