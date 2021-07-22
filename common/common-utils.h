#ifndef _UTIL_H_
#define _UTIL_H_

/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2020 by Hannu Jokinen
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

// Safely copy utf-8 string. Bytes are removed from the end as necessary.
#define STRCPY_UTF8(_dst, _src)						\
    do {								\
	if (g_strlcpy(_dst, _src, sizeof(_dst)) >= sizeof(_dst)) {	\
	    gchar *end = NULL;						\
	    if (g_utf8_validate(_dst, -1, (const gchar **)&end) == FALSE) \
		if (end) *end = 0;					\
	}} while (0)

#define FIX_UTF8(_dst)							\
    do {								\
	gchar *end = NULL;						\
	if (g_utf8_validate(_dst, -1, (const gchar **)&end) == FALSE)	\
	    if (end) *end = 0;						\
    } while (0)

#define SNPRINTF_UTF8(_dst, _fmt...)				\
    do {							\
	if (snprintf(_dst, sizeof(_dst), _fmt) >= sizeof(_dst)) \
	    FIX_UTF8(_dst);					\
    } while (0)

#define CHECK_UTF8(_s)							\
    do {								\
	if (g_utf8_validate(_s, -1, NULL) == FALSE)			\
	    g_print("%s:%d: STRING %s NOT UTF-8\n", __FILE__, __LINE__, _s); \
    } while (0)


/* For stopwatch variables */
#define SNPRINTF(_dst, _fmt...)					  \
    do { gchar *dst = get_##_dst(); gint s = sizeof_##_dst();	  \
	if (snprintf(dst, s, _fmt) >= s)			  \
	    FIX_UTF8(dst);					  \
    } while (0)

#define STRCPY(_dst, _src)						\
    do { gchar *dst = get_##_dst(); gint s = sizeof_##_dst();		\
	if (g_strlcpy(dst, _src, s) >= s) {				\
	    gchar *end = NULL;						\
	    if (g_utf8_validate(dst, -1, (const gchar **)&end) == FALSE) \
		if (end) *end = 0;					\
	}} while (0)

#ifndef EMSCRIPTEN
void color_dec_to_rgba(gint dec, GdkRGBA *rgba);
gint color_rgba_to_dec(GdkRGBA *rgba);
#endif

/** string functions **/

typedef struct string_ {
    char *buf;
    int   len;
    int   size;
} string;

void string_init(string *s);
void string_free(string *s);
int string_concat(string *s, char *fmt, ...);
int string_append(string *s, string *append_to_tail);
int string_clone(string *dst, string *src);

/***/

void print_trace(void);

#endif
