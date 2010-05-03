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
/*
 * Wireless Extension Example
 * http://www.krugle.org/examples/p-OZYzuisV6gyQIaTu/iwconfig.c
 */
static gchar *network_interfaces = NULL, *network_icons = NULL;

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
#endif	/* HAS_LINUX_WE */

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
    int	 wi_quality_level, wi_signal_level, wi_noise_level;
    gboolean is_wireless;
#endif
};

#ifdef HAS_LINUX_WE
const gchar *wi_operation_modes[] = { "Auto", "Ad-Hoc", "Managed", "Master", "Repeater", "Secondary", "Unknown" };

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
              
              if (strstr(buf1, ".")) {
                  sscanf(buf1, "%d %d. %d %d %d %d %d %d %d %d",
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
  
  if ((r = ioctl(fd, SIOCGIWESSID, &wi_req) < 0)) {
    strcpy(netinfo->wi_essid, "");
  } else {
    netinfo->wi_essid[wi_req.u.essid.length] = '\0';
  }

  /* obtain bit rate */
  if ((r = ioctl(fd, SIOCGIWRATE, &wi_req) < 0)) {
    netinfo->wi_rate = 0;
  } else {
    netinfo->wi_rate = wi_req.u.bitrate.value;
  }
  
  /* obtain operation mode */
  if ((r = ioctl(fd, SIOCGIWMODE, &wi_req) < 0)) {
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
  if ((r = ioctl(fd, SIOCGIWTXPOW, &wi_req) < 0)) {
    netinfo->wi_has_txpower = FALSE;
  } else {
    netinfo->wi_has_txpower = TRUE;
          
    memcpy(&netinfo->wi_txpower, &wi_req.u.txpower, sizeof(struct iw_param));
  }
#else
  netinfo->wi_has_txpower = FALSE;
#endif	/* WIRELESS_EXT >= 10 */
}
#endif /* HAS_LINUX_WE */

void get_net_info(char *if_name, NetInfo * netinfo)
{
    struct ifreq ifr;
    int fd;

    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    /* IPv4 */
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(netinfo->name, if_name);

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
	sprintf(netinfo->ip, "%s",
		inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->
			  sin_addr));
    }

    /* Mask Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
	netinfo->mask[0] = 0;
    } else {
	sprintf(netinfo->mask, "%s",
		inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->
			  sin_addr));
    }

    /* Broadcast Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
	netinfo->broadcast[0] = 0;
    } else {
	sprintf(netinfo->broadcast, "%s",
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
    { "eth", "Ethernet", "network-interface" },
    { "lo", "Loopback", "network" },
    { "ppp", "Point-to-Point", "modem" },
    { "ath", "Wireless", "wireless" },
    { "wlan", "Wireless", "wireless" },
    { "ra", "Wireless", "wireless" },
    { "wl", "Wireless", "wireless" },
    { "wmaster", "Wireless", "wireless" },
    { "tun", "Virtual Point-to-Point (TUN)", "network" },
    { "tap", "Ethernet (TAP)", "network" },
    { "plip", "Parallel Line Internet Protocol", "network" },
    { "irlan", "Infrared", "network" },
    { "slip", "Serial Line Internet Protocol", "network" },
    { "isdn", "Integrated Services Digital Network", "modem" },
    { "sit", "IPv6-over-IPv4 Tunnel", "network" },
    { "vmnet8", "VMWare Virtual Network Interface (NAT)", "computer" },
    { "vmnet", "VMWare Virtual Network Interface", "computer" },
    { "pan", "Personal Area Network (PAN)", "bluetooth" },
    { "bnep", "Bluetooth", "bluetooth" },
    { "br", "Bridge Interface", "network" },
    { "ham", "Hamachi Virtual Personal Network", "network"},
    { "net", "Ethernet", "network-interface" },
    { "ifb", "Intermediate Functional Block", "network" },
    { "gre", "GRE Network Tunnel", "network" },
    { "msh", "Mesh Network", "wireless" },
    { "wmaster", "Wireless Master Interface", "wireless" },
    { NULL, "Unknown", "network" },
};

static void net_get_iface_type(gchar * name, gchar ** type, gchar ** icon, NetInfo *ni)
{
    int i;

#ifdef HAS_LINUX_WE
    if (ni->is_wireless) {
        *type = "Wireless";
        *icon = "wireless";
        
        return;
    }
#endif

    for (i = 0; netdev2type[i].type; i++) {
	if (g_str_has_prefix(name, netdev2type[i].type))
	    break;
    }

    *type = netdev2type[i].label;
    *icon = netdev2type[i].icon;
}

static gboolean
remove_net_devices(gpointer key, gpointer value, gpointer data)
{
    return g_str_has_prefix(key, "NET");
}

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
	    network_interfaces = g_strdup("[Network Interfaces]\n"
					  "None found=\n");
	}

	return;
    }

    if (network_interfaces) {
	g_free(network_interfaces);
    }

    if (network_icons) {
	g_free(network_icons);
    }

    network_interfaces = g_strdup("[Network Interfaces]\n");
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
		("$%s$%s=%s|%.2lfMiB|%.2lfMiB\n",
		 network_interfaces, devid, ifacename, ni.ip[0] ? ni.ip : "",
		 trans_mb, recv_mb);
	    net_get_iface_type(ifacename, &iface_type, &iface_icon, &ni);

	    network_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n",
					     network_icons, devid,
					     ifacename, iface_icon);

	    detailed = g_strdup_printf("[Network Adapter Properties]\n"
				       "Interface Type=%s\n"
				       "Hardware Address (MAC)=%02x:%02x:%02x:%02x:%02x:%02x\n"
				       "MTU=%d\n"
				       "[Transfer Details]\n"
				       "Bytes Received=%.0lf (%.2fMiB)\n"
				       "Bytes Sent=%.0lf (%.2fMiB)\n",
				       iface_type,
				       ni.mac[0], ni.mac[1],
				       ni.mac[2], ni.mac[3],
				       ni.mac[4], ni.mac[5],
				       ni.mtu,
				       recv_bytes, recv_mb,
				       trans_bytes, trans_mb);
            
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
                
                txpower = g_strdup_printf("%ddBm (%dmW)", dbm, mw);
              } else {
                txpower = g_strdup("Unknown");
              }
            
              detailed = h_strdup_cprintf("\n[Wireless Properties]\n"
                                          "Network Name (SSID)=%s\n"
                                          "Bit Rate=%dMb/s\n"
                                          "Transmission Power=%s\n"
                                          "Mode=%s\n"
                                          "Status=%d\n"
                                          "Link Quality=%d\n"
                                          "Signal / Noise=%d / %d\n",
                                          detailed,
                                          ni.wi_essid,
                                          ni.wi_rate / 1000000,
                                          txpower,
                                          wi_operation_modes[ni.wi_mode],
                                          ni.wi_status,
                                          ni.wi_quality_level,
                                          ni.wi_signal_level,
                                          ni.wi_noise_level);

              g_free(txpower);
            }
#endif

	    if (ni.ip[0] || ni.mask[0] || ni.broadcast[0]) {
		detailed =
		    h_strdup_cprintf("\n[Internet Protocol (IPv4)]\n"
				     "IP Address=%s\n" "Mask=%s\n"
				     "Broadcast Address=%s\n", detailed,
				     ni.ip[0] ? ni.ip : "Not set",
				     ni.mask[0] ? ni.mask : "Not set",
				     ni.broadcast[0] ? ni.
				     broadcast : "Not set");
	    }

	    g_hash_table_insert(moreinfo, devid, detailed);
	}
    }
    fclose(proc_net);
}

static void scan_net_interfaces(void)
{
    /* FIXME: See if we're running Linux 2.6 and if /sys is mounted, then use
       that instead of /proc/net/dev */

    /* remove old devices from global device table */
    g_hash_table_foreach_remove(moreinfo, remove_net_devices, NULL);

    scan_net_interfaces_24();
}
