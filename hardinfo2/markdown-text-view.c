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
#include <gdk/gdkkeysyms.h>

#include "markdown-text-view.h"
#include "config.h"

static GdkCursor *hand_cursor = NULL;

static void markdown_textview_finalize(GObject * object);

G_DEFINE_TYPE(MarkdownTextView, markdown_textview, GTK_TYPE_TEXT_VIEW);

enum {
     LINK_CLICKED,
     HOVERING_OVER_LINK,
     HOVERING_OVER_TEXT,
     FILE_LOAD_COMPLETE,
     LAST_SIGNAL
};

static guint markdown_textview_signals[LAST_SIGNAL] = { 0 };

GtkWidget *markdown_textview_new()
{
    return g_object_new(TYPE_MARKDOWN_TEXTVIEW, NULL);
}

static void markdown_textview_class_init(MarkdownTextViewClass * klass)
{
    GObjectClass *object_class;

    if (!hand_cursor) {
	hand_cursor = gdk_cursor_new(GDK_HAND2);
    }

    object_class = G_OBJECT_CLASS(klass);

    markdown_textview_signals[LINK_CLICKED] = g_signal_new(
             "link-clicked",
             G_OBJECT_CLASS_TYPE(object_class),
             G_SIGNAL_RUN_FIRST,
             G_STRUCT_OFFSET(MarkdownTextViewClass, link_clicked),
             NULL, NULL,
             g_cclosure_marshal_VOID__STRING,
             G_TYPE_NONE,
             1,
             G_TYPE_STRING);

    markdown_textview_signals[HOVERING_OVER_LINK] = g_signal_new(
             "hovering-over-link",
             G_OBJECT_CLASS_TYPE(object_class),
             G_SIGNAL_RUN_FIRST,
             G_STRUCT_OFFSET(MarkdownTextViewClass, hovering_over_link),
             NULL, NULL,
             g_cclosure_marshal_VOID__STRING,
             G_TYPE_NONE,
             1,
             G_TYPE_STRING);

    markdown_textview_signals[HOVERING_OVER_TEXT] = g_signal_new(
             "hovering-over-text",
             G_OBJECT_CLASS_TYPE(object_class),
             G_SIGNAL_RUN_FIRST,
             G_STRUCT_OFFSET(MarkdownTextViewClass, hovering_over_text),
             NULL, NULL,
             g_cclosure_marshal_VOID__VOID,
             G_TYPE_NONE,
             0);

    markdown_textview_signals[FILE_LOAD_COMPLETE] = g_signal_new(
             "file-load-complete",
             G_OBJECT_CLASS_TYPE(object_class),
             G_SIGNAL_RUN_FIRST,
             G_STRUCT_OFFSET(MarkdownTextViewClass, file_load_complete),
             NULL, NULL,
             g_cclosure_marshal_VOID__STRING,
             G_TYPE_NONE,
             1,
             G_TYPE_STRING);
}

static void
gtk_text_buffer_insert_markup(GtkTextBuffer * buffer, GtkTextIter * iter,
			      const gchar * markup)
{
    PangoAttrIterator *paiter;
    PangoAttrList *attrlist;
    GtkTextMark *mark;
    GError *error = NULL;
    gchar *text;

    g_return_if_fail(GTK_IS_TEXT_BUFFER(buffer));
    g_return_if_fail(markup != NULL);

    if (*markup == '\000')
	return;

    /* invalid */
    if (!pango_parse_markup(markup, -1, 0, &attrlist, &text, NULL, &error)) {
	g_warning("Invalid markup string: %s", error->message);
	g_error_free(error);
	return;
    }

    /* trivial, no markup */
    if (attrlist == NULL) {
	gtk_text_buffer_insert(buffer, iter, text, -1);
	g_free(text);
	return;
    }

    /* create mark with right gravity */
    mark = gtk_text_buffer_create_mark(buffer, NULL, iter, FALSE);
    paiter = pango_attr_list_get_iterator(attrlist);

    do {
	PangoAttribute *attr;
	GtkTextTag *tag;
	GtkTextTag *tag_para;
	gint start, end;

	pango_attr_iterator_range(paiter, &start, &end);

	if (end == G_MAXINT)	/* last chunk */
	    end = start - 1;	/* resulting in -1 to be passed to _insert */

	tag = gtk_text_tag_new(NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_LANGUAGE)))
	    g_object_set(tag, "language",
			 pango_language_to_string(((PangoAttrLanguage *)
						   attr)->value), NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FAMILY)))
	    g_object_set(tag, "family", ((PangoAttrString *) attr)->value,
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STYLE)))
	    g_object_set(tag, "style", ((PangoAttrInt *) attr)->value,
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_WEIGHT)))
	    g_object_set(tag, "weight", ((PangoAttrInt *) attr)->value,
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_VARIANT)))
	    g_object_set(tag, "variant", ((PangoAttrInt *) attr)->value,
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STRETCH)))
	    g_object_set(tag, "stretch", ((PangoAttrInt *) attr)->value,
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SIZE)))
	    g_object_set(tag, "size", ((PangoAttrInt *) attr)->value,
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FONT_DESC)))
	    g_object_set(tag, "font-desc",
			 ((PangoAttrFontDesc *) attr)->desc, NULL);

	if ((attr =
	     pango_attr_iterator_get(paiter, PANGO_ATTR_FOREGROUND))) {
	    GdkColor col = { 0,
		((PangoAttrColor *) attr)->color.red,
		((PangoAttrColor *) attr)->color.green,
		((PangoAttrColor *) attr)->color.blue
	    };

	    g_object_set(tag, "foreground-gdk", &col, NULL);
	}

	if ((attr =
	     pango_attr_iterator_get(paiter, PANGO_ATTR_BACKGROUND))) {
	    GdkColor col = { 0,
		((PangoAttrColor *) attr)->color.red,
		((PangoAttrColor *) attr)->color.green,
		((PangoAttrColor *) attr)->color.blue
	    };

	    g_object_set(tag, "background-gdk", &col, NULL);
	}

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_UNDERLINE)))
	    g_object_set(tag, "underline", ((PangoAttrInt *) attr)->value,
			 NULL);

	if ((attr =
	     pango_attr_iterator_get(paiter, PANGO_ATTR_STRIKETHROUGH)))
	    g_object_set(tag, "strikethrough",
			 (gboolean) (((PangoAttrInt *) attr)->value != 0),
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_RISE)))
	    g_object_set(tag, "rise", ((PangoAttrInt *) attr)->value,
			 NULL);

	if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SCALE)))
	    g_object_set(tag, "scale", ((PangoAttrFloat *) attr)->value,
			 NULL);

	gtk_text_tag_table_add(gtk_text_buffer_get_tag_table(buffer), tag);

	tag_para =
	    gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
				      (buffer), "para");
	gtk_text_buffer_insert_with_tags(buffer, iter, text + start,
					 end - start, tag, tag_para, NULL);

	/* mark had right gravity, so it should be
	 *      at the end of the inserted text now */
	gtk_text_buffer_get_iter_at_mark(buffer, iter, mark);
    } while (pango_attr_iterator_next(paiter));

    gtk_text_buffer_delete_mark(buffer, mark);
    pango_attr_iterator_destroy(paiter);
    pango_attr_list_unref(attrlist);
    g_free(text);
}


static void
set_cursor_if_appropriate(MarkdownTextView * self, gint x, gint y)
{
    GSList *tags = NULL, *tagp = NULL;
    GtkTextIter iter;
    gboolean hovering = FALSE;
    gchar *link_uri = NULL;

    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(self), &iter, x,
				       y);

    tags = gtk_text_iter_get_tags(&iter);
    for (tagp = tags; tagp != NULL; tagp = tagp->next) {
	GtkTextTag *tag = tagp->data;
	gint is_underline = 0;
	gchar *lang = NULL;

	g_object_get(G_OBJECT(tag),
		     "underline", &is_underline,
		     "language", &lang,
		     NULL);

	if (is_underline == 1 && lang) {
	    link_uri = egg_markdown_get_link_uri(self->markdown, atoi(lang));
	    g_free(lang);
	    hovering = TRUE;
	    break;
	}

	g_free(lang);
    }

    if (hovering != self->hovering_over_link) {
	self->hovering_over_link = hovering;

	if (self->hovering_over_link) {
            g_signal_emit(self, markdown_textview_signals[HOVERING_OVER_LINK],
                          0, link_uri);
	    gdk_window_set_cursor(gtk_text_view_get_window
				  (GTK_TEXT_VIEW(self),
				   GTK_TEXT_WINDOW_TEXT), hand_cursor);
	} else {
            g_signal_emit(self, markdown_textview_signals[HOVERING_OVER_TEXT], 0);
	    gdk_window_set_cursor(gtk_text_view_get_window
				  (GTK_TEXT_VIEW(self),
				   GTK_TEXT_WINDOW_TEXT), NULL);
        }
    }
    
    if (link_uri)
        g_free(link_uri);

    if (tags)
	g_slist_free(tags);
}

/* Update the cursor image if the pointer moved. 
 */
static gboolean
motion_notify_event(GtkWidget * self, GdkEventMotion * event)
{
    gint x, y;

    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(self),
					  GTK_TEXT_WINDOW_WIDGET,
					  event->x, event->y, &x, &y);

    set_cursor_if_appropriate(MARKDOWN_TEXTVIEW(self), x, y);

    gdk_window_get_pointer(self->window, NULL, NULL, NULL);
    return FALSE;
}

/* Also update the cursor image if the window becomes visible
 * (e.g. when a window covering it got iconified).
 */
static gboolean
visibility_notify_event(GtkWidget * self, GdkEventVisibility * event)
{
    gint wx, wy, bx, by;

    gdk_window_get_pointer(self->window, &wx, &wy, NULL);

    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(self),
					  GTK_TEXT_WINDOW_WIDGET,
					  wx, wy, &bx, &by);

    set_cursor_if_appropriate(MARKDOWN_TEXTVIEW(self), bx, by);

    return FALSE;
}

static void
follow_if_link(GtkWidget * widget, GtkTextIter * iter)
{
    GSList *tags = NULL, *tagp = NULL;
    MarkdownTextView *self = MARKDOWN_TEXTVIEW(widget);
    
    tags = gtk_text_iter_get_tags(iter);
    for (tagp = tags; tagp != NULL; tagp = tagp->next) {
	GtkTextTag *tag = tagp->data;
	gint is_underline = 0;
	gchar *lang = NULL;

	g_object_get(G_OBJECT(tag),
		     "underline", &is_underline,
		     "language", &lang,
		     NULL);

	if (is_underline == 1 && lang) {
	    gchar *link = egg_markdown_get_link_uri(self->markdown, atoi(lang));
	    if (link) {
	        g_signal_emit(self, markdown_textview_signals[LINK_CLICKED],
                              0, link);
		g_free(link);
	    }
	    g_free(lang);
	    break;
	}

	g_free(lang);
    }

    if (tags)
	g_slist_free(tags);
}

static gboolean
key_press_event(GtkWidget * self,
		GdkEventKey * event)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;

    switch (event->keyval) {
    case GDK_Return:
    case GDK_KP_Enter:
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	gtk_text_buffer_get_iter_at_mark(buffer, &iter,
					 gtk_text_buffer_get_insert
					 (buffer));
	follow_if_link(self, &iter);
	break;

    default:
	break;
    }

    return FALSE;
}

static gboolean
event_after(GtkWidget * self, GdkEvent * ev)
{
    GtkTextIter start, end, iter;
    GtkTextBuffer *buffer;
    GdkEventButton *event;
    gint x, y;

    if (ev->type != GDK_BUTTON_RELEASE)
	return FALSE;

    event = (GdkEventButton *) ev;

    if (event->button != 1)
	return FALSE;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));

    /* we shouldn't follow a link if the user has selected something */
    gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
    if (gtk_text_iter_get_offset(&start) != gtk_text_iter_get_offset(&end))
	return FALSE;

    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(self),
					  GTK_TEXT_WINDOW_WIDGET,
					  event->x, event->y, &x, &y);

    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(self), &iter, x,
				       y);

    follow_if_link(self, &iter);

    return FALSE;
}

void
markdown_textview_clear(MarkdownTextView * self)
{
    GtkTextBuffer *text_buffer;
    
    g_return_if_fail(IS_MARKDOWN_TEXTVIEW(self));

    egg_markdown_clear(self->markdown);

    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
    gtk_text_buffer_set_text(text_buffer, "\n", 1);
}

static void
load_images(MarkdownTextView * self)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    GSList *tags, *tagp;
    gchar *image_path;
    
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
    gtk_text_buffer_get_start_iter(buffer, &iter);
    
    do {
       tags = gtk_text_iter_get_tags(&iter);
       for (tagp = tags; tagp != NULL; tagp = tagp->next) {
           GtkTextTag *tag = tagp->data;
           gint is_underline = 0;
           gchar *lang = NULL;

           g_object_get(G_OBJECT(tag),
                        "underline", &is_underline,
                        "language", &lang,
                        NULL);

           if (is_underline == 2 && lang) {
               GdkPixbuf *pixbuf;
               gchar *path;
               
               image_path = egg_markdown_get_link_uri(self->markdown, atoi(lang));
               path = g_build_filename(self->image_directory, image_path, NULL);
               pixbuf = gdk_pixbuf_new_from_file(path, NULL);
               if (pixbuf) {
                   GtkTextMark *mark;
                   GtkTextIter start;

                   mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
                   
                   gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
                   gtk_text_iter_forward_to_tag_toggle(&iter, tag);
                   gtk_text_buffer_delete(buffer, &start, &iter);

                   gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
  
                   g_object_unref(pixbuf);
                   gtk_text_buffer_delete_mark(buffer, mark);
               }
               
               g_free(image_path);
               g_free(lang);
               g_free(path);
               break;
           }

           g_free(lang);
       }
       
       if (tags)
           g_slist_free(tags);
   } while (gtk_text_iter_forward_to_tag_toggle(&iter, NULL));
}

static gboolean
append_text(MarkdownTextView * self,
            const gchar * text)
{
    GtkTextIter iter;
    GtkTextBuffer *text_buffer;
    gchar *line;
    
    g_return_val_if_fail(IS_MARKDOWN_TEXTVIEW(self), FALSE);
    
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
    gtk_text_buffer_get_end_iter(text_buffer, &iter);

    line = egg_markdown_parse(self->markdown, text);
    if (line && *line) {
        gtk_text_buffer_insert_markup(text_buffer, &iter, line);
        gtk_text_buffer_insert(text_buffer, &iter, "\n", 1);
        g_free(line);
        
        return TRUE;
    }
    
    return FALSE;
}

gboolean
markdown_textview_set_text(MarkdownTextView * self,
                           const gchar * text)
{
    gboolean result = TRUE;
    gchar **lines;
    gint line;
    
    g_return_val_if_fail(IS_MARKDOWN_TEXTVIEW(self), FALSE);

    markdown_textview_clear(self);
    
    lines = g_strsplit(text, "\n", 0);
    for (line = 0; result && lines[line]; line++) {
         result = append_text(self, (const gchar *)lines[line]);
    }
    g_strfreev(lines);

    load_images(self);
    
    return result;
}                           

gboolean
markdown_textview_load_file(MarkdownTextView * self,
			    const gchar * file_name)
{
    FILE *text_file;
    gchar *path;

    g_return_val_if_fail(IS_MARKDOWN_TEXTVIEW(self), FALSE);
    
    path = g_build_filename(self->image_directory, file_name, NULL);

    /* we do assume UTF-8 encoding */
    if ((text_file = fopen(path, "rb"))) {
	GtkTextBuffer *text_buffer;
	GtkTextIter iter;
	gchar *line;
	gchar buffer[EGG_MARKDOWN_MAX_LINE_LENGTH];

	text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));

	gtk_text_buffer_set_text(text_buffer, "\n", 1);
	gtk_text_buffer_get_start_iter(text_buffer, &iter);

	egg_markdown_clear(self->markdown);

	while (fgets(buffer, EGG_MARKDOWN_MAX_LINE_LENGTH, text_file)) {
	    line = egg_markdown_parse(self->markdown, buffer);

	    if (line && *line) {
		gtk_text_buffer_insert_markup(text_buffer, &iter, line);
		gtk_text_buffer_insert(text_buffer, &iter, "\n", 1);
	    }

	    g_free(line);
	}
	fclose(text_file);
	
	load_images(self);
	
        g_signal_emit(self, markdown_textview_signals[FILE_LOAD_COMPLETE], 0, file_name);
        
        g_free(path);

	return TRUE;
    }
    
    g_free(path);

    return FALSE;
}

void
markdown_textview_set_image_directory(MarkdownTextView * self, const gchar *directory)
{
    g_return_if_fail(IS_MARKDOWN_TEXTVIEW(self));

    g_free(self->image_directory);
    self->image_directory = g_strdup(directory);
}

static void markdown_textview_init(MarkdownTextView * self)
{
    self->markdown = egg_markdown_new();
    self->image_directory = g_strdup(".");

    egg_markdown_set_output(self->markdown, EGG_MARKDOWN_OUTPUT_PANGO);
    egg_markdown_set_escape(self->markdown, TRUE);
    egg_markdown_set_autocode(self->markdown, TRUE);
    egg_markdown_set_smart_quoting(self->markdown, TRUE);

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self), GTK_WRAP_WORD);
    gtk_text_view_set_justification(GTK_TEXT_VIEW(self),
				    GTK_JUSTIFY_FILL);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(self), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(self), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(self), 10);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(self), 3);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(self), 3);
    
    g_signal_connect(self, "event-after",
		     G_CALLBACK(event_after), NULL);
    g_signal_connect(self, "key-press-event",
		     G_CALLBACK(key_press_event), NULL);
    g_signal_connect(self, "motion-notify-event",
		     G_CALLBACK(motion_notify_event), NULL);
    g_signal_connect(self, "visibility-notify-event",
		     G_CALLBACK(visibility_notify_event), NULL);
}

static void markdown_textview_finalize(GObject * object)
{
    MarkdownTextView *self;

    g_return_if_fail(IS_MARKDOWN_TEXTVIEW(object));

    self = MARKDOWN_TEXTVIEW(object);
    g_object_unref(self->markdown);
}
