/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2017 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms-compat.h>

#ifdef WIN32
#include <process.h>
//#include <glib/gwin32.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "common-utils.h"

static gint current_x, current_y;

void set_xy(gint x, gint y)
{
    current_x = x;
    current_y = y;
}

#define TEXT_FONT_CHANGE (TEXT_BOLD | TEXT_ITALIC | TEXT_OTHER)

void write_text(cairo_t *cr,
		const gchar *txt,
		gdouble *x, gdouble *y, // box
		gdouble *w, gdouble *h,
		gint h_align, gint v_align,
		PangoFontDescription *desc,
		gint size, gint flags)
{
    PangoLayout *layout;
    gboolean free_desc = FALSE;
    gdouble x1, y1;
    gint width, height;

    if (!g_utf8_validate(txt, -1, NULL)) {
	g_print("NOT VALID UTF-8: %s\n", txt);
	g_print("STRLEN=%d\n", (int)strlen(txt));
	txt = "NOT UTF-8";
    }

    if (desc == NULL && size == 0 && h && *h > 0) size = 7*(*h)*PANGO_SCALE/10;
    else if (size > 0) size *= PANGO_SCALE;

    if (!desc || size > 0 || (flags & TEXT_FONT_CHANGE)) {
	if (desc)
	    desc = pango_font_description_copy_static(desc);
	else
	    desc = pango_font_description_from_string("Arial 12");

	if (size > 0) pango_font_description_set_absolute_size(desc, size);
	if (flags & TEXT_BOLD) pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
	if (flags & TEXT_ITALIC) pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);

	free_desc = TRUE;
    }

    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, txt, -1);
    pango_layout_set_font_description(layout, desc);

    pango_layout_get_size(layout, &width, &height);
    width /= PANGO_SCALE;
    height /= PANGO_SCALE;

    if (w && *w > 0) { // align inside defined box
	if (h_align < 0)
	    x1 = *x;
	else if (h_align > 0)
	    x1 = *x + *w - width;
	else
	    x1 = *x + *w/2 - width/2;

	if ((flags & TEXT_KEEP_RIGHT) && x1 < *x)
	    x1 = *x;
    } else { // align related to x
	if (*x >= 0) {
	    x1 = *x;
	} else {
	    x1 = current_x;
	}
	if (h_align == 0)
	    x1 -= width/2;
	else if (h_align > 0)
	    x1 -= width;
    }

    if (h && *h > 0) {
	if (v_align < 0)
	    y1 = *y;
	else if (v_align > 0)
	    y1 = *y + *h - height;
	else
	    y1 = *y + *h/2 - height/2;
    } else {
	if (*y >= 0) {
	    y1 = *y;
	} else {
	    y1 = current_y;
	}
	if (v_align == 0)
	    y1 -= height/2;
	else if (v_align > 0)
	    y1 -= height;
    }

    gboolean saved = FALSE;
    if (w && h) {
	if (*w > 0 && *h > 0 && (flags & TEXT_CLIP)) {
	    saved = TRUE;
	    cairo_save(cr);
	    cairo_rectangle(cr, *x, *y, *w, *h);
	    cairo_clip(cr);
	}

	*x = x1;
	*y = y1;
	*w = width;
	*h = height;
    }

    cairo_move_to(cr, x1, y1);
    pango_cairo_show_layout(cr, layout);
    current_x = x1 + width;
    current_y = y1;

    if (saved)
	cairo_restore(cr);

    g_object_unref(layout);
    if (free_desc) pango_font_description_free(desc);

    if (flags & TEXT_UNDERLINE) {
	cairo_move_to(cr, x1, y1 + height + 1);
	cairo_rel_line_to(cr, width, 0);
	cairo_stroke(cr);
    }
    if (flags & TEXT_STRIKETHROUGH) {
	cairo_move_to(cr, x1, y1 + height/2);
	cairo_rel_line_to(cr, width, 0);
	cairo_stroke(cr);
    }
}

void get_text_extents(cairo_t *cr, gchar *txt, PangoFontDescription *desc, gdouble *w, gdouble *h)
{
    gint width, height;
    PangoLayout *layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, txt, -1);
    pango_layout_set_font_description(layout, desc);
    pango_layout_get_size(layout, &width, &height);
    if (w) *w = (gdouble)width/PANGO_SCALE;
    if (h) *h = (gdouble)height/PANGO_SCALE;
    g_object_unref(layout);
}
