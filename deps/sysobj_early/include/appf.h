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

#ifndef _APPF_H_
#define _APPF_H_

/* Appends a formatted element to a string, adding an optional
 * separator string if the string is not empty.
 * The string is created if str is null.
 * ex: ret = appf(ret, "; ", "%s = %d", name, value); */
char *appf(char *str, const char *sep, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));

/* Same as above except that str is untouched.
 * ex: ret = appf(keeper, "; ", "%s = %d", name, value); */
char *appfdup(const char *str, const char *sep, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));

/* for convenience */
#define appfsp(str, fmt, ...) appf(str, " ", fmt, __VA_ARGS__)
#define appfnl(str, fmt, ...) appf(str, "\n", fmt, __VA_ARGS__)

#endif
