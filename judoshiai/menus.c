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

#if (GTKVER == 3)
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

#include "judoshiai.h"
#include "language.h"

extern gint webpwcrc32;

extern void about_shiai(GtkWidget *w, gpointer data);
extern void new_shiai(GtkWidget *w, gpointer data);
extern void open_shiai_from_net(GtkWidget *w, gpointer data);
extern void open_shiai(GtkWidget *w, gpointer data);
extern void font_dialog(GtkWidget *w, gpointer data);
extern void set_lang(gpointer data, guint action, GtkWidget *w);
extern void set_club_text(gpointer data, guint action, GtkWidget *w);
extern void set_club_abbr(GtkWidget *menu_item, gpointer data);
extern void toggle_automatic_sheet_update(GtkWidget *menu_item, gpointer data);
extern void toggle_automatic_web_page_update(GtkWidget *menu_item, gpointer data);
extern void toggle_weights_in_sheets(GtkWidget *menu_item, gpointer data);
extern void toggle_grade_visible(GtkWidget *menu_item, gpointer data);
extern void toggle_name_layout(GtkWidget *menu_item, gpointer data);
extern void toggle_name_layout_format_first(GtkWidget *menu_item, gpointer data);
extern void toggle_name_layout_format_last(GtkWidget *menu_item, gpointer data);
extern void toggle_pool_style(GtkWidget *menu_item, gpointer data);
extern void toggle_belt_colors(GtkWidget *menu_item, gpointer data);
extern void toggle_col_visible(GtkWidget *menu_item, gpointer data);
extern void get_from_old_competition(GtkWidget *w, gpointer data);
extern void get_weights_from_old_competition(GtkWidget *w, gpointer data);
extern void get_comp_data_from_json_file(GtkWidget *w, gpointer data);
extern void put_comp_data_to_json_file(GtkWidget *w, gpointer data);
extern void start_help(GtkWidget *w, gpointer data);
extern void locate_to_tatamis(GtkWidget *w, gpointer data);
extern void set_tatami_state(GtkWidget *menu_item, gpointer data);
extern void backup_shiai(GtkWidget *w, gpointer data);
extern gchar *db_validation(GtkWidget *w, gpointer data);
extern void toggle_mirror(GtkWidget *menu_item, gpointer data);
extern void toggle_auto_arrange(GtkWidget *menu_item, gpointer data);
extern void select_use_logo(GtkWidget *w, gpointer data);
extern void set_serial_dialog(GtkWidget *w, gpointer data);
extern void serial_set_device(gchar *dev);
extern void serial_set_baudrate(gint baud);
extern void serial_set_type(gint type);
extern void print_weight_notes(GtkWidget *menuitem, gpointer userdata);
extern void ftp_to_server(GtkWidget *w, gpointer data);
extern void select_custom_dir(GtkWidget *menu_item, gpointer data);
extern void toggle_show_colors(GtkWidget *menu_item, gpointer data);

static GtkWidget *menubar, *submenu, *subsubmenu, *jsonsubmenu,
    *tournament_menu_item, *competitors_menu_item,
    *categories_menu_item, *drawing_menu_item, *results_menu_item,
    *judotimer_menu_item, *preferences_menu_item, *help_menu_item, *lang_menu_item,
    *tournament_menu, *competitors_menu,
    *categories_menu, *drawing_menu, *results_menu,
    *judotimer_menu, *preferences_menu, *help_menu, *sql_dialog,
    *tournament_new, *tournament_choose, *tournament_choose_net, *tournament_properties,
    *tournament_quit, *tournament_backup, *tournament_validation, *tournament_custom,
    *competitor_new, *competitor_search, *competitor_select_from_tournament, *competitor_add_from_text_file,
    *competitor_add_all_from_shiai, *competitor_update_weights, *competitor_remove_unweighted,
    *competitor_json, *competitor_read_json, *competitor_write_json,
    *competitor_restore_removed, *competitor_delete_removed, *competitor_bar_code_search, *competitor_print_weigh_notes, *competitor_print_with_template,
    *category_new, *category_team_new, *category_team_event_new, *category_remove_empty, *category_create_official,
    *category_colorize, *category_show_colors,
    *category_print_all, *category_print_all_pdf, *category_print_matches,
    *category_properties, *category_to_tatamis[NUM_TATAMIS],
    *draw_all_categories,
    *results_print_all, *results_print_schedule_printer, *results_print_schedule_pdf,
    *results_ftp, *results_gdpr,
    *preference_comm, *preference_comm_node, *preference_own_ip_addr, *preference_show_connections,
    *preference_auto_sheet_update, *preference_result_languages[NUM_PRINT_LANGS], *preference_langsel,
    *preference_weights_to_pool_sheets,
    *preference_grade_visible, *preference_name_layout, *preference_name_layout_0, *preference_name_layout_1,
    *preference_name_layout_2, *preference_name_layout_3,
    *preference_name_layout_first_format, *preference_name_layout_last_format,
    *preference_name_layout_first_uc, *preference_name_layout_first_fu, *preference_name_layout_first_au,
    *preference_name_layout_last_uc, *preference_name_layout_last_fu, *preference_name_layout_last_au,
    *preference_layout, *preference_pool_style, *preference_belt_colors,
    *preference_columns, *preference_show_defaults, *preference_show_col[NUM_COMP_COLS],
    *preference_sheet_font, *preference_svg, *preference_password, *judotimer_control[NUM_TATAMIS],
    *preference_mirror, *preference_auto_arrange, *preference_club_text,
    *preference_club_text_club, *preference_club_text_country, *preference_club_text_both,
    *preference_club_text_abbr, *preference_use_logo,
    *preference_serial, *preference_medal_matches,
    *help_manual, *help_about;

static GSList *lang_group = NULL, *club_group = NULL, *name_group = NULL,
              *first_name_group = NULL, *last_name_group = NULL;

void set_col_defaults(GtkWidget *menu_item, gpointer data)
{
    gint i;
    show_columns = DEFAULT_SHOW_COLUMNS;

    for (i = 0; i < NUM_COMP_COLS; i++) {
	gboolean new_state = show_columns & (1 << i);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_show_col[i]), new_state);
    }
    toggle_col_visible(NULL, gint_to_ptr(1));
}

GtkWidget *get_menubar_menu(GtkWidget  *window)
{
    GtkAccelGroup *group;
    gint i;
    gchar buf[64];

    group = gtk_accel_group_new ();
    menubar = gtk_menu_bar_new ();

    tournament_menu_item  = gtk_menu_item_new_with_label (_("Tournament"));
    competitors_menu_item = gtk_menu_item_new_with_label (_("Competitors"));
    categories_menu_item  = gtk_menu_item_new_with_label (_("Categories"));
    drawing_menu_item     = gtk_menu_item_new_with_label (_("Drawing"));
    results_menu_item     = gtk_menu_item_new_with_label (_("Results"));
    judotimer_menu_item   = gtk_menu_item_new_with_label (_("JudoTimer"));
    preferences_menu_item = gtk_menu_item_new_with_label (_("Preferences"));
    help_menu_item        = gtk_menu_item_new_with_label (_("Help"));
    lang_menu_item        = get_language_menu(window, change_language);

    tournament_menu  = gtk_menu_new();
    competitors_menu = gtk_menu_new();
    categories_menu  = gtk_menu_new();
    drawing_menu     = gtk_menu_new();
    results_menu     = gtk_menu_new();
    judotimer_menu   = gtk_menu_new();
    preferences_menu = gtk_menu_new();
    help_menu        = gtk_menu_new();

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(tournament_menu_item), tournament_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(competitors_menu_item), competitors_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(categories_menu_item), categories_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(drawing_menu_item), drawing_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(results_menu_item), results_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(judotimer_menu_item), judotimer_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preferences_menu_item), preferences_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_item), help_menu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), tournament_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), competitors_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), categories_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), drawing_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), results_menu_item);
    //gtk_menu_shell_append(GTK_MENU_SHELL(menubar), judotimer_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), preferences_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), lang_menu_item);

    /* Create the Tournament menu content. */
    tournament_new        = gtk_menu_item_new_with_label(_("New Tournament"));
    tournament_choose     = gtk_menu_item_new_with_label(_("Open Tournament"));
    tournament_choose_net = gtk_menu_item_new_with_label(_("Open Tournament from Net"));
    tournament_properties = gtk_menu_item_new_with_label(_("Properties"));
    tournament_backup     = gtk_menu_item_new_with_label(_("Tournament Backup"));
    tournament_validation = gtk_menu_item_new_with_label(_("Validate Database"));
    tournament_custom     = gtk_menu_item_new_with_label(_("Load Custom Systems"));
    sql_dialog            = gtk_menu_item_new_with_label(_("SQL Dialog"));
    tournament_quit       = gtk_menu_item_new_with_label(_("Quit"));

    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_choose);
    //gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_choose_net);
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_properties);
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_backup);
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_validation);
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_custom);
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), sql_dialog);
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(tournament_menu), tournament_quit);

    g_signal_connect(G_OBJECT(tournament_new),        "activate", G_CALLBACK(new_shiai), 0);
    g_signal_connect(G_OBJECT(tournament_choose),     "activate", G_CALLBACK(open_shiai), 0);
    g_signal_connect(G_OBJECT(tournament_choose_net), "activate", G_CALLBACK(open_shiai_from_net), 0);
    g_signal_connect(G_OBJECT(tournament_properties), "activate", G_CALLBACK(open_properties), 0);
    g_signal_connect(G_OBJECT(tournament_backup),     "activate", G_CALLBACK(backup_shiai), 0);
    g_signal_connect(G_OBJECT(tournament_validation), "activate", G_CALLBACK(db_validation), 0);
    g_signal_connect(G_OBJECT(tournament_custom),     "activate", G_CALLBACK(select_custom_dir), 0);
    g_signal_connect(G_OBJECT(sql_dialog),            "activate", G_CALLBACK(sql_window), 0);
    g_signal_connect(G_OBJECT(tournament_quit),       "activate", G_CALLBACK(destroy), 0);

    gtk_widget_add_accelerator(tournament_quit, "activate", group, GDK_Q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    /* Create the Competitors menu content. */
    competitor_new                  = gtk_menu_item_new_with_label(_("New Competitor"));
    competitor_search               = gtk_menu_item_new_with_label(_("Search Competitor"));
    competitor_select_from_tournament  = gtk_menu_item_new_with_label(_("Select From Another Shiai"));
    competitor_add_from_text_file   = gtk_menu_item_new_with_label(_("Add From a Text File"));
    competitor_add_all_from_shiai   = gtk_menu_item_new_with_label(_("Add All From Another Shiai"));
    competitor_update_weights       = gtk_menu_item_new_with_label(_("Update Weights From Another Shiai"));
    competitor_json                 = gtk_menu_item_new_with_label(_("JSON"));
    competitor_read_json            = gtk_menu_item_new_with_label(_("Read From Directory"));
    competitor_write_json           = gtk_menu_item_new_with_label(_("Write To File"));
    competitor_remove_unweighted    = gtk_menu_item_new_with_label(_("Remove Unweighted"));
    competitor_restore_removed      = gtk_menu_item_new_with_label(_("Restore Removed"));
    competitor_delete_removed       = gtk_menu_item_new_with_label(_("Delete Removed"));
    competitor_bar_code_search      = gtk_menu_item_new_with_label(_("Bar Code Search"));
    competitor_print_weigh_notes    = gtk_menu_item_new_with_label(_("Print Weight Notes"));
    competitor_print_with_template  = gtk_menu_item_new_with_label("");

    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_search);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_select_from_tournament);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_add_from_text_file);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_add_all_from_shiai);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_update_weights);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_remove_unweighted);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_restore_removed);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_delete_removed);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_bar_code_search);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_print_weigh_notes);
    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), gtk_separator_menu_item_new());

    gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_json);
    jsonsubmenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(competitor_json), jsonsubmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(jsonsubmenu), competitor_read_json);
    gtk_menu_shell_append(GTK_MENU_SHELL(jsonsubmenu), competitor_write_json);

    //gtk_menu_shell_append(GTK_MENU_SHELL(competitors_menu), competitor_print_with_template);

    g_signal_connect(G_OBJECT(competitor_new),                 "activate", G_CALLBACK(new_judoka), 0);
    g_signal_connect(G_OBJECT(competitor_search),              "activate", G_CALLBACK(search_competitor), 0);
    g_signal_connect(G_OBJECT(competitor_select_from_tournament), "activate", G_CALLBACK(set_old_shiai_display), 0);
    g_signal_connect(G_OBJECT(competitor_add_from_text_file),  "activate", G_CALLBACK(import_txt_dialog), 0);
    g_signal_connect(G_OBJECT(competitor_add_all_from_shiai),  "activate", G_CALLBACK(get_from_old_competition), 0);
    g_signal_connect(G_OBJECT(competitor_update_weights),      "activate", G_CALLBACK(get_weights_from_old_competition), 0);
    g_signal_connect(G_OBJECT(competitor_read_json),           "activate", G_CALLBACK(get_comp_data_from_json_file), 0);
    g_signal_connect(G_OBJECT(competitor_write_json),          "activate", G_CALLBACK(put_comp_data_to_json_file), 0);
    g_signal_connect(G_OBJECT(competitor_remove_unweighted),   "activate", G_CALLBACK(remove_unweighed_competitors), 0);
    g_signal_connect(G_OBJECT(competitor_restore_removed),     "activate", G_CALLBACK(db_restore_removed_competitors), 0);
    g_signal_connect(G_OBJECT(competitor_delete_removed),      "activate", G_CALLBACK(db_delete_removed_competitors), 0);
	
    g_signal_connect(G_OBJECT(competitor_bar_code_search),     "activate", G_CALLBACK(barcode_search), 0);
    g_signal_connect(G_OBJECT(competitor_print_weigh_notes),   "activate", G_CALLBACK(print_weight_notes), NULL);

    gtk_widget_add_accelerator(competitor_new, "activate", group, GDK_N, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(competitor_search, "activate", group, GDK_F, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(competitor_add_from_text_file, "activate", group, GDK_I, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    /* Create the Category menu content. */
    category_new             = gtk_menu_item_new_with_label(_("New Category"));
    category_team_event_new  = gtk_menu_item_new_with_label("");
    category_team_new        = gtk_menu_item_new_with_label("");
    category_remove_empty    = gtk_menu_item_new_with_label(_("Remove Empty"));
    category_create_official = gtk_menu_item_new_with_label(_("Create Categories"));
    category_colorize        = gtk_menu_item_new_with_label(_("Init Colors"));
    category_show_colors     = gtk_check_menu_item_new_with_label(_("Show Colors"));
    category_print_all       = gtk_menu_item_new_with_label(_("Print All"));
    category_print_all_pdf   = gtk_menu_item_new_with_label(_("Print All (PDF)"));
    category_print_matches   = gtk_menu_item_new_with_label(_("Print Matches (CSV)"));
    category_properties      = gtk_menu_item_new_with_label(_("Properties"));

    for (i = 0; i < NUM_TATAMIS; i++) {
        SPRINTF(buf, ngettext("Place to one Tatami","Place to %d Tatamis", i+1), i+1);
        category_to_tatamis[i] = gtk_menu_item_new_with_label(buf);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_team_event_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_team_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_remove_empty);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_create_official);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_colorize);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_show_colors);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_print_all);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_print_all_pdf);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_print_matches);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_properties);
    gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), gtk_separator_menu_item_new());
    for (i = 0; i < NUM_TATAMIS; i++)
        gtk_menu_shell_append(GTK_MENU_SHELL(categories_menu), category_to_tatamis[i]);

    g_signal_connect(G_OBJECT(category_new),             "activate", G_CALLBACK(new_regcategory), 0);
    g_signal_connect(G_OBJECT(category_team_new),        "activate", G_CALLBACK(new_regcategory), (gpointer)1);
    g_signal_connect(G_OBJECT(category_team_event_new),  "activate", G_CALLBACK(new_regcategory), (gpointer)2);
    g_signal_connect(G_OBJECT(category_remove_empty),    "activate", G_CALLBACK(remove_empty_regcategories), 0);
    g_signal_connect(G_OBJECT(category_create_official), "activate", G_CALLBACK(create_categories), 0);
    g_signal_connect(G_OBJECT(category_colorize)       , "activate", G_CALLBACK(colorize_categories), 0);
    g_signal_connect(G_OBJECT(category_show_colors)    , "activate", G_CALLBACK(toggle_show_colors), 0);
    g_signal_connect(G_OBJECT(category_print_all),       "activate", G_CALLBACK(print_doc),
                     (gpointer)(PRINT_ALL_CATEGORIES | PRINT_TO_PRINTER));
    g_signal_connect(G_OBJECT(category_print_all_pdf),   "activate", G_CALLBACK(print_doc),
                     (gpointer)(PRINT_ALL_CATEGORIES | PRINT_TO_PDF));
    g_signal_connect(G_OBJECT(category_print_matches),   "activate", G_CALLBACK(print_matches),
                     (gpointer)(PRINT_ALL_CATEGORIES | PRINT_TO_PDF));
    g_signal_connect(G_OBJECT(category_properties),      "activate", G_CALLBACK(set_categories_dialog), 0);

    for (i = 0; i < NUM_TATAMIS; i++)
        g_signal_connect(G_OBJECT(category_to_tatamis[i]), "activate", G_CALLBACK(locate_to_tatamis),
                         gint_to_ptr(i+1));

    /* Create the Drawing menu content. */
    draw_all_categories = gtk_menu_item_new_with_label(_("Draw All Categories"));
    gtk_menu_shell_append(GTK_MENU_SHELL(drawing_menu), draw_all_categories);
    g_signal_connect(G_OBJECT(draw_all_categories), "activate", G_CALLBACK(draw_all), 0);

    /* Create the Results menu content. */
    results_print_all              = gtk_menu_item_new_with_label(_("Print All (Web And PDF)"));
    results_print_schedule_printer = gtk_menu_item_new_with_label(_("Print Schedule to Printer"));
    results_print_schedule_pdf     = gtk_menu_item_new_with_label(_("Print Schedule to PDF"));
    results_ftp                    = gtk_menu_item_new_with_label(_("Copy to Server"));
    results_gdpr                   = gtk_menu_item_new_with_label("");

    gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_print_all);
    gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_print_schedule_printer);
    gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_ftp);
    gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_gdpr);
    //gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_print_schedule_pdf);

    g_signal_connect(G_OBJECT(results_print_all),              "activate", G_CALLBACK(make_png_all), 0);
    g_signal_connect(G_OBJECT(results_print_schedule_printer), "activate", G_CALLBACK(print_schedule_cb), NULL);
    g_signal_connect(G_OBJECT(results_print_schedule_pdf),      "activate", G_CALLBACK(print_doc),
                     (gpointer)(PRINT_SCHEDULE | PRINT_TO_PDF));
    g_signal_connect(G_OBJECT(results_ftp),                    "activate", G_CALLBACK(ftp_to_server), 0);
    g_signal_connect(G_OBJECT(results_gdpr),                   "activate", G_CALLBACK(set_gdpr), 0);


    /* Create the Judotimer menu content. */
    for (i = 0; i < NUM_TATAMIS; i++) {
        SPRINTF(buf, "%s %d", _("Control Tatami"), i+1);
        judotimer_control[i] = gtk_check_menu_item_new_with_label(buf);
        gtk_menu_shell_append(GTK_MENU_SHELL(judotimer_menu), judotimer_control[i]);
        g_signal_connect(G_OBJECT(judotimer_control[i]), "activate", G_CALLBACK(set_tatami_state),
                         gint_to_ptr(i+1));
    }


    /* Create the Preferences menu content. */
    preference_comm_node              = gtk_menu_item_new_with_label(_("Communication Node"));
    preference_own_ip_addr            = gtk_menu_item_new_with_label(_("Own IP Addresses"));
    preference_show_connections       = gtk_menu_item_new_with_label(_("Show Attached Connections"));
    preference_auto_sheet_update      = gtk_check_menu_item_new_with_label(_("Automatic Sheet Update"));
    //preference_auto_web_update        = gtk_check_menu_item_new_with_label(_("Automatic Web Page Update"));

    for (i = 0; i < NUM_PRINT_LANGS; i++) {
        if (print_lang_menu_texts[i]) {
            preference_result_languages[i] = gtk_radio_menu_item_new_with_label(lang_group, _(print_lang_menu_texts[i]));
            lang_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_result_languages[i]));
        }
    }

    preference_club_text_club         = gtk_radio_menu_item_new_with_label(club_group, _("Club Name Only"));
    club_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_club_text_club));
    preference_club_text_country      = gtk_radio_menu_item_new_with_label(club_group, _("Country Name Only"));
    club_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_club_text_country));
    preference_club_text_both         = gtk_radio_menu_item_new_with_label(club_group, _("Both Club and Country"));
    preference_club_text_abbr         = gtk_check_menu_item_new_with_label(_("Abbreviations in Sheets"));

    preference_weights_to_pool_sheets = gtk_check_menu_item_new_with_label(_("Weights Visible in Pool Sheets"));
    preference_grade_visible          = gtk_check_menu_item_new_with_label("");

    preference_name_layout_0          = gtk_radio_menu_item_new_with_label(name_group,
                                                                           _("Name Surname, Country/Club"));
    name_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_name_layout_0));
    preference_name_layout_1          = gtk_radio_menu_item_new_with_label(name_group,
                                                                           _("Surname Name, Country/Club"));
    name_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_name_layout_1));
    preference_name_layout_2          = gtk_radio_menu_item_new_with_label(name_group,
                                                                          _("Country/Club,  Name Surname"));
    name_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_name_layout_2));
    preference_name_layout_3          = gtk_radio_menu_item_new_with_label(name_group,
                                                                           _("Country/Club,  Surname Name"));
    preference_name_layout_first_uc   = gtk_radio_menu_item_new_with_label(first_name_group,
                                                                           _("As Registered"));
    first_name_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_name_layout_first_uc));
    preference_name_layout_first_fu   = gtk_radio_menu_item_new_with_label(first_name_group,
                                                                           _("Capitalised"));
    first_name_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_name_layout_first_fu));
    preference_name_layout_first_au   = gtk_radio_menu_item_new_with_label(first_name_group,
                                                                           _("UPPERCASE"));
    preference_name_layout_last_uc    = gtk_radio_menu_item_new_with_label(last_name_group,
                                                                           _("As Registered"));
    last_name_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_name_layout_last_uc));
    preference_name_layout_last_fu    = gtk_radio_menu_item_new_with_label(last_name_group,
                                                                           _("Capitalised"));
    last_name_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(preference_name_layout_last_fu));
    preference_name_layout_last_au    = gtk_radio_menu_item_new_with_label(last_name_group,
                                                                           _("UPPERCASE"));

    preference_pool_style             = gtk_check_menu_item_new_with_label("");
    preference_belt_colors            = gtk_check_menu_item_new_with_label("");
    preference_sheet_font             = gtk_menu_item_new_with_label(_("Sheet Font"));
    preference_svg                    = gtk_menu_item_new_with_label(_("SVG Templates"));
    preference_password               = gtk_menu_item_new_with_label(_("Password"));
    preference_mirror                 = gtk_check_menu_item_new_with_label("");
    preference_auto_arrange           = gtk_check_menu_item_new_with_label("");
    preference_use_logo               = gtk_menu_item_new_with_label("");

    preference_serial                 = gtk_menu_item_new_with_label("");
    preference_medal_matches          = gtk_menu_item_new_with_label("");

    //gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_comm_node);
    preference_comm = gtk_menu_item_new_with_label("");
    submenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_comm);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_comm), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_own_ip_addr);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_show_connections);

    //gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_auto_web_update);
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), gtk_separator_menu_item_new());

    preference_langsel = gtk_menu_item_new_with_label(_("Language Selection"));
    submenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_langsel);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_langsel), submenu);

    for (i = 0; i < NUM_PRINT_LANGS; i++) {
        if (preference_result_languages[i])
            gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_result_languages[i]);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), gtk_separator_menu_item_new());

    preference_club_text = gtk_menu_item_new_with_label("");
    submenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_club_text);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_club_text), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_club_text_club);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_club_text_country);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_club_text_both);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_club_text_abbr);

    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), gtk_separator_menu_item_new());

    preference_name_layout = gtk_menu_item_new_with_label("");
    submenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_name_layout);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_name_layout), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_name_layout_0);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_name_layout_1);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_name_layout_2);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_name_layout_3);

    preference_name_layout_first_format = gtk_menu_item_new_with_label(_(""));
    subsubmenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_name_layout_first_format), subsubmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_name_layout_first_format);
    gtk_menu_shell_append(GTK_MENU_SHELL(subsubmenu), preference_name_layout_first_uc);
    gtk_menu_shell_append(GTK_MENU_SHELL(subsubmenu), preference_name_layout_first_fu);
    gtk_menu_shell_append(GTK_MENU_SHELL(subsubmenu), preference_name_layout_first_au);

    preference_name_layout_last_format = gtk_menu_item_new_with_label(_(""));
    subsubmenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_name_layout_last_format), subsubmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_name_layout_last_format);
    gtk_menu_shell_append(GTK_MENU_SHELL(subsubmenu), preference_name_layout_last_uc);
    gtk_menu_shell_append(GTK_MENU_SHELL(subsubmenu), preference_name_layout_last_fu);
    gtk_menu_shell_append(GTK_MENU_SHELL(subsubmenu), preference_name_layout_last_au);

    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), gtk_separator_menu_item_new());

    preference_layout = gtk_menu_item_new_with_label("");
    submenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_layout);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_layout), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_weights_to_pool_sheets);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_grade_visible);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_pool_style);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_belt_colors);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_use_logo);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_sheet_font);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_svg);

    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), gtk_separator_menu_item_new());

    preference_columns = gtk_menu_item_new_with_label("");
    submenu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_columns);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(preference_columns), submenu);

    preference_show_defaults = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_show_defaults);

    for (i = 0; i < NUM_COMP_COLS; i++) {
	preference_show_col[i] = gtk_check_menu_item_new_with_label("");
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), preference_show_col[i]);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), gtk_separator_menu_item_new());

    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_auto_sheet_update);
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_mirror);
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_auto_arrange);
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_password);

    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_serial);
    gtk_menu_shell_append(GTK_MENU_SHELL(preferences_menu), preference_medal_matches);

    g_signal_connect(G_OBJECT(preference_comm_node),              "activate", G_CALLBACK(ask_node_ip_address), 0);
    g_signal_connect(G_OBJECT(preference_own_ip_addr),            "activate", G_CALLBACK(show_my_ip_addresses), 0);
    g_signal_connect(G_OBJECT(preference_show_connections),       "activate", G_CALLBACK(show_node_connections), 0);
    g_signal_connect(G_OBJECT(preference_auto_sheet_update),      "activate", G_CALLBACK(toggle_automatic_sheet_update), 0);
    //g_signal_connect(G_OBJECT(preference_auto_web_update),        "activate", G_CALLBACK(toggle_automatic_web_page_update), 0);

    for (i = 0; i < NUM_PRINT_LANGS; i++) {
        if (preference_result_languages[i])
            g_signal_connect(G_OBJECT(preference_result_languages[i]), "activate", G_CALLBACK(set_lang), gint_to_ptr(i));
    }

    g_signal_connect(G_OBJECT(preference_club_text_club),    "activate",
		     G_CALLBACK(set_club_text), (gpointer)CLUB_TEXT_CLUB);
    g_signal_connect(G_OBJECT(preference_club_text_country), "activate",
		     G_CALLBACK(set_club_text), (gpointer)CLUB_TEXT_COUNTRY);
    g_signal_connect(G_OBJECT(preference_club_text_both),    "activate",
		     G_CALLBACK(set_club_text), (gpointer)(CLUB_TEXT_CLUB|CLUB_TEXT_COUNTRY));
    g_signal_connect(G_OBJECT(preference_club_text_abbr),    "activate",
		     G_CALLBACK(set_club_abbr), (gpointer)NULL);

    g_signal_connect(G_OBJECT(preference_weights_to_pool_sheets), "activate", G_CALLBACK(toggle_weights_in_sheets), 0);
    g_signal_connect(G_OBJECT(preference_grade_visible),          "activate", G_CALLBACK(toggle_grade_visible), 0);
    g_signal_connect(G_OBJECT(preference_name_layout_0),          "activate", G_CALLBACK(toggle_name_layout), (gpointer)NAME_LAYOUT_N_S_C);
    g_signal_connect(G_OBJECT(preference_name_layout_1),          "activate", G_CALLBACK(toggle_name_layout), (gpointer)NAME_LAYOUT_S_N_C);
    g_signal_connect(G_OBJECT(preference_name_layout_2),          "activate", G_CALLBACK(toggle_name_layout), (gpointer)NAME_LAYOUT_C_N_S);
    g_signal_connect(G_OBJECT(preference_name_layout_3),          "activate", G_CALLBACK(toggle_name_layout), (gpointer)NAME_LAYOUT_C_S_N);
    g_signal_connect(G_OBJECT(preference_name_layout_first_uc),   "activate", G_CALLBACK(toggle_name_layout_format_first), (gpointer)NAME_LAYOUT_FORMAT_FIRST_UNCHANGED);
    g_signal_connect(G_OBJECT(preference_name_layout_first_fu),   "activate", G_CALLBACK(toggle_name_layout_format_first), (gpointer)NAME_LAYOUT_FORMAT_FIRST_FIRST_UPPERCASE);
    g_signal_connect(G_OBJECT(preference_name_layout_first_au),   "activate", G_CALLBACK(toggle_name_layout_format_first), (gpointer)NAME_LAYOUT_FORMAT_FIRST_ALL_UPPERCASE);
    g_signal_connect(G_OBJECT(preference_name_layout_last_uc),    "activate", G_CALLBACK(toggle_name_layout_format_last), (gpointer)NAME_LAYOUT_FORMAT_LAST_UNCHANGED);
    g_signal_connect(G_OBJECT(preference_name_layout_last_fu),    "activate", G_CALLBACK(toggle_name_layout_format_last), (gpointer)NAME_LAYOUT_FORMAT_LAST_FIRST_UPPERCASE);
    g_signal_connect(G_OBJECT(preference_name_layout_last_au),    "activate", G_CALLBACK(toggle_name_layout_format_last), (gpointer)NAME_LAYOUT_FORMAT_LAST_ALL_UPPERCASE);
    g_signal_connect(G_OBJECT(preference_pool_style),             "activate", G_CALLBACK(toggle_pool_style), 0);
    g_signal_connect(G_OBJECT(preference_belt_colors),            "activate", G_CALLBACK(toggle_belt_colors), 0);
    g_signal_connect(G_OBJECT(preference_sheet_font),             "activate", G_CALLBACK(font_dialog), 0);
    g_signal_connect(G_OBJECT(preference_svg),                    "activate", G_CALLBACK(select_svg_dir), 0);

    g_signal_connect(G_OBJECT(preference_show_defaults),          "activate", G_CALLBACK(set_col_defaults), NULL);
    for (i = 0; i < NUM_COMP_COLS; i++)
	g_signal_connect(G_OBJECT(preference_show_col[i]),        "activate",
			 G_CALLBACK(toggle_col_visible), gint_to_ptr(i));

    g_signal_connect(G_OBJECT(preference_password),               "activate", G_CALLBACK(set_webpassword_dialog), 0);
    g_signal_connect(G_OBJECT(preference_mirror),                 "activate", G_CALLBACK(toggle_mirror), 0);
    g_signal_connect(G_OBJECT(preference_auto_arrange),           "activate", G_CALLBACK(toggle_auto_arrange), 0);
    g_signal_connect(G_OBJECT(preference_use_logo),               "activate", G_CALLBACK(select_use_logo), 0);
    g_signal_connect(G_OBJECT(preference_serial),                 "activate", G_CALLBACK(set_serial_dialog), 0);
    g_signal_connect(G_OBJECT(preference_medal_matches),          "activate", G_CALLBACK(move_medal_matches), 0);


    /* Create the Drawing menu content. */
    help_manual = gtk_menu_item_new_with_label(_("Manual"));
    help_about = gtk_menu_item_new_with_label(_("About"));

    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help_manual);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help_about);

    g_signal_connect(G_OBJECT(help_manual), "activate", G_CALLBACK(start_help), 0);
    g_signal_connect(G_OBJECT(help_about), "activate", G_CALLBACK(about_shiai), 0);

    /* Attach the new accelerator group to the window. */
    gtk_window_add_accel_group(GTK_WINDOW(window), group);

    set_menu_active();

    return menubar;
}

void set_configuration(void)
{
}

void set_preferences(void)
{
    GError  *error = NULL;
    gchar   *str;
    gint     i, x1, x2;
    gboolean b;

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "nodeipaddress", &error);
    if (!error) {
        gulong a,b1,c,d;
        sscanf(str, "%ld.%ld.%ld.%ld", &a, &b1, &c, &d);
        node_ip_addr = host2net((a << 24) | (b1 << 16) | (c << 8) | d);
        g_free(str);
    }

    for (i = 1; i <= NUM_TATAMIS; i++) {
        gchar t[10];
        sprintf(t, "tatami%d", i);
        error = NULL;
        b = g_key_file_get_boolean(keyfile, "preferences", t, &error);
        if (b && !error) {
            gtk_menu_item_activate(GTK_MENU_ITEM(judotimer_control[i-1]));
        }
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "automatic_sheet_update", &error)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_auto_sheet_update), TRUE);
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "weightvisible", &error)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_weights_to_pool_sheets), TRUE);
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "gradevisible", &error) || error) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_grade_visible), TRUE);
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "poolstyle", &error)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_pool_style), TRUE);
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "beltcolors", &error)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_belt_colors), TRUE);
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "showcolumns", &error);
    if (!error)
        show_columns = x1;
    else
        show_columns = DEFAULT_SHOW_COLUMNS;
    for (i = 0; i < NUM_COMP_COLS; i++) {
        if (show_columns & (1 << i))
            gtk_menu_item_activate(GTK_MENU_ITEM(preference_show_col[i]));
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "win_x_pos", &error);
    if (!error) {
        x2 = g_key_file_get_integer(keyfile, "preferences", "win_y_pos", &error);
        if (!error)
            gtk_window_move(GTK_WINDOW(main_window), x1, x2);
    }

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "sheetfont", &error);
    if (!error) {
        set_font(str);
        g_free(str);
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "webpassword", &error);
    if (!error)
        webpwcrc32 = x1;
    else
        webpwcrc32 = 0;

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "language", &error);
    if (!error && x1 < NUM_LANGS)
        language = x1;
    else
        language = LANG_IS;

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "mirror", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(preference_mirror));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "autoarrange", &error) || error) {
        gtk_menu_item_activate(GTK_MENU_ITEM(preference_auto_arrange));
    }

    print_lang = 0;
    gchar *l;
    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "printlanguagestr", &error);
    if (!error)
        l = str;
    else
        l = g_strdup("is");

    for (i = 0; i < NUM_PRINT_LANGS; i++) {
        if (print_lang_names[i][0] == l[0] && print_lang_names[i][1] == l[1]) {
            print_lang = i;
            if (preference_result_languages[i])
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_result_languages[i]), TRUE);
            break;
        }
    }

    g_free(l);

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "clubtext", &error);
    if (!error)
        club_text = x1;
    else
        club_text = CLUB_TEXT_CLUB;

    if (club_text == CLUB_TEXT_CLUB)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_club_text_club), TRUE);
    else if (club_text == CLUB_TEXT_COUNTRY)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_club_text_country), TRUE);
    else if (club_text == (CLUB_TEXT_CLUB|CLUB_TEXT_COUNTRY))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_club_text_both), TRUE);

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "clubabbr", &error) || error) {
        gtk_menu_item_activate(GTK_MENU_ITEM(preference_club_text_abbr));
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "namelayout", &error);
    if (!error)
        name_layout = x1;
    else
        name_layout = NAME_LAYOUT_N_S_C;

    if (name_layout == NAME_LAYOUT_N_S_C)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_0), TRUE);
    else if (name_layout == NAME_LAYOUT_S_N_C)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_1), TRUE);
    else if (name_layout == NAME_LAYOUT_C_N_S)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_2), TRUE);
    else if (name_layout == NAME_LAYOUT_C_S_N)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_3), TRUE);

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "namelayout_format_first", &error);
    if (!error)
        name_layout_format_first = x1;
    else
        name_layout_format_first = NAME_LAYOUT_FORMAT_FIRST_UNCHANGED;

    if (name_layout_format_first == NAME_LAYOUT_FORMAT_FIRST_UNCHANGED)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_first_uc), TRUE);
    else if (name_layout_format_first == NAME_LAYOUT_FORMAT_FIRST_FIRST_UPPERCASE)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_first_fu), TRUE);
    else if (name_layout_format_first == NAME_LAYOUT_FORMAT_FIRST_ALL_UPPERCASE)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_first_au), TRUE);

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "namelayout_format_last", &error);
    if (!error)
        name_layout_format_last = x1;
    else
        name_layout_format_last = NAME_LAYOUT_FORMAT_LAST_UNCHANGED;

    if (name_layout_format_last == NAME_LAYOUT_FORMAT_LAST_UNCHANGED)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_last_uc), TRUE);
    else if (name_layout_format_last == NAME_LAYOUT_FORMAT_LAST_FIRST_UPPERCASE)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_last_fu), TRUE);
    else if (name_layout_format_last == NAME_LAYOUT_FORMAT_LAST_ALL_UPPERCASE)
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(preference_name_layout_last_au), TRUE);

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "drawsystem", &error);
    if (!error)
        draw_system = x1;
    else
        draw_system = DRAW_INTERNATIONAL;

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "logofile", &error);
    if (!error) {
        if (str[0])
            use_logo = str;
        else
            g_free(str);
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "printheaders", &error) && !error) {
        print_headers = TRUE;
    }

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "serialdevice", &error);
    if (!error) {
        serial_set_device(str);
        g_free(str);
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "serialbaudrate", &error);
    if (!error)
        serial_set_baudrate(x1);

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "serialtype", &error);
    if (!error)
        serial_set_type(x1);

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "svgdir", &error);
    if (!error) {
        if (str[0]) {
            svg_directory = str;
            read_svg_files(TRUE);
            read_lisp_files(TRUE);
        } else
            g_free(str);
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "gdprcompage", &error);
    if (!error)
        gdpr_comp_age = x1;
    else
        gdpr_comp_age = 0;

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "gdprcatage", &error);
    if (!error)
        gdpr_cat_age = x1;
    else
        gdpr_cat_age = 0;

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "showcolors", &error)) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(category_show_colors), TRUE);
    }

    //auth_init();
}

static void change_menu_label(GtkWidget *item, const gchar *new_text)
{
    GtkWidget *menu_label = gtk_bin_get_child(GTK_BIN(item));
    gtk_label_set_text(GTK_LABEL(menu_label), new_text);
}

#define DB_OK (db_name != NULL)

#define SET_SENSITIVE(_menu, _yes) gtk_widget_set_sensitive(GTK_WIDGET(_menu), _yes)

void set_menu_active(void)
{
    gint i;

    SET_SENSITIVE(tournament_properties, DB_OK);
    SET_SENSITIVE(tournament_backup    , DB_OK);
    SET_SENSITIVE(tournament_validation, DB_OK);
    SET_SENSITIVE(sql_dialog           , DB_OK);

    SET_SENSITIVE(competitor_new                 , DB_OK);
    SET_SENSITIVE(competitor_search              , DB_OK);
    SET_SENSITIVE(competitor_select_from_tournament, DB_OK);
    SET_SENSITIVE(competitor_add_from_text_file  , DB_OK);
    SET_SENSITIVE(competitor_add_all_from_shiai  , DB_OK);
    SET_SENSITIVE(competitor_update_weights      ,DB_OK);
    SET_SENSITIVE(competitor_json                ,DB_OK);
    SET_SENSITIVE(competitor_read_json           ,DB_OK);
    SET_SENSITIVE(competitor_write_json          ,DB_OK);
    SET_SENSITIVE(competitor_remove_unweighted   , DB_OK);
    SET_SENSITIVE(competitor_restore_removed     , DB_OK);
    SET_SENSITIVE(competitor_delete_removed      , DB_OK);
    SET_SENSITIVE(competitor_bar_code_search     , DB_OK);
    SET_SENSITIVE(competitor_print_weigh_notes   , DB_OK);
    //SET_SENSITIVE(competitor_print_with_template , DB_OK);

    SET_SENSITIVE(category_new            , DB_OK);
    SET_SENSITIVE(category_team_new       , DB_OK);
    SET_SENSITIVE(category_team_event_new , DB_OK);
    SET_SENSITIVE(category_remove_empty   , DB_OK);
    SET_SENSITIVE(category_create_official, DB_OK);
    SET_SENSITIVE(category_colorize       , DB_OK);
    SET_SENSITIVE(category_show_colors    , DB_OK);
    SET_SENSITIVE(category_print_all      , DB_OK);
    SET_SENSITIVE(category_print_all_pdf  , DB_OK);
    SET_SENSITIVE(category_print_matches  , DB_OK);
    SET_SENSITIVE(category_properties     , DB_OK);

    for (i = 0; i < NUM_TATAMIS; i++)
        SET_SENSITIVE(category_to_tatamis[i], DB_OK);

    SET_SENSITIVE(draw_all_categories, DB_OK);

    SET_SENSITIVE(results_print_all             , DB_OK);
    SET_SENSITIVE(results_print_schedule_printer, DB_OK);
    SET_SENSITIVE(results_print_schedule_pdf    , DB_OK);
    SET_SENSITIVE(results_ftp                   , DB_OK && (current_directory != NULL));
}

extern gchar *menu_text_with_dots(gchar *text);

gboolean change_language(GtkWidget *eventbox, GdkEventButton *event, void *param)
{
    gint i;
    gchar buf[64];

    language = ptr_to_gint(param);
    if (language >= NUM_LANGS) language = LANG_IS;

    set_gui_language(language);

    change_menu_label(tournament_menu_item , _("Tournament"));
    change_menu_label(competitors_menu_item, _("Competitors"));
    change_menu_label(categories_menu_item , _("Categories"));

    change_menu_label(drawing_menu_item    , _("Drawing"));
    change_menu_label(results_menu_item    , _("Results"));
    change_menu_label(judotimer_menu_item  , _("JudoTimer"));
    change_menu_label(preferences_menu_item, _("Preferences"));
    change_menu_label(help_menu_item       , _("Help"));

    change_menu_label(tournament_new       , _("New Tournament"));
    change_menu_label(tournament_choose    , _("Open Tournament"));
    change_menu_label(tournament_choose_net, _("Open Tournament from Net"));
    change_menu_label(tournament_properties, _("Properties"));
    change_menu_label(tournament_backup    , _("Tournament Backup"));
    change_menu_label(tournament_validation, _("Validate Database"));
    change_menu_label(tournament_custom    , _("Load Custom Systems"));
    change_menu_label(sql_dialog,            _("SQL Dialog"));
    change_menu_label(tournament_quit      , _("Quit"));

    change_menu_label(competitor_new                 , _("New Competitor"));
    change_menu_label(competitor_search              , _("Search Competitor"));
    change_menu_label(competitor_select_from_tournament , _("Select From Another Shiai"));
    change_menu_label(competitor_add_from_text_file  , _("Add From a Text File"));
    change_menu_label(competitor_add_all_from_shiai  , _("Add All From Another Shiai"));
    change_menu_label(competitor_update_weights      , _("Update Weights From Another Shiai"));
    change_menu_label(competitor_json                , _("JSON"));
    change_menu_label(competitor_read_json           , _("Read From Directory"));
    change_menu_label(competitor_write_json          , _("Write To File"));
    change_menu_label(competitor_remove_unweighted   , _("Remove Unweighted"));
    change_menu_label(competitor_restore_removed     , _("Restore Removed"));
    change_menu_label(competitor_delete_removed      , _("Delete Removed"));
    change_menu_label(competitor_bar_code_search     , _("Bar Code Search"));
    change_menu_label(competitor_print_weigh_notes   , _("Print Accreditation Cards"));
    //change_menu_label(competitor_print_with_template , _("Print Competitors With Template"));

    change_menu_label(category_new            , _("New Category"));
    change_menu_label(category_team_new       , _("New Team"));
    change_menu_label(category_team_event_new , _("New Team Category"));
    change_menu_label(category_remove_empty   , _("Remove Empty"));
    change_menu_label(category_create_official, _("Create Categories"));
    change_menu_label(category_colorize       , _("Init Colors"));
    change_menu_label(category_show_colors    , _("Show Colors"));
    change_menu_label(category_print_all      , _("Print All"));
    change_menu_label(category_print_all_pdf  , _("Print All (PDF)"));
    change_menu_label(category_print_matches  , _("Print Matches (CSV)"));
    change_menu_label(category_properties     , _("Properties"));

    for (i = 0; i < NUM_TATAMIS; i++) {
        SPRINTF(buf, ngettext("Place to one Tatami","Place to %d Tatamis", i+1) , i+1);
        change_menu_label(category_to_tatamis[i], buf);
    }

    change_menu_label(draw_all_categories, _("Draw All Categories"));

    change_menu_label(results_print_all             , _("Print All (Web And PDF)"));
    change_menu_label(results_print_schedule_printer, _("Print Schedule"));
    change_menu_label(results_print_schedule_pdf    , _("Print Schedule to PDF"));
    change_menu_label(results_ftp                   , _("Copy to Server"));
    change_menu_label(results_gdpr                  , _("GDPR"));

    for (i = 0; i < NUM_TATAMIS; i++) {
        SPRINTF(buf, "%s %d", _("Control Tatami"), i+1);
        change_menu_label(judotimer_control[i], buf);
    }

    change_menu_label(preference_comm                  , _("Communication"));
    change_menu_label(preference_comm_node             , _("Communication Node"));
    change_menu_label(preference_own_ip_addr           , _("Own IP Addresses"));
    change_menu_label(preference_show_connections      , _("Show Attached Connections"));
    change_menu_label(preference_auto_sheet_update     , _("Automatic Sheet Update"));
    //change_menu_label(preference_auto_web_update       , _("Automatic Web Page Update"));

    change_menu_label(preference_langsel               , _("Language Selection"));

    for (i = 0; i < NUM_PRINT_LANGS; i++) {
        if (preference_result_languages[i])
            change_menu_label(preference_result_languages[i], _(print_lang_menu_texts[i]));
    }

    change_menu_label(preference_club_text             , _("Club Text Selection"));
    change_menu_label(preference_club_text_club        , _("Club Name Only"));
    change_menu_label(preference_club_text_country     , _("Country Name Only"));
    change_menu_label(preference_club_text_both        , _("Both Club and Country"));
    change_menu_label(preference_club_text_abbr        , _("Abbreviations in Sheets"));

    change_menu_label(preference_layout                , _("Sheet Layout"));
    change_menu_label(preference_weights_to_pool_sheets, _("Weights Visible"));
    change_menu_label(preference_grade_visible         , _("Grade Visible"));
    change_menu_label(preference_name_layout           , _("Name Layout Selection"));
    change_menu_label(preference_name_layout_0         , _("Name Surname, Country/Club"));
    change_menu_label(preference_name_layout_1         , _("Surname Name, Country/Club"));
    change_menu_label(preference_name_layout_2         , _("Country/Club,  Name Surname"));
    change_menu_label(preference_name_layout_3         , _("Country/Club,  Surname Name"));
    change_menu_label(preference_name_layout_first_format  , _("Name Format"));
    change_menu_label(preference_name_layout_last_format  , _("Surname Format"));
    change_menu_label(preference_name_layout_first_uc  , _("As Registered"));
    change_menu_label(preference_name_layout_first_fu  , _("Capitalised"));
    change_menu_label(preference_name_layout_first_au  , _("UPPERCASE"));
    change_menu_label(preference_name_layout_last_uc   , _("As Registered"));
    change_menu_label(preference_name_layout_last_fu   , _("Capitalised"));
    change_menu_label(preference_name_layout_last_au   , _("UPPERCASE"));

    change_menu_label(preference_pool_style            , _("Pool Style 2"));
    change_menu_label(preference_belt_colors           , _("Use Belt Colors"));
    change_menu_label(preference_sheet_font            , _("Sheet Font"));
    change_menu_label(preference_svg                   , _("SVG Templates"));
    change_menu_label(preference_password              , _("Password"));
    change_menu_label(preference_mirror                , _("Mirror Tatami Order"));
    change_menu_label(preference_auto_arrange          , _("Automatic Match Delay"));
    change_menu_label(preference_use_logo              , _("Print Logo"));

    change_menu_label(preference_columns               , _("Show"));
    change_menu_label(preference_show_defaults         , _("Reset to Defaults"));
    for (i = 0; i < NUM_COMP_COLS; i++)
	change_menu_label(preference_show_col[i]       , _(competitor_column_names[i]));

    change_menu_label(preference_serial                , _("Scale Serial Interface..."));
    change_menu_label(preference_medal_matches         , _("Medal Matches..."));

    change_menu_label(preference_serial                , menu_text_with_dots(_("Scale Serial Interface")));
    change_menu_label(preference_medal_matches         , menu_text_with_dots(_("Move Medal Matches")));

    change_menu_label(help_manual, _("Manual"));
    change_menu_label(help_about , _("About"));

    set_competitors_col_titles();
    set_sheet_titles();
    set_match_col_titles();
    set_log_col_titles();
    set_cat_graph_titles();
    set_match_graph_titles();

    g_key_file_set_integer(keyfile, "preferences", "language", language);

    return TRUE;
}
