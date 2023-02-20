/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright 2006-2023 Hannu Jokinen
 * 
 * This file is part of JudoShiai.
 *
 * JudoShiai is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * JudoShiai is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with JudoShiai. If not, see
 * <https://www.gnu.org/licenses/>.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#if (GTKVER == 3)
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

#include <cairo.h>
#include <cairo-pdf.h>

#include "sqlite3.h"
#include "judoshiai.h"

#define SIZEX 400
#define SIZEY 400
#define HISTORY_LEN 8

extern int run_basic_script(char *file, GtkTextBuffer *buffer);
extern void svg_lisp_window(gchar *filename, GtkTextBuffer *buffer);

static gchar *history[HISTORY_LEN];
static gint history_line;
static gchar *filename = NULL;

static gboolean delete_event_sql( GtkWidget *widget,
                                  GdkEvent  *event,
                                  gpointer   data )
{
    return FALSE;
}

static void destroy_sql( GtkWidget *widget,
                         gpointer   data )
{
}

#if 0
static void char_typed(GtkWidget *w, gpointer arg) 
{
    const gchar *txt = gtk_entry_get_text(GTK_ENTRY(w));
    mylog("txt=%s\n", txt);
}
#endif

static void enter_typed(GtkWidget *w, gpointer arg) 
{
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(arg);
    gint i;
    gchar *ret;
    const gchar *txt = gtk_entry_get_text(GTK_ENTRY(w));
        
    g_free(history[HISTORY_LEN-1]);
    for (i = HISTORY_LEN-1; i > 0; i--)
        history[i] = history[i-1];
    history[0] = g_strdup(txt);
    history_line = 0;

    ret = db_sql_command(txt);

    if (ret == NULL || ret[0] == 0)
        gtk_text_buffer_set_text(buffer, "OK", -1);
    else
        gtk_text_buffer_set_text(buffer, ret, -1);

    g_free(ret);

    gtk_entry_set_text(GTK_ENTRY(w), "");
}

static gboolean key_press(GtkWidget *widget, GdkEventKey *event, gpointer userdata)
{
    if (event->keyval == GDK_Up ||
        event->keyval == GDK_KP_Up) {
        if (history[history_line]) {
            gtk_entry_set_text(GTK_ENTRY(widget), history[history_line]);
            if (history_line < HISTORY_LEN - 1)
                history_line++;
        }
        return TRUE;
    } else if (event->keyval == GDK_Down ||
               event->keyval == GDK_KP_Down) {
        if (history_line > 0) {
            history_line--;
            gtk_entry_set_text(GTK_ENTRY(widget), history[history_line]);
        } else
            gtk_entry_set_text(GTK_ENTRY(widget), "");
        return TRUE;
    }
    return FALSE;
}

static void run_script(GtkWidget *w, GdkEventButton *event, gpointer *arg) 
{
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(arg);
    GtkWidget *dialog;
    GtkFileFilter *filter = gtk_file_filter_new();
    GtkFileFilter *filter2 = gtk_file_filter_new();
    gboolean run = FALSE;

    gtk_file_filter_add_pattern(filter, "*.bas");
    gtk_file_filter_set_name(filter, _("Basic Scripts"));
    gtk_file_filter_add_pattern(filter2, "*.svg");
    gtk_file_filter_set_name(filter2, _("SVG+Lisp"));

    dialog = gtk_file_chooser_dialog_new (_("Select Script"),
                                          GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);
    if (filename) {
	gchar *dirname = g_path_get_dirname(filename);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
	g_free(dirname);
    }

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter2);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        g_free(filename);
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_text_buffer_set_text(buffer, "", -1);
        run = TRUE;
        //gtk_widget_set_sensitive(GTK_WIDGET(repeat), filename != NULL)
    }

    gtk_widget_destroy (dialog);

    if (run) {
	if (strstr(filename, ".svg"))
	    svg_lisp_window(filename, buffer);
	else
	    run_basic_script(filename, buffer);
    }
}

static void repeat_script(GtkWidget *w, GdkEventButton *event, gpointer *arg) 
{
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(arg);

    if (filename) {
        gtk_text_buffer_set_text(buffer, "", -1);

	if (strstr(filename, ".svg"))
	    svg_lisp_window(filename, buffer);
	else
	    run_basic_script(filename, buffer);
    } else
        gtk_text_buffer_set_text(buffer, _("No script available"), -1);
}

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include "windows.h"
#include "shellapi.h"
#endif /* WIN32 */

void help_script(GtkWidget *w, gpointer data)
{
    gchar *docfile = g_build_filename(installation_dir, "doc", 
                                      "sql-guide-en.pdf", NULL);
#ifdef WIN32
    ShellExecute(NULL, TEXT("open"), docfile, NULL, ".\\", SW_SHOWMAXIMIZED);
#else /* ! WIN32 */
    gchar *cmd;
    cmd = g_strdup_printf("if which acroread ; then PDFR=acroread ; "
                          "elif which evince ; then PDFR=evince ; "
                          "else PDFR=xpdf ; fi ; $PDFR \"%s\" &", docfile);
    system(cmd);
    g_free(cmd);
#endif /* ! WIN32 */
    g_free(docfile);
}

void sql_window(GtkWidget *w, gpointer data)
{
    GtkTextBuffer *buffer;
    GtkWidget *vbox, *hbox, *line, *result, *script, *repeat, *help;
    GtkWindow *window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_title(GTK_WINDOW(window), _("SQL"));
    gtk_widget_set_size_request(GTK_WIDGET(window), SIZEX, SIZEY);
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(main_window));
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);

#if (GTKVER == 3)
    vbox = gtk_grid_new();
    hbox = gtk_grid_new();
#else
    vbox = gtk_vbox_new(FALSE, 1);
    hbox = gtk_hbox_new(FALSE, 1);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
#endif
    gtk_container_add (GTK_CONTAINER (window), vbox);

    result = gtk_text_view_new();
    line = gtk_entry_new();
    script = gtk_button_new_with_label(_("Run Script"));
    repeat = gtk_button_new_with_label(_("Repeat Script"));
    help = gtk_button_new_with_label(_("Help"));

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
#if (GTKVER == 3) && GTK_CHECK_VERSION(3,8,0)
    gtk_container_add(GTK_CONTAINER(scrolled_window), result);
#else
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), result);
#endif

#if (GTKVER == 3)
    gtk_grid_attach(GTK_GRID(vbox), scrolled_window, 0, 0, 1, 1);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_grid_attach(GTK_GRID(vbox), line,            0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(hbox), script,          0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(hbox), repeat,          1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(hbox), help,            2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(vbox), hbox,            0, 2, 1, 1);
#else
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), line, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), script, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), repeat, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), help, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
#endif
    gtk_widget_show_all(GTK_WIDGET(window));
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(result));

    PangoFontDescription *font_desc = pango_font_description_from_string("Courier 10");
#if (GTKVER == 3)
    gtk_widget_override_font (GTK_WIDGET(result), font_desc);
#else
    gtk_widget_modify_font(GTK_WIDGET(result), font_desc);
#endif
    pango_font_description_free (font_desc);

    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (delete_event_sql), NULL);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (destroy_sql), NULL);

    g_signal_connect(G_OBJECT(line), "activate", G_CALLBACK(enter_typed), buffer);
    g_signal_connect(G_OBJECT(line), "key-press-event", G_CALLBACK(key_press), buffer); 
    g_signal_connect(G_OBJECT(script), "button-press-event", G_CALLBACK(run_script), buffer);
    g_signal_connect(G_OBJECT(repeat), "button-press-event", G_CALLBACK(repeat_script), buffer);
    g_signal_connect(G_OBJECT(help), "button-press-event", G_CALLBACK(help_script), buffer);

    gtk_entry_set_text(GTK_ENTRY(line), "SELECT * FROM sqlite_master");
    gtk_widget_grab_focus(GTK_WIDGET(line));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(result), FALSE);

    //gtk_widget_set_sensitive(GTK_WIDGET(repeat), filename != NULL);
}

