/*
 * BinReloc - a library for creating relocatable executables
 * Written by: Hongli Lai <h.lai@chello.nl>
 * Copyright by: hardinfo2 project
 * http://autopackage.org/
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License v2.0 or later.
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

#ifndef __BINRELOC_H__
#define __BINRELOC_H__

#include <glib.h>

G_BEGIN_DECLS


/** These error codes can be returned by br_init(), br_init_lib(), gbr_init() or gbr_init_lib(). */
typedef enum {
	/** Cannot allocate memory. */
	GBR_INIT_ERROR_NOMEM,
	/** Unable to open /proc/self/maps; see errno for details. */
	GBR_INIT_ERROR_OPEN_MAPS,
	/** Unable to read from /proc/self/maps; see errno for details. */
	GBR_INIT_ERROR_READ_MAPS,
	/** The file format of /proc/self/maps is invalid; kernel bug? */
	GBR_INIT_ERROR_INVALID_MAPS,
	/** BinReloc is disabled (the ENABLE_BINRELOC macro is not defined). */
	GBR_INIT_ERROR_DISABLED
} GbrInitError;


#ifndef BINRELOC_RUNNING_DOXYGEN
/* Mangle symbol names to avoid symbol collisions with other ELF objects. */
	#define gbr_find_exe         UfUy21856259474323_gbr_find_exe
	#define gbr_find_exe_dir     UfUy21856259474323_gbr_find_exe_dir
	#define gbr_find_prefix      UfUy21856259474323_gbr_find_prefix
	#define gbr_find_bin_dir     UfUy21856259474323_gbr_find_bin_dir
	#define gbr_find_sbin_dir    UfUy21856259474323_gbr_find_sbin_dir
	#define gbr_find_data_dir    UfUy21856259474323_gbr_find_data_dir
	#define gbr_find_locale_dir  UfUy21856259474323_gbr_find_locale_dir
	#define gbr_find_lib_dir     UfUy21856259474323_gbr_find_lib_dir
	#define gbr_find_libexec_dir UfUy21856259474323_gbr_find_libexec_dir
	#define gbr_find_etc_dir     UfUy21856259474323_gbr_find_etc_dir


#endif
gboolean gbr_init             (GError **error);
gboolean gbr_init_lib         (GError **error);

gchar   *gbr_find_exe         (const gchar *default_exe);
gchar   *gbr_find_exe_dir     (const gchar *default_dir);
gchar   *gbr_find_prefix      (const gchar *default_prefix);
gchar   *gbr_find_bin_dir     (const gchar *default_bin_dir);
gchar   *gbr_find_sbin_dir    (const gchar *default_sbin_dir);
gchar   *gbr_find_data_dir    (const gchar *default_data_dir);
gchar   *gbr_find_locale_dir  (const gchar *default_locale_dir);
gchar   *gbr_find_lib_dir     (const gchar *default_lib_dir);
gchar   *gbr_find_libexec_dir (const gchar *default_libexec_dir);
gchar   *gbr_find_etc_dir     (const gchar *default_etc_dir);


G_END_DECLS

#endif /* __BINRELOC_H__ */
