/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _AUTO_FREE_H_
#define _AUTO_FREE_H_

#include <glib.h>

/* DEBUG_AUTO_FREE messages level:
 * 0 - none
 * 1 - some
 * 2 - much
 */
#ifndef DEBUG_AUTO_FREE
#define DEBUG_AUTO_FREE 0
#endif
/* the period between free_auto_free()s in the main loop */
#define AF_SECONDS 11
/* the minimum time between auto_free(p) and free(p) */
#define AF_DELAY_SECONDS 10

#define AF_USE_SYSOBJ 0

#if (DEBUG_AUTO_FREE > 0)
#define auto_free(p) auto_free_ex_(p, (GDestroyNotify)g_free, __FILE__, __LINE__, __FUNCTION__)
#define auto_free_ex(p, f) auto_free_ex_(p, f, __FILE__, __LINE__, __FUNCTION__)
#define auto_free_on_exit(p) auto_free_on_exit_ex_(p, (GDestroyNotify)g_free, __FILE__, __LINE__, __FUNCTION__)
#define auto_free_on_exit_ex(p, f) auto_free_on_exit_ex_(p, f, __FILE__, __LINE__, __FUNCTION__)
#else
#define auto_free(p) auto_free_ex_(p, (GDestroyNotify)g_free, NULL, 0, NULL)
#define auto_free_ex(p, f) auto_free_ex_(p, f, NULL, 0, NULL)
#define auto_free_on_exit(p) auto_free_on_exit_ex_(p, (GDestroyNotify)g_free, NULL, 0, NULL)
#define auto_free_on_exit_ex(p, f) auto_free_on_exit_ex_(p, f, NULL, 0, NULL)
#endif
gpointer auto_free_ex_(gpointer p, GDestroyNotify f, const char *file, int line, const char *func);
gpointer auto_free_on_exit_ex_(gpointer p, GDestroyNotify f, const char *file, int line, const char *func);

/* free all the auto_free marked items in the
 * current thread with age > AF_DELAY_SECONDS */
void free_auto_free();

/* call at thread termination:
 * free all the auto_free marked items in the
 * current thread regardless of age */
void free_auto_free_thread_final();

/* call at program termination */
void free_auto_free_final();

#endif
