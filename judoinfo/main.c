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

#if defined(__WIN32__) || defined(WIN32)

#define  __USE_W32_SOCKETS
//#define Win32_Winsock

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <initguid.h>
#include <ws2tcpip.h>

#else /* UNIX */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#endif /* WIN32 */

#include <gtk/gtk.h>
#include <glib.h>
#include <librsvg/rsvg.h>

#if (GTKVER == 3)
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

#ifdef WIN32
#include <process.h>
//#include <glib/gwin32.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "judoinfo.h"
#include "language.h"
#include "binreloc.h"
#include "common-utils.h"

static gboolean button_pressed(GtkWidget *sheet_page,
			       GdkEventButton *event,
			       gpointer userdata);

gchar         *program_path;
GtkWidget     *main_vbox = NULL;
GtkWidget     *main_window = NULL;
GtkWidget     *menubar = NULL;
gchar          current_directory[1024] = {0};
gint           my_address;
gchar         *installation_dir = NULL;
GTimer        *timer;
static PangoFontDescription *font;
GKeyFile      *keyfile;
gchar         *conffile;
GdkCursor     *cursor = NULL;
guint          current_year;
static GtkWidget *darea = NULL;
gint           language = LANG_EN;
gint           num_lines = NUM_LINES;
gint           display_type = NORMAL_DISPLAY;
gboolean       mirror_display = FALSE;
static gboolean white_first = TRUE;
gboolean       red_background = FALSE;
gboolean       display_bracket = FALSE;
gboolean       menu_hidden = FALSE;
gchar         *filename = NULL;
gint           bracket_x = 0, bracket_y = 0, bracket_w = 0, bracket_h = 0;
gint           bracket_space_w = 0, bracket_space_h = 0;
gchar         *font_face = NULL;

#define MY_FONT (font_face ? font_face : "Arial")

#define THIN_LINE     (paper_width < 700.0 ? 1.0 : paper_width/700.0)
#define THICK_LINE    (2*THIN_LINE)

#define W(_w) ((_w)*paper_width)
#define H(_h) ((_h)*paper_height)

#define BOX_HEIGHT (horiz ? paper_height/(4.0*(num_lines+0.25)*2.0) : paper_height/(4.0*(num_lines+0.25)))
//#define BOX_HEIGHT (1.4*extents.height)

#define NUM_RECTANGLES 1000

static struct {
    double x1, y1, x2, y2;
    gint   tatami;
    gint   group;
    gint   category;
    gint   number;
} point_click_areas[NUM_RECTANGLES];

static gint num_rectangles;

static gboolean button_drag = FALSE;
static gchar    dragged_text[32];
static gdouble  dragged_x, dragged_y;

struct match match_list[NUM_TATAMIS][NUM_LINES];

static struct {
    gint cat;
    gint num;
} last_wins[NUM_TATAMIS];

static void refresh_darea(void)
{
    gtk_widget_queue_draw(darea);
}

static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    return FALSE;
}

extern void print_stat(void);

void destroy( GtkWidget *widget,
	      gpointer   data )
{
    gsize length;
    gchar *inidata = g_key_file_to_data (keyfile,
                                         &length,
                                         NULL);

    print_stat();

    g_file_set_contents(conffile, inidata, length, NULL);
    g_free(inidata);
    g_key_file_free(keyfile);

    gtk_main_quit ();
}

gint refresh_now = 0;

static gboolean refresh_graph(gpointer data)
{
    static time_t last = 0;
    time_t now = time(NULL);

    if (g_atomic_int_get(&refresh_now) || now > last + 1) {
        last = now;
        refresh_now = FALSE;
        refresh_window();
    }
    return TRUE;
}

#define SHOW_BRACKET (display_bracket && !horiz && num_tatamis == 1)

gboolean show_bracket(void)
{
    gboolean horiz = (display_type == HORIZONTAL_DISPLAY);
    gint num_tatamis = number_of_tatamis();
    return SHOW_BRACKET;
}

gint first_shown_tatami(void)
{
    gint i;
    for (i = 0; i < NUM_TATAMIS; i++)
        if (show_tatami[i])
	    return i + 1;
    return 0;
}

static void paint(cairo_t *c, gdouble paper_width, gdouble paper_height, gpointer userdata)
{
    gint i;
    cairo_text_extents_t extents;
    gint num_tatamis = number_of_tatamis();
    gint num_columns = num_tatamis;
    gdouble y_pos = 0.0, colwidth = W(1.0/num_tatamis);
    gdouble left = 0;
    gdouble right;
    time_t now = time(NULL);
    //gboolean update_later = FALSE;
    gboolean upper = TRUE, horiz = (display_type == HORIZONTAL_DISPLAY);

    if (SHOW_BRACKET)
        colwidth = W(1.0/4);

    if (horiz) {
        num_columns = num_tatamis/2;
        if (num_tatamis & 1)
            num_columns++;
        colwidth = W(1.0/num_columns);
    }

    if (mirror_display && num_tatamis)
            left = (num_columns - 1)*colwidth;

    num_rectangles = 0;

    cairo_select_font_face(c, MY_FONT, 0, 0);
    cairo_set_font_size(c, 0.8*BOX_HEIGHT);
    cairo_text_extents(c, "Hj", &extents);

#ifdef USE_PANGO
    PangoFontDescription *desc, *desc_bold;
    desc = pango_font_description_from_string(MY_FONT);
    pango_font_description_set_absolute_size(desc, 0.8*BOX_HEIGHT*PANGO_SCALE);
    desc_bold = pango_font_description_copy(desc);
    pango_font_description_set_weight(desc_bold, PANGO_WEIGHT_BOLD);
#endif

    cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
    cairo_rectangle(c, 0.0, 0.0, paper_width, paper_height);
    cairo_fill(c);

    cairo_set_line_width(c, THIN_LINE);
    cairo_set_source_rgb(c, 0.0, 0.0, 0.0);

    y_pos = BOX_HEIGHT;

    for (i = 0; i < NUM_TATAMIS; i++) {
        gchar buf[32];
        gint k;

        if (!show_tatami[i])
            continue;

        right = left + colwidth;

        if (horiz) {
            if (upper)
                y_pos = BOX_HEIGHT;
            else
                y_pos = H(0.5) + BOX_HEIGHT;
        } else
            y_pos = BOX_HEIGHT;

        for (k = 0; k < num_lines; k++) {
            struct match *m = &match_list[i][k];
            gdouble e = (k == 0) ? colwidth/2 : 0.0;

            if (m->number >= 1000)
                break;

            gint catix = m->category;
            if (catix & 0x00ff0000) { // is this a team event?
                if (m->number < 999)
                    catix = (catix & 0x00ffffff) | (m->number << 24);
            }
            struct name_data *catdata = avl_get_data(catix);

            cairo_save(c);
            if (k == 0) {
                if (last_wins[i].cat == m->category &&
                    last_wins[i].num == m->number)
                    cairo_set_source_rgb(c, 0.7, 1.0, 0.7);
                else
                    cairo_set_source_rgb(c, 1.0, 1.0, 0.0);
            } else
                cairo_set_source_rgb(c, 1.0, 1.0, 1.0);

            cairo_rectangle(c, left, y_pos, colwidth, 4*BOX_HEIGHT);
            cairo_fill(c);
            cairo_restore(c);

            cairo_save(c);
            if (k == 0) {
#ifdef USE_PANGO
		WRITE_TEXT(left+5, y_pos, _("Prev. winner:"), desc);
#else
                cairo_move_to(c, left+5, y_pos+extents.height);
                cairo_show_text(c, _("Prev. winner:"));
#endif
            } else if (m->rest_end > now && k == 1) {
                gint t = m->rest_end - now;
                sprintf(buf, "** %d:%02d **", t/60, t%60);
                cairo_save(c);
                cairo_set_source_rgb(c, 0.8, 0.0, 0.0);
#ifdef USE_PANGO
		WRITE_TEXT(left+5+colwidth/2, y_pos, buf, desc);
#else
                cairo_move_to(c, left+5+colwidth/2, y_pos+extents.height);
                cairo_show_text(c, buf);
#endif
                cairo_restore(c);
                //update_later = TRUE;
            } else if (m->number == 1 && m->round == 0) {
                cairo_save(c);
                cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
#ifdef USE_PANGO
		WRITE_TEXT(left+5+colwidth/2, y_pos, _("Category starts"), desc);
#else
                cairo_move_to(c, left+5+colwidth/2, y_pos+extents.height);
                cairo_show_text(c, _("Category starts"));
#endif
                cairo_restore(c);
	    } else if (m->round) {
                cairo_save(c);
		if (m->number == 1)
		    cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
		else
		    cairo_set_source_rgb(c, 0.0, 0.0, 1.0);
#ifdef USE_PANGO
		WRITE_TEXT(left+5+colwidth/2, y_pos, round_to_str(m->round), desc);
#else
                cairo_move_to(c, left+5+colwidth/2, y_pos+extents.height);
		cairo_show_text(c, round_to_str(m->round));
#endif
#if 0
                if (m->flags & MATCH_FLAG_GOLD)
                    cairo_show_text(c, _("Gold medal match"));
                else if (m->flags & MATCH_FLAG_BRONZE_A)
                    cairo_show_text(c, _("Bronze match A"));
                else if (m->flags & MATCH_FLAG_BRONZE_B)
                    cairo_show_text(c, _("Bronze match B"));
                else if (m->flags & MATCH_FLAG_SEMIFINAL_A)
                    cairo_show_text(c, _("Semifinal A"));
                else if (m->flags & MATCH_FLAG_SEMIFINAL_B)
                    cairo_show_text(c, _("Semifinal B"));
                else if (m->flags & MATCH_FLAG_SILVER)
                    cairo_show_text(c, _("Silver medal match"));
#endif
                cairo_restore(c);
            }
            cairo_select_font_face(c, MY_FONT, 0, CAIRO_FONT_WEIGHT_BOLD);
            if (k == 0)
                cairo_move_to(c, left+5, y_pos+extents.height+BOX_HEIGHT);
            else
                cairo_move_to(c, left+5, y_pos+extents.height);

	    if (m->category & 0x00ff0000)
		SNPRINTF_UTF8(buf, "%s%s #%d", m->number == 999 ? "*" : "",
                              catdata ? catdata->last : "?", (m->category >> 16) & 0xff);
            else
		SNPRINTF_UTF8(buf, "%s #%d", catdata ? catdata->last : "?", m->number);
#ifdef USE_PANGO
            if (k == 0)
		WRITE_TEXT(left+5, y_pos+BOX_HEIGHT, buf, desc_bold);
	    else
		WRITE_TEXT(left+5, y_pos, buf, desc_bold);
#else
            cairo_show_text(c, buf);
#endif
            //cairo_show_text(c, catdata ? catdata->last : "?");
            cairo_restore(c);

            struct name_data *j = avl_get_data(m->blue);
            if (j) {
                cairo_save(c);

#if 0 // new white first rule disables this
                if (k == 1 && m->flags & MATCH_FLAG_BLUE_DELAYED) {
                    if (m->flags & MATCH_FLAG_BLUE_REST)
                        cairo_set_source_rgb(c, 0.5, 0.5, 1.0);
                    else
                        cairo_set_source_rgb(c, 1.0, 0.5, 0.5);
                }
#else
                if (!white_first) {
                    if (red_background)
                        cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
                    else
                        cairo_set_source_rgb(c, 0.0, 0.0, 1.0);
                }
#endif
                else
                    cairo_set_source_rgb(c, 1.0, 1.0, 1.0);

                if (k)
                    cairo_rectangle(c, left, y_pos+BOX_HEIGHT,
                                    colwidth/2,
                                    3*BOX_HEIGHT);
                cairo_fill(c);

                if (k && !white_first)
                    cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
                else
                    cairo_set_source_rgb(c, 0, 0, 0);

                cairo_select_font_face(c, MY_FONT, 0, CAIRO_FONT_WEIGHT_BOLD);
#ifdef USE_PANGO
		WRITE_TEXT(left+5+e, y_pos+2*BOX_HEIGHT, j->last, desc_bold);
		WRITE_TEXT(left+5+e, y_pos+BOX_HEIGHT, j->first, desc);
		WRITE_TEXT(left+5+e, y_pos+3*BOX_HEIGHT, j->club, desc);
#else
                cairo_move_to(c, left+5+e, y_pos+2*BOX_HEIGHT+extents.height);
                cairo_show_text(c, j->last);

                cairo_select_font_face(c, MY_FONT, 0, CAIRO_FONT_WEIGHT_NORMAL);

                cairo_move_to(c, left+5+e, y_pos+BOX_HEIGHT+extents.height);
                cairo_show_text(c, j->first);

                cairo_move_to(c, left+5+e, y_pos+3*BOX_HEIGHT+extents.height);
                cairo_show_text(c, j->club);
#endif

                cairo_restore(c);
            }
            j = avl_get_data(m->white);
            if (j && k > 0) {
                cairo_save(c);

                if (k == 0) {
                    if (last_wins[i].cat == m->category &&
                        last_wins[i].num == m->number)
                        cairo_set_source_rgb(c, 0.7, 1.0, 0.7);
                    else
                        cairo_set_source_rgb(c, 1.0, 1.0, 0.0);
                }
#if 0
                else if (k == 1 && m->flags & MATCH_FLAG_WHITE_DELAYED) {
                    if (m->flags & MATCH_FLAG_WHITE_REST)
                        cairo_set_source_rgb(c, 0.5, 0.5, 1.0);
                    else
                        cairo_set_source_rgb(c, 1.0, 0.5, 0.5);
                }
#else
                else if (white_first) {
                    if (red_background)
                        cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
                    else
                        cairo_set_source_rgb(c, 0.0, 0.0, 1.0);
                }
#endif
                else
                    cairo_set_source_rgb(c, 1.0, 1.0, 1.0);

                cairo_rectangle(c, left+colwidth/2, y_pos+BOX_HEIGHT, colwidth/2, 3*BOX_HEIGHT);
                cairo_fill(c);

                if (k && white_first)
                    cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
                else
                    cairo_set_source_rgb(c, 0, 0, 0);

#ifdef USE_PANGO
		WRITE_TEXT(left+5+colwidth/2, y_pos+2*BOX_HEIGHT, j->last, desc_bold);
		WRITE_TEXT(left+5+colwidth/2, y_pos+BOX_HEIGHT, j->first, desc);
		WRITE_TEXT(left+5+colwidth/2, y_pos+3*BOX_HEIGHT, j->club, desc);
#else
                cairo_select_font_face(c, MY_FONT, 0, CAIRO_FONT_WEIGHT_BOLD);
                cairo_move_to(c, left+5+colwidth/2, y_pos+2*BOX_HEIGHT+extents.height);
                cairo_show_text(c, j->last);

                cairo_select_font_face(c, MY_FONT, 0, CAIRO_FONT_WEIGHT_NORMAL);
                cairo_move_to(c, left+5+colwidth/2, y_pos+BOX_HEIGHT+extents.height);
                cairo_show_text(c, j->first);

                cairo_move_to(c, left+5+colwidth/2, y_pos+3*BOX_HEIGHT+extents.height);
                cairo_show_text(c, j->club);
#endif
                cairo_restore(c);
            }

            point_click_areas[num_rectangles].category = m->category;
            point_click_areas[num_rectangles].number = m->number;
            point_click_areas[num_rectangles].group = 0;
            point_click_areas[num_rectangles].tatami = i+1;
            point_click_areas[num_rectangles].x1 = left;
            point_click_areas[num_rectangles].y1 = y_pos;
            point_click_areas[num_rectangles].x2 = right;
            point_click_areas[num_rectangles].y2 = y_pos + 4*BOX_HEIGHT;

            y_pos += 4*BOX_HEIGHT;

            if (num_rectangles < NUM_RECTANGLES-1)
                num_rectangles++;

            cairo_move_to(c, left, y_pos);
            cairo_line_to(c, right, y_pos);
            cairo_stroke(c);
        } // for (k = 0; k < num_lines; k++)

#ifdef USE_PANGO
        if (horiz) {
            if (upper)
                set_xy(10 + left, 0);
            else
                set_xy(10 + left, H(0.5));
        } else
            set_xy(10 + left, 0);

	sprintf(buf, "%s %d", _("Tatami"), i+1);
	WRITE_TEXT(-1, -1, buf, desc_bold);
#else
        if (horiz) {
            if (upper)
                cairo_move_to(c, 10 + left, extents.height);
            else
                cairo_move_to(c, 10 + left, H(0.5)+extents.height);
        } else
            cairo_move_to(c, 10 + left, extents.height);

	sprintf(buf, "%s %d", _("Tatami"), i+1);
        cairo_show_text(c, buf);
#endif
#if 0
        point_click_areas[num_rectangles].category = 0;
        point_click_areas[num_rectangles].number = 0;
        point_click_areas[num_rectangles].group = 0;
        point_click_areas[num_rectangles].tatami = i+1;
        point_click_areas[num_rectangles].x1 = left;
        point_click_areas[num_rectangles].y1 = y_pos;
        point_click_areas[num_rectangles].x2 = right;
        point_click_areas[num_rectangles].y2 = H(1.0);
        if (num_rectangles < NUM_RECTANGLES-1)
            num_rectangles++;
#endif
        cairo_save(c);
        cairo_set_line_width(c, THICK_LINE);
        cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
        cairo_move_to(c, left, 0);
        cairo_line_to(c, left, H(1.0));
        cairo_stroke(c);
        cairo_restore(c);

        if (horiz) {
            if (!upper) {
                if (mirror_display)
                    left -= colwidth;
                else
                    left += colwidth;
            }
        } else if (mirror_display)
            left -= colwidth;
        else
            left += colwidth;

        upper = !upper;
    } // tatamis


    cairo_save(c);
    cairo_set_line_width(c, THICK_LINE);

    cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
    cairo_move_to(c, 0, BOX_HEIGHT);
    cairo_line_to(c, W(1.0), BOX_HEIGHT);
    if (horiz) {
        cairo_move_to(c, 0, H(0.5));
        cairo_line_to(c, W(1.0), H(0.5));
        cairo_move_to(c, 0, H(0.5)+BOX_HEIGHT);
        cairo_line_to(c, W(1.0), H(0.5)+BOX_HEIGHT);
    }
    cairo_stroke(c);

    if (!horiz && !SHOW_BRACKET) {
        cairo_set_source_rgb(c, 0.0, 0.0, 1.0);
        cairo_move_to(c, 0, 5*BOX_HEIGHT);
        cairo_line_to(c, W(1.0), 5*BOX_HEIGHT);
        cairo_move_to(c, 0, 13*BOX_HEIGHT);
        cairo_line_to(c, W(1.0), 13*BOX_HEIGHT);
        cairo_stroke(c);
    }

    cairo_restore(c);

    if (button_drag) {
        cairo_set_line_width(c, THIN_LINE);
        cairo_text_extents(c, dragged_text, &extents);
        cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
        cairo_rectangle(c, dragged_x - extents.width/2.0, dragged_y - extents.height,
                        extents.width + 4, extents.height + 4);
        cairo_fill(c);
        cairo_set_source_rgb(c, 0, 0, 0);
        cairo_rectangle(c, dragged_x - extents.width/2.0 - 1, dragged_y - extents.height - 1,
                        extents.width + 4, extents.height + 4);
        cairo_stroke(c);
        cairo_move_to(c, dragged_x - extents.width/2.0, dragged_y);
        cairo_show_text(c, dragged_text);
    }

#ifdef USE_PANGO
    pango_font_description_free(desc);
    pango_font_description_free(desc_bold);
#endif

#if 0
    if (update_later)
        g_timeout_add(5000, refresh_graph, NULL);
#endif
}

void paint_bracket(cairo_t *c, gdouble paper_width, gdouble paper_height)
{
    gboolean horiz = (display_type == HORIZONTAL_DISPLAY);

    if (!show_bracket() || !bracket_ok)
        return;

    cairo_surface_t *image;

    if (bracket_x == 0 || bracket_w == 0) {
	bracket_x = paper_width/4;
	bracket_y = BOX_HEIGHT + 2;
	bracket_w = paper_width - bracket_x;
	bracket_h = paper_height - bracket_y;
	bracket_space_w = paper_width;
	bracket_space_h = paper_height;
    }

    if (bracket_type() == BRACKET_TYPE_PNG) {
	bracket_pos = 0;
	image = cairo_image_surface_create_from_png_stream(bracket_read, NULL);
	if (cairo_surface_status(image) == CAIRO_STATUS_SUCCESS) {
	    gint w, h;
	    w = cairo_image_surface_get_width(image);
	    h = cairo_image_surface_get_height(image);
	    gdouble scale_w = (gdouble)(bracket_w*paper_width)/(gdouble)(w*bracket_space_w);
	    gdouble scale_h = (gdouble)(bracket_h*paper_height)/(gdouble)(h*bracket_space_h);
	    gdouble scale = scale_w < scale_h ? scale_w : scale_h;
	    cairo_translate(c,
			    (gdouble)(bracket_x*paper_width)/(double)bracket_space_w,
			    (gdouble)(bracket_y*paper_height)/(double)bracket_space_h);
	    cairo_save(c);
	    cairo_scale(c, scale, scale);
	    //cairo_set_source_surface(c, image, paper_width*0.25/scale, (BOX_HEIGHT + 2)/scale);
	    cairo_set_source_surface(c, image, 0, 0);
	    cairo_paint(c);
	    cairo_surface_destroy(image);
	    cairo_restore(c);
	} else
	    mylog("image fails\n");
    } else if (bracket_type() == BRACKET_TYPE_SVG) {
	GError *err = NULL;
	RsvgHandle *handle = rsvg_handle_new();
	if (!rsvg_handle_write(handle, bracket_start, bracket_len, &err)) {
	    mylog("\nJudoInfo: SVG error %s: %s %d\n",
		    err->message, __FUNCTION__, __LINE__);
	}
	rsvg_handle_close(handle, NULL);

	RsvgDimensionData dim;
	rsvg_handle_get_dimensions(handle, &dim);
	gdouble scale_w = (gdouble)(bracket_w*paper_width)/(gdouble)(dim.width*bracket_space_w);
	gdouble scale_h = (gdouble)(bracket_h*paper_height)/(gdouble)(dim.height*bracket_space_h);
	gdouble scale = scale_w < scale_h ? scale_w : scale_h;

	cairo_save(c);
	cairo_translate(c,
			(gdouble)(bracket_x*paper_width)/(double)bracket_space_w,
			(gdouble)(bracket_y*paper_height)/(double)bracket_space_h);
	cairo_scale(c, scale, scale);
	if (!rsvg_handle_render_cairo(handle, c))
	    mylog("SVG rendering failed\n");
	cairo_restore(c);
	g_object_unref(handle);
    } else
	mylog("BRACKET TYPE error\n");
}

/* This is called when we need to draw the windows contents */
static gboolean expose(GtkWidget *widget, GdkEventExpose *event, gpointer userdata)
{
    cairo_t *c = (cairo_t *)event;
    struct paint_data pd;

    pd.c = c;
    pd.paper_width = gtk_widget_get_allocated_width(widget);
    pd.paper_height = gtk_widget_get_allocated_height(widget);

    if (paint_svg(&pd) == FALSE)
        paint(c, gtk_widget_get_allocated_width(widget),
              gtk_widget_get_allocated_height(widget), userdata);
    paint_bracket(c, pd.paper_width, pd.paper_height);

    return FALSE;
}

static gint mjpeg_width = 0, mjpeg_height = 0;
static gint mjpeg_size = 0;
static gchar *mjpeg_image = NULL;
static gint mjpeg_space = 0;

static gboolean write_mjpeg_mem(const gchar *buf, gsize count, GError **error, gpointer data)
{
    if (!mjpeg_image) {
	mjpeg_size = 0;
	mjpeg_space = 16*1024;
	mjpeg_image = g_malloc(mjpeg_space);
    }
    while (mjpeg_size + count >= mjpeg_space) {
	mjpeg_space *= 2;
	mjpeg_image = g_realloc(mjpeg_image, mjpeg_space);
	mylogerr("mjpeg_space=%d\n", mjpeg_space);
    }
    memcpy(mjpeg_image + mjpeg_size, buf, count);
    mjpeg_size += count;
    return TRUE;
}

static void write_mjpeg_out(void)
{
    if (!mjpeg_image) return;
    fwrite(mjpeg_image, 1, mjpeg_size, stdout);
}

static void convert_rgb_to_bgr(GdkPixbuf *pixbuf)
{
    gint width, height, rowstride, n_channels;
    guchar *pixels, *p, c;
    gint x, y;

    n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    width      = gdk_pixbuf_get_width(pixbuf);
    height     = gdk_pixbuf_get_height(pixbuf);
    rowstride  = gdk_pixbuf_get_rowstride(pixbuf);
    pixels     = gdk_pixbuf_get_pixels(pixbuf);

    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            p = pixels + y * rowstride + x * n_channels;
            c = p[0];
            p[0] = p[2];
            p[2] = c;
        }
    }
}

static gboolean write_mjpeg(gpointer data)
{
    cairo_surface_t *cs;
    cairo_t *c;
    GError *error = NULL;
    static gint frame_num = 0;

    if (mjpeg_width == 0 || mjpeg_height == 0)
        return FALSE;

    static GdkPixbuf *pixbuf = NULL;
    if (!pixbuf)
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, mjpeg_width, mjpeg_height);

    if (frame_num == 0) {
	mjpeg_size = 0;
        cs = cairo_image_surface_create_for_data(gdk_pixbuf_get_pixels(pixbuf),
                                                 CAIRO_FORMAT_RGB24,
                                                 mjpeg_width, mjpeg_height,
                                                 gdk_pixbuf_get_rowstride(pixbuf));

        c = cairo_create(cs);
        paint(c, mjpeg_width, mjpeg_height, NULL);
        cairo_show_page(c);
        cairo_destroy(c);
        convert_rgb_to_bgr(pixbuf);
	gdk_pixbuf_save_to_callback(pixbuf, write_mjpeg_mem, gdk_pixbuf_get_pixels(pixbuf), "jpeg",
                                &error, NULL);
        cairo_surface_destroy(cs);
    }

    write_mjpeg_out();
    if (++frame_num > 50) frame_num = 0;
    fflush(stdout);
    return TRUE;
}

void toggle_full_screen(GtkWidget *menu_item, gpointer data)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item))) {
        gtk_window_fullscreen(GTK_WINDOW(main_window));
        g_key_file_set_boolean(keyfile, "preferences", "fullscreen", TRUE);
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(main_window));
        g_key_file_set_boolean(keyfile, "preferences", "fullscreen", FALSE);
    }
    refresh_darea();
}

void toggle_small_display(GtkWidget *menu_item, gpointer data)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item))) {
        num_lines = NUM_LINES;
        display_type = ptr_to_gint(data);
        g_key_file_set_integer(keyfile, "preferences", "displaytype", ptr_to_gint(data));

        switch (ptr_to_gint(data)) {
        case NORMAL_DISPLAY:
            break;
        case SMALL_DISPLAY:
            num_lines = 6;
            break;
        case HORIZONTAL_DISPLAY:
            num_lines = 3;
            break;
        }
    }
    refresh_darea();
}

void toggle_mirror(GtkWidget *menu_item, gpointer data)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item))) {
        mirror_display = TRUE;
        g_key_file_set_boolean(keyfile, "preferences", "mirror", TRUE);
    } else {
        mirror_display = FALSE;
        g_key_file_set_boolean(keyfile, "preferences", "mirror", FALSE);
    }
    refresh_darea();
}

void toggle_whitefirst(GtkWidget *menu_item, gpointer data)
{
    return; /* always white first*/
    if (TRUE || gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item))) {
        white_first = TRUE;
        g_key_file_set_boolean(keyfile, "preferences", "whitefirst", TRUE);
    } else {
        white_first = FALSE;
        g_key_file_set_boolean(keyfile, "preferences", "whitefirst", FALSE);
    }
    refresh_darea();
}

void toggle_redbackground(GtkWidget *menu_item, gpointer data)
{
    red_background = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
    g_key_file_set_boolean(keyfile, "preferences", "redbackground", red_background);
    refresh_darea();
}

void toggle_bracket(GtkWidget *menu_item, gpointer data)
{
    display_bracket = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
    g_key_file_set_boolean(keyfile, "preferences", "bracket", display_bracket);
    refresh_darea();
}

static gboolean key_press(GtkWidget *widget, GdkEventKey *event, gpointer userdata)
{
    gboolean ctl = event->state & 4;

    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    if (event->keyval == GDK_m && ctl) {
        if (menu_hidden) {
            gtk_widget_show(menubar);
            menu_hidden = FALSE;
        } else {
            gtk_widget_hide(menubar);
            menu_hidden = TRUE;
        }
    }

    return FALSE;
}

int main( int   argc,
          char *argv[] )
{
    /* GtkWidget is the storage type for widgets */
    GtkWidget *window;
    time_t     now;
    struct tm *tm;
    GThread   *gth = NULL;         /* thread id */
    gboolean   run_flag = TRUE;   /* used as exit flag for threads */
    gint       i;
    gint       fps = 25;
    
    putenv("UBUNTU_MENUPROXY=");

    init_trees();

    font = pango_font_description_from_string("Sans bold 12");

#ifdef WIN32
    installation_dir = g_win32_get_package_installation_directory_of_module(NULL);
#else
    gbr_init(NULL);
    installation_dir = gbr_find_prefix(NULL);
#endif

    program_path = argv[0];

    current_directory[0] = 0;

    if (current_directory[0] == 0)
        strcpy(current_directory, ".");

    conffile = g_build_filename(g_get_user_data_dir(), "judoinfo.ini", NULL);

    keyfile = g_key_file_new();
    g_key_file_load_from_file(keyfile, conffile, 0, NULL);

    now = time(NULL);
    tm = localtime(&now);
    current_year = tm->tm_year+1900;
    srand(now); //srandom(now);
    my_address = now + getpid()*10000;

    gtk_init (&argc, &argv);

    for (i = 1; i < argc-1; i++) {
        if (!strcmp(argv[i], "-mjpeg")) {
            gchar *p = strchr(argv[i+1], 'x');
            if (p) {
                mjpeg_width = atoi(argv[i+1]);
                mjpeg_height = atoi(p+1);
            }
	    i++;
        } else if (!strcmp(argv[i], "-a")) {
	    node_ip_addr = inet_addr(argv[i+1]);
	    i++;
        } else if (!strcmp(argv[i], "-fps")) {
	    fps = atoi(argv[i+1]);
	    i++;
	}
    }

    main_window = window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "JudoInfo");
    gtk_widget_set_size_request(window, FRAME_WIDTH, FRAME_HEIGHT);

    gchar *iconfile = g_build_filename(installation_dir, "etc", "png", "judoinfo.png", NULL);
    gtk_window_set_default_icon_from_file(iconfile, NULL);
    g_free(iconfile);

    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (delete_event), NULL);

    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (destroy), NULL);

    g_signal_connect(G_OBJECT(window),
                     "key-press-event", G_CALLBACK(key_press), NULL);

    gtk_container_set_border_width (GTK_CONTAINER (window), 0);

    main_vbox = gtk_grid_new();
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
    gtk_container_add (GTK_CONTAINER (window), main_vbox);
    gtk_widget_show(main_vbox);

    /* menubar */
    menubar = get_menubar_menu(window);
    gtk_widget_show(menubar);

    gtk_grid_attach(GTK_GRID(main_vbox), menubar, 0, 0, 1, 1);
    gtk_widget_set_hexpand(menubar, TRUE);
    //gtk_widget_set_halign(menubar, GTK_ALIGN_FILL);
    darea = gtk_drawing_area_new();
    gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);

    gtk_widget_show(darea);

    gtk_grid_attach(GTK_GRID(main_vbox), darea, 0, 1, 1, 1);
    //gtk_grid_attach_next_to(GTK_GRID(main_vbox), darea, menubar, GTK_POS_BOTTOM, 1, 1);
    gtk_widget_set_hexpand(darea, TRUE);
    gtk_widget_set_vexpand(darea, TRUE);

    g_signal_connect(G_OBJECT(darea),
                     "draw", G_CALLBACK(expose), NULL);
    g_signal_connect(G_OBJECT(darea),
                     "button-press-event", G_CALLBACK(button_pressed), NULL);

    /* timers */

    timer = g_timer_new();

    /*g_timeout_add(100, timeout, NULL);*/
    g_timeout_add(100, refresh_graph, NULL);
    if (mjpeg_width && mjpeg_height)
        g_timeout_add(1000/fps, write_mjpeg, NULL);

    gtk_widget_show_all(window);

    set_preferences();
    change_language(NULL, NULL, gint_to_ptr(language));

    open_comm_socket();

    gth = g_thread_new("Client",
                       (GThreadFunc)client_thread,
                       (gpointer)&run_flag);

    extern gpointer ssdp_thread(gpointer args);
    g_snprintf(ssdp_id, sizeof(ssdp_id), "JudoInfo");
    gth = g_thread_new("SSDP",
                       (GThreadFunc)ssdp_thread,
                       (gpointer)&run_flag);
    gth = gth; // make compiler happy

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

    cursor = gdk_cursor_new(GDK_HAND2);
    //gdk_window_set_cursor(GTK_WIDGET(main_window)->window, cursor);

    g_timeout_add(100, timeout_ask_for_data, NULL);

    for (i = 1; i < argc - 1; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 't') {
	    extern gboolean conf_show_tatami[NUM_TATAMIS];

	    gint j, t;
	    t = atoi(argv[i+1]) - 1;

	    for (j = 0; j < NUM_TATAMIS; j++)
		if (j == t) {
		    show_tatami[j] = conf_show_tatami[i] = TRUE;
		} else
		    show_tatami[j] = conf_show_tatami[i] = FALSE;

	    refresh_window();
	}
    }

    /* All GTK applications must have a gtk_main(). Control ends here
     * and waits for an event to occur (like a key press or
     * mouse event). */
    gtk_main();
    run_flag = FALSE;     /* flag threads to stop and exit */
    //g_thread_join(gth);   /* wait for thread to exit */

    return 0;
}

gboolean this_is_shiai(void)
{
    return FALSE;
}

gint application_type(void)
{
    return APPLICATION_TYPE_INFO;
}

void refresh_window(void)
{
    gtk_widget_queue_draw(GTK_WIDGET(main_window));
    return;
}

static gint find_box(gdouble x, gdouble y)
{
    gint t;

    for (t = 0; t < num_rectangles; t++) {
        if (x >= point_click_areas[t].x1 &&
            x <= point_click_areas[t].x2 &&
            y >= point_click_areas[t].y1 &&
            y <= point_click_areas[t].y2) {
            return t;
        }
    }
    return -1;
}

static gboolean button_pressed(GtkWidget *sheet_page,
			       GdkEventButton *event,
			       gpointer userdata)
{
    if (event->type == GDK_BUTTON_PRESS &&
        (event->button == 1)) {
        gdouble x = event->x, y = event->y;
        gint t, tatami;

        t = find_box(x, y);
        if (t < 0)
            return FALSE;

        tatami = point_click_areas[t].tatami;
        last_wins[tatami-1].cat = match_list[tatami-1][0].category;
        last_wins[tatami-1].num = match_list[tatami-1][0].number;

        refresh_window();

        return TRUE;

    } else if (event->type == GDK_BUTTON_PRESS &&
               (event->button == 3)) {
        struct message msg;
        extern gint my_address;

        msg.type = MSG_ALL_REQ;
        msg.sender = my_address;
        send_packet(&msg);
    }

    return FALSE;
}

void set_write_file(GtkWidget *menu_item, gpointer data)
{
    GtkWidget *dialog;
    static gchar *last_dir = NULL;

    dialog = gtk_file_chooser_dialog_new (_("Save file"),
                                          GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                          NULL);

    //gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

    if (last_dir)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);

    //gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), dflt);

    g_free(filename);
    filename = NULL;

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        if (last_dir)
            g_free(last_dir);
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (dialog));
    }

    gtk_widget_destroy (dialog);
}

void write_matches(void)
{
    gint t, k;

    if (!filename)
        return;

    for (t = 0; t < NUM_TATAMIS; t++)
        if (show_tatami[t])
            break;

    if (t >= NUM_TATAMIS)
        return;

    FILE *fout = fopen(filename, "w");
    if (!fout)
        return;

    for (k = 1; k < num_lines; k++) {
        struct match *m = &match_list[t][k];
        struct name_data *j = avl_get_data(m->category);
        if (j)
            fprintf(fout, "%s;", j->last);
        else
            fprintf(fout, ";");

        j = avl_get_data(m->blue);
        if (j)
            fprintf(fout, "%s;%s;%s;", j->last, j->first, j->club);
        else
            fprintf(fout, ";;;");

        j = avl_get_data(m->white);
        if (j)
            fprintf(fout, "%s;%s;%s\r\n", j->last, j->first, j->club);
        else
            fprintf(fout, ";;\r\n");
    }

    fclose(fout);
}

void font_dialog(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;
    dialog = gtk_font_chooser_dialog_new(_("Select font"), NULL);
    if (font_face)
	gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), font_face);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
	if (font_face) g_free(font_face);
	font_face = NULL;
        font_face = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
	if (font_face) {
	    gchar *p = strrchr(font_face, ' ');
	    if (p) *p = 0;
	    p = strstr(font_face, " Bold");
	    if (p) *p = 0;
	    p = strstr(font_face, " Italic");
	    if (p) *p = 0;
	}
	g_key_file_set_string(keyfile, "preferences", "displayfont", font_face);
    }

    gtk_widget_destroy(dialog);
}

/*** profiling stuff ***/
struct profiling_data prof_data[NUM_PROF];
gint num_prof_data;
guint64 prof_start;
gboolean prof_started = FALSE;
