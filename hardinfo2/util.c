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
#include <string.h>
#include <hardinfo.h>

inline void
remove_quotes(gchar *str)
{
    if (!str)
        return;

    while (*str == '"')
        *(str++) = ' ';

    gchar *p;
    if ((p = strchr(str, '"')))
        *p = 0;
}

inline void
strend(gchar *str, gchar chr)
{
    if (!str)
        return;
        
    char *p;
    if ((p = strchr(str, chr)))
        *p = 0;
}

inline void
remove_linefeed(gchar * str)
{
    strend(str, '\n');
}
