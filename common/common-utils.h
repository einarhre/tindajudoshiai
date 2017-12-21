#ifndef _UTIL_H_
#define _UTIL_H_

/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2017 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */

#define TEXT_BOLD           1
#define TEXT_ITALIC         2
#define TEXT_UNDERLINE      4
#define TEXT_STRIKETHROUGH  8
#define TEXT_OVERLINE      16
#define TEXT_OTHER         32
#define TEXT_CLIP          64
#define TEXT_KEEP_RIGHT   128

#define TEXT_ALIGN_LEFT    -1
#define TEXT_ALIGN_MIDDLE   0
#define TEXT_ALIGN_RIGHT    1
#define TEXT_ALIGN_TOP     -1
#define TEXT_ALIGN_BOTTOM   1

#define TEXT_ANCHOR_START  -1
#define TEXT_ANCHOR_TOP    -1
#define TEXT_ANCHOR_MIDDLE  0
#define TEXT_ANCHOR_END     1
#define TEXT_ANCHOR_BOTTOM  1

#define WRITE_TEXT(_x, _y, _txt, _desc)	do {				\
	gdouble __x = _x, __y = _y;					\
	write_text(c, _txt, &__x, &__y, NULL, NULL, -1, -1, _desc, 0, 0); \
    } while (0)


void set_xy(gint x, gint y);

void write_text(cairo_t *cr,
		const gchar *txt,
		gdouble *x, gdouble *y, // box
		gdouble *w, gdouble *h,
		gint h_align, gint v_align,
		PangoFontDescription *desc,
		gint size, gint flags);

void get_text_extents(cairo_t *cr, gchar *txt, PangoFontDescription *desc,
		      gdouble *w, gdouble *h);

#endif
