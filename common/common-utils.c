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

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms-compat.h>
#include <glib/gprintf.h>

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
#include "comm.h"

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
	mylog("NOT VALID UTF-8: %s\n", txt);
	mylog("STRLEN=%d\n", (int)strlen(txt));
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
	if (*y != CURRENT_Y) {
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

void color_dec_to_rgba(gint dec, GdkRGBA *rgba)
{
    rgba->red   = ((dec >> 16) & 0xff)/255.0;
    rgba->green = ((dec >> 8) & 0xff)/255.0;
    rgba->blue  = (dec & 0xff)/255.0;
    rgba->alpha = 1.0;
}

gint color_rgba_to_dec(GdkRGBA *rgba)
{
    return
	((int)(rgba->red*255) << 16) +
	((int)(rgba->green*255) << 8) +
	(int)(rgba->blue*255);
}

/** string functions **/

void string_init(string *s)
{
    s->buf = NULL;
    s->len = 0;
    s->size = 0;
}

void string_free(string *s)
{
    g_free(s->buf);
    s->buf = NULL;
    s->len = 0;
    s->size = 0;
}

gint string_concat(string *s, gchar *fmt, ...)
{
    gboolean esc = FALSE;
    
    if (fmt[0] == '!') {
	esc = TRUE;
	fmt++;
    }

    gchar *r = NULL;
    va_list ap;
    va_start(ap, fmt);
    gint n = g_vasprintf(&r, fmt, ap);
    va_end(ap);
    gint n_esc = esc ? n*2+1 : n+1;
    
    if (!s->buf || !s->size) {
	s->len = 0;
	s->size = 128;
	while (n_esc >= s->size)
	    s->size *= 2;
	s->buf = g_malloc(s->size);
    } else if (s->len + n_esc >= s->size) {
	while (s->len + n_esc >= s->size)
	    s->size *= 2;
	s->buf = g_realloc(s->buf, s->size);
    }

    if (esc) {
	gchar *p = r;
	while (*p) {
	    if (*p == '"') s->buf[s->len++] = '\\';
	    s->buf[s->len++] = *p;
	    p++;
	}
    } else {
	memcpy(s->buf + s->len, r, n);
	s->len += n;
    }
    s->buf[s->len] = 0;

    g_free(r);
    return s->len;
}

int string_clone(string *dst, string *src)
{
    if (src->buf) {
        dst->buf = g_malloc(src->size);
        memcpy(dst->buf, src->buf, src->len + 1);
    } else
        dst->buf = NULL;

    dst->len = src->len;
    dst->size = src->size;
    return 0;
}

static gchar *legends[] =
{"?", "(T)", "(H)", "(C)", "(L)", "TH", "HT", "TT", "HH", "FG",
 "HM", "/HM\\", "KG", "/P\\", "T", "H", "S", NULL};

GtkWidget *get_legends_widget(void)
{
    GtkWidget *w;
    gint i;
    w = gtk_combo_box_text_new();
    for (i = 0; legends[i]; i++)
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(w), NULL, legends[i]);
    return w;
}

#if defined(__WIN32__) || defined(WIN32)

void print_trace(void)
{
    guint32 ebp, eip;
    /* greg_t ebp, eip; GNU type for a general register */
    /*
     * GNU flavored inline assembly.
     *Fetch the frame pointer for this frame
     *it _points_ to the saved base pointer that
     *we really want (our caller).
     */
    asm("movl %%ebp, %0" : "=r" (ebp) : );
    /*
     * We want the information for the calling frame, the frame for
     *"dumpstack" need not be in the trace.
     */
    while (ebp) {
        /* the return address is 1 word past the saved base pointer */
        eip = *( (guint32 *)ebp + 1 );
        mylog("- ebp=%p eip=%p\n", (void *)ebp, (void *)eip);
        ebp = *(guint32 *)ebp;
    }
}

#else

#include <execinfo.h>

void print_trace(void)
{
    void *array[16];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, 16);
    strings = backtrace_symbols(array, size);

    mylog("--- Obtained %zd stack frames.", size);

    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;

    for (i = 0; i < size; i++) {
        if (strncmp(strings[i], "judoshiai", 4)) continue;
        mylog("\n%ld\n%s\n", i, strings[i]);

        char syscom[256];
        sprintf(syscom, "addr2line %p -e %s", array[i], name_buf);
        system(syscom);
    }
    mylog("---\n");

    free (strings);
}
#endif
