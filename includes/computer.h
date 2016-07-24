/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
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
#ifndef __COMPUTER_H__
#define __COMPUTER_H__

#include "hardinfo.h"

#define DB_PREFIX "/etc/"

static struct {
    gchar *file, *codename;
} distro_db[] = {
    { DB_PREFIX "fatdog-version",	"fatdog"  },
    { DB_PREFIX "debian_version",	"deb"  },
    { DB_PREFIX "slackware-version",	"slk"  },
    { DB_PREFIX "mandrake-release",	"mdk"  },
    { DB_PREFIX "mandriva-release",     "mdv"  },
    { DB_PREFIX "fedora-release",       "fdra" },
    { DB_PREFIX "coas",                 "coas" },
    { DB_PREFIX "environment.corel",    "corel"},
    { DB_PREFIX "gentoo-release",	"gnt"  },
    { DB_PREFIX "conectiva-release",	"cnc"  },
    { DB_PREFIX "versão-conectiva",	"cnc"  },
    { DB_PREFIX "turbolinux-release",	"tl"   },
    { DB_PREFIX "yellowdog-release",	"yd"   },
    { DB_PREFIX "sabayon-release",      "sbn"  },
    { DB_PREFIX "arch-release",         "arch" },
    { DB_PREFIX "enlisy-release",       "enlsy"},
    { DB_PREFIX "SuSE-release",		"suse" },
    { DB_PREFIX "sun-release",		"sun"  },
    { DB_PREFIX "zenwalk-version",	"zen"  },
    { DB_PREFIX "DISTRO_SPECS",		"ppy"  },
    { DB_PREFIX "puppyversion",		"ppy"  },
    { DB_PREFIX "distro-release",	"fl"   },
    { DB_PREFIX "vine-release",         "vine" },
    { DB_PREFIX "PartedMagic-version",	"pmag" },
     /*
     * RedHat must be the *last* one to be checked, since
     * some distros (like Mandrake) includes a redhat-relase
     * file too.
     */
    { DB_PREFIX "redhat-release",	"rh"   },
    { NULL,				NULL   }
};

typedef struct _Computer	Computer;
typedef struct _OperatingSystem	OperatingSystem;
typedef struct _MemoryInfo	MemoryInfo;
typedef struct _UptimeInfo	UptimeInfo;
typedef struct _LoadInfo	LoadInfo;
typedef struct _DisplayInfo	DisplayInfo;

typedef struct _AlsaInfo	AlsaInfo;
typedef struct _AlsaCard	AlsaCard;

typedef struct _FileSystem	FileSystem;
typedef struct _FileSystemEntry	FileSystemEntry;

struct _AlsaCard {
    gchar *alsa_name;
    gchar *friendly_name;
/*  
  gchar   *board;
  gchar    revision, compat_class;
  gint     subsys_vendorid, subsys_id;
  
  gint     cap_dac_res, cap_adc_res;
  gboolean cap_3d_enh;
  
  gint     curr_mic_gain;
  gboolean curr_3d_enh,
           curr_loudness,
           curr_simstereo;
  gchar   *curr_mic_select;
*/
};

struct _AlsaInfo {
    GSList *cards;
};

struct _DisplayInfo {
    gchar *ogl_vendor, *ogl_renderer, *ogl_version;
    gboolean dri;
    
    gchar *display_name, *vendor, *version;
    gchar *extensions;
    gchar *monitors;
    
    gint width, height;
};

struct _LoadInfo {
    float load1, load5, load15;
};

struct _UptimeInfo {
    int days, hours, minutes;
};

struct _Computer {
    MemoryInfo *memory;
    OperatingSystem *os;
    DisplayInfo *display;
    AlsaInfo *alsa;

    gchar *date_time;
};

struct _OperatingSystem {
    gchar *kernel;
    gchar *libc;
    gchar *distrocode, *distro;
    gchar *hostname;
    gchar *language;
    gchar *homedir;
    gchar *kernel_version;

    gchar *languages;

    gchar *desktop;
    gchar *username;
    
    gchar *boots;

    gchar *entropy_avail;
};

struct _MemoryInfo {
    gint total, used, free, cached;
    gfloat ratio;
};

#define get_str(field_name,ptr)               \
  if (g_str_has_prefix(tmp[0], field_name)) { \
    ptr = g_strdup(tmp[1]);                   \
    g_strfreev(tmp);                          \
    continue;                                 \
  }
#define get_int(field_name,ptr)               \
  if (g_str_has_prefix(tmp[0], field_name)) { \
    ptr = atoi(tmp[1]);                       \
    g_strfreev(tmp);                          \
    continue;                                 \
  }
#define get_float(field_name,ptr)             \
  if (g_str_has_prefix(tmp[0], field_name)) { \
    ptr = atof(tmp[1]);                       \
    g_strfreev(tmp);                          \
    continue;                                 \
  }

extern gchar *users;
extern gchar *groups;
extern gchar *fs_list;
extern GHashTable *_module_hash_table;
extern Computer *computer;
extern gchar *module_list;

gchar *computer_get_formatted_loadavg();
gchar *computer_get_formatted_uptime();
gchar *computer_get_alsacards(Computer * computer);
gchar *computer_get_entropy_avail();

OperatingSystem *computer_get_os(void);
AlsaInfo *computer_get_alsainfo(void);
LoadInfo *computer_get_loadinfo(void);
MemoryInfo *computer_get_memory(void);
UptimeInfo *computer_get_uptime(void);
DisplayInfo *computer_get_display(void);

void scan_modules_do(void);
void scan_filesystems(void);
void scan_users_do(void);

#endif				/* __COMPUTER_H__ */
