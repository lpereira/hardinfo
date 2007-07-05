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

static gchar *network_interfaces = NULL,
             *network_icons = NULL;

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <sys/socket.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef struct _NetInfo NetInfo;
struct _NetInfo {
    char name[16]; 
    int  mtu;
    unsigned char mac[8];
    char ip[16];
    char mask[16];
    char broadcast[16];
};

void get_net_info(char *if_name, NetInfo *netinfo)
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
        sprintf(netinfo->ip, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)); 
    }
    
    /* Mask Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
        netinfo->mask[0] = 0;
    } else {
        sprintf(netinfo->mask, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }

    /* Broadcast Address */
    strcpy(ifr.ifr_name, if_name);
    if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
        netinfo->broadcast[0] = 0;
    } else {
        sprintf(netinfo->broadcast, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }

    shutdown(fd, 0);
}

static struct {
    char *type;
    char *label;
    char *icon;
} netdev2type[] = {
    { "eth",	"Ethernet",				  "network" },
    { "lo",	"Loopback",				  "network-generic" },
    { "ppp",	"Point-to-Point",			  "modem" },
    { "ath",	"Wireless",				  "wireless" },
    { "wlan",	"Wireless",				  "wireless" },
    { "tun",    "Virtual Point-to-Point (TUN)",		  "network-generic" },
    { "tap",    "Ethernet (TAP)",			  "network-generic" },
    { "plip",   "Parallel Line Internet Protocol",	  "network" },
    { "irlan",  "Infrared",				  "network-generic" },
    { "slip",   "Serial Line Internet Protocol",	  "network-generic" },
    { "isdn",	"Integrated Services Digital Network",	  "modem" },
    { "sit",	"IPv6-over-IPv4 Tunnel",		  "network-generic" },
    { "vmnet8", "VMWare Virtual Network Interface (NAT)", "computer" },
    { "vmnet",  "VMWare Virtual Network Interface",	  "computer"},
    { NULL,	"Unknown",				  "network-generic" },
};

static void
net_get_iface_type(gchar *name, gchar **type, gchar **icon)
{
    int i;
    
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

static void
scan_net_interfaces_24(void)
{
    FILE *proc_net;
    NetInfo ni;
    gchar buffer[256];
    gchar *devid, *detailed;
    gulong recv_bytes;
    gulong recv_errors;
    gulong recv_packets;
    
    gulong trans_bytes;
    gulong trans_errors;
    gulong trans_packets;
    
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
    while (fgets(buffer, 256, proc_net)) {
	if (strchr(buffer, ':')) {
	    gint trash;
	    gchar ifacename[16];
	    gchar *buf = buffer;
	    gchar *iface_type, *iface_icon, *ip;
	    gint i;

	    buf = g_strstrip(buf);

	    memset(ifacename, 0, 16);

	    for (i = 0; buffer[i] != ':' && i < 16; i++) {
		ifacename[i] = buffer[i];
	    }

	    buf = strchr(buf, ':') + 1;

	    /* iface: bytes packets errs drop fifo frame compressed multicast */
	    sscanf(buf, "%ld %ld %ld %d %d %d %d %d %ld %ld %ld",
		   &recv_bytes, &recv_packets,
		   &recv_errors, &trash, &trash, &trash, &trash,
		   &trash, &trans_bytes, &trans_packets,
		   &trans_errors);

            gfloat recv_mb = recv_bytes / 1048576.0;
            gfloat trans_mb = trans_bytes / 1048576.0;
            
            get_net_info(ifacename, &ni);

            devid = g_strdup_printf("NET%s", ifacename);
            
            ip = g_strdup_printf(" (%s)", ni.ip);
	    network_interfaces = h_strdup_cprintf("$%s$%s=Sent %.2fMiB, received %.2fMiB%s\n",
                                                  network_interfaces,
                                                  devid,
                                                  ifacename,
                                                  trans_mb,
                                                  recv_mb,
						  ni.ip[0] ? ip: "");
            g_free(ip);
            
            net_get_iface_type(ifacename, &iface_type, &iface_icon);
	    network_icons = h_strdup_cprintf("Icon$%s$%s=%s.png\n",
	                                    network_icons, devid,
	                                    ifacename, iface_icon);
            
            detailed = g_strdup_printf("[Network Adapter Properties]\n"
                                        "Interface Type=%s\n"
                                        "Hardware Address (MAC)=%02x:%02x:%02x:%02x:%02x:%02x\n"
                                        "MTU=%d\n"
                                        "[Transfer Details]\n"
                                        "Bytes Received=%ld (%.2fMiB)\n"
                                        "Bytes Sent=%ld (%.2fMiB)\n",
                                        iface_type,
                                        ni.mac[0], ni.mac[1],
                                        ni.mac[2], ni.mac[3],
                                        ni.mac[4], ni.mac[5],
                                        ni.mtu,
                                        recv_bytes, recv_mb,
                                        trans_bytes, trans_mb);
                                        
            if (ni.ip[0] || ni.mask[0] || ni.broadcast[0]) {
                 detailed = h_strdup_cprintf("\n[Internet Protocol (IPv4)]\n"
                                            "IP Address=%s\n"
                                            "Mask=%s\n"
                                            "Broadcast Address=%s\n",
                                            detailed,
                                            ni.ip[0]        ? ni.ip        : "Not set",
                                            ni.mask[0]      ? ni.mask      : "Not set",
                                            ni.broadcast[0] ? ni.broadcast : "Not set");
            }
              
            g_hash_table_insert(moreinfo, devid, detailed);
	}
    }
    fclose(proc_net);
}

static void
scan_net_interfaces(void)
{
    /* FIXME: See if we're running Linux 2.6 and if /sys is mounted, then use
              that instead of /proc/net/dev */

    /* remove old devices from global device table */
    g_hash_table_foreach_remove(moreinfo, remove_net_devices, NULL);

    scan_net_interfaces_24();
}
