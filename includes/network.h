#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "hardinfo.h"

extern gchar *smb_shares_list;
extern gchar *nfs_shares_list;
extern gchar *network_interfaces;
extern gchar *network_icons;

void scan_net_interfaces(void);

#endif /* __NETWORK_H__ */