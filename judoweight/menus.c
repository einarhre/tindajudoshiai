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

#include "judoweight.h"
#include "language.h"

void start_help(GtkWidget *w, gpointer data);
extern void set_serial_dialog(GtkWidget *w, gpointer data);
extern void serial_set_device(gchar *dev);
extern void serial_set_baudrate(gint baud);
extern void serial_set_type(gint type);
extern void set_svg_file(GtkWidget *menu_item, gpointer data);

static GtkWidget *menubar, *preferences, *help, *preferencesmenu, *helpmenu;
static GtkWidget *quit, *manual;
static GtkWidget *node_ip, *my_ip, *preference_serial, *preference_calibration, *about;
static GtkWidget *light, *menu_light, *lang_menu_item;
static GtkWidget *m_password_protected, *m_automatic_send, *password,
    *m_print_label, *m_nomarg, *m_scale, *svgfile;
//static GtkTooltips *menu_tips;
static gboolean calibration_used = FALSE;

#define CALIB_POINTS 8
static struct calib {
    gint real, display;
    gdouble k;
} scale_calib[CALIB_POINTS];
static gint cal_points = 0;

static void about_judoinfo( GtkWidget *w,
			    gpointer   data )
{
    gtk_show_about_dialog (NULL,
                           "name", "JudoWeight",
                           "title", _("About JudoWeight"),
                           "copyright", "Copyright 2006-2023 Hannu Jokinen",
                           "version", full_version(),
                           "website", "https://www.judoshiai.org/",
                           NULL);
}

#define DIFF(_a, _b) (ABS((_a) - (_b)))

static void initialize_calib(void)
{
    gdouble x1, y1, x2, y2;

    x1 = scale_calib[0].display;
    y1 = scale_calib[0].real;

    for (cal_points = 1; cal_points < CALIB_POINTS; cal_points++) {
	if (scale_calib[cal_points].real == 0)
	    break;

	x2 = scale_calib[cal_points].display;
	y2 = scale_calib[cal_points].real;

	scale_calib[cal_points-1].k = (y2 - y1)/(x2 - x1);

	x1 = x2;
	y1 = y2;
    }
    cal_points--;

    if (cal_points == 0 &&
	scale_calib[0].real == 0 &&
	scale_calib[0].display != 0) {
	scale_calib[0].k = 1.0;
	scale_calib[1].display = scale_calib[0].display + 100000;
	scale_calib[1].real = 100000;
	cal_points = 1;
    }
}

gint get_calibrated_weight(gint weight)
{
    gint i;
    gdouble w;

    if (cal_points < 1 || !calibration_used)
	return weight;

    for (i = 0; i < cal_points; i++) {
	if (weight >= scale_calib[i].display &&
	    weight <= scale_calib[i+1].display) {
		w = (weight - scale_calib[i].display)*scale_calib[i].k +
		    scale_calib[i].real;
	    return (gint)w;
	}
    }

    w = (weight - scale_calib[cal_points].display)*scale_calib[cal_points - 1].k +
	scale_calib[cal_points].real;
	return (gint)w;
}

static GtkWidget *create_entry(gint num)
{
    GtkWidget *tmp;
    tmp = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(tmp), 6);
#if GTK_CHECK_VERSION(3,8,0)
    gtk_entry_set_max_width_chars(GTK_ENTRY(tmp), 6);
#endif
    gtk_entry_set_text(GTK_ENTRY(tmp), weight_to_str(num));
    return tmp;
}

static GtkWidget *create_label(gint num)
{
    return gtk_label_new(weight_to_str(num));
}

static void set_calibration_dialog(GtkWidget *w, gpointer data)
{
    gint i, j;
    GtkWidget *dialog, *used, *table, *note;
    struct {
	GtkWidget *real_w, *display_w, *error_w;
    } calib_w[CALIB_POINTS];

    dialog = gtk_dialog_new_with_buttons (_("Scale Correction"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_MODAL,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);

    table = gtk_grid_new();
    used = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(used), calibration_used);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("In Use")), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table), used, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Real")), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Display")), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(table), gtk_label_new(_("Error")), 2, 1, 1, 1);
    for (i = 0; i < CALIB_POINTS; i++) {
	calib_w[i].real_w = create_entry(scale_calib[i].real);
	calib_w[i].display_w = create_entry(scale_calib[i].display);
	calib_w[i].error_w = create_label(scale_calib[i].display - scale_calib[i].real);
	gtk_grid_attach(GTK_GRID(table), calib_w[i].real_w,    0, 2+i, 1, 1);
	gtk_grid_attach(GTK_GRID(table), calib_w[i].display_w, 1, 2+i, 1, 1);
	gtk_grid_attach(GTK_GRID(table), calib_w[i].error_w,   2, 2+i, 1, 1);
    }

    note = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(table), note, 0, 2+i, 3, 1);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       table, FALSE, FALSE, 0);
    gtk_widget_show_all(dialog);

    gboolean error = TRUE;
    while (error) {
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
	    error = FALSE;

	    for (i = 0; i < CALIB_POINTS; i++) {
		scale_calib[i].real =
		    weight_grams(gtk_entry_get_text(GTK_ENTRY(calib_w[i].real_w)));
		scale_calib[i].display =
		    weight_grams(gtk_entry_get_text(GTK_ENTRY(calib_w[i].display_w)));
	    }

	    for (i = 0; i < CALIB_POINTS - 1; i++) {
		for (j = i + 1; j < CALIB_POINTS; j++) {
		    if ((scale_calib[i].real > scale_calib[j].real &&
			 (i == 0 || scale_calib[j].real > 0)) ||
			(scale_calib[i].real == 0 && i)) {
			struct calib c;
			c = scale_calib[i];
			scale_calib[i] = scale_calib[j];
			scale_calib[j] = c;
		    }
		}
	    }

	    for (i = 0; i < CALIB_POINTS - 1; i++) {
		if (scale_calib[i+1].display == 0)
		    break;

		if (scale_calib[i].display >= scale_calib[i+1].display) {
		    mylog("ERROR in calibration list!\n");
		    gtk_label_set_text(GTK_LABEL(note), _("ERROR"));
		    error = TRUE;
		    break;
		}
	    }

	    if (!error) {
		calibration_used = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(used));
		g_key_file_set_integer(keyfile, "preferences", "scalecalibused", calibration_used);

		gint val[CALIB_POINTS];
		for (i = 0; i < CALIB_POINTS; i++)
		    val[i] = scale_calib[i].real;
		g_key_file_set_integer_list (keyfile, "preferences", "scalecalibreal",
					     val, CALIB_POINTS);
		for (i = 0; i < CALIB_POINTS; i++)
		    val[i] = scale_calib[i].display;
		g_key_file_set_integer_list (keyfile, "preferences", "scalecalibdisplay",
					     val, CALIB_POINTS);

		initialize_calib();
	    }
	} else
	    break;
    }

    gtk_widget_destroy(dialog);
}

static void change_menu_label(GtkWidget *item, const gchar *new_text)
{
    GtkWidget *menu_label = gtk_bin_get_child(GTK_BIN(item));
    gtk_label_set_text(GTK_LABEL(menu_label), new_text);
}

static GtkWidget *get_picture(const gchar *name)
{
    gchar *file = g_build_filename(installation_dir, "etc", "png", name, NULL);
    GtkWidget *pic = gtk_image_new_from_file(file);
    g_free(file);
    return pic;
}

static void set_menu_item_picture(GtkImageMenuItem *menu_item, gchar *name)
{
    GtkImage *image = GTK_IMAGE(gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(menu_item)));
    gchar *file = g_build_filename(installation_dir, "etc", "png", name, NULL);
    gtk_image_set_from_file(image, file);
    g_free(file);
}

static gint light_callback(gpointer data)
{
    extern gboolean connection_ok;
    static gboolean last_ok = FALSE;
#if 0
    extern time_t traffic_last_rec_time;
    static gboolean yellow_set = FALSE;

    if (yellow_set == FALSE && connection_ok && time(NULL) > traffic_last_rec_time + 6) {
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_light), "yellowlight.png");
        yellow_set = TRUE;
        return TRUE;
    } else if (yellow_set && connection_ok && time(NULL) < traffic_last_rec_time + 6) {
        yellow_set = FALSE;
        last_ok = !connection_ok;
    }
#endif

    if (connection_ok == last_ok)
        return TRUE;

    last_ok = connection_ok;

    if (connection_ok)
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_light), "greenlight.png");
    else
        set_menu_item_picture(GTK_IMAGE_MENU_ITEM(menu_light), "redlight.png");

    return TRUE;
}

static GtkWidget *create_menu_item(GtkWidget *menu, void *cb, gint param)
{
    GtkWidget *w = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), w);
    g_signal_connect(G_OBJECT(w), "activate", G_CALLBACK(cb), gint_to_ptr(param));
    return w;
}

static GtkWidget *create_check_menu_item(GtkWidget *menu, void *cb, gint param)
{
    GtkWidget *w = gtk_check_menu_item_new_with_label("");
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), w);
    g_signal_connect(G_OBJECT(w), "activate", G_CALLBACK(cb), gint_to_ptr(param));
    return w;
}

static void create_separator(GtkWidget *menu)
{
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
}

/* Returns a menubar widget made from the above menu */
GtkWidget *get_menubar_menu(GtkWidget  *window)
{
    GtkAccelGroup *group;

    //    menu_tips = gtk_tooltips_new ();
    group = gtk_accel_group_new ();
    menubar = gtk_menu_bar_new ();

    preferences = gtk_menu_item_new_with_label (_("Preferences"));
    help        = gtk_menu_item_new_with_label (_("Help"));
    lang_menu_item = get_language_menu(window, change_language);

    light      = get_picture("redlight.png");
    menu_light = gtk_image_menu_item_new();
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_light), light);
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(menu_light), TRUE);

    preferencesmenu  = gtk_menu_new ();
    helpmenu         = gtk_menu_new ();

    gtk_menu_item_set_submenu (GTK_MENU_ITEM (preferences), preferencesmenu);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (help), helpmenu);

    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), preferences);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), help);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), lang_menu_item);

    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), menu_light);
#if (GTKVER != 3)
    gtk_menu_item_set_right_justified(GTK_MENU_ITEM(menu_light), TRUE);
#endif
    g_signal_connect(G_OBJECT(menu_light), "button_press_event",
                     G_CALLBACK(ask_node_ip_address), (gpointer)NULL);


    /* Create the Preferences menu content. */
    node_ip = create_menu_item(preferencesmenu, ask_node_ip_address, 0);
    my_ip   = create_menu_item(preferencesmenu, show_my_ip_addresses, 0);
    preference_serial = create_menu_item(preferencesmenu, set_serial_dialog, 0);
    preference_calibration = create_menu_item(preferencesmenu, set_calibration_dialog, 0);

    create_separator(preferencesmenu);
    m_password_protected = create_check_menu_item(preferencesmenu, set_password_protected, 0);
    m_automatic_send = create_check_menu_item(preferencesmenu, set_automatic_send, 0);
    password = create_menu_item(preferencesmenu, set_password_dialog, 0);

    create_separator(preferencesmenu);
    m_print_label = create_check_menu_item(preferencesmenu, set_print_label, 0);
    m_nomarg = create_check_menu_item(preferencesmenu, set_nomarg, 0);
    m_scale = create_check_menu_item(preferencesmenu, set_scale, 0);
    svgfile = create_menu_item(preferencesmenu, set_svg_file, 0);

    create_separator(preferencesmenu);
    quit    = create_menu_item(preferencesmenu, destroy, 0);

    /* Create the Help menu content. */
    manual = create_menu_item(helpmenu, start_help, 0);
    about  = create_menu_item(helpmenu, about_judoinfo, 0);

    /* Attach the new accelerator group to the window. */
    gtk_widget_add_accelerator(quit, "activate", group, GDK_Q, GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_window_add_accel_group (GTK_WINDOW (window), group);

    g_timeout_add(1000, light_callback, NULL);

    /* Finally, return the actual menu bar created by the item factory. */
    return menubar;
}

void set_preferences(void)
{
    GError *error = NULL;
    gchar  *str;
    gint    i, x1, *val;
    gsize length;

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "nodeipaddress", &error);
    if (!error) {
        gulong a,b,c,d;
        sscanf(str, "%ld.%ld.%ld.%ld", &a, &b, &c, &d);
        node_ip_addr = host2net((a << 24) | (b << 16) | (c << 8) | d);
        g_free(str);
    }

    error = NULL;
    i = g_key_file_get_integer(keyfile, "preferences", "language", &error);
    if (!error && i < NUM_LANGS)
        language = i;
    else
        language = LANG_EN;

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

    weightpwcrc32 = 0; // password is temporarily 0 to enable checkbox editing
    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "pwprotected", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(m_password_protected));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "autosend", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(m_automatic_send));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "printlabel", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(m_print_label));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "nomarg", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(m_nomarg));
    }

    error = NULL;
    if (g_key_file_get_boolean(keyfile, "preferences", "scale", &error)) {
        gtk_menu_item_activate(GTK_MENU_ITEM(m_scale));
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "password", &error);
    if (!error)
        weightpwcrc32 = x1;

    error = NULL;
    str = g_key_file_get_string(keyfile, "preferences", "svgfile", &error);
    if (!error) {
        svg_file = str;
        read_svg_file();
    }

    error = NULL;
    x1 = g_key_file_get_integer(keyfile, "preferences", "scalecalibused", &error);
    if (!error)
        calibration_used = x1;

    error = NULL;
    val = g_key_file_get_integer_list(keyfile, "preferences", "scalecalibreal",
				      &length, &error);
    if (val) {
	if (length > CALIB_POINTS) length = CALIB_POINTS;
	for (i = 0; i < length; i++)
	    scale_calib[i].real = val[i];
	g_free(val);
    } else
	calibration_used = 0;

    error = NULL;
    val = g_key_file_get_integer_list(keyfile, "preferences", "scalecalibdisplay",
				      &length, &error);
    if (val) {
	if (length > CALIB_POINTS) length = CALIB_POINTS;
	for (i = 0; i < length; i++)
	    scale_calib[i].display = val[i];
	g_free(val);
    } else
	calibration_used = 0;

    initialize_calib();
}

extern gchar *menu_text_with_dots(gchar *text);

gboolean change_language(GtkWidget *eventbox, GdkEventButton *event, void *param)
{
    language = ptr_to_gint(param);
    if (language >= NUM_LANGS) language = LANG_EN;

    set_gui_language(language);

    change_menu_label(preferences,  _("Preferences"));
    change_menu_label(help,         _("Help"));

    change_menu_label(quit,         _("Quit"));

    change_menu_label(node_ip,      _("Communication node"));
    change_menu_label(my_ip,        _("Own IP addresses"));
    change_menu_label(preference_serial, menu_text_with_dots(_("Scale Serial Interface")));
    change_menu_label(preference_calibration, menu_text_with_dots(_("Scale Correction Table")));
    change_menu_label(m_password_protected, _("No manual edit"));
    change_menu_label(m_automatic_send, _("Automatic send"));
    change_menu_label(password,     _("Password"));
    change_menu_label(m_print_label, _("Print label"));
    change_menu_label(m_nomarg,      _("No margins"));
    change_menu_label(m_scale,       _("Fit page"));
    change_menu_label(svgfile,       _("SVG Templates"));

    change_menu_label(manual,       _("Manual"));
    change_menu_label(about,        _("About"));

    change_language_1();

    g_key_file_set_integer(keyfile, "preferences", "language", language);

    return TRUE;
}
