#ifndef _SVG_H_
#define _SVG_H_

#ifdef WIN32
typedef struct RsvgRectangle_ {
    gdouble x, y, width, height;
} RsvgRectangle;
#endif

#define WRITE2(_s, _l)                                                  \
    do {                                                                \
        if (!rsvg_handle_write(handle, (guchar *)_s, _l, &err)) {       \
            g_print("\nERROR %s: %s %d\n",                              \
                    err->message, __FUNCTION__, __LINE__); err = NULL; return TRUE; } } while (0)

#define WRITE1(_s, _l)                                                  \
    do { gint _i; for (_i = 0; _i < _l; _i++) {                         \
        if (_s[_i] == '&')                                              \
            WRITE2("&amp;", 5);                                         \
        else if (_s[_i] == '<')                                         \
            WRITE2("&lt;", 4);                                          \
        else if (_s[_i] == '>')                                         \
            WRITE2("&gt;", 4);                                          \
        else if (_s[_i] == '\'')                                        \
            WRITE2("&apos;", 6);                                        \
        else if (_s[_i] == '"')                                         \
            WRITE2("&quot;", 6);                                        \
        else                                                            \
            WRITE2(&_s[_i], 1);                                         \
        }} while (0)

#define WRITE(_a) WRITE1(_a, strlen(_a))

#define IS_LABEL_CHAR(_x) ((_x >= 'a' && _x <= 'z') || (_x >= 'A' && _x <= 'Z'))
#define IS_VALUE_CHAR(_x) (_x >= '0' && _x <= '9')

#define IS_SAME(_a, _b) (!strcmp((char *)_a, (char *)_b))
#define IS_SAME_P(_a, _b) is_same_by_page(pd, _a, _b)

#define NUM_SVG_PAGES 5
#define NUM_SVG_IMAGES 16

typedef struct svg_image_ {
    RsvgRectangle rect;
    gboolean exists;
} svg_image;

typedef struct svg_handle_ {
    RsvgRectangle win;
    RsvgHandle *handle;
    gchar *svg_file;
    gchar *svg_data;
    gchar *datamax;
    gsize  svg_datalen;
    gint   svg_width;
    gint   svg_height;
    gboolean svg_ok;
    gint page_mask;
    gint current_page;
    const gchar *imagename[NUM_SVG_IMAGES];
    svg_image images[NUM_SVG_PAGES][NUM_SVG_IMAGES];
} svg_handle;


gchar *get_svg_file_name(svg_handle *svg);

#endif // _SVG_H_
