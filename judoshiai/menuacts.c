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
#include <locale.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "sqlite3.h"
#include "judoshiai.h"
#include "cJSON.h"

//extern void test_judoshiai(void);

extern gboolean auto_arrange;

void get_from_old_competition(GtkWidget *w, gpointer data);
void get_weights_from_old_competition(GtkWidget *w, gpointer data);
void get_weights_from_old_competition_txt(GtkWidget *w, gpointer data);
void start_help(GtkWidget *w, gpointer data);

static gchar *backup_directory = NULL;
gboolean mirror_display = FALSE;

void about_shiai( GtkWidget *w, gpointer data)
{
    //test_judoshiai();
    //return;
    gtk_show_about_dialog (NULL, 
                           "name", "JudoShiai",
                           "title", "About JudoShiai",
                           "copyright", "Copyright 2006-2023 Hannu Jokinen",
                           "version", full_version(),
                           "website", "https://www.judoshiai.org/",
                           NULL);
}

void new_shiai(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new (_("Tournament"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *name;
                
        name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (strstr(name, ".shi") == NULL) {
            sprintf(database_name, "%s.shi", name);
        } else {
            sprintf(database_name, "%s", name);
        }
        snprintf(logfile_name, sizeof(logfile_name), "%s", database_name);
        gchar *p = strstr(logfile_name, ".shi");
        if (p)
            strcpy(p, ".log");
        else
            logfile_name[0] = 0;

        g_free (name);

        valid_ascii_string(database_name);

        db_new(database_name);

        gtk_widget_destroy (dialog);

        open_shiai_display();
        open_properties(NULL, NULL);
        set_match_col_titles();
        return;
    }
        
    gtk_widget_destroy (dialog);
    set_match_col_titles();
}

void open_shiai_from_net(GtkWidget *w, gpointer data)
{
    GtkWidget *places[NUM_OTHERS];
    GtkWidget *dialog;
#if (GTKVER == 3)
    GtkWidget *table = gtk_grid_new();
#else
    GtkWidget *table = gtk_table_new(1, NUM_OTHERS, FALSE);
#endif
    gint i;

    dialog = gtk_file_chooser_dialog_new (_("Tournament from the net"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    for (i = 0; i < NUM_OTHERS; i++) {
        places[i] = gtk_check_button_new_with_label(other_info(i));
#if (GTKVER == 3)
        gtk_grid_attach(GTK_GRID(table), places[i], 0, i, 1, 1);
#else
        gtk_table_attach_defaults(GTK_TABLE(table), places[i], 0, 1, i, i+1);
#endif
    }

    gtk_widget_show_all(table);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), table);

    if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *name;

        for (i = 0; i < NUM_OTHERS; i++) {
            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(places[i])))
                break;
        }
                 
        if (i >= NUM_OTHERS)
            goto out;

        name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (strstr(name, ".shi") == NULL) {
            sprintf(database_name, "%s.shi", name);
        } else {
            sprintf(database_name, "%s", name);
        }
        snprintf(logfile_name, sizeof(logfile_name), "%s", database_name);
        gchar *p = strstr(logfile_name, ".shi");
        if (p)
            strcpy(p, ".log");
        else
            logfile_name[0] = 0;

        g_free (name);

        valid_ascii_string(database_name);

        if (read_file_from_net(database_name, i))
            goto out;

        open_shiai_display();
    }
out:
    gtk_widget_destroy (dialog);        
}

void open_shiai(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;
    GtkFileFilter *filter = gtk_file_filter_new();

    gtk_file_filter_add_pattern(filter, "*.shi");
    gtk_file_filter_set_name(filter, _("Contests"));

    dialog = gtk_file_chooser_dialog_new (_("Contest"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar *name;

        name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        sprintf(database_name, "%s", name);
        snprintf(logfile_name, sizeof(logfile_name), "%s", database_name);
        gchar *p = strstr(logfile_name, ".shi");
        if (p)
            strcpy(p, ".log");
        else
            logfile_name[0] = 0;

        g_free (name);

#if (GTKVER == 3)
        gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(dialog)), wait_cursor);
        open_shiai_display();
        gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(dialog)), NULL);
#else
        gdk_window_set_cursor(GTK_WIDGET(dialog)->window, wait_cursor);
        open_shiai_display();
        gdk_window_set_cursor(GTK_WIDGET(dialog)->window, NULL);
#endif
        valid_ascii_string(database_name);
    }

    gtk_widget_destroy (dialog);
}

void font_dialog(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;

#if (GTKVER == 3)
    dialog = gtk_font_chooser_dialog_new(_("Select sheet font"), NULL);
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), get_font_face());
#else
    dialog = gtk_font_selection_dialog_new (_("Select sheet font"));
    gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), get_font_face());
#endif
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
#if (GTKVER == 3)
        gchar *font = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
#else
        gchar *font = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));
#endif
        set_font(font);
	g_key_file_set_string(keyfile, "preferences", "sheetfont", font);
	draw_match_graph();
        mylog("font=%s\n", font);
    }

    gtk_widget_destroy(dialog);
}

void select_use_logo(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog, *phdr;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new(_("Choose a file"),
					 GTK_WINDOW(main_window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         "No Logo", 1000,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.[pP][nN][gG]");
    gtk_file_filter_set_name(filter, _("Picture Files"));
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    phdr = gtk_check_button_new_with_label(_("Print Headers"));
    gtk_widget_show(phdr);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), phdr);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(phdr), print_headers);

    gint r = gtk_dialog_run(GTK_DIALOG(dialog));

    if (r == 1000) { // no logo
        g_free(use_logo);
        use_logo = NULL;
        g_key_file_set_string(keyfile, "preferences", "logofile", "");
        gtk_widget_destroy(dialog);
        return;
    }

    if (r != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }
               
    g_free(use_logo);
    use_logo = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    print_headers = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(phdr));

    g_key_file_set_string(keyfile, "preferences", "logofile", use_logo);
    g_key_file_set_boolean(keyfile, "preferences", "printheaders", print_headers);

    gtk_widget_destroy(dialog);
}

void set_lang(GtkWidget *w, gpointer data)
{
    print_lang = ptr_to_gint(data);
    gchar l[3];
    l[0] = print_lang_names[print_lang][0];
    l[1] = print_lang_names[print_lang][1];
    l[2] = 0;
    g_key_file_set_string(keyfile, "preferences", "printlanguagestr", l);
}

void set_club_text(GtkWidget *w, gpointer data)
{
    club_text = ptr_to_gint(data);
    g_key_file_set_integer(keyfile, "preferences", "clubtext", club_text);
}

void set_club_abbr(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    club_abbr = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    club_abbr = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    g_key_file_set_integer(keyfile, "preferences", "clubabbr", club_abbr);
}

void toggle_automatic_sheet_update(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    automatic_sheet_update = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    automatic_sheet_update = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    g_key_file_set_boolean(keyfile, "preferences", "automatic_sheet_update", automatic_sheet_update);
}

void toggle_automatic_web_page_update(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    automatic_web_page_update = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    automatic_web_page_update = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    if (automatic_web_page_update)
        get_output_directory();
}

void toggle_weights_in_sheets(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    weights_in_sheets = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    weights_in_sheets = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    g_key_file_set_boolean(keyfile, "preferences", "weightvisible", weights_in_sheets);
}

void toggle_grade_visible(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    grade_visible = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    grade_visible = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    g_key_file_set_boolean(keyfile, "preferences", "gradevisible", grade_visible);
}

void toggle_name_layout(GtkWidget *menu_item, gpointer data)
{
    name_layout = ptr_to_gint(data);
    g_key_file_set_integer(keyfile, "preferences", "namelayout", name_layout);
}

void toggle_name_layout_format_first(GtkWidget *menu_item, gpointer data)
{
    name_layout_format_first = ptr_to_gint(data);
    gint x1 = g_key_file_get_integer(keyfile, "preferences", "namelayout_format_first", NULL);

    if (x1 != name_layout_format_first) {
        g_key_file_set_integer(keyfile, "preferences", "namelayout_format_first", name_layout_format_first);
        update_judoka_name_layout_format_first();
    }
}

void toggle_name_layout_format_last(GtkWidget *menu_item, gpointer data)
{
    name_layout_format_last = ptr_to_gint(data);
    gint x1 = g_key_file_get_integer(keyfile, "preferences", "namelayout_format_last", NULL);

    if (x1 != name_layout_format_last) {
        g_key_file_set_integer(keyfile, "preferences", "namelayout_format_last", name_layout_format_last);
        update_judoka_name_layout_format_last();
    }
}

void toggle_pool_style(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    pool_style = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    pool_style = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    g_key_file_set_boolean(keyfile, "preferences", "poolstyle", pool_style);
}

void toggle_belt_colors(GtkWidget *menu_item, gpointer data)
{
    belt_colors = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
    g_key_file_set_boolean(keyfile, "preferences", "beltcolors", pool_style);
}

void toggle_mirror(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    mirror_display = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    mirror_display = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    g_key_file_set_boolean(keyfile, "preferences", "mirror", mirror_display);
    refresh_window();
}

void toggle_auto_arrange(GtkWidget *menu_item, gpointer data)
{
#if (GTKVER == 3)
    auto_arrange = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
#else
    auto_arrange = GTK_CHECK_MENU_ITEM(menu_item)->active;
#endif
    g_key_file_set_boolean(keyfile, "preferences", "autoarrange", auto_arrange);
}

static gboolean with_weight;

static void get_competitors(void)
{
    GtkWidget *dialog;
    GtkWidget *cleanup, *only_weighted, *with_weights, *vbox;
    GtkFileFilter *filter = gtk_file_filter_new();
    gint added = 0, not_added = 0;

    gtk_file_filter_add_pattern(filter, "*.shi");
    gtk_file_filter_set_name(filter, _("Tournaments"));

    dialog = gtk_file_chooser_dialog_new (_("Add competitors"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

#if (GTKVER == 3)
    vbox = gtk_grid_new();
#else
    vbox = gtk_vbox_new(FALSE, 0);
#endif
    cleanup = gtk_check_button_new_with_label(_("Clean up duplicates and update reg. categories"));
    gtk_widget_show(cleanup);
    only_weighted = gtk_check_button_new_with_label(_("Weighed only"));
    gtk_widget_show(only_weighted);
    with_weights = gtk_check_button_new_with_label(_("With weights"));
    gtk_widget_show(with_weights);
#if (GTKVER == 3)
    gtk_grid_attach(GTK_GRID(vbox), with_weights,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(vbox), only_weighted, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(vbox), cleanup,       0, 2, 1, 1);
#else
    gtk_box_pack_start(GTK_BOX(vbox), with_weights, FALSE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), only_weighted, FALSE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), cleanup, FALSE, TRUE, 2);
#endif
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), vbox);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar *name;

        name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        db_add_competitors(name, 
                           gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(with_weights)), 
                           gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(only_weighted)), 
                           gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cleanup)), 
                           &added, &not_added);
        valid_ascii_string(name);
        g_free (name);

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cleanup)))
            SHOW_MESSAGE("%d %s (%d %s).", 
                         added, _("competitors added and updated"), not_added, _("competitors already existed"));
        else
            SHOW_MESSAGE("%d %s.", added, _("competitors added unchanged"));
    }

    gtk_widget_destroy (dialog);        
}


void get_from_old_competition(GtkWidget *w, gpointer data)
{
    with_weight = FALSE;
    get_competitors();
}

void get_weights_from_old_competition(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;
    GtkFileFilter *filter = gtk_file_filter_new();
    gint weight_updated = 0;

    gtk_file_filter_add_pattern(filter, "*.shi");
    gtk_file_filter_set_name(filter, _("Tournaments"));

    dialog = gtk_file_chooser_dialog_new (_("Copy Weights"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (database_name[0] == 0) {
        if (current_directory[0] != '.')
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_directory);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar *name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        db_update_weights(name, &weight_updated);
        valid_ascii_string(name);
        g_free (name);

        SHOW_MESSAGE("%d %s.", weight_updated, _("weights updated"));
    }

    gtk_widget_destroy (dialog);        
}

#define CH  ((i < len) ? contents[i] : 0)
#define GET_WORD while (CH && CH <= ' ') i++
#define GET_NUM while (CH && (CH < '0' || CH > '9')) i++
#define NEXT_LINE do { while (CH && CH != '\r' && CH != '\n') i++; GET_WORD; } while (0)
#define COPY_WORD(_dst) do { gint _d = 0; while (CH && CH > ' ' && _d < sizeof(_dst)-1) { \
            _dst[_d++] = CH; i++; } _dst[_d] = 0; } while(0)

#define JSON_GET_VAL(_root, _item) cJSON *_item = cJSON_GetObjectItem(_root, #_item)
#define JSON_GET_STR(_root, _item)                        \
    const gchar *_item;                                   \
    do { cJSON *tmp = cJSON_GetObjectItem(_root, #_item); \
        _item = tmp ? tmp->valuestring : NULL; } while (0)
#define JSON_GET_INT(_root, _item)                        \
    int _item;                                            \
    do { cJSON *tmp = cJSON_GetObjectItem(_root, #_item); \
        _item = tmp ? tmp->valueint : 0; } while (0)
#define JSON_GET_DST_STR(_root, _dst, _item)                  \
    do { cJSON *tmp = cJSON_GetObjectItem(_root, #_item); \
        _dst = tmp ? tmp->valuestring : ""; } while (0)
#define JSON_GET_DST_INT(_root, _dst, _item)                  \
    do { cJSON *tmp = cJSON_GetObjectItem(_root, #_item); \
        _dst = tmp ? tmp->valueint : 0; } while (0)
#define JSON_PUT_DST_STR(_c, _src, _item)                  \
    do { cJSON_AddStringToObject(_c, #_item, _src); } while (0)
#define JSON_PUT_DST_INT(_c, _src, _item)                  \
    do { cJSON_AddNumberToObject(_c, #_item, _src); } while (0)

static gboolean ok_all = FALSE;

gboolean json_fill_judoka_struct(cJSON *root, struct judoka *j)
{
    gboolean is_new = FALSE;
    
    memset(j, 0, sizeof(*j));
    JSON_GET_DST_STR(root, j->last, last);
    JSON_GET_DST_STR(root, j->first, first);
    JSON_GET_DST_STR(root, j->club, club);
    JSON_GET_DST_STR(root, j->regcategory, regcat);
    JSON_GET_DST_STR(root, j->category, category);
    JSON_GET_DST_STR(root, j->country, country);
    JSON_GET_DST_STR(root, j->id, id);
    JSON_GET_DST_STR(root, j->comment, comment);
    JSON_GET_DST_STR(root, j->coachid, coachid);
    JSON_GET_DST_INT(root, j->index, ix);
    JSON_GET_DST_INT(root, j->birthyear, birthyear);
    JSON_GET_DST_INT(root, j->belt, belt);
    JSON_GET_DST_INT(root, j->weight, weight);
    JSON_GET_DST_INT(root, j->deleted, flags);
    JSON_GET_DST_INT(root, j->seeding, seeding);
    JSON_GET_DST_INT(root, j->clubseeding, clubseeding);
    JSON_GET_DST_INT(root, j->gender, gender);
    j->visible = TRUE;

    if (j->gender == IS_FEMALE)
        j->deleted |= GENDER_FEMALE;
    else if (j->gender == IS_MALE)
        j->deleted |= GENDER_MALE;

    struct judoka *j1 = get_data(j->index);
    if (j1)
        free_judoka(j1);
    else
        is_new = TRUE;

    if (is_new) {
        j->index = comp_index_get_free();
        j->category = "?";
    }

    return is_new;
}

gint json_edit_or_create_judoka(cJSON *root)
{
    struct judoka j;
    gboolean is_new = json_fill_judoka_struct(root, &j);

    if (is_new) {
        mylog("Adding [%d] %s\n", j.index, j.last);
        gint rc = db_add_judoka(j.index, &j);
        if (rc == SQLITE_OK)
            display_one_judoka(&j);
        else
            mylog("Could not add [%d] %s\n", j.index, j.last);
    } else {
        mylog("Update [%d] %s\n", j.index, j.last);
        db_update_judoka(j.index, &j);
        display_one_judoka(&j);
    }

    return j.index;
}

void json_create_new_judoka(cJSON *root, gboolean ok)
{
    struct judoka j1;
    gboolean is_new = json_fill_judoka_struct(root, &j1);

    if (!is_new)
        return;
    
    GtkTreeIter iter;
    if (find_iter_name_2(&iter, j1.last, j1.first, j1.club, j1.regcategory))
        return;
    
    if (ok == FALSE && ok_all == FALSE) {
        GtkWidget *dialog, *box;
        dialog = gtk_dialog_new_with_buttons (_("Add competitor"),
                                              GTK_WINDOW(main_window),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_OK, GTK_RESPONSE_OK,
                                              GTK_STOCK_SELECT_ALL, GTK_RESPONSE_APPLY,
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              NULL);
        box = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(box), 5);
        gtk_grid_set_column_spacing(GTK_GRID(box), 5);
        gtk_grid_attach(GTK_GRID(box), gtk_label_new(j1.regcategory), 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(box), gtk_label_new(j1.last), 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(box), gtk_label_new(j1.first), 2, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(box), gtk_label_new(j1.club), 3, 0, 1, 1);
        gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                           box, FALSE, FALSE, 0);
            
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
        gtk_widget_show_all(dialog);

        gint resp = gtk_dialog_run (GTK_DIALOG (dialog));
        if (resp == GTK_RESPONSE_OK || resp == GTK_RESPONSE_APPLY) {
            if (resp == GTK_RESPONSE_APPLY) ok_all = TRUE;
            ok = TRUE;
        }
        gtk_widget_destroy (dialog);
    }

    if (ok || ok_all) {
        mylog("Adding [%d] %s\n", j1.index, j1.last);
        gint rc = db_add_judoka(j1.index, &j1);
        if (rc == SQLITE_OK)
            display_one_judoka(&j1);
        else
            mylog("Could not add [%d] %s\n", j1.index, j1.last);
    }
}

void json_set_weight(cJSON *root)
{
    JSON_GET_STR(root, id);
    JSON_GET_INT(root, ix);
    JSON_GET_INT(root, weight);
    if (!id && ix == 0)
        return;
    mylog("json id=%s ix=%d weight=%d\n", id ? id : "NULL", ix, weight);
    
    struct judoka *j = NULL;
    if (ix) j = get_data(ix);
    else {
        gboolean coach;
        gint indx = db_get_index_by_id(id, &coach);
        if (indx) j = get_data(indx);
    }
    if (!j) {
        /* This is a new judoka */
        json_create_new_judoka(root, FALSE);
        return;
    }

    if (weight) {
        j->weight = weight;
        display_one_judoka(j);
        db_update_judoka(j->index, j);
    }
    free_judoka(j);
}

void get_comp_data_from_json_file(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;

    ok_all = FALSE;
    
    dialog = gtk_file_chooser_dialog_new (_("Choose a directory"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *dirname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        GDir *dir = g_dir_open(dirname, 0, NULL);
        if (dir) {
            const gchar *fname = g_dir_read_name(dir);
            while (fname) {
                gchar *fullname = g_build_filename(dirname, fname, NULL);
		gchar *contents;
		gsize len;
		if (g_file_get_contents(fullname, &contents, &len, NULL)) {
                    gint i = 0;

                    /* json format */
                    cJSON *root = cJSON_Parse(&contents[i]);
                    if (root) {
                        if (root->type == cJSON_Object)
                            json_set_weight(root);
                        else if (root->type == cJSON_Array) {
                            cJSON *e = root->child;
                            while (e) {
                                json_set_weight(e);
                                e = e->next;
                            }
                        }
                        cJSON_Delete(root);
                    }
                    g_free(contents);
                }
                g_free(fullname);
                fname = g_dir_read_name(dir);
            }
        }
        g_free (dirname);
    }
    gtk_widget_destroy (dialog);
}

void json_add_judoka(cJSON *root, struct judoka *j)
{
    cJSON *c = cJSON_CreateObject();
    cJSON_AddItemToArray(root, c);
    JSON_PUT_DST_INT(c, j->index, ix);
    if (IS_LANG_IS) {
        JSON_PUT_DST_STR(c, j->first, first);
        JSON_PUT_DST_STR(c, j->last, last);
    } else {
        JSON_PUT_DST_STR(c, j->last, last);
        JSON_PUT_DST_STR(c, j->first, first);
    }
    JSON_PUT_DST_STR(c, j->club, club);
    JSON_PUT_DST_STR(c, j->regcategory, regcat);
    JSON_PUT_DST_STR(c, j->category, category);
    JSON_PUT_DST_STR(c, j->country, country);
    JSON_PUT_DST_STR(c, j->id, id);
    JSON_PUT_DST_STR(c, j->comment, comment);
    JSON_PUT_DST_STR(c, j->coachid, coachid);
    JSON_PUT_DST_INT(c, j->birthyear, birthyear);
    JSON_PUT_DST_INT(c, j->belt, belt);
    JSON_PUT_DST_INT(c, j->weight, weight);
    JSON_PUT_DST_INT(c, j->deleted, flags);
    JSON_PUT_DST_INT(c, j->seeding, seeding);
    JSON_PUT_DST_INT(c, j->clubseeding, clubseeding);
    JSON_PUT_DST_INT(c, j->gender, gender);
}

void put_comp_data_to_json_file(GtkWidget *w, gpointer data)
{
    extern void db_comp_print_json(cJSON *root);
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new (_("Choose a directory"),
					  GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        FILE *f = g_fopen(filename, "w");
        if (f) {
            cJSON *root = cJSON_CreateArray();
            db_comp_print_json(root);
            char *out = cJSON_PrintUnformatted(root);
            fputs(out, f);
            fclose(f);
        }
        g_free(filename);
    }

    gtk_widget_destroy (dialog);

}

gboolean show_colors = FALSE;

void toggle_show_colors(GtkWidget *menu_item, gpointer data)
{
    show_colors = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
    g_key_file_set_boolean(keyfile, "preferences", "showcolors", show_colors);
    draw_match_graph();
    refresh_window();
}



#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include "windows.h"
#include "shellapi.h"
#endif /* WIN32 */

#if 0
#ifndef WIN32

static int run_command( const gchar *szCmd ) {

    if ( system( szCmd ) < 0 ) {
        perror("Error launching browser");
        return -1;
    }

    return 0;
}
#endif

static void open_url(const char *szURL) {

#ifdef WIN32

    ShellExecute( NULL, TEXT("open"), szURL, NULL, ".\\", SW_SHOWMAXIMIZED );

#else /* ! WIN32 */

    gchar *pchBrowser;
    gchar *pchTemp;
    const gchar *pch;
    int rc;

    if ( ( pch = g_getenv( "BROWSER" ) ) )
        pchBrowser = g_strdup( pch );
    else {
#ifdef __APPLE__
        pchBrowser = g_strdup( "open %s" );
#else
        pchBrowser = g_strdup( "mozilla \"%s\" &");
#endif
    }

    pchTemp = g_strdup_printf(pchBrowser, szURL);
    rc = run_command(pchTemp);
    g_free(pchTemp);
    g_free(pchBrowser);

#endif /* ! WIN32 */
}
#endif


void backup_shiai(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog, *do_backup;

    dialog = gtk_file_chooser_dialog_new(_("Choose a directory"),
					 GTK_WINDOW(main_window),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    do_backup = gtk_check_button_new_with_label(_("Do Backup"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(do_backup), TRUE);
    gtk_widget_show(do_backup);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), do_backup);

    if (backup_directory)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            backup_directory);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }

    g_free(backup_directory);
    backup_directory = NULL;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(do_backup))) {
        backup_directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        make_backup();
    } else
        show_note(" ");

    valid_ascii_string(backup_directory);

    gtk_widget_destroy (dialog);
}

#define NUM_BACKUPS 16

void make_backup(void)
{
    static gchar buf[1024];
    static gchar *filenames[NUM_BACKUPS];
    static gint ix = 0;

    if (!backup_directory)
        return;

    time_t t = time(NULL);

    struct tm *tm = localtime((time_t *)&t);
    if (tm)
	    sprintf(buf, "shiai_%04d%02d%02d_%02d%02d%02d.shi",
	            tm->tm_year+1900, 
	            tm->tm_mon+1,
	            tm->tm_mday,
	            tm->tm_hour,
	            tm->tm_min,
	            tm->tm_sec);

    filenames[ix] = g_build_filename(backup_directory, buf, NULL);

    FILE *f = g_fopen(filenames[ix], "wb");
    if (f) {
        gint n;
        FILE *db = g_fopen(database_name, "rb");
        if (!db) {
            fclose(f);
            return;
        }

        while ((n = fread(buf, 1, sizeof(buf), db)) > 0) {
            fwrite(buf, 1, n, f);
        }

        fclose(db);
        fclose(f);
    } else {
        show_note("%s %s", _("CANNOT OPEN BACKUP FILE"), filenames[ix]);		
    }

    if (++ix >= NUM_BACKUPS)
        ix = 0;

    if (filenames[ix]) {
        g_unlink(filenames[ix]);
        g_free(filenames[ix]);
        filenames[ix] = NULL;
    }
}
