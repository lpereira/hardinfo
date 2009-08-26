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

#define DB_PREFIX "/etc/"

static struct {
    gchar *file, *codename;
} distro_db[] = {
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
    { DB_PREFIX "puppyversion",		"ppy"  },
    { DB_PREFIX "distro-release",	"fl"   },
    { DB_PREFIX "vine-release",         "vine" },
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

struct _Processor {
    gchar *model_name;
    gchar *vendor_id;
    gchar *flags;
    gint cache_size;
    gfloat bogomips, cpu_mhz;

    gchar *has_fpu;
    gchar *bug_fdiv, *bug_hlt, *bug_f00f, *bug_coma;
    
    gint model, family, stepping;
    gchar *strmodel;
    
    gint id;
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

#endif				/* __COMPUTER_H__ */
