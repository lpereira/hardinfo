/*
 * Copyright (c) 2002-2008 Leandro Pereira <leandro@hardinfo.org>
 * Copyright (c) 2002 the Sylpheed Claws Team and Hiroyuki Yamamoto
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

#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/utsname.h>

#include <gnu/libc-version.h>

#include "crash.h"
#include "hardinfo.h"
#include "config.h"
#include "iconcache.h"

#if !GLIB_CHECK_VERSION(2,8,0)
#undef CRASH_DIALOG
#endif

/***/

static GtkWidget *
crash_dialog_show(const gchar * text,
		  const gchar * debug_output);
static gboolean crash_create_debugger_file(void);
static void 
crash_debug(unsigned long crash_pid,
	    gchar * exe_image, GString * debug_output);
static const gchar *get_compiled_in_features(void);
static const gchar *get_lib_version(void);
static const gchar *get_operating_system(void);
static gboolean is_crash_dialog_allowed(void);
static void     crash_handler(int sig);
static void     crash_cleanup_exit(void);

/***/

static const gchar *DEBUG_SCRIPT = "bt full\n" "kill\n" "quit\n";

/***/

/*
 * ! \brief	install crash handlers
 */
void 
crash_install_handlers(void)
{
#if CRASH_DIALOG
	sigset_t        mask;

	if (!is_crash_dialog_allowed())
		return;

	sigemptyset(&mask);

#ifdef SIGSEGV
	signal(SIGSEGV, crash_handler);
	sigaddset(&mask, SIGSEGV);
#endif

#ifdef SIGFPE
	signal(SIGFPE, crash_handler);
	sigaddset(&mask, SIGFPE);
#endif

#ifdef SIGILL
	signal(SIGILL, crash_handler);
	sigaddset(&mask, SIGILL);
#endif

#ifdef SIGABRT
	signal(SIGABRT, crash_handler);
	sigaddset(&mask, SIGABRT);
#endif

	sigprocmask(SIG_UNBLOCK, &mask, 0);

#endif				/* CRASH_DIALOG */
}

/***/

/*
 * ! \brief	crash dialog entry point
 */
void 
crash_main(const char *arg)
{
#if CRASH_DIALOG
	gchar          *text;
	gchar         **tokens;
	unsigned long   pid;
	GString        *output;

	crash_create_debugger_file();
	tokens = g_strsplit(arg, ",", 0);

	pid = atol(tokens[0]);
	text = g_strdup_printf("HardInfo process (%ld) died due to an error (%s)",
			       pid, g_strsignal(atol(tokens[1])));

	output = g_string_new("");
	crash_debug(pid, tokens[2], output);

	crash_dialog_show(text, output->str);
	g_string_free(output, TRUE);
	g_free(text);
	g_strfreev(tokens);
#endif				/* CRASH_DIALOG */
}

/*
 * ! \brief	(can't get pixmap working, so discarding it)
 */
static GtkWidget *
crash_dialog_show(const gchar * text,
		  const gchar * debug_output)
{
	GtkWidget      *window1;
	GtkWidget      *vbox1;
	GtkWidget      *hbox1;
	GtkWidget      *label1;
	GtkWidget      *frame1;
	GtkWidget      *scrolledwindow1;
	GtkWidget      *text1;
	GtkWidget      *hbuttonbox3;
	GtkWidget      *hbuttonbox4;
	GtkWidget      *button3;
	GtkWidget      *pixwid;
	GtkTextBuffer  *buffer;
	GtkTextIter	iter;
	gchar          *crash_report, *header;

	window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window1), 5);
	gtk_window_set_title(GTK_WINDOW(window1), "HardInfo has crashed");
	gtk_window_set_position(GTK_WINDOW(window1), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(window1), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(window1), 460, 350);

	vbox1 = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(window1), vbox1);

	hbox1 = gtk_hbox_new(FALSE, 4);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox1), 4);

	pixwid = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
 	gtk_widget_show(pixwid);
	gtk_box_pack_start(GTK_BOX(hbox1), pixwid, TRUE, TRUE, 0);

	header = g_strdup_printf("%s.\nPlease file a bug report and include the information below.", text);
	label1 = gtk_label_new(header);
	g_free(header);
	
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox1), label1, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(label1), 0, 0.5);

	frame1 = gtk_frame_new("Debug log");
	gtk_widget_show(frame1);
	gtk_box_pack_start(GTK_BOX(vbox1), frame1, TRUE, TRUE, 0);

	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow1);
	gtk_container_add(GTK_CONTAINER(frame1), scrolledwindow1);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow1), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1),
				       GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

	text1 = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text1), FALSE);
	gtk_widget_show(text1);
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), text1);

	crash_report =
		g_strdup_printf
		("HardInfo version %s\nGTK+ version %d.%d.%d\nFeatures: %s\nOperating system: %s\nC Library: %s\n--\n%s",
	         VERSION, gtk_major_version, gtk_minor_version, gtk_micro_version,
		 get_compiled_in_features(), get_operating_system(),
		 get_lib_version(), debug_output);

        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
        gtk_text_buffer_get_start_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, crash_report, -1);
        
        g_free(crash_report);
                        
	hbuttonbox3 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox3);
	gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox3, FALSE, FALSE, 0);

	hbuttonbox4 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox4);
	gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox4, FALSE, FALSE, 0);

	button3 = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_widget_show(button3);
	gtk_container_add(GTK_CONTAINER(hbuttonbox4), button3);

	g_signal_connect(G_OBJECT(window1), "delete_event",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(button3), "clicked",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

	gtk_widget_show(window1);

	gtk_main();
	return window1;
}


/*
 * ! \brief	create debugger script file in publicit directory. all the
 * other options (creating temp files) looked too convoluted.
 */
static gboolean 
crash_create_debugger_file(void)
{
	gchar *filename = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S,
					".hardinfo", G_DIR_SEPARATOR_S,
					DEBUGGERRC, NULL);

	g_file_set_contents(filename, DEBUG_SCRIPT, -1, NULL);
	g_free(filename);

	return (TRUE);
}

/*
 * ! \brief	launches debugger and attaches it to crashed publicit
 */
static void 
crash_debug(unsigned long crash_pid,
	    gchar * exe_image, GString * debug_output)
{
	int             choutput[2];
	pid_t           pid;

	pipe(choutput);

	if (0 == (pid = fork())) {
		char           *argp[10];
		char          **argptr = argp;

		setgid(getgid());
		setuid(getuid());

		/*
		 * setup debugger to attach to crashed publicit
		 */
		*argptr++ = "gdb";
		*argptr++ = "--nw";
		*argptr++ = "--nx";
		*argptr++ = "--quiet";
		*argptr++ = "--batch";
		*argptr++ = "-x";
		*argptr++ = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S,
					".hardinfo", G_DIR_SEPARATOR_S,
					DEBUGGERRC, NULL);
		*argptr++ = exe_image;
		*argptr++ = g_strdup_printf("%ld", crash_pid);
		*argptr = NULL;

		/*
		 * redirect output to write end of pipe
		 */
		close(1);
		dup(choutput[1]);
		close(choutput[0]);
		if (-1 == execvp("gdb", argp))
			puts("error execvp\n");
	} else {
		char            buf[100];
		int             r;

		waitpid(pid, NULL, 0);

		/*
		 * make it non blocking
		 */
		if (-1 == fcntl(choutput[0], F_SETFL, O_NONBLOCK))
			puts("set to non blocking failed\n");

		/*
		 * get the output
		 */
		do {
			r = read(choutput[0], buf, sizeof buf - 1);
			if (r > 0) {
				buf[r] = 0;
				g_string_append(debug_output, buf);
			}
		} while (r > 0);

		close(choutput[0]);
		close(choutput[1]);

		/*
		 * kill the process we attached to
		 */
		kill(crash_pid, SIGCONT);
	}
}

/***/

/*
 * ! \brief	features
 */
static const gchar *
get_compiled_in_features(void)
{
	return g_strdup("None");
}

/***/

/*
 * ! \brief	library version
 */
static const gchar *
get_lib_version(void)
{
#if defined(__GNU_LIBRARY__)
	return g_strdup_printf("GNU libc %s", gnu_get_libc_version());
#else
	return g_strdup("Unknown");
#endif
}

/***/

/*
 * ! \brief	operating system
 */
static const gchar *
get_operating_system(void)
{
	struct utsname  utsbuf;
	uname(&utsbuf);
	return g_strdup_printf("%s %s (%s)",
			    utsbuf.sysname, utsbuf.release, utsbuf.machine);
}

/***/

/*
 * ! \brief	see if the crash dialog is allowed (because some developers
 * may prefer to run hardinfo under gdb...)
 */
static gboolean 
is_crash_dialog_allowed(void)
{
	return !getenv("HARDINFO_NO_CRASH");
}

/*
 * ! \brief	this handler will probably evolve into something better.
 */
static void 
crash_handler(int sig)
{
	pid_t           pid;
	static volatile unsigned long crashed_ = 0;

	/*
         * besides guarding entrancy it's probably also better
         * to mask off signals
         */
	if (crashed_)
		return;

	crashed_++;

	/*
         * gnome ungrabs focus, and flushes gdk. mmmh, good idea.
         */
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	gdk_flush();

	if (0 == (pid = fork())) {
		char            buf[50];
		char           *args[4];

		/*
		 * probably also some other parameters (like GTK+ ones).
		 * also we pass the full startup dir and the real command
		 * line typed in (argv0)
		 */
		args[0] = params.argv0;
		args[1] = "--crash";
		sprintf(buf, "%ld,%d,%s", (long) getppid(), sig, params.argv0);
		args[2] = buf;
		args[3] = NULL;

		setgid(getgid());
		setuid(getuid());
		execvp(params.argv0, args);
	} else {
		waitpid(pid, NULL, 0);
		crash_cleanup_exit();
		_exit(253);
	}

	_exit(253);
}

/*
 * ! \brief	put all the things here we can do before letting the program
 * die
 */
static void 
crash_cleanup_exit(void)
{
	;
}
