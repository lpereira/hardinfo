#ifndef __GTK_ABOUT_H__
#define __GTK_ABOUT_H__

typedef struct _GtkAbout GtkAbout;

struct _GtkAbout {
	GtkWidget *window;
};

GtkAbout *gtk_about_new(const gchar * name,
			const gchar * version,
			const gchar * comments,
			const gchar * authors[],
			const gchar * logo_img);

#endif				/* __GTK_ABOUT_H__ */
