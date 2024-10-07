/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
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

#ifndef __STOCK_H__
#define __STOCK_H__

//#define HI_STOCK_CLIPBOARD	"hi-stock-clipboard"
#define HI_STOCK_ABOUT "hi-stock-about"
#define HI_STOCK_REFRESH	"hi-stock-refresh"
#define HI_STOCK_REPORT "hi-stock-report"
#define HI_STOCK_INTERNET "hi-stock-internet"
#define HI_STOCK_SYNC "hi-stock-sync"
#define HI_STOCK_QUIT "hi-stock-quit"

void stock_icons_init(void);
void stock_icon_register(gchar *filename, gchar *stock_id);
void stock_icon_register_pixbuf(GdkPixbuf *pixbuf, gchar *stock_id);

#endif	/* __STOCK_H__ */
