#ifndef _SVG_COMMON_H_
#define _SVG_COMMON_H_

#include <librsvg/rsvg.h>

#include "cJSON.h"

#if LIBRSVG_CHECK_VERSION(2,46,0)
#else
typedef struct _RsvgRectangle {
    gdouble x, y, width, height;
} RsvgRectangle;
#endif

static inline const gchar *ret_same(const gchar *a) {
    return a;
}

#define WRITE2(_s, _l)                                                  \
    do {GError *err1 = NULL;                                                                \
        if (!rsvg_handle_write(svg->handle, (guchar *)(_s), (_l), &err1)) { \
            g_print("\nERROR %s: %s %d\n",                              \
                    err1->message, __FUNCTION__, __LINE__); err1 = NULL; return FALSE; } } while (0)

#define WRITE1(_s, _l)                                                  \
    do { gint _i; for (_i = 0; _i < (_l); _i++) {                       \
            if ((_s)[_i] == '&')                                        \
                WRITE2("&amp;", 5);                                     \
            else if ((_s)[_i] == '<')                                   \
                WRITE2("&lt;", 4);                                      \
            else if ((_s)[_i] == '>')                                   \
                WRITE2("&gt;", 4);                                      \
            else if ((_s)[_i] == '\'')                                  \
                WRITE2("&apos;", 6);                                    \
            else if ((_s)[_i] == '"')                                   \
                WRITE2("&quot;", 6);                                    \
            else                                                        \
                WRITE2(&(_s)[_i], 1);                                   \
        }} while (0)

#define WRITE(_a) do { if (ret_same(_a)) { WRITE1((_a), strlen((const char *)(_a))); }} while (0)

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
    cairo_t *c;
    gdouble  paper_width;
    gdouble  paper_height;
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
    const gchar **imagename;
    svg_image images[NUM_SVG_PAGES][NUM_SVG_IMAGES];
#define CODE_LEN_COMMON 16
    struct {
        guchar code[CODE_LEN_COMMON];
        gint codecnt;
        gint value;
    } attr[16];
    gint cnt;
    gint (*svg_cb)(struct svg_handle_ *svg);
    gint (*img_cb)(struct svg_handle_ *svg);
} svg_handle;

struct svg_memory {
    char *response;
    size_t size;
    cJSON *json;
};


gchar *get_svg_file_name_common(GtkWindow *mainwin, GKeyFile *keyfile, const gchar *keyname);
void read_svg_file_common(svg_handle *svg, GtkWindow *mainwin);
gint paint_svg_common(svg_handle *svg);
void draw_flag_common(svg_handle *svg, RsvgRectangle *rect, const gchar *country);
void draw_image_file_common(svg_handle *svg, RsvgRectangle *rect, const gchar *file, gboolean fit);
void draw_image_common(svg_handle *svg, RsvgRectangle *rect, cairo_surface_t *image, gboolean fit);

gint ask_json_common(const gchar *s, struct svg_memory *chunk);
void free_svg_memory_common(struct svg_memory *m);
struct cJSON *get_array_1_common(struct cJSON *lst, gint ix1);
struct cJSON *get_array_2_common(struct cJSON *lst, gint ix1, gint ix2);
gchar *get_array_1_str_common(struct cJSON *lst, gint ix1);
gint get_array_1_int_common(struct cJSON *lst, gint ix1);
gchar *get_array_2_str_common(struct cJSON *lst, gint ix1, gint ix2);
gint get_array_2_int_common(struct cJSON *lst, gint ix1, gint ix2);


#endif // _SVG_COMMON_H_
