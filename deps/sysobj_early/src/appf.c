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

#include "appf.h"
#define _GNU_SOURCE /* for vasprintf() */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char *appf(char *str, const char *sep, const char *fmt, ...) {
    char *buf = NULL;
    int inlen, seplen, len;
    va_list args;
    va_start(args, fmt);
    len = vasprintf(&buf, fmt, args);
    va_end(args);
    if (len < 0) return str;
    if (!str) return buf;
    inlen = strlen(str);
    seplen = (inlen && sep) ? strlen(sep) : 0;
    str = realloc(str, inlen + seplen + len + 1);
    if (seplen) strcpy(str + inlen, sep);
    strcpy(str + inlen + seplen, buf);
    free(buf);
    return str;
}

char *appfdup(const char *str, const char *sep, const char *fmt, ...) {
    char *buf = NULL, *ret = NULL;
    int inlen, seplen, len;
    va_list args;
    va_start(args, fmt);
    len = vasprintf(&buf, fmt, args);
    va_end(args);
    if (len < 0) return NULL;
    if (!str) return buf;
    inlen = strlen(str);
    seplen = (inlen && sep) ? strlen(sep) : 0;
    ret = malloc(inlen + seplen + len + 1);
    strcpy(ret, str);
    if (seplen) strcpy(ret + inlen, sep);
    strcpy(ret + inlen + seplen, buf);
    free(buf);
    return ret;
}
