/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2019 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <cairo.h>
#include <cairo-pdf.h>

#include "judoshiai.h"

gint gdpr_comp_age = 0, gdpr_cat_age = 0;
gint gdpr_enable = 0;

void set_gdpr(GtkWidget *menuitem, gpointer userdata)
{
    GtkWidget *dialog, *comp_age_w, *cat_age_w;
    GtkWidget *table = gtk_grid_new();
    gchar buf[8];

    dialog = gtk_dialog_new_with_buttons (_("General Data Protection Regulation"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                          NULL);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Hide category if age less than")),  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Hide name if age less than")),      0, 1, 1, 1);
    comp_age_w = gtk_entry_new();
    cat_age_w = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(table), cat_age_w, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table), comp_age_w, 1, 1, 1, 1);
    snprintf(buf, sizeof(buf), "%d", gdpr_comp_age);
    gtk_entry_set_text(GTK_ENTRY(comp_age_w), buf);
    snprintf(buf, sizeof(buf), "%d", gdpr_cat_age);
    gtk_entry_set_text(GTK_ENTRY(cat_age_w), buf);
    gtk_widget_show_all(table);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 
                       table, FALSE, FALSE, 0);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
	gdpr_comp_age = atoi(gtk_entry_get_text(GTK_ENTRY(comp_age_w)));
	gdpr_cat_age = atoi(gtk_entry_get_text(GTK_ENTRY(cat_age_w)));
        g_key_file_set_integer(keyfile, "preferences", "gdprcompage", gdpr_comp_age);
        g_key_file_set_integer(keyfile, "preferences", "gdprcatage", gdpr_cat_age);
    }
    gtk_widget_destroy(dialog);
}

gboolean gdpr_ok(struct judoka *j)
{
    if (gdpr_enable == FALSE || j == NULL || j->index < 10)
	return TRUE;

    if (gdpr_enable < 0) {
	g_print("%s:%d Error, gdpr_enable=%d\n", __FUNCTION__, __LINE__, gdpr_enable);
	gdpr_enable = 0;
	return TRUE;
    }

    if (j->index >= 10000) {
	if (gdpr_cat_age <= 0)
	    return TRUE;

	gint age = find_max_age(j->last);
	if (age == 0) return TRUE;
	if (age + 1 >= gdpr_cat_age)
	    return TRUE;
	return FALSE;
    }

    if (j->deleted & DO_NOT_SHOW)
	return FALSE;

    if (gdpr_comp_age <= 0)
	return TRUE;

    if (current_year - j->birthyear >= gdpr_comp_age)
	return TRUE;
    return FALSE;
}
