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
#include <gdk/gdkkeysyms.h> 

#include "judoshiai.h"

gint webpwcrc32;

void set_webpassword_dialog(GtkWidget *w, gpointer data )
{
    GtkWidget *dialog, *label, *password, *hbox;

    dialog = gtk_dialog_new_with_buttons (_("Password"),
					  GTK_WINDOW(main_window),
                                          GTK_DIALOG_MODAL,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);

    label = gtk_label_new(_("Password:"));
    password = gtk_entry_new();
    hbox = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(hbox), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(hbox), password, 1, 0, 1, 1);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 
                       hbox, FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
        const gchar *pw = gtk_entry_get_text(GTK_ENTRY(password));
        webpwcrc32 = pwcrc32((guchar *)pw, strlen(pw));
        g_key_file_set_integer(keyfile, "preferences", "webpassword", webpwcrc32);
    }

    gtk_widget_destroy(dialog);
}
