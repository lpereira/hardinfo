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
 
#ifndef __REPORT_H__
#define __REPORT_H__
#include <gtk/gtk.h>
#include <shell.h>

typedef struct _ReportDialog	ReportDialog;
typedef struct _ReportContext	ReportContext;

struct _ReportContext {
  ReportDialog		*rd;
  ShellModuleEntry	*entry;
  
  FILE			*stream;
};

struct _ReportDialog {
  GtkWidget *dialog;
  GtkWidget *filechooser;
  GtkWidget *btn_cancel;
  GtkWidget *btn_generate;
  GtkWidget *btn_sel_all;
  GtkWidget *btn_sel_none;
  GtkWidget *treeview;
  
  GtkTreeModel *model;
};

void report_dialog_show();

#endif	/* __REPORT_H__ */
