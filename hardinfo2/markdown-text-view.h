/*
 * Markdown Text View
 * GtkTextView subclass that supports Markdown syntax
 *
 * Copyright (C) 2009 Leandro Pereira <leandro@hardinfo.org>
 * Portions Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
 * Portions Copyright (C) GTK+ Team (based on hypertext textview demo)
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __MARKDOWN_TEXTVIEW_H__
#define __MARKDOWN_TEXTVIEW_H__

#include <gtk/gtk.h>
#include "egg-markdown.h"

G_BEGIN_DECLS
#define TYPE_MARKDOWN_TEXTVIEW			(markdown_textview_get_type())
#define MARKDOWN_TEXTVIEW(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_MARKDOWN_TEXTVIEW, MarkdownTextView))
#define MARKDOWN_TEXTVIEW_CLASS(obj)		(G_TYPE_CHECK_CLASS_CAST((obj), MARKDOWN_TEXTVIEW, MarkdownTextViewClass))
#define IS_MARKDOWN_TEXTVIEW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_MARKDOWN_TEXTVIEW))
#define IS_MARKDOWN_TEXTVIEW_CLASS(obj)		(G_TYPE_CHECK_CLASS_TYPE((obj), TYPE_MARKDOWN_TEXTVIEW))
#define MARKDOWN_TEXTVIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_MARKDOWN_TEXTVIEW, MarkdownTextViewClass))

typedef struct _MarkdownTextView MarkdownTextView;
typedef struct _MarkdownTextViewClass MarkdownTextViewClass;

struct _MarkdownTextView {
    GtkTextView parent;

    EggMarkdown *markdown;
    gboolean hovering_over_link;
    gchar *image_directory;
};

struct _MarkdownTextViewClass {
    GtkTextViewClass parent_class;
    
    void	(*link_clicked)		(MarkdownTextView *text_view, gchar *uri);
    void	(*hovering_over_link)	(MarkdownTextView *text_view, gchar *uri);
    void	(*hovering_over_text)	(MarkdownTextView *text_view);
    void	(*file_load_complete)	(MarkdownTextView *text_view, gchar *file);
};

GtkWidget	*markdown_textview_new();
gboolean	 markdown_textview_load_file(MarkdownTextView * textview,
		  	                     const gchar * file_name);
gboolean 	 markdown_textview_set_text(MarkdownTextView * textview,
                                            const gchar * text);
void		 markdown_textview_clear(MarkdownTextView * textview);
void		 markdown_textview_set_image_directory(MarkdownTextView * self,
                                                       const gchar * directory);

G_END_DECLS

#endif				/* __MARKDOWN_TEXTVIEW_H__ */
