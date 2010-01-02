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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <config.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netdb.h>

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
gchar *callback_statistics();

/* Scan callbacks */
void scan_network(gboolean reload);
void scan_route(gboolean reload);
void scan_dns(gboolean reload);
void scan_connections(gboolean reload);
void scan_shares(gboolean reload);
void scan_arp(gboolean reload);
void scan_statistics(gboolean reload);

static ModuleEntry entries[] = {
    {"Interfaces", "network-interface.png", callback_network, scan_network, MODULE_FLAG_NONE},
    {"IP Connections", "network-connections.png", callback_connections, scan_connections, MODULE_FLAG_NONE},
    {"Routing Table", "network.png", callback_route, scan_route, MODULE_FLAG_NONE},
    {"ARP Table", "module.png", callback_arp, scan_arp, MODULE_FLAG_NONE},
    {"DNS Servers", "dns.png", callback_dns, scan_dns, MODULE_FLAG_NONE},
    {"Statistics", "network-statistics.png", callback_statistics, scan_statistics, MODULE_FLAG_NONE},
    {"Shared Directories", "shares.png", callback_shares, scan_shares, MODULE_FLAG_NONE},
    {NULL},
};

#include <arch/this/samba.h>
#include <arch/this/nfs.h>
#include <arch/this/net.h>

void scan_shares(gboolean reload)
{
    SCAN_START();
    scan_samba();
    scan_nfs_shared_directories();
    SCAN_END();
}

static gchar *__statistics = NULL;
void scan_statistics(gboolean reload)
{
    FILE *netstat;
    gchar buffer[256];
    gchar *netstat_path;
    
    SCAN_START();
    
    g_free(__statistics);
    __statistics = g_strdup("");
    
    if ((netstat_path = find_program("netstat"))) {
      gchar *command_line = g_strdup_printf("%s -s", netstat_path);
      
      if ((netstat = popen(command_line, "r"))) {
        while (fgets(buffer, 256, netstat)) {
          if (!isspace(buffer[0]) && strchr(buffer, ':')) {
            gchar *tmp;
            
            tmp = g_ascii_strup(strend(buffer, ':'), -1);
            
            __statistics = h_strdup_cprintf("[%s]\n",
                                            __statistics,
                                            tmp);
            
            g_free(tmp);
          } else if (isdigit(buffer[4])) {
            gchar *tmp1 = buffer + 4,
                  *tmp2 = tmp1;
            
            while (*tmp2 && !isspace(*tmp2)) tmp2++;
            *tmp2 = 0;
            tmp2++;
            
            *tmp2 = toupper(*tmp2);
            
            __statistics = h_strdup_cprintf("%s=%s\n",
                                            __statistics,
                                            g_strstrip(tmp1),
                                            g_strstrip(tmp2));
          }
        }

        pclose(netstat);
      }
      
      g_free(command_line);
      g_free(netstat_path);
    }
    
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
          gchar *ip;
          struct sockaddr_in sa;
          char hbuf[NI_MAXHOST];
          
          ip = g_strstrip(buffer + sizeof("nameserver"));
          
          sa.sin_family = AF_INET;
          sa.sin_addr.s_addr = inet_addr(ip);
          
          if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD)) {
              __nameservers = h_strdup_cprintf("%s=\n",
                                               __nameservers,
                                               ip);
          } else {
              __nameservers = h_strdup_cprintf("%s=%s\n",
                                               __nameservers,
                                               ip, hbuf);
          
          }          
          
          shell_status_pulse();
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
    gchar *route_path;
    
    SCAN_START();

    g_free(__routing_table);
    __routing_table = g_strdup("");
    
    if ((route_path = find_program("route"))) {
      gchar *command_line = g_strdup_printf("%s -n", route_path);
      
      if ((route = popen(command_line, "r"))) {
        /* eat first two lines */
        (void)fgets(buffer, 256, route);
        (void)fgets(buffer, 256, route);

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
      
      g_free(command_line);
      g_free(route_path);
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
      (void)fgets(buffer, 256, arp);

      while (fgets(buffer, 256, arp)) {
        buffer[15] = '\0';
        buffer[58] = '\0';
        
        __arp_table = h_strdup_cprintf("%s=%s|%s\n",
                                       __arp_table,
                                       g_strstrip(buffer),
                                       g_strstrip(buffer + 72),
                                       g_strstrip(buffer + 41));
      }
      
      fclose(arp);
    }
    
    SCAN_END();
}

static gchar *__connections = NULL;
void scan_connections(gboolean reload)
{
    FILE *netstat;
    gchar buffer[256];
    gchar *netstat_path;
    
    SCAN_START();

    g_free(__connections);
    __connections = g_strdup("");
    
    if ((netstat_path = find_program("netstat"))) {
      gchar *command_line = g_strdup_printf("%s -an", netstat_path);
      
      if ((netstat = popen("netstat -an", "r"))) {
        while (fgets(buffer, 256, netstat)) {
          buffer[6] = '\0';
          buffer[43] = '\0';
          buffer[67] = '\0';

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
      
      g_free(command_line);
      g_free(netstat_path);
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
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ColumnTitle$TextValue=IP Address\n"
                           "ColumnTitle$Value=Name\n"
                           "ShowColumnHeaders=true\n", __nameservers);
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
			   "ColumnTitle$TextValue=Interface\n"
			   "ColumnTitle$Value=IP Address\n"
			   "ColumnTitle$Extra1=Sent\n"
			   "ColumnTitle$Extra2=Received\n"
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

gchar *callback_statistics()
{
    return g_strdup_printf("%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n",
                            __statistics);
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
    return 160;
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
