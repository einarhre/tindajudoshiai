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

#include "sqlite3.h"
#include "judoshiai.h"
#include "common-utils.h"


//static guint destination;
guint selected_judokas[TOTAL_NUM_COMPETITORS];
guint num_selected_judokas;
static gchar *dest_category = NULL;
static gint   dest_category_ix = 0;

static gboolean for_each_row_selected(GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer data)
{
    GtkTreeSelection *selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(current_view));

    if (gtk_tree_selection_iter_is_selected(selection, iter)) {
        guint index;
        gboolean visible;

        gtk_tree_model_get(model, iter,
                           COL_INDEX, &index,
                           COL_VISIBLE, &visible,
                           -1);

        if (visible) {
            if (num_selected_judokas < TOTAL_NUM_COMPETITORS - 1)
                selected_judokas[num_selected_judokas++] = index;
        } else {
            GtkTreeIter tmp_iter;
            gboolean ok;

            ok = gtk_tree_model_iter_children(model, &tmp_iter, iter);
            while (ok) {
                guint index2;

                gtk_tree_model_get(model, &tmp_iter,
                                   COL_INDEX, &index2,
                                   -1);

                if (num_selected_judokas < TOTAL_NUM_COMPETITORS - 1)
                    selected_judokas[num_selected_judokas++] = index2;

                ok = gtk_tree_model_iter_next(model, &tmp_iter);
            }
        }
    }

    return FALSE;
}

static void view_popup_menu_move_judoka(GtkWidget *menuitem, gpointer userdata)
{
    int i;

    GtkTreeView *treeview = GTK_TREE_VIEW(current_view);
    GtkTreeModel *model = current_model;
    GtkTreeSelection *selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (!dest_category)
        return;

    num_selected_judokas = 0;
    gtk_tree_model_foreach(model, for_each_row_selected, userdata);
    gtk_tree_selection_unselect_all(selection);

    for (i = 0; i < num_selected_judokas; i++) {
        struct judoka *j = get_data(selected_judokas[i]);
        if (!j)
            continue;

        if (db_competitor_match_status(j->index) & MATCH_EXISTS) {
            SHOW_MESSAGE("%s %s: %s.",
                         j->first, j->last, _("Remove drawing first"));
        } else {
            if (j->category)
                g_free((void *)j->category);
            j->category = strdup(dest_category);
            display_one_judoka(j);
            db_update_judoka(j->index, j);
        }
        free_judoka(j);
    }
}

static void view_popup_menu_copy_judoka(GtkWidget *menuitem, gpointer userdata)
{
    int i;

    GtkTreeView *treeview = GTK_TREE_VIEW(current_view);
    GtkTreeModel *model = current_model;
    GtkTreeSelection *selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    num_selected_judokas = 0;
    gtk_tree_model_foreach(model, for_each_row_selected, userdata);
    gtk_tree_selection_unselect_all(selection);

    for (i = 0; i < num_selected_judokas; i++) {
        struct judoka *j = get_data(selected_judokas[i]);
        if (!j)
            continue;

        if (j->category)
            g_free((void *)j->category);

        j->index = comp_index_get_free();//current_index++;
        j->category = g_strdup("?");
        db_add_judoka(j->index, j);
        display_one_judoka(j);

        free_judoka(j);
    }
}

void view_popup_menu_print_cards(GtkWidget *menuitem, gpointer userdata)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(current_view);
    GtkTreeModel *model = current_model;
    GtkTreeSelection *selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    num_selected_judokas = 0;
    gtk_tree_model_foreach(model, for_each_row_selected, userdata);
    gtk_tree_selection_unselect_all(selection);

    print_accreditation_cards(FALSE);
}

extern void print_weight_notes_to_default_printer(GtkWidget *menuitem, gpointer userdata);

void view_popup_menu_print_cards_to_default(GtkWidget *menuitem, gpointer userdata)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(current_view);
    GtkTreeModel *model = current_model;
    GtkTreeSelection *selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    num_selected_judokas = 0;
    gtk_tree_model_foreach(model, for_each_row_selected, userdata);
    gtk_tree_selection_unselect_all(selection);

    print_weight_notes_to_default_printer(NULL, NULL);
}

static void view_popup_menu_change_category(GtkWidget *menuitem, gpointer userdata)
{
    struct judoka *j = get_data(ptr_to_gint(userdata));
    if (!j)
        return;

    if (dest_category)
        g_free(dest_category);

    dest_category = strdup(j->last);
    dest_category_ix = j->index;
    free_judoka(j);

    view_popup_menu_move_judoka(NULL, userdata);
}

static void view_popup_menu_remove_draw(GtkWidget *menuitem, gpointer userdata)
{
    if (db_category_get_match_status(ptr_to_gint(userdata)) & MATCH_MATCHED) {
        SHOW_MESSAGE(_("Matches matched. Clear the results first."));
        return;
    }

    struct compsys cs;
    cs = get_cat_system(ptr_to_gint(userdata));
    cs.system = cs.numcomp = cs.table = 0; // leave wishsys as is

    db_set_system(ptr_to_gint(userdata), cs);
    db_remove_matches(ptr_to_gint(userdata));
    update_category_status_info(ptr_to_gint(userdata));
    matches_refresh();
    refresh_window();
}


#define MMLEN 64
struct match_move {
    gint round;
    gint used;
    GtkWidget *sel_w, *t_w;
};

static gint round_to_pos(gint r)
{
    gint p = 0;

    if (r & ROUND_TYPE_MASK) {
	p = ((r & ROUND_TYPE_MASK)>>7) + 20;
    } else
	p = (r & ROUND_MASK) << 1;

    if ((r & ROUND_UP_DOWN_MASK) == ROUND_LOWER)
	p |= 1;
    return p;
}

static gint pos_to_round(gint p)
{
    gint r = 0;

    if (p >= 20) {
	p -= 20;
	r = (p & 0xfe) << 7;
    } else
	r = (p & 0xfe) >> 1;

    if (p & 1) r |= ROUND_LOWER;

    return r;
}

void view_popup_menu_move_matches(GtkWidget *menuitem, gpointer userdata)
{
    struct match_move *r;
    gint i, j;
    const gchar *rname;
    GtkWidget *dialog;
    GtkWidget *table = gtk_grid_new();
    gchar buf[64];

    struct category_data *catdata = avl_get_category(ptr_to_gint(userdata));
    if (!catdata)
	return;

    r = g_malloc0_n(MMLEN, sizeof(struct match_move));
    if (!r) return;

    for (i = 1; i < NUM_MATCHES; i++) {
	gint n = round_to_pos(round_number(catdata, i));
	if (n < MMLEN) r[n].round++;
	else mylog("ERROR %s:%d\n", __FUNCTION__, __LINE__);
    }

    SNPRINTF_UTF8(buf, "%s (%s)", _("Move Matches"), catdata->category);
    dialog = gtk_dialog_new_with_buttons (buf,
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Select a tatami")), 1, 0, 3, 1);

    for (i = 2; i < MMLEN-1; i += 2) {
	gint r_id = pos_to_round(i);
	gboolean up_down = r[i].round && r[i+1].round;

	if (r[i].round == 0)
	    continue;

	rname = round_to_str(r_id & ~ROUND_UP_DOWN_MASK);

	if (up_down) {
	    SNPRINTF_UTF8(buf, "%s A/B", _(rname));
	    r[i].sel_w = gtk_check_button_new_with_label(buf);
	    gtk_grid_attach(GTK_GRID(table), r[i].sel_w, 0, i, 1, 1);
	    r[i+1].sel_w = gtk_check_button_new_with_label("C/D");
	    gtk_grid_attach(GTK_GRID(table), r[i+1].sel_w, 2, i, 1, 1);
	} else {
	    r[i].sel_w = gtk_check_button_new_with_label(_(rname));
	    gtk_grid_attach(GTK_GRID(table), r[i].sel_w, 0, i, 1, 1);
	}

	r[i].t_w = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r[i].t_w), NULL, "Default");
	if (up_down) {
	    r[i+1].t_w = gtk_combo_box_text_new();
	    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r[i+1].t_w), NULL, "Default");
	}

	for (j = 0; j < number_of_tatamis; j++) {
	    snprintf(buf, sizeof(buf), "T%d", j+1);
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r[i].t_w), NULL, buf);
	    if (up_down) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(r[i+1].t_w), NULL, buf);
	    }
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(r[i].t_w), 0);
	gtk_grid_attach(GTK_GRID(table), r[i].t_w, 1, i, 1, 1);
	if (up_down) {
	    gtk_combo_box_set_active(GTK_COMBO_BOX(r[i+1].t_w), 0);
	    gtk_grid_attach(GTK_GRID(table), r[i+1].t_w, 3, i, 1, 1);
	}
    }

    gtk_widget_show_all(table);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       table, FALSE, FALSE, 0);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
	for (i = 1; i < NUM_MATCHES; i++) {
	    gint n = round_number(catdata, i);
	    n = round_to_pos(n);
	    if (n >= MMLEN) continue;

	    if (r[n].round == 0)
		continue;

	    if (r[n].sel_w && r[n].t_w &&
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(r[n].sel_w))) {
		gint dst_tatami = gtk_combo_box_get_active(GTK_COMBO_BOX(r[n].t_w));
		mylog("cat %d match %d to tatami %d\n",
			ptr_to_gint(userdata), i, dst_tatami);
		db_set_forced_tatami(dst_tatami, ptr_to_gint(userdata), i);
	    }
	} /* for */

	db_read_matches();
	for (i = 1; i <= number_of_tatamis; i++)
	    update_matches(0, (struct compsys){0,0,0,0}, i);
	db_category_set_match_status(ptr_to_gint(userdata));
    } /* dialog run */

    g_free(r);
    gtk_widget_destroy(dialog);
}


static void random_select(gint num, GtkTextBuffer *msgwin)
{
    gint i, j;

    for (i = 0; i < num; i++) {
	if (num_selected_judokas == 0)
	    return;

	gint ix = rand()%num_selected_judokas;
	gint sel = selected_judokas[ix];

	for (j = ix; j < num_selected_judokas-1; j++) {
	    selected_judokas[j] = selected_judokas[j+1];
	}

	num_selected_judokas--;

	struct judoka *ju = get_data(sel);
	if (!ju) continue;

	gint maxweight = 0;
	gchar catbuf[32];
	g_strlcpy(catbuf, ju->category, sizeof(catbuf));
	gchar *p = strrchr(catbuf, '-');
	if (!p) maxweight = 0;
	else maxweight = atoi(p+1)*1000*105/100;

	gchar *name = g_strdup_printf("%s, %s", ju->last, ju->first);
	show_msg(msgwin, "bold", "%8s: %-24s\t Max 5%%: %3d.%02d kg\n",
		 ju->category, name,
		 maxweight/1000, (maxweight%1000)/10);
	g_free(name);
	free_judoka(ju);
    }
}

void view_popup_menu_random_weighin(GtkWidget *menuitem, gpointer userdata)
{
    GtkWidget *dialog;
    GtkWidget *table = gtk_grid_new(), *num_w;
    GtkWidget *from_all, *from_each;

    dialog = gtk_dialog_new_with_buttons (_("Random Weigh-In"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                          NULL);

    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Select")), 0, 0, 1, 1);
    num_w = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(num_w), 3);
    gtk_entry_set_text(GTK_ENTRY(num_w), "4");
    gtk_grid_attach(GTK_GRID(table), num_w, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("competitors from")), 2, 0, 1, 1);

    from_each = gtk_radio_button_new_with_label_from_widget(NULL, _("each selected category"));
    from_all = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(from_each), _("all selected categories"));
    gtk_grid_attach(GTK_GRID(table), from_each, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table), from_all, 3, 1, 1, 1);

    gtk_widget_show_all(table);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       table, FALSE, FALSE, 0);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
	gint num = atoi(gtk_entry_get_text(GTK_ENTRY(num_w)));
	gboolean each = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(from_each));
	gboolean ok;
	GtkTreeIter iter;
	GtkTreeSelection *selection =
	    gtk_tree_view_get_selection(GTK_TREE_VIEW(current_view));

	GtkTextBuffer *msgwin = message_window();

	num_selected_judokas = 0;

	ok = gtk_tree_model_get_iter_first(current_model, &iter);
	while (ok) {
	    if (gtk_tree_selection_iter_is_selected(selection, &iter)) {
		gint index;
		gboolean visible;

		gtk_tree_model_get(current_model, &iter,
				   COL_INDEX, &index,
				   COL_VISIBLE, &visible,
				   -1);

		if (visible) {
		    if (num_selected_judokas < TOTAL_NUM_COMPETITORS - 1)
			selected_judokas[num_selected_judokas++] = index;
		} else {
		    GtkTreeIter iter2;
		    gboolean ok2;

		    if (each)
			num_selected_judokas = 0;

		    ok2 = gtk_tree_model_iter_children(current_model, &iter2, &iter);
		    while (ok2) {
			guint index2;

			gtk_tree_model_get(current_model, &iter2,
					   COL_INDEX, &index2,
					   -1);

			if (num_selected_judokas < TOTAL_NUM_COMPETITORS - 1)
			    selected_judokas[num_selected_judokas++] = index2;

			ok2 = gtk_tree_model_iter_next(current_model, &iter2);
		    }

		    if (each)
			random_select(num, msgwin);
		}
	    } // if is selected
	    ok = gtk_tree_model_iter_next(current_model, &iter);
	} // while ok

	if (!each)
	    random_select(num, msgwin);
    } // response accept

    num_selected_judokas = 0;
    gtk_widget_destroy(dialog);
}

static void color_dialog_response(GtkDialog *dialog, gint response)
{
    GdkRGBA color;
    gchar *buf;

    if (response == GTK_RESPONSE_OK) {
	gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog), &color);
	buf = gdk_rgba_to_string(&color);

	gboolean ok;
	GtkTreeIter iter;
	GtkTreeSelection *selection =
	    gtk_tree_view_get_selection(GTK_TREE_VIEW(current_view));

	ok = gtk_tree_model_get_iter_first(current_model, &iter);
	while (ok) {
	    if (gtk_tree_selection_iter_is_selected(selection, &iter)) {
		gint index;
		gboolean visible;
		
		gtk_tree_model_get(current_model, &iter,
				   COL_INDEX, &index,
				   COL_VISIBLE, &visible,
				   -1);
		
		if (!visible) {
		    gtk_tree_store_set(GTK_TREE_STORE(current_model), &iter,
				       COL_CLUB, buf,
				       -1);
		    db_set_category_color(index, buf);
		}
	    } // if is selected
	    ok = gtk_tree_model_iter_next(current_model, &iter);
	} // while ok

	g_free(buf);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));

    draw_match_graph();
}

void view_popup_menu_color(GtkWidget *menuitem, gpointer userdata)
{
    GtkWidget *dialog;

    dialog = gtk_color_chooser_dialog_new(_("Select a color"), NULL);
    g_signal_connect(dialog, "response", G_CALLBACK (color_dialog_response), NULL);
    gtk_widget_show_all(dialog);
}

static void change_display(GtkWidget *menuitem, gpointer userdata)
{
    if (userdata)
        gtk_tree_view_collapse_all(GTK_TREE_VIEW(current_view));
    else
        gtk_tree_view_expand_all(GTK_TREE_VIEW(current_view));
}

#if 0
static void do_nothing(GtkWidget *menuitem, gpointer userdata)
{

}
#endif

extern struct cat_def category_definitions[];
extern gint find_age_index_by_age(gint age, gint gender);

static void create_new_category(GtkWidget *menuitem, gpointer userdata)
{
    int i;
    gint age = 0, weight = 0;
    gboolean male = FALSE, female = FALSE;
    gchar cbuf[64];

    GtkTreeView *treeview = GTK_TREE_VIEW(current_view);
    GtkTreeModel *model = current_model;
    GtkTreeSelection *selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    num_selected_judokas = 0;
    gtk_tree_model_foreach(model, for_each_row_selected, userdata);
    gtk_tree_selection_unselect_all(selection);

    for (i = 0; i < num_selected_judokas; i++) {
        struct judoka *j = get_data(selected_judokas[i]);
        if (!j)
            continue;

        if (j->birthyear) {
            if ((current_year - j->birthyear) > age)
                age = current_year - j->birthyear;
        }

        if (j->regcategory && j->regcategory[0]) {
            gint age1 = 0;
            gint n = find_age_index(j->regcategory);
            if (n >= 0) {
                age1 = category_definitions[n].age;
                if (category_definitions[n].flags & IS_FEMALE)
                    female = TRUE;
                else
                    male = TRUE;
            }

            if (j->birthyear == 0 && age1 > age)
                age = age1;
        } else {
            if (j->deleted & GENDER_MALE)
                male = TRUE;
            else if (j->deleted & GENDER_FEMALE)
                female = TRUE;
        }

        if (j->weight > weight)
            weight = j->weight;

        free_judoka(j);
    }

    gint k = 0;

    // Finnish speciality removed
    if (0 && draw_system == DRAW_FINNISH) {
        if (age > 19 && male)
            k += sprintf(cbuf+k, "M");
        if (age > 19 && female)
            k += sprintf(cbuf+k, "N");
        if (age >= 0 && age <= 19) {
            k += sprintf(cbuf+k, "%c",
                         "JIIHHGGFFEEDDCCBBAAA"[age]);
            if (male)
                k += sprintf(cbuf+k, "P");
            if (female)
                k += sprintf(cbuf+k, "T");
        }
    } else {
        gint n = find_age_index_by_age(age, male ? IS_MALE : (female ? IS_FEMALE : IS_MALE));
        if (n >= 0)
            k += sprintf(cbuf+k, "%s", category_definitions[n].agetext);
        else
            k += sprintf(cbuf+k, "U%d", age+1);
    }

    k += sprintf(cbuf+k, "-%d,%d",
                 weight/1000,
                 (weight%1000)/100);


    GtkTreeIter tmp_iter;
    if (find_iter_category(&tmp_iter, cbuf)) {
        SHOW_MESSAGE("%s %s!", cbuf, _("already exists"));
        return;
    }


    for (i = 0; i < num_selected_judokas; i++) {
        struct judoka *j = get_data(selected_judokas[i]);

        if (!j)
            continue;

        if (db_competitor_match_status(j->index) & MATCH_EXISTS) {
            SHOW_MESSAGE("%s %s: %s.",
                         j->first, j->last, _("Remove drawing first"));
        } else {
            if (j->category)
                g_free((void *)j->category);
            j->category = strdup(cbuf);
            gint ret = display_one_judoka(j);
            if (ret >= 0) {
                /* new category */
                struct judoka e;
                memset(&e, 0, sizeof(e));
                e.index = ret;
                e.last = cbuf;
                e.belt = 0;
                e.deleted = 0;
                e.birthyear = 0;
                db_add_category(e.index, &e);
            }

            db_update_judoka(j->index, j);
        }
        free_judoka(j);
    }
}

static void view_popup_menu_draw_and_print_category(GtkWidget *menuitem, gpointer userdata)
{
    view_popup_menu_draw_category(menuitem, userdata);
    print_doc(menuitem, gint_to_ptr(ptr_to_gint(userdata) | PRINT_SHEET | PRINT_TO_PRINTER));
}

void show_category_window(GtkWidget *menuitem, gpointer userdata)
{
    if (ptr_to_gint(userdata) >= 10000)
	category_window(ptr_to_gint(userdata));
}

extern gchar *menu_text_with_dots(gchar *text);

void view_popup_menu(GtkWidget *treeview,
                     GdkEventButton *event,
                     gpointer userdata,
                     gchar *regcategory,
                     gboolean visible)
{
    gboolean team = FALSE;
    GtkWidget *menu, *menuitem;
    gint matched = db_category_get_match_status(ptr_to_gint(userdata));
    //db_matched_matches_exist(ptr_to_gint(userdata));

    if (dest_category)
        g_free(dest_category);

    dest_category = strdup(regcategory);
    dest_category_ix = ptr_to_gint(userdata);

    struct category_data *catdata = avl_get_category(dest_category_ix);
    if (catdata && (catdata->deleted & TEAM))
        team = TRUE;

    menu = gtk_menu_new();

    menuitem = gtk_menu_item_new_with_label(_("Expand All"));
    g_signal_connect(menuitem, "activate",
                     (GCallback) change_display, (gpointer)0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Collapse All"));
    g_signal_connect(menuitem, "activate",
                     (GCallback) change_display, (gpointer)1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget *submenu;
    GtkTreeIter iter;
    gboolean ok;

    menuitem = gtk_menu_item_new_with_label(menu_text_with_dots(_("Move Competitors to Category")));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);

    ok = gtk_tree_model_get_iter_first(current_model, &iter);
    while (ok) {
        gint index;
        gchar *cat = NULL;
        gtk_tree_model_get(current_model, &iter,
                           COL_INDEX, &index,
                           COL_LAST_NAME, &cat,
                           -1);
        menuitem = gtk_menu_item_new_with_label(cat);
        g_signal_connect(menuitem, "activate",
                         (GCallback) view_popup_menu_change_category,
                         gint_to_ptr(index));
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menuitem);
        g_free(cat);

        ok = gtk_tree_model_iter_next(current_model, &iter);
    }

    if ((matched & SYSTEM_DEFINED /*REAL_MATCH_EXISTS*/) == 0) {
        menuitem = gtk_menu_item_new_with_label(_("Move Competitors Here"));
        g_signal_connect(menuitem, "activate",
                         (GCallback) view_popup_menu_move_judoka, userdata);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

        if (!team) {
            menuitem = gtk_menu_item_new_with_label(_("Compose Unofficial Category"));
            g_signal_connect(menuitem, "activate",
                             (GCallback) create_new_category, userdata);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

            menuitem = gtk_menu_item_new_with_label(_("Draw Selected"));
            g_signal_connect(menuitem, "activate",
                             (GCallback) view_popup_menu_draw_category, userdata);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

            menuitem = gtk_menu_item_new_with_label(_("Draw and Print Selected"));
            g_signal_connect(menuitem, "activate",
                             (GCallback) view_popup_menu_draw_and_print_category, userdata);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

            menuitem = gtk_menu_item_new_with_label(_("Draw Manually"));
            g_signal_connect(menuitem, "activate",
                             (GCallback) view_popup_menu_draw_category_manually, userdata);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        }

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    } else if ((matched & MATCH_MATCHED) == 0) {
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

        if (!team) {
            menuitem = gtk_menu_item_new_with_label(_("Remove Drawing"));
            g_signal_connect(menuitem, "activate",
                             (GCallback) view_popup_menu_remove_draw, userdata);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

            menuitem = gtk_menu_item_new_with_label(_("Edit Drawing"));
            g_signal_connect(menuitem, "activate",
                             (GCallback) view_popup_menu_draw_category_manually,
                             gint_to_ptr(ptr_to_gint(userdata) | 0x01000000));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        }
    }

    menuitem = gtk_menu_item_new_with_label(_("Copy Competitors"));
    g_signal_connect(menuitem, "activate",
		     (GCallback) view_popup_menu_copy_judoka, userdata);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    if ((matched & SYSTEM_DEFINED /*REAL_MATCH_EXISTS*/) == 0) {
        menuitem = gtk_menu_item_new_with_label(_("Remove Competitors"));
        g_signal_connect(menuitem, "activate",
                         (GCallback) remove_competitors, userdata);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    }

    if (!team) {
        menuitem = gtk_menu_item_new_with_label(_("Show Sheet"));
        g_signal_connect(menuitem, "activate",
                         (GCallback) show_category_window, userdata);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

        menuitem = gtk_menu_item_new_with_label(_("Print Selected Sheets (Printer)"));
        g_signal_connect(menuitem, "activate",
                         (GCallback) print_doc,
			 gint_to_ptr(ptr_to_gint(userdata) | PRINT_SHEET | PRINT_TO_PRINTER));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

        menuitem = gtk_menu_item_new_with_label(_("Print Selected Sheets (PDF)"));
        g_signal_connect(menuitem, "activate",
                         (GCallback) print_doc,
			 gint_to_ptr(ptr_to_gint(userdata) | PRINT_SHEET | PRINT_TO_PDF));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    menuitem = gtk_menu_item_new_with_label(_("Print Selected Accreditation Cards"));
    g_signal_connect(menuitem, "activate",
                     (GCallback) view_popup_menu_print_cards, userdata);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Print Selected Accreditation Cards To Default Printer"));
    g_signal_connect(menuitem, "activate",
                     (GCallback) view_popup_menu_print_cards_to_default, userdata);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menuitem = gtk_menu_item_new_with_label(menu_text_with_dots(_("Move Matches")));
    g_signal_connect(menuitem, "activate",
                     (GCallback) view_popup_menu_move_matches, userdata);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menuitem = gtk_menu_item_new_with_label(menu_text_with_dots(_("Random Weigh-In")));
    g_signal_connect(menuitem, "activate",
                     (GCallback) view_popup_menu_random_weighin, userdata);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    menuitem = gtk_menu_item_new_with_label(_("Color..."));
    g_signal_connect(menuitem, "activate",
                     (GCallback) view_popup_menu_color, userdata);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    gtk_widget_show_all(menu);

    /* Note: event can be NULL here when called from view_onPopupMenu;
     *  gdk_event_get_time() accepts a NULL argument */
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}
