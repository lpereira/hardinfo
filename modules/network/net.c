/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 L. A. F. Pereira <l@tia.mat.br>
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
/*
 * Wireless Extension Example
 * http://www.krugle.org/examples/p-OZYzuisV6gyQIaTu/iwconfig.c
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <linux/sockios.h>

#include <arpa/inet.h>

#ifdef HAS_LINUX_WE
#include <linux/if.h>
#include <linux/wireless.h>
#else
#include <net/if.h>
#endif  /* HAS_LINUX_WE */

#include "hardinfo.h"
#include "network.h"

gchar *network_interfaces = NULL, *network_icons = NULL;

typedef struct _NetInfo NetInfo;
struct _NetInfo {
    char name[16];
    int mtu;
    unsigned char mac[8];
    char ip[16];
    char mask[16];
    char broadcast[16];

#ifdef HAS_LINUX_WE
    char wi_essid[IW_ESSID_MAX_SIZE + 1];
    int  wi_rate;
    int  wi_mode, wi_status;
    gboolean wi_has_txpower;
    struct iw_param wi_txpower;
    int  wi_quality_level, wi_signal_level, wi_noise_level;
    gboolean is_wireless;
#endif
};

#ifdef HAS_LINUX_WE
const gchar *wi_operation_modes[] = {
    NC_("wi-op-mode", "Auto"),
    NC_("wi-op-mode", "Ad-Hoc"),
    NC_("wi-op-mode", "Managed"),
    NC_("wi-op-mode", "Master"),
    NC_("wi-op-mode", "Repeater"),
    NC_("wi-op-mode", "Secondary"),
    NC_("wi-op-mode", "(Unknown)")
};

void get_wireless_info(int fd, NetInfo *netinfo)
{
  FILE *wrls;
  char wbuf[256];
  struct iwreq wi_req;
  int r, trash;

  netinfo->is_wireless = FALSE;

  if ((wrls = fopen("/proc/net/wireless", "r"))) {
      while (fgets(wbuf, 256, wrls)) {
          if (strchr(wbuf, ':') && strstr(wbuf, netinfo->name)) {
              gchar *buf1 = wbuf;

              netinfo->is_wireless = TRUE;

              buf1 = strchr(buf1, ':') + 1;

              if (strchr(buf1, '.')) {
                  sscanf(buf1, "%d %d. %d. %d %d %d %d %d %d %d",
                         &(netinfo->wi_status),
                         &(netinfo->wi_quality_level),
                         &(netinfo->wi_signal_level),
                         &(netinfo->wi_noise_level),
                         &trash, &trash, &trash, &trash, &trash, &trash);
              } else {
                  sscanf(buf1, "%d %d %d %d %d %d %d %d %d %d",
                         &(netinfo->wi_status),
                         &(netinfo->wi_quality_level),
                         &(netinfo->wi_signal_level),
                         &(netinfo->wi_noise_level),
                         &trash, &trash, &trash, &trash, &trash,
                         &trash);
              }

              break;
          }
      }
      fclose(wrls);
  }

  if (!netinfo->is_wireless)
    return;

  strncpy(wi_req.ifr_name, netinfo->name, 16);

  /* obtain essid */
  wi_req.u.essid.pointer = netinfo->wi_essid;
  wi_req.u.essid.length  = IW_ESSID_MAX_SIZE + 1;
  wi_req.u.essid.flags   = 0;

  if (ioctl(fd, SIOCGIWESSID, &wi_req) < 0) {
    strcpy(netinfo->wi_essid, "");
  } else {
    netinfo->wi_essid[wi_req.u.essid.length] = '\0';
  }

  /* obtain bit rate */
  if (ioctl(fd, SIOCGIWRATE, &wi_req) < 0) {
    netinfo->wi_rate = 0;
  } else {
    netinfo->wi_rate = wi_req.u.bitrate.value;
  }

  /* obtain operation mode */
  if (ioctl(fd, SIOCGIWMODE, &wi_req) < 0) {
    netinfo->wi_mode = 0;
  } else {
    if (wi_req.u.mode >= 0 && wi_req.u.mode < 6) {
      netinfo->wi_mode = wi_req.u.mode;
    } else {
      netinfo->wi_mode = 6;
    }
  }

#if WIRELESS_EXT >= 10
  /* obtain txpower */
  if (ioctl(fd, SIOCGIWTXPOW, &wi_req) < 0) {
    netinfo->wi_has_txpower = FALSE;
  } else {
    netinfo->wi_has_txpower = TRUE;

    memcpy(&netinfo->wi_txpower, &wi_req.u.txpower, sizeof(struct iw_param));
  }
#else
  netinfo->wi_has_txpower = FALSE;
#endif  /* WIRELESS_EXT >= 10 */
}
#endif /* HAS_LINUX_WE */

void get_net_info(char *if_name, NetInfo * netinfo)
{
    struct ifreq ifr;
    int fd;

    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    /* IPv4 */
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(netinfo->name, if_name, sizeof(netinfo->name));

    /* MTU */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
    netinfo->mtu = 0;
    } else {
    netinfo->mtu = ifr.ifr_mtu;
    }

    /* HW Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
    memset(netinfo->mac, 0, 8);
    } else {
    memcpy(netinfo->mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 8);
    }

    /* IP Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
    netinfo->ip[0] = 0;
    } else {
    snprintf(netinfo->ip, sizeof(netinfo->ip), "%s",
        inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->
              sin_addr));
    }

    /* Mask Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
    netinfo->mask[0] = 0;
    } else {
    snprintf(netinfo->mask, sizeof(netinfo->mask), "%s",
        inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->
              sin_addr));
    }

    /* Broadcast Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
    netinfo->broadcast[0] = 0;
    } else {
    snprintf(netinfo->broadcast, sizeof(netinfo->broadcast), "%s",
        inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->
              sin_addr));
    }

#ifdef HAS_LINUX_WE
    get_wireless_info(fd, netinfo);
#endif

    shutdown(fd, 0);
    close(fd);
}

static struct {
    char *type;
    char *label;
    char *icon;
} netdev2type[] = {
    /* Classic */
    { "eth", NC_("net-if-type", "Ethernet"), "network-interface" },
    { "lo", NC_("net-if-type", "Loopback"), "network" },
    { "ppp", NC_("net-if-type", "Point-to-Point"), "modem" },
    { "ath", NC_("net-if-type", "Wireless"), "wireless" },
    { "wlan", NC_("net-if-type", "Wireless"), "wireless" },
    { "ra", NC_("net-if-type", "Wireless"), "wireless" },
    { "wmaster", NC_("net-if-type", "Wireless"), "wireless" },
    { "tun", NC_("net-if-type", "Virtual Point-to-Point (TUN)"), "network" },
    { "tap", NC_("net-if-type", "Ethernet (TAP)"), "network" },
    { "plip", NC_("net-if-type", "Parallel Line Internet Protocol"), "network" },
    { "irlan", NC_("net-if-type", "Infrared"), "network" },
    { "slip", NC_("net-if-type", "Serial Line Internet Protocol"), "network" },
    { "isdn", NC_("net-if-type", "Integrated Services Digital Network"), "modem" },
    { "sit", NC_("net-if-type", "IPv6-over-IPv4 Tunnel"), "network" },
    { "vmnet8", NC_("net-if-type", "VMWare Virtual Network Interface (NAT)"), "computer" },
    { "vmnet", NC_("net-if-type", "VMWare Virtual Network Interface"), "computer" },
    { "pan", NC_("net-if-type", "Personal Area Network (PAN)"), "bluetooth" },
    { "bnep", NC_("net-if-type", "Bluetooth"), "bluetooth" },
    { "br", NC_("net-if-type", "Bridge Interface"), "network" },
    { "ham", NC_("net-if-type", "Hamachi Virtual Personal Network"), "network"},
    { "net", NC_("net-if-type", "Ethernet"), "network-interface" },
    { "ifb", NC_("net-if-type", "Intermediate Functional Block"), "network" },
    { "gre", NC_("net-if-type", "GRE Network Tunnel"), "network" },
    { "msh", NC_("net-if-type", "Mesh Network"), "wireless" },
    { "wmaster", NC_("net-if-type", "Wireless Master Interface"), "wireless" },
    { "vboxnet", NC_("net-if-type", "VirtualBox Virtual Network Interface"), "network" },

    /* Predictable network interface device names (systemd) */
    { "en", NC_("net-if-type", "Ethernet"), "network-interface" },
    { "sl", NC_("net-if-type", "Serial Line Internet Protocol"), "network" },
    { "wl", NC_("net-if-type", "Wireless"), "wireless" },
    { "ww", NC_("net-if-type", "Wireless (WAN)"), "wireless" },

    { NULL, NC_("net-if-type", "(Unknown)"), "network" },
};

static void net_get_iface_type(gchar * name, gchar ** type, gchar ** icon, NetInfo *ni)
{
    int i;

#ifdef HAS_LINUX_WE
    if (ni->is_wireless) {
        *type = "Wireless"; /* translated when used */
        *icon = "wireless";

        return;
    }
#endif

    for (i = 0; netdev2type[i].type; i++) {
    if (g_str_has_prefix(name, netdev2type[i].type))
        break;
    }

    *type = netdev2type[i].label; /* translated when used */
    *icon = netdev2type[i].icon;
}

static gboolean
remove_net_devices(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "NET");
}

#ifdef HAS_LINUX_WE
const char *wifi_bars(int signal, int noise)
{
    signal = -signal;

    if (signal > 80)
        return "▰▰▰▰▰";
    if (signal > 55)
        return "▰▰▰▰▱";
    if (signal > 30)
        return "▰▰▰▱▱";
    if (signal > 15)
        return "▰▰▱▱▱";
    if (signal > 5)
        return "▰▱▱▱▱";
    return "▱▱▱▱▱";
}
#endif

static void scan_net_interfaces_24(void)
{
    FILE *proc_net;
    NetInfo ni;
    gchar buffer[256];
    gchar *devid, *detailed;
    gdouble recv_bytes;
    gdouble recv_errors;
    gdouble recv_packets;

    gdouble trans_bytes;
    gdouble trans_errors;
    gdouble trans_packets;

    if (!g_file_test("/proc/net/dev", G_FILE_TEST_EXISTS)) {
    if (network_interfaces) {
        g_free(network_interfaces);
        network_interfaces = g_strdup_printf("[%s]]\n%s=\n",
            _("Network Interfaces"), _("None Found") );
    }

    return;
    }

    g_free(network_interfaces);

    g_free(network_icons);

    network_interfaces = g_strdup_printf("[%s]\n", _("Network Interfaces"));
    network_icons = g_strdup("");

    proc_net = fopen("/proc/net/dev", "r");
    if (!proc_net)
        return;

    while (fgets(buffer, 256, proc_net)) {
    if (strchr(buffer, ':')) {
        gint trash;
        gchar ifacename[16];
        gchar *buf = buffer;
        gchar *iface_type, *iface_icon;
        gint i;

        buf = g_strstrip(buf);

        memset(ifacename, 0, 16);

        for (i = 0; buffer[i] != ':' && i < 16; i++) {
        ifacename[i] = buffer[i];
        }

        buf = strchr(buf, ':') + 1;

        /* iface: bytes packets errs drop fifo frame compressed multicast */
        sscanf(buf, "%lf %lf %lf %d %d %d %d %d %lf %lf %lf",
           &recv_bytes, &recv_packets,
           &recv_errors, &trash, &trash, &trash, &trash,
           &trash, &trans_bytes, &trans_packets, &trans_errors);

        gdouble recv_mb = recv_bytes / 1048576.0;
        gdouble trans_mb = trans_bytes / 1048576.0;

        get_net_info(ifacename, &ni);

        devid = g_strdup_printf("NET%s", ifacename);

        network_interfaces =
        h_strdup_cprintf
        ("$%s$%s=%s|%.2lf%s|%.2lf%s\n",
         network_interfaces, devid, ifacename, ni.ip[0] ? ni.ip : "",
         trans_mb, _("MiB"), recv_mb, _("MiB"));
        net_get_iface_type(ifacename, &iface_type, &iface_icon, &ni);

        network_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n",
                         network_icons, devid,
                         ifacename, iface_icon);

        detailed = g_strdup_printf("[%s]\n"
                       "%s=%s\n" /* Interface Type */
                       "%s=%02x:%02x:%02x:%02x:%02x:%02x\n" /* MAC */
                       "%s=%d\n" /* MTU */
                       "[%s]\n" /*Transfer Details*/
                       "%s=%.0lf (%.2f%s)\n" /* Bytes Received */
                       "%s=%.0lf (%.2f%s)\n" /* Bytes Sent */,
                       _("Network Adapter Properties"),
                       _("Interface Type"), C_("net-if-type", iface_type),
                       _("Hardware Address (MAC)"),
                       ni.mac[0], ni.mac[1],
                       ni.mac[2], ni.mac[3],
                       ni.mac[4], ni.mac[5],
                       _("MTU"), ni.mtu,
                       _("Transfer Details"),
                       _("Bytes Received"), recv_bytes, recv_mb, _("MiB"),
                       _("Bytes Sent"), trans_bytes, trans_mb, _("MiB"));

#ifdef HAS_LINUX_WE
        if (ni.is_wireless) {
            gchar *txpower;

            if (ni.wi_has_txpower) {
                gint mw, dbm;

                if (ni.wi_txpower.flags & IW_TXPOW_MWATT) {
                    mw = ni.wi_txpower.value;
                    dbm = (int) ceil(10.0 * log10((double) ni.wi_txpower.value));
                } else {
                    dbm = ni.wi_txpower.value;
                    mw = (int) floor(pow(10.0, ((double) dbm / 10.0)));
                }

                txpower = g_strdup_printf("%d%s (%d%s)", dbm, _("dBm"), mw, _("mW"));
            } else {
                txpower = g_strdup(_("(Unknown)"));
            }

            detailed = h_strdup_cprintf("\n[%s]\n"
                "%s=%s\n" /* Network Name (SSID) */
                "%s=%d%s\n" /* Bit Rate */
                "%s=%s\n" /* Transmission Power */
                "%s=%s\n" /* Mode */
                "%s=%d\n" /* Status */
                "%s=%d\n" /* Link Quality */
                "%s=%d %s / %d %s (%s)\n",
                detailed,
                _("Wireless Properties"),
                _("Network Name (SSID)"), ni.wi_essid,
                _("Bit Rate"), ni.wi_rate / 1000000, _("Mb/s"),
                _("Transmission Power"), txpower,
                _("Mode"), C_("wi-op-mode", wi_operation_modes[ni.wi_mode]),
                _("Status"), ni.wi_status,
                _("Link Quality"), ni.wi_quality_level,
                _("Signal / Noise"),
                    ni.wi_signal_level, _("dBm"),
                    ni.wi_noise_level, _("dBm"),
                wifi_bars(ni.wi_signal_level, ni.wi_noise_level));

            g_free(txpower);
        }
#endif

        if (ni.ip[0] || ni.mask[0] || ni.broadcast[0]) {
        detailed =
            h_strdup_cprintf("\n[%s]\n"
                     "%s=%s\n"
                     "%s=%s\n"
                     "%s=%s\n", detailed,
                     _("Internet Protocol (IPv4)"),
                     _("IP Address"), ni.ip[0] ? ni.ip : _("(Not set)"),
                     _("Mask"), ni.mask[0] ? ni.mask : _("(Not set)"),
                     _("Broadcast Address"),
                        ni.broadcast[0] ? ni.broadcast : _("(Not set)") );
        }

        moreinfo_add_with_prefix("NET", devid, detailed);
        g_free(devid);
    }
    }
    fclose(proc_net);
}

void scan_net_interfaces(void)
{
    /* FIXME: See if we're running Linux 2.6 and if /sys is mounted, then use
       that instead of /proc/net/dev */

    /* remove old devices from global device table */
    moreinfo_del_with_prefix("NET");

    scan_net_interfaces_24();
}
