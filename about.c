#include "hardinfo.h"
#include "about.h"

static void about_close(GtkWidget *widget, gpointer data)
{
	GtkAbout *about = (GtkAbout*) data;

	gtk_widget_destroy(about->window);
}

GtkAbout *
gtk_about_new(const gchar * name, const gchar * version,
	      const gchar * description, const gchar * authors[], const gchar * logo_img)
{
#ifdef GTK2
	GtkWidget *img;
#endif
	gchar *buf;
	const gchar *auth;
	GtkWidget *window, *vbox, *label, *btn, *hr, *hbox;
	GtkAbout *about;
	gint i;

	about = g_new0(GtkAbout, 1);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "About");
	gtk_container_set_border_width(GTK_CONTAINER(window), 8);

	about->window = window;

#ifdef GTK2
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
#else
	gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
#endif

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_set_spacing(GTK_BOX(vbox), 3);
	gtk_container_add(GTK_CONTAINER(window), vbox);

#ifdef GTK2
	img = gtk_image_new_from_file(logo_img);
	gtk_widget_show(img);
	gtk_box_pack_start(GTK_BOX(vbox), img, FALSE, FALSE, 0);
	gtk_widget_set_usize(GTK_WIDGET(img), 64, 64);
#endif

#ifdef GTK2
#define	ADD_LABEL(x,y)	label = gtk_label_new(x); \
			gtk_label_set_use_markup(GTK_LABEL(label), TRUE); \
			gtk_widget_show(label);	\
			gtk_box_pack_start(GTK_BOX(y), label, TRUE, TRUE, 0);
#else
#define	ADD_LABEL(x,y)	label = gtk_label_new(x); \
			gtk_widget_show(label);	\
			gtk_box_pack_start(GTK_BOX(y), label, TRUE, TRUE, 0);
#endif

#ifdef GTK2
	buf =
	    g_strdup_printf
	    ("<span size=\"xx-large\" weight=\"bold\">%s %s</span>", name,
	     version);
#else
	buf = g_strdup_printf("%s %s", name, version);
#endif
	ADD_LABEL(buf, vbox);
	g_free(buf);

	ADD_LABEL(description, vbox);

	for (i = 0; authors[i] != NULL; i++) {
		auth = authors[i];
		
		if (*auth == '>') {
			auth++;

#ifdef GTK2
			buf = g_strdup_printf("<b>%s</b>", auth);
#else
			buf = g_strdup_printf("%s", auth);
#endif
			ADD_LABEL(buf, vbox);
			g_free(buf);
		} else {
#ifdef GTK2
			buf = g_strdup_printf("<span size=\"small\">%s</span>", auth);
#else
			buf = g_strdup_printf("    %s", auth);
#endif
			ADD_LABEL(buf, vbox);
			g_free(buf);
		}
	}

	hr = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), hr, FALSE, FALSE, 0);

	hbox = gtk_hbutton_box_new();
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
        gtk_widget_show(hbox);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 6);

#ifdef GTK2
        btn = gtk_button_new_from_stock(GTK_STOCK_OK);
        g_signal_connect(G_OBJECT(btn), "clicked", (GCallback)about_close, about);
#else
        btn = gtk_button_new_with_label(_("OK"));
        gtk_signal_connect(GTK_OBJECT(btn), "clicked", about_close, about);
#endif
        gtk_widget_show(btn);
        gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

	gtk_widget_show_all(window);

	return about;

}
