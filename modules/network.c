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

#include "network.h"

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
    {N_("Interfaces"), "network-interface.png", callback_network, scan_network, MODULE_FLAG_NONE},
    {N_("IP Connections"), "network-connections.png", callback_connections, scan_connections, MODULE_FLAG_NONE},
    {N_("Routing Table"), "network.png", callback_route, scan_route, MODULE_FLAG_NONE},
    {N_("ARP Table"), "module.png", callback_arp, scan_arp, MODULE_FLAG_NONE},
    {N_("DNS Servers"), "dns.png", callback_dns, scan_dns, MODULE_FLAG_NONE},
    {N_("Statistics"), "network-statistics.png", callback_statistics, scan_statistics, MODULE_FLAG_NONE},
    {N_("Shared Directories"), "shares.png", callback_shares, scan_shares, MODULE_FLAG_NONE},
    {NULL},
};

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
    int line = 0;

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

          } else {
            gchar *tmp = buffer;

            while (*tmp && isspace(*tmp)) tmp++;

            __statistics = h_strdup_cprintf("<b> </b>#%d=%s\n",
                                            __statistics,
                                            line++, tmp);
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
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=%s\n" /* IP Address */
                           "ColumnTitle$Value=%s\n" /* Interface */
                           "ColumnTitle$Extra1=%s\n" /* MAC Address */
                           "ShowColumnHeaders=true\n",
                           _("ARP Table"), __arp_table,
                           _("IP Address"), _("Interface"), _("MAC Address") );
}

gchar *callback_shares()
{
    return g_strdup_printf("[%s]\n"
                "%s\n"
                "[%s]\n"
                "%s",
                _("SAMBA"), smb_shares_list,
                _("NFS"), nfs_shares_list);
}

gchar *callback_dns()
{
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ColumnTitle$TextValue=%s\n" /* IP Address */
                           "ColumnTitle$Value=%s\n" /* Name */
                           "ShowColumnHeaders=true\n",
                           _("Name Servers"), __nameservers,
                           _("IP Address"), _("Name") );
}

gchar *callback_connections()
{
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=%s\n" /* Local Address */
                           "ColumnTitle$Value=%s\n" /* Protocol */
                           "ColumnTitle$Extra1=%s\n" /* Foreign Address */
                           "ColumnTitle$Extra2=%s\n" /* State */
                           "ShowColumnHeaders=true\n",
                           _("Connections"), __connections,
                           _("Local Address"), _("Protocol"), _("Foreign Address"), _("State") );
}

gchar *callback_network()
{
    return g_strdup_printf("%s\n"
               "[$ShellParam$]\n"
               "ReloadInterval=3000\n"
               "ViewType=1\n"
               "ColumnTitle$TextValue=%s\n" /* Interface */
               "ColumnTitle$Value=%s\n" /* IP Address */
               "ColumnTitle$Extra1=%s\n" /* Sent */
               "ColumnTitle$Extra2=%s\n" /* Received */
               "ShowColumnHeaders=true\n"
               "%s",
               network_interfaces,
               _("Interface"), _("IP Address"), _("Sent"), _("Received"),
               network_icons);
}

gchar *callback_route()
{
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ViewType=0\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=%s\n" /* Destination / Gateway */
                           "ColumnTitle$Value=%s\n" /* Interface */
                           "ColumnTitle$Extra1=%s\n" /* Flags */
                           "ColumnTitle$Extra2=%s\n" /* Mask */
                           "ShowColumnHeaders=true\n",
                           _("IP routing table"), __routing_table,
                           _("Destination/Gateway"), _("Interface"), _("Flags"), _("Mask") );
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
    gchar *info = moreinfo_lookup_with_prefix("NET", entry);

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
    return g_strdup(_("Network"));
}

guchar hi_module_get_weight(void)
{
    return 160;
}

void hi_module_init(void)
{
}

void hi_module_deinit(void)
{
    moreinfo_del_with_prefix("NET");

    g_free(smb_shares_list);
    g_free(nfs_shares_list);
    g_free(network_interfaces);
    g_free(network_icons);

    g_free(__statistics);
    g_free(__nameservers);
    g_free(__arp_table);
    g_free(__routing_table);
    g_free(__connections);
}

ModuleAbout *hi_module_get_about(void)
{
    static ModuleAbout ma[] = {
	{
	 .author = "Leandro A. F. Pereira",
	 .description = N_("Gathers information about this computer's network connection"),
	 .version = VERSION,
	 .license = "GNU GPL version 2"}
    };

    return ma;
}
