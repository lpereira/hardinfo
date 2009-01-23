/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <config.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#include <hardinfo.h>
#include <iconcache.h>
#include <shell.h>

#include <vendor.h>

static GHashTable *moreinfo = NULL;

/* Callbacks */
gchar *callback_network();
gchar *callback_route();
gchar *callback_dns();
gchar *callback_connections();
gchar *callback_shares();
gchar *callback_arp();

/* Scan callbacks */
void scan_network(gboolean reload);
void scan_route(gboolean reload);
void scan_dns(gboolean reload);
void scan_connections(gboolean reload);
void scan_shares(gboolean reload);
void scan_arp(gboolean reload);

static ModuleEntry entries[] = {
    {"Interfaces", "network.png", callback_network, scan_network},
    {"IP Connections", "module.png", callback_connections, scan_connections},
    {"Routing Table", "network-generic.png", callback_route, scan_route},
    {"ARP Table", "module.png", callback_arp, scan_arp},
    {"DNS Servers", "module.png", callback_dns, scan_dns},
    {"Shared Directories", "shares.png", callback_shares, scan_shares},
    {NULL},
};

#include <arch/this/samba.h>
#include <arch/this/nfs.h>
#include <arch/this/net.h>

void scan_shares(gboolean reload)
{
    SCAN_START();
    scan_samba_shared_directories();
    scan_nfs_shared_directories();
    SCAN_END();
}

static gchar *__nameservers = NULL;
void scan_dns(gboolean reload)
{
    FILE *resolv;
    gchar buffer[256];
    
    SCAN_START();
    
    g_free(__nameservers);
    __nameservers = g_strdup("");
    
    if ((resolv = fopen("/etc/resolv.conf", "r"))) {
      while (fgets(buffer, 256, resolv)) {
        if (g_str_has_prefix(buffer, "nameserver")) {
          __nameservers = h_strdup_cprintf("%s=\n",
                                           __nameservers,
                                           g_strstrip(buffer + sizeof("nameserver")));
        } 
      }
      fclose(resolv);
    }
    
    SCAN_END();
}

void scan_network(gboolean reload)
{
    SCAN_START();
    scan_net_interfaces();
    SCAN_END();
}

static gchar *__routing_table = NULL;
void scan_route(gboolean reload)
{
    FILE *route;
    gchar buffer[256];
    
    SCAN_START();

    g_free(__routing_table);
    __routing_table = g_strdup("");
    
    if ((route = popen("route -n", "r"))) {
      /* eat first two lines */
      fgets(buffer, 256, route);
      fgets(buffer, 256, route);

      while (fgets(buffer, 256, route)) {
        buffer[15] = '\0';
        buffer[31] = '\0';
        buffer[47] = '\0';
        buffer[53] = '\0';
        
        __routing_table = h_strdup_cprintf("%s / %s=%s|%s|%s\n",
                                           __routing_table,
                                           g_strstrip(buffer), g_strstrip(buffer + 16),
                                           g_strstrip(buffer + 72),
                                           g_strstrip(buffer + 48),
                                           g_strstrip(buffer + 32));
      }
      
      pclose(route);
    }
    
    SCAN_END();
}

static gchar *__arp_table = NULL;
void scan_arp(gboolean reload)
{
    FILE *arp;
    gchar buffer[256];
    
    SCAN_START();

    g_free(__arp_table);
    __arp_table = g_strdup("");
    
    if ((arp = fopen("/proc/net/arp", "r"))) {
      /* eat first line */
      fgets(buffer, 256, arp);

      while (fgets(buffer, 256, arp)) {
        buffer[15] = '\0';
        buffer[58] = '\0';
        
        __arp_table = h_strdup_cprintf("%s=%s|%s\n",
                                       __arp_table,
                                       g_strstrip(buffer),
                                       g_strstrip(buffer + 72),
                                       g_strstrip(buffer + 41));
      }
      
      pclose(arp);
    }
    
    SCAN_END();
}

static gchar *__connections = NULL;
void scan_connections(gboolean reload)
{
    FILE *netstat;
    gchar buffer[256];
    
    SCAN_START();

    g_free(__connections);
    __connections = g_strdup("");
    
    if ((netstat = popen("netstat -an", "r"))) {
      while (fgets(buffer, 256, netstat)) {
        buffer[6] = 0;
        buffer[43] = 0;
        buffer[67] = 0;

        if (g_str_has_prefix(buffer, "tcp") || g_str_has_prefix(buffer, "udp")) {
          __connections = h_strdup_cprintf("%s=%s|%s|%s\n",
                                           __connections,
                                           g_strstrip(buffer + 20),	/* local address */
                                           g_strstrip(buffer),		/* protocol */
                                           g_strstrip(buffer + 44),	/* foreign address */
                                           g_strstrip(buffer + 68));	/* state */
        }
      }
      
      pclose(netstat);
    }
    
    SCAN_END();
}

gchar *callback_arp()
{
    return g_strdup_printf("[ARP Table]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=IP Address\n"
                           "ColumnTitle$Value=Interface\n"
                           "ColumnTitle$Extra1=MAC Address\n"
                           "ShowColumnHeaders=true\n",
                           __arp_table);
}

gchar *callback_shares()
{
    return g_strdup_printf("[SAMBA]\n"
			   "%s\n"
			   "[NFS]\n"
			   "%s", smb_shares_list, nfs_shares_list);
}

gchar *callback_dns()
{
    return g_strdup_printf("[Name servers]\n"
                           "%s\n", __nameservers);
}

gchar *callback_connections()
{
    return g_strdup_printf("[Connections]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=Local Address\n"
                           "ColumnTitle$Value=Protocol\n"
                           "ColumnTitle$Extra1=Foreign Address\n"
                           "ColumnTitle$Extra2=State\n"
                           "ShowColumnHeaders=true\n",
                           __connections);
}

gchar *callback_network()
{
    return g_strdup_printf("%s\n"
                           "[$ShellParam$]\n"
			   "ReloadInterval=3000\n"
			   "ViewType=1\n"
			   "ColumnTitle$TextValue=Device\n"
			   "ColumnTitle$Value=IP Address\n"
			   "ColumnTitle$Extra1=Statistics\n"
			   "ShowColumnHeaders=true\n"
			   "%s",
			   network_interfaces,
			   network_icons);
}

gchar *callback_route()
{
    return g_strdup_printf("[IP routing table]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ViewType=0\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=Destination / Gateway\n"
                           "ColumnTitle$Value=Interface\n"
                           "ColumnTitle$Extra1=Flags\n"
                           "ColumnTitle$Extra2=Mask\n"
                           "ShowColumnHeaders=true\n",
                           __routing_table);
}

gchar *hi_more_info(gchar * entry)
{
    gchar *info = (gchar *) g_hash_table_lookup(moreinfo, entry);

    if (info)
	return g_strdup(info);

    return g_strdup_printf("[%s]", entry);
}

ModuleEntry *hi_module_get_entries(void)
{
    return entries;
}

gchar *hi_module_get_name(void)
{
    return g_strdup("Network");
}

guchar hi_module_get_weight(void)
{
    return 85;
}

void hi_module_init(void)
{
    moreinfo =
	g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

ModuleAbout *hi_module_get_about(void)
{
    static ModuleAbout ma[] = {
	{
	 .author = "Leandro A. F. Pereira",
	 .description = "Gathers information about this computer's network connection",
	 .version = VERSION,
	 .license = "GNU GPL version 2"}
    };

    return ma;
}
