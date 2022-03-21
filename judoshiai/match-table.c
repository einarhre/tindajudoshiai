#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "judoshiai.h"
#include "common-utils.h"
#include "match-table.h"

#define g_print(_a...) do { } while (0)

#define get_data(_j) avl_get_competitor(_j)
#define free_judoka(_j) do { } while (0)

typedef struct _CustomCellRendererMatchPrivate CustomCellRendererMatchPrivate;
struct _CustomCellRendererMatchPrivate
{
    guint    match;
    gint     tatami;
    gint     position;
};
G_DEFINE_TYPE_WITH_PRIVATE(CustomCellRendererMatch, custom_cell_renderer_match,  GTK_TYPE_CELL_RENDERER)


/* Some boring function declarations: GObject type system stuff */

static void custom_cell_renderer_match_init       (CustomCellRendererMatch      *cellmatch);

static void custom_cell_renderer_match_class_init (CustomCellRendererMatchClass *klass);

static void custom_cell_renderer_match_get_property  (GObject                    *object,
                                                      guint                       param_id,
                                                      GValue                     *value,
                                                      GParamSpec                 *pspec);

static void custom_cell_renderer_match_set_property  (GObject                    *object,
                                                      guint                       param_id,
                                                      const GValue               *value,
                                                      GParamSpec                 *pspec);

static void custom_cell_renderer_match_finalize (GObject *gobject);

#if 0
static void
gtk_cell_renderer_match_get_size (GtkCellRenderer    *cell,
                                  GtkWidget          *widget,
                                  const GdkRectangle *cell_area,
                                  gint               *x_offset,
                                  gint               *y_offset,
                                  gint               *width,
                                  gint               *height);
#endif

/* These functions are the heart of our custom cell renderer: */

static void custom_cell_renderer_match_get_preferred_width (
              GtkCellRenderer       *cell_renderer,
              GtkWidget             *parent_widget,
              int                   *minimal_size,
              int                   *natural_size);

static void custom_cell_renderer_match_get_preferred_height (
              GtkCellRenderer       *cell_renderer,
              GtkWidget             *parent_widget,
              int                   *minimal_size,
              int                   *natural_size);

static void custom_cell_renderer_match_render (
              GtkCellRenderer      *cell_renderer,
              cairo_t              *cr,
              GtkWidget            *widget,
              const GdkRectangle   *background_area,
              const GdkRectangle   *cell_area,
              GtkCellRendererState  flags);

enum
{
  PROP_MATCH_TP = 1,
  PROP_MATCH_TATAMI,
  PROP_MATCH_POS
};

static   gpointer parent_class;

/******************************************************************
 *
 * CustomCellRendererMatchPrivate: shortcut function to access
 *                                    private data.
 *
 ******************************************************************/
static inline CustomCellRendererMatchPrivate *
private (CustomCellRendererMatch *self)
{
   return custom_cell_renderer_match_get_instance_private(self);
}

/***************************************************************************
 *
 *  custom_cell_renderer_match_init: set some default properties of the
 *                                      parent (GtkCellRenderer).
 *
 ***************************************************************************/

static void
custom_cell_renderer_match_init (CustomCellRendererMatch *self)
{
    /* init parent */
    {
        GtkCellRenderer *parent = GTK_CELL_RENDERER(self);

        g_object_set (parent, "mode", GTK_CELL_RENDERER_MODE_INERT, NULL);
        g_object_set (parent, "xpad", 1, NULL);
        g_object_set (parent, "ypad", 1, NULL);
    }

    /* init private variables */
    {
        private (self)->match = 0;
    }
}


/***************************************************************************
 *
 *  custom_cell_renderer_match_class_init:
 *
 *  set up our own get_property and set_property functions, and
 *  override the parent's functions that we need to implement.
 *  And make our new "percentage" property known to the type system.
 *  If you want cells that can be activated on their own (ie. not
 *  just the whole row selected) or cells that are editable, you
 *  will need to override 'activate' and 'start_editing' as well.
 *
 ***************************************************************************/

static void
custom_cell_renderer_match_class_init (CustomCellRendererMatchClass *klass)
{
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS(klass);
  GObjectClass         *object_class = G_OBJECT_CLASS(klass);

  parent_class           = g_type_class_peek_parent (klass);
  object_class->finalize = custom_cell_renderer_match_finalize;

  /* Hook up functions to set and get our
   *   custom cell renderer properties */
  object_class->get_property = custom_cell_renderer_match_get_property;
  object_class->set_property = custom_cell_renderer_match_set_property;

  /* Override the crucial functions that are the heart
   *   of a cell renderer in the parent class */
  cell_class->get_preferred_width = custom_cell_renderer_match_get_preferred_width;
  cell_class->get_preferred_height = custom_cell_renderer_match_get_preferred_height;
  //cell_class->get_size = gtk_cell_renderer_match_get_size;

  cell_class->render   = custom_cell_renderer_match_render;

  /* Install our very own properties */
  g_object_class_install_property (object_class,
                                   PROP_MATCH_TP,
                                   g_param_spec_uint ("match",
                                                      "Match",
                                                      "Tatami and position",
                                                      0, 0x0fffffff, 0,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_MATCH_TATAMI,
                                   g_param_spec_int ("tatami",
                                                     "Tatami",
                                                     "Tatami number",
                                                     0, NUM_TATAMIS, 1,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_MATCH_POS,
                                   g_param_spec_int ("position",
                                                     "Position",
                                                     "Position number",
                                                     0, 1024, 1,
                                                     G_PARAM_READWRITE));
}


/***************************************************************************
 *
 *  custom_cell_renderer_match_finalize: free any resources here
 *
 ***************************************************************************/

static void
custom_cell_renderer_match_finalize (GObject *object)
{
/*
  CustomCellRendererMatch *cellrenderermatch = CUSTOM_CELL_RENDERER_MATCH(object);
*/

  /* Free any dynamically allocated resources here */

  (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/***************************************************************************
 *
 *  custom_cell_renderer_match_get_property: as it says
 *
 ***************************************************************************/

static void
custom_cell_renderer_match_get_property (GObject    *object,
                                         guint       param_id,
                                         GValue     *value,
                                         GParamSpec *psec)
{
  CustomCellRendererMatch  *self = CUSTOM_CELL_RENDERER_MATCH(object);

  switch (param_id)
  {
    case PROP_MATCH_TP:
      g_value_set_uint(value, private (self)->match);
      break;

    case PROP_MATCH_TATAMI:
      g_value_set_int(value, private (self)->tatami);
      break;

    case PROP_MATCH_POS:
      g_value_set_int(value, private (self)->position);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
      break;
  }
}


/***************************************************************************
 *
 *  custom_cell_renderer_match_set_property: as it says
 *
 ***************************************************************************/

static void
custom_cell_renderer_match_set_property (GObject      *object,
                                         guint         param_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  CustomCellRendererMatch *self = CUSTOM_CELL_RENDERER_MATCH (object);

  switch (param_id)
  {
    case PROP_MATCH_TP:
      private (self)->match = g_value_get_uint(value);
      break;

    case PROP_MATCH_TATAMI:
      private (self)->tatami = g_value_get_int(value);
      break;

    case PROP_MATCH_POS:
      private (self)->position = g_value_get_int(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
  }
}

/***************************************************************************
 *
 *  custom_cell_renderer_match_new: return a new cell renderer instance
 *
 ***************************************************************************/

GtkCellRenderer *
custom_cell_renderer_match_new (void)
{
  return g_object_new(CUSTOM_TYPE_CELL_RENDERER_MATCH, NULL);
}


enum {
  TARGET_STRING,
  TARGET_ROOTWIN
};

static gboolean have_drag = FALSE;
static guint drag_data = 0;
static gint drag_x0, drag_y0, drag_x, drag_y;
static gint adj_x, adj_y;
static gboolean drag_after;
static gchar drag_text[32];
static gboolean invalidate = FALSE;


/***************************************************************************
 *
 *  custom_cell_renderer_match_get_preferred_width:
 *            crucial - calculate the size of our cell, taking into account
 *             padding and alignment properties of parent.
 *
 ***************************************************************************/

#define FIXED_WIDTH   240
#define FIXED_HEIGHT  60

static void
custom_cell_renderer_match_get_preferred_width (GtkCellRenderer *cell_renderer,
                                                   GtkWidget       *parent_widget,
                                                   int             *minimal_size,
                                                   int             *natural_size)
{
  gint calc_width;
  gint xpad;

  g_object_get( cell_renderer, "xpad", &xpad, NULL);

  //calc_width  = (gint) xpad * 2 + FIXED_WIDTH;
  calc_width = gtk_widget_get_allocated_width(main_window)/(number_of_tatamis+1);
  if (calc_width < FIXED_WIDTH)
      calc_width = FIXED_WIDTH;
  
  if (minimal_size)
    *minimal_size = FIXED_WIDTH;

  if (natural_size)
    *natural_size = calc_width;
}


/***************************************************************************
 *
 *  custom_cell_renderer_match_get_preferred_height:
 *            crucial - calculate the size of our cell, taking into account
 *             padding and alignment properties of parent.
 *
 ***************************************************************************/

static void
custom_cell_renderer_match_get_preferred_height (GtkCellRenderer *cell_renderer,
                                                    GtkWidget       *parent_widget,
                                                    int             *minimal_size,
                                                    int             *natural_size)
{
  gint calc_height;
  gint ypad;

  g_object_get( cell_renderer, "ypad", &ypad, NULL);

  calc_height = (gint) ypad * 2 + FIXED_HEIGHT;

  if (minimal_size)
    *minimal_size = calc_height;

  if (natural_size)
    *natural_size = calc_height;
}

#if 0
static void
gtk_cell_renderer_match_get_size (GtkCellRenderer    *cell,
                                     GtkWidget          *widget,
                                     const GdkRectangle *cell_area,
                                     gint               *x_offset,
                                     gint               *y_offset,
                                     gint               *width,
                                     gint               *height)
{
    //CustomCellRendererMatch *cellmatch = CUSTOM_CELL_RENDERER_MATCH (cell);
    //CustomCellRendererMatchPrivate *priv = cellmatch->priv;
    gint w, h;
    
    custom_cell_renderer_match_get_preferred_width (cell, NULL, NULL, &w);
    custom_cell_renderer_match_get_preferred_height (cell, NULL, NULL, &h);
    w = 200; h = 20;
    
    if (width)
        *width = w;
  
    if (height)
        *height = h;

    /* FIXME: at the moment cell_area is only set when we are requesting
     * the size for drawing the focus rectangle. We now just return
     * the last size we used for drawing the match bar, which will
     * work for now. Not a really nice solution though.
     */
    if (cell_area)
    {
        if (width)
            *width = cell_area->width;
        if (height)
            *height = cell_area->height;
    }

    if (x_offset) *x_offset = 0;
    if (y_offset) *y_offset = 0;
}
#endif

/***************************************************************************
 *
 *  custom_cell_renderer_match_render: crucial - do the rendering.
 *
 ***************************************************************************/

#define BGCOLOR cairo_set_source_rgb(c, catdata->color.red, catdata->color.green, catdata->color.blue)
#define MY_FONT get_font_props(NULL, NULL)

#define MATCH_DATA_TO_TATAMI(_m) (_m & 0xff)
#define MATCH_DATA_TO_POSITION(_m) ((_m >> 8) & 0xfff)
#define MATCH_DATA_FROM_T_POS(_t, _p) ((_t) | ((_p) << 8))

static struct {
    gint cat;
    gint num;
} last_wins[NUM_TATAMIS];

#define AREA_SHIFT        4
#define ARG_MASK        0xf

struct traverse {
    guint match_data;
    gint  op;
};

enum {
    TRAVERSE_UPDATE_CELL,
    TRAVERSE_UPDATE_COLUMN,
    TRAVERSE_UPDATE_ALL,
    NUM_TRAVERSE
};

static GtkWidget *match_table_view = NULL;
static GtkListStore *match_table_store = NULL;
static GtkTreeViewColumn *columns[NUM_TATAMIS+1];
static GtkWidget *match_scrolled_window;
static GtkWidget *match_table_label = NULL;

static time_t rest_times[NUM_TATAMIS];
static gint   rest_flags[NUM_TATAMIS];

void match_table_clear(void)
{
    have_drag = FALSE;
    drag_data = 0;
    drag_x0 = drag_y0 = drag_x = drag_y = 0;
    adj_x = adj_y = 0;
    drag_after = FALSE;
    drag_text[0] = 0;
    memset(&last_wins, 0, sizeof(last_wins));
    match_table_view = NULL;
    match_table_store = NULL;
    memset(&columns, 0, sizeof(columns));
    match_scrolled_window = NULL;
    match_table_label = NULL;
    memset(&rest_times, 0, sizeof(rest_times));
    memset(&rest_flags, 0, sizeof(rest_flags));
}

static void
custom_cell_renderer_match_render(GtkCellRenderer      *cell_renderer,
                                  cairo_t              *c,
                                  GtkWidget            *widget,
                                  const GdkRectangle   *background_area,
                                  const GdkRectangle   *cell_area,
                                  GtkCellRendererState  flags)
{
    CustomCellRendererMatch *self = CUSTOM_CELL_RENDERER_MATCH (cell_renderer);
    GtkStateType             state;
    gint                     x, y, w, h, x_pad, y_pad, y_pos;
    gint                     BOX_HEIGHT;
    struct match            *m, *mlist;
    gint                     tatami = MATCH_DATA_TO_TATAMI(private (self)->match);
    gint                     position = MATCH_DATA_TO_POSITION(private (self)->match);
    gchar                    buf[64];
    //cairo_text_extents_t     extents;

    if (tatami == 0)
        mlist = db_matches_waiting();
    else
        mlist = get_cached_next_matches(tatami);

#if 0
    g_print("Render match=%p x=%d y=%d w=%d h=%d tatami=%d position=%d\n", m,
            cell_area->x, cell_area->y,
            cell_area->width, cell_area->height, tatami, position);

#endif
    //custom_cell_renderer_match_get_preferred_height (cell_renderer, widget, &height, &height);
    //custom_cell_renderer_match_get_preferred_width (cell_renderer, widget, &width, &width);
    
    if (gtk_widget_is_focus (widget))
        state = GTK_STATE_ACTIVE;
    else
        state = GTK_STATE_NORMAL;

    g_object_get (cell_renderer, "xpad", &x_pad, NULL);
    g_object_get (cell_renderer, "ypad", &y_pad, NULL);
    w = cell_area->width - x_pad * 2;
    h = cell_area->height - y_pad * 2;
    BOX_HEIGHT = h/4;
    
    PangoFontDescription *desc, *desc_bold;
    desc = pango_font_description_from_string(get_font_props(NULL, NULL));
    if (number_of_tatamis > 10)
	pango_font_description_set_absolute_size(desc, 8*PANGO_SCALE);
    else
	pango_font_description_set_absolute_size(desc, 12*PANGO_SCALE);
    desc_bold = pango_font_description_copy(desc);
    pango_font_description_set_weight(desc_bold, PANGO_WEIGHT_BOLD);
    
    GtkStyleContext *style = gtk_widget_get_style_context(widget);
    gtk_style_context_save(style);
    gtk_style_context_set_state(style, state);

    x = cell_area->x + x_pad;
    y = cell_area->y + y_pad;
    y_pos = y;
    
    /* draw border */
    gtk_render_frame (style, c, x, y, w - 1, h - 1);

    /* fallback, if gtk_render_frame is now shown (as on my system) */
    //gtk_render_background (style, c, x, y, w -1, h - 1);
#if 0
    cairo_set_source_rgb(c, 1.0, 0, 0);
    cairo_rectangle(c,
                    draw_width, draw_height,
                    width - 1, height - 1);
    cairo_fill(c);
#endif

    if (tatami > 0 && position == 0) {
        if (last_wins[tatami-1].cat == next_matches_info[tatami-1][0].won_catnum &&
            last_wins[tatami-1].num == next_matches_info[tatami-1][0].won_matchnum)
            cairo_set_source_rgb(c, 0.7, 1.0, 0.7);
        else
            cairo_set_source_rgb(c, 1.0, 1.0, 0.0);

        cairo_rectangle(c, x, y, w, h);
        cairo_fill(c);
        cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
        WRITE_TEXT_2(x+5, y, _("Prev. winner:"), desc);
        y_pos += BOX_HEIGHT;
        WRITE_TEXT_2(x+5, y_pos, next_matches_info[tatami-1][0].won_cat, desc);

        gchar *txt = get_match_number_text(next_matches_info[tatami-1][0].won_catnum,
                                           next_matches_info[tatami-1][0].won_matchnum);
        if (txt)
            WRITE_TEXT_2(x+5+w/2, 0, txt, desc);

        y_pos += BOX_HEIGHT;
        WRITE_TEXT_2(x+5, y_pos, next_matches_info[tatami-1][0].won_first, desc);
        WRITE_TEXT(-1, -1, " ", desc);
        WRITE_TEXT(-1, -1, next_matches_info[tatami-1][0].won_last, desc);
        y_pos += BOX_HEIGHT;
    } else if (position > 0) {
        m = &mlist[position-1];
        if (m->number >= 1000) {
#if 0
            cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
            cairo_rectangle(c, x, y, w, h);
            cairo_fill(c);
#endif
        } else {
            struct category_data *catdata = avl_get_category(m->category);

            //cairo_save(c);

            if (have_drag && private (self)->match == drag_data)
                cairo_set_source_rgb(c, 0.5, 0.5, 0.5);
	    else if (catdata && show_colors)
                BGCOLOR;
            else if (m->forcednumber)
                cairo_set_source_rgb(c, 1.0, 1.0, 0.6);
            else if (m->forcedtatami)
                cairo_set_source_rgb(c, 1.0, 1.0, 0.9);
	    else
		cairo_set_source_rgb(c, 1.0, 1.0, 1.0);

            cairo_rectangle(c, x, y, w, h);
            cairo_fill(c);

            //cairo_restore(c);

            cairo_set_source_rgb(c, 0, 0, 0);

            //cairo_save(c);
            //cairo_select_font_face(c, MY_FONT, 0, CAIRO_FONT_WEIGHT_BOLD);
            //cairo_move_to(c, x+5, y_pos+extents.height);

            if (catdata) {
                if (catdata->deleted & TEAM_EVENT) {
                    gint ageix = find_age_index(catdata->category);
                    gint n = m->number;
                    if (n == 999)
                        n = MATCH_CATEGORY_CAT_GET(m->category);
                    
                    if (ageix >= 0 && n > 0 && n <= NUM_CAT_DEF_WEIGHTS) {
                        SNPRINTF_UTF8(buf, "%s%s #%d%s",
                                      m->number == 999 ? "*" : "",
                                      catdata->category,
                                      MATCH_CATEGORY_SUB_GET(m->category),
                                      category_definitions[ageix].weights[n - 1].weighttext);
                    } else
                        SNPRINTF_UTF8(buf, "%s", catdata->category);
                } else
                    SNPRINTF_UTF8(buf, "%s #%d",
                                  catdata->category, m->number);
            } else
                snprintf(buf, sizeof(buf), "?");

            WRITE_TEXT_2(x+5, y, buf, desc_bold);

            const gchar *txt = round_to_str(m->round);

	    if ((txt && txt[0]) || m->forcedtatami || tatami == 0) {
		buf[0] = 0;
#if 0
                cairo_text_extents(c, "", &extents);
                cairo_move_to(c, x+5+w/2, y_pos+extents.height);
#endif
                if (tatami == 0) {
                    if (txt && txt[0] && m->forcedtatami)
                        SNPRINTF_UTF8(buf, "T%d:%s", m->forcedtatami, txt);
                    else if (txt && txt[0])
                        SNPRINTF_UTF8(buf, "T%d:%s",
                                 catdata ? catdata->tatami : 0, txt);
                    else
                        SNPRINTF_UTF8(buf, "T%d",
                                 catdata ? catdata->tatami : 0);
                } else if (txt && txt[0] && m->forcedtatami)
                    SNPRINTF_UTF8(buf, "T%d:%s",
                             catdata ? catdata->tatami : 0,
                             txt);
                else if (m->forcedtatami)
                    snprintf(buf, sizeof(buf), "T%d", catdata ? catdata->tatami : 0);
                else if (txt && txt[0])
                    SNPRINTF_UTF8(buf, "%s", txt);
		WRITE_TEXT_2(x+5+w/2, y, buf, desc_bold);
            }
#if 0
            gint rt = (gint)rest_times[tatami-1] - (gint)time(NULL);
            if (position == 1 && tatami > 0 && rt > 0) {
                if (rest_flags[tatami-1] & MATCH_FLAG_BLUE_REST)
                    sprintf(buf, "<= %d:%02d ==", rt/60, rt%60);
                else
                    sprintf(buf, "== %d:%02d =>", rt/60, rt%60);
                cairo_set_source_rgb(c, 0.8, 0.0, 0.0);
		WRITE_TEXT_2(left+5+colwidth/2, y_pos, buf, desc_bold);
            }
#endif
            //cairo_restore(c);

            if (tatami >= 0) {
                gdouble _x, _y;
                struct judoka *j = get_data(m->blue);
                if (j) {
                    //cairo_save(c);

                    if (m->flags & MATCH_FLAG_BLUE_DELAYED) {
                        if (m->flags & MATCH_FLAG_BLUE_REST)
                            cairo_set_source_rgb(c, 0.5, 0.5, 1.0);
                        else
                            cairo_set_source_rgb(c, 1.0, 0.5, 0.5);
                        cairo_rectangle(c, x, y+BOX_HEIGHT, w/2, 3*BOX_HEIGHT);
                        cairo_fill(c);
                    }

                    cairo_set_source_rgb(c, 0, 0, 0);

                    _x = x+5;
                    _y = y+2*BOX_HEIGHT;
                    write_text(c, j->last, &_x, &_y, NULL, NULL,
                               TEXT_ANCHOR_START, TEXT_ANCHOR_TOP,
                               desc_bold, 0, 0);
                    //cairo_restore(c);
                    _y = y+BOX_HEIGHT;
                    write_text(c, j->first, &_x, &_y, NULL, NULL,
                               TEXT_ANCHOR_START, TEXT_ANCHOR_TOP,
                               desc, 0, 0);
                    _y = y+3*BOX_HEIGHT;
                    write_text(c, get_club_text(j, 0), &_x, &_y, NULL, NULL,
                               TEXT_ANCHOR_START, TEXT_ANCHOR_TOP,
                               desc, 0, 0);

                    free_judoka(j);
                }
                j = get_data(m->white);
                if (j) {
                    //cairo_save(c);

                    if (m->flags & MATCH_FLAG_WHITE_DELAYED) {
                        if (m->flags & MATCH_FLAG_WHITE_REST)
                            cairo_set_source_rgb(c, 0.5, 0.5, 1.0);
                        else
                            cairo_set_source_rgb(c, 1.0, 0.5, 0.5);
                        cairo_rectangle(c, x+w/2, y_pos+BOX_HEIGHT, w/2, 3*BOX_HEIGHT);
                        cairo_fill(c);

                    }
                    
		    if (txt && txt[0] && show_colors) {
			GdkRGBA *rgba = round_to_color(m->round);
			cairo_set_source_rgb(c, rgba->red, rgba->green, rgba->blue);
			cairo_rectangle(c, x+w*7/8, y_pos+BOX_HEIGHT, w/8, 3*BOX_HEIGHT);
			cairo_fill(c);
		    }
		    
                    cairo_set_source_rgb(c, 0, 0, 0);

                    _x = x+5+w/2;
                    _y = y+2*BOX_HEIGHT;
                    write_text(c, j->last, &_x, &_y, NULL, NULL,
                               TEXT_ANCHOR_START, TEXT_ANCHOR_TOP,
                               desc_bold, 0, 0);
                    //cairo_restore(c);
                    _y = y+BOX_HEIGHT;
                    write_text(c, j->first, &_x, &_y, NULL, NULL,
                               TEXT_ANCHOR_START, TEXT_ANCHOR_TOP,
                               desc, 0, 0);
                    _y = y+3*BOX_HEIGHT;
                    write_text(c, get_club_text(j, 0), &_x, &_y, NULL, NULL,
                               TEXT_ANCHOR_START, TEXT_ANCHOR_TOP,
                               desc, 0, 0);

                    free_judoka(j);
                } else if (txt && txt[0] && show_colors) {
		    GdkRGBA *rgba = round_to_color(m->round);
		    cairo_set_source_rgb(c, rgba->red, rgba->green, rgba->blue);
		    cairo_rectangle(c, x+w*7/8, y_pos+BOX_HEIGHT, w/8, 3*BOX_HEIGHT);
		    cairo_fill(c);
		}
            }
        } // match num < 1000
    }
    
    cairo_set_line_width(c, 1);
    cairo_set_source_rgb(c, 0, 0, 0);
    cairo_rectangle(c, x, y, w, h);
    cairo_stroke(c);

    gint dx = drag_x + adj_x;
    if (have_drag && dx >= x && dx <= x+w && drag_y >= y && drag_y <= y+h) {
        gint w1, h1;
        PangoLayout *layout;

        drag_after = drag_y > y + h/2;
        cairo_set_line_width(c, 4);
        cairo_set_source_rgb(c, 0, 0, 1.0);
        cairo_move_to(c, x, drag_after ? y + h - 3 : y + 2);
        cairo_rel_line_to(c, w, 0);
        cairo_stroke(c);

        layout = pango_cairo_create_layout(c);
	pango_layout_set_text(layout, drag_text, -1);
	pango_layout_set_font_description(layout, desc);
	pango_layout_get_size(layout, &w1, &h1);
	w1 /= PANGO_SCALE;
	h1 /= PANGO_SCALE;

        cairo_set_line_width(c, 1);
        cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
        cairo_rectangle(c, drag_x - w1/2, drag_y - h1/2, w1, h1);
        cairo_fill(c);
        cairo_set_source_rgb(c, 0, 0, 0);
        cairo_rectangle(c, drag_x - w1/2, drag_y - h1/2, w1, h1);
        cairo_stroke(c);

	cairo_move_to(c, drag_x - w1/2, drag_y - h1/2);
	pango_cairo_show_layout(c, layout);
        g_object_unref(layout);
    }

    pango_font_description_free(desc);
    pango_font_description_free(desc_bold);
    gtk_style_context_restore(style);
}

static gboolean traverse_rows(GtkTreeModel *model,
			      GtkTreePath *path,
			      GtkTreeIter *iter,
			      gpointer data)
{
    struct traverse *t = (struct traverse *)data;
    gint tatami = MATCH_DATA_TO_TATAMI(t->match_data);
    gint position = MATCH_DATA_TO_POSITION(t->match_data);
    guint match_data;
    
    gtk_tree_model_get(model, iter, tatami, &match_data, -1);

    switch (t->op) {
    case TRAVERSE_UPDATE_CELL:
        if (match_data == t->match_data) {
            gtk_list_store_set(match_table_store, iter, tatami, match_data, -1);
            return TRUE;
        }
        break;
    case TRAVERSE_UPDATE_COLUMN:
        gtk_list_store_set(match_table_store, iter, tatami, match_data, -1);
        break;
    case TRAVERSE_UPDATE_ALL:
        position = MATCH_DATA_TO_POSITION(match_data);
        gtk_list_store_set(match_table_store, iter,
                           0, MATCH_DATA_FROM_T_POS(0, position),
                           1, MATCH_DATA_FROM_T_POS(1, position),
                           2, MATCH_DATA_FROM_T_POS(2, position),
                           3, MATCH_DATA_FROM_T_POS(3, position),
                           4, MATCH_DATA_FROM_T_POS(4, position),
                           5, MATCH_DATA_FROM_T_POS(5, position),
                           6, MATCH_DATA_FROM_T_POS(6, position),
                           7, MATCH_DATA_FROM_T_POS(7, position),
                           8, MATCH_DATA_FROM_T_POS(8, position),
                           9, MATCH_DATA_FROM_T_POS(9, position),
                           10, MATCH_DATA_FROM_T_POS(10, position),
                           11, MATCH_DATA_FROM_T_POS(11, position),
                           12, MATCH_DATA_FROM_T_POS(12, position),
                           13, MATCH_DATA_FROM_T_POS(13, position),
                           14, MATCH_DATA_FROM_T_POS(14, position),
                           15, MATCH_DATA_FROM_T_POS(15, position),
                           16, MATCH_DATA_FROM_T_POS(16, position),
                           17, MATCH_DATA_FROM_T_POS(17, position),
                           18, MATCH_DATA_FROM_T_POS(18, position),
                           19, MATCH_DATA_FROM_T_POS(19, position),
                           20, MATCH_DATA_FROM_T_POS(20, position),
                           -1);
        break;
    }

    return FALSE;
}

#if 0
static void refresh_tatami(gint tatami)
{
    struct traverse t;
    t.match_data = MATCH_DATA_FROM_T_POS(tatami, 0);
    t.op = TRAVERSE_UPDATE_COLUMN;
    gtk_tree_model_foreach(GTK_TREE_MODEL(match_table_store), traverse_rows, &t);
}
#endif

void draw_match_table(void)
{
    struct traverse t;
    t.match_data = 0;
    t.op = TRAVERSE_UPDATE_ALL;
    if (match_table_store)
        gtk_tree_model_foreach(GTK_TREE_MODEL(match_table_store), traverse_rows, &t);
}

void set_table_rest_time(gint tatami, time_t rest_end, gint flags)
{
    rest_times[tatami-1] = rest_end;
    rest_flags[tatami-1] = flags;
}

static guint find_match_data_by_xy_update(gint x, gint y, gboolean update)
{
    GtkTreePath *path;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeViewColumn *col = NULL;
    gint i;
    guint match_data = 0;
    gint tatami = -1;
    //gint position;

    if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(match_table_view), x, y,
                                      &path, &col, NULL, NULL))
        return 0;
    
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(match_table_view));
    gtk_tree_model_get_iter(model, &iter, path);

    for (i = 0; i <= NUM_TATAMIS; i++) {
        if (col == columns[i]) {
            tatami = i;
            break;
        }
    }

    if (tatami < 0)
        goto out;
        
    gtk_tree_model_get(model, &iter, tatami, &match_data, -1);
    //position = MATCH_DATA_TO_POSITION(match_data);
    if (match_data & 0xfff00000) {
        g_print("ERROR: match_data=0x%x\n", match_data);
        return 0;
    }
    
    if (!update)
        goto out;
    
    gtk_widget_queue_draw(match_table_view);

#if 0
    gtk_list_store_set(match_table_store, &iter, tatami, match_data, -1);

    if (tatami > 0)
        gtk_list_store_set(match_table_store, &iter, tatami-1,
                           MATCH_DATA_FROM_T_POS(tatami-1, position), -1);

    if (tatami < number_of_tatamis)
        gtk_list_store_set(match_table_store, &iter, tatami+1,
                           MATCH_DATA_FROM_T_POS(tatami+1, position), -1);

    if (position > 1 /*gtk_tree_path_prev(path)*/) {
        gtk_tree_model_get_iter(model, &iter, path);
        gtk_list_store_set(match_table_store, &iter, tatami,
                           MATCH_DATA_FROM_T_POS(tatami, position-1), -1);
    }

    if (position < NEXT_MATCH_NUM - 1) {
        gtk_tree_path_next(path);
        gtk_tree_path_next(path);
        gtk_tree_model_get_iter(model, &iter, path);
        gtk_list_store_set(match_table_store, &iter, tatami,
                           MATCH_DATA_FROM_T_POS(tatami, position+1), -1);
    }
#endif
out:
    gtk_tree_path_free(path);

    return match_data;
}

static guint find_match_data_by_xy(gint x, gint y)
{
    return find_match_data_by_xy_update(x, y, FALSE);
}

static struct match *get_match_by_tatami_position(gint tatami, gint position)
{
    struct match *mlist;
    
    if (tatami == 0)
        mlist = db_matches_waiting();
    else
        mlist = get_cached_next_matches(tatami);

    return &mlist[position-1];
}

static struct match *get_match_by_match_data(gint match_data)
{
    return get_match_by_tatami_position(
        MATCH_DATA_TO_TATAMI(match_data),
        MATCH_DATA_TO_POSITION(match_data));
}

static void change_comment(GtkWidget *menuitem, gpointer userdata)
{
    gint match_data = (ptr_to_gint(userdata)) >> 4;
    gint cmd = (ptr_to_gint(userdata)) & 0x0f;
    struct match *m = get_match_by_match_data(match_data);
    //struct compsys sys = db_get_system(m->category);

    db_set_comment(m->category, m->number, cmd);
    update_matches(0, (struct compsys){0,0,0,0}, db_find_match_tatami(m->category, m->number));
}

static void freeze(GtkWidget *menuitem, gpointer userdata)
{
    gint match_data = (ptr_to_gint(userdata)) >> AREA_SHIFT;
    gint arg = (ptr_to_gint(userdata)) & ARG_MASK;
    struct match *m = get_match_by_match_data(match_data);

    db_freeze_matches(m->tatami, m->category, m->number, arg);
}

static void view_match_popup_menu(GtkWidget *treeview,
                                  GdkEventButton *event,
                                  guint match_data)
{
    GtkWidget *menu, *menuitem;
    gpointer p;
    struct match *m = get_match_by_match_data(match_data);

    menu = gtk_menu_new();

    menuitem = gtk_menu_item_new_with_label(_("Next match"));
    p = gint_to_ptr(COMMENT_MATCH_1 | (match_data << 4));
    g_signal_connect(menuitem, "activate",
                     (GCallback) change_comment, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Preparing"));
    p = gint_to_ptr(COMMENT_MATCH_2 | (match_data << 4));
    g_signal_connect(menuitem, "activate",
                     (GCallback) change_comment, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Delay the match"));
    p = gint_to_ptr(COMMENT_WAIT | (match_data << 4));
    g_signal_connect(menuitem, "activate",
                     (GCallback) change_comment, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Remove delay"));
    p = gint_to_ptr(COMMENT_EMPTY | (match_data << 4));
    g_signal_connect(menuitem, "activate",
                     (GCallback) change_comment, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    menuitem = gtk_menu_item_new_with_label(_("Freeze match order"));
    p = gint_to_ptr(FREEZE_MATCHES | (match_data << AREA_SHIFT));
    g_signal_connect(menuitem, "activate",
                     (GCallback) freeze, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Unfreeze exported"));
    p = gint_to_ptr(UNFREEZE_EXPORTED | (match_data << AREA_SHIFT));
    g_signal_connect(menuitem, "activate",
                     (GCallback) freeze, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Unfreeze imported"));
    p = gint_to_ptr(UNFREEZE_IMPORTED | (match_data << AREA_SHIFT));
    g_signal_connect(menuitem, "activate",
                     (GCallback) freeze, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label(_("Unfreeze this"));
    p = gint_to_ptr(UNFREEZE_THIS | (match_data << AREA_SHIFT));
    g_signal_connect(menuitem, "activate",
                     (GCallback) freeze, p);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    if (m && m->category >= 10000) {
        menuitem = gtk_menu_item_new_with_label(_("Show Sheet"));
        g_signal_connect(menuitem, "activate",
                         (GCallback) show_category_window,
                         gint_to_ptr(m->category));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));

    //refresh_window();
}

static void view_onRowActivated (GtkTreeView        *treeview,
				 GtkTreePath        *path,
				 GtkTreeViewColumn  *col,
				 gpointer            userdata)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    gint cat;

    model = gtk_tree_view_get_model(treeview);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter,
                           COL_MATCH_CAT, &cat,
                           -1);

        if (cat < 10000)
            return;

        category_window(cat);
    }
}

static gboolean view_onButtonPressed(GtkWidget *treeview,
                                     GdkEventButton *event,
                                     gpointer userdata)
{
    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS) {
        guint match_data;
#if 0
        GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(match_scrolled_window));
        GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(match_scrolled_window));
        gint drag_adjx = (gint)gtk_adjustment_get_value(hadj);
        GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(match_scrolled_window));
        gint drag_adjy = (gint)gtk_adjustment_get_value(vadj);
#endif
        drag_x0 = event->x;
        drag_y0 = event->y;
        
        match_data = find_match_data_by_xy(drag_x0, drag_y0);
        drag_data = match_data;
        gint tatami = MATCH_DATA_TO_TATAMI(match_data);
        gint position = MATCH_DATA_TO_POSITION(match_data);
        g_print("%s: x,y=%d,%d tatami=%d pos=%d\n", __FUNCTION__, drag_x0, drag_y0,
                tatami, position);
        
        if (tatami > 0 && tatami <= number_of_tatamis && position == 0) {
            last_wins[tatami-1].cat = next_matches_info[tatami-1][0].won_catnum;
            last_wins[tatami-1].num = next_matches_info[tatami-1][0].won_matchnum;
            return TRUE;
        }
        
        drag_text[0] = 0;
        struct match *m = get_match_by_match_data(drag_data);
        if (m) {
            struct category_data *catdata = avl_get_category(m->category);
            if (catdata)
                SNPRINTF_UTF8(drag_text, "%s:%d",
                              catdata->category, m->number);
        }
        
        if (match_data > 0 && event->button == 3) {
            view_match_popup_menu(treeview, event, match_data);
            return TRUE;
        }
    }

    return FALSE; /* we did not handle this */
}

GtkWidget *
create_view_and_model (void)
{
    GtkTreeViewColumn   *col;
    GtkCellRenderer     *renderer;
    GtkTreeIter          iter;
    int i, c;
    char                 buf[128];
  
    match_table_store = gtk_list_store_new(
        NUM_TATAMIS+1, // 21
        G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
        G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
        G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
        G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
        G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT,
        G_TYPE_UINT);

    for (i = 0; i < NEXT_MATCH_NUM; i++) {
        gtk_list_store_append(match_table_store, &iter);
        for (c = 0; c <= NUM_TATAMIS; c++) {
            gtk_list_store_set (match_table_store, &iter, c,
                                MATCH_DATA_FROM_T_POS(c, i), -1);
        }
    }

    match_table_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(match_table_store));

    g_object_unref(match_table_store); /* destroy store automatically with view */

    for (i = 0; i <= NUM_TATAMIS; i++) {
        if (i == 0) snprintf(buf, sizeof(buf), "%s", _("Delayed"));
        else snprintf(buf, sizeof(buf), "Tatami %d", i);

        renderer = custom_cell_renderer_match_new();
        //g_object_set(renderer,  "tatami", i, NULL);

        col = gtk_tree_view_column_new();
        columns[i] = col;
        g_print("columns[%d]=%p\n", i, col);
        gtk_tree_view_column_set_expand(col, TRUE);
        gtk_tree_view_column_pack_start (col, renderer, TRUE);
        gtk_tree_view_column_add_attribute (col, renderer, "match", i);
        gtk_tree_view_column_set_title (col, buf);
        gtk_tree_view_append_column(GTK_TREE_VIEW(match_table_view), col);
    }

    update_match_table();
  

    g_signal_connect(match_table_view, "row-activated", (GCallback) view_onRowActivated, NULL);
    g_signal_connect(match_table_view, "button-press-event", (GCallback) view_onButtonPressed, NULL);
    //g_signal_connect(view, "popup-menu", (GCallback) view_onPopupMenu, NULL);

    GtkTreeSelection    *selection;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(match_table_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(match_table_view), GTK_TREE_VIEW_GRID_LINES_BOTH);
#if 0
    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(match_table_view), GDK_BUTTON1_MASK,
                                           entries, G_N_ELEMENTS (entries), GDK_ACTION_COPY);
#endif
    return match_table_view;
}

void update_match_table(void)
{
    gint i;
    
    for (i = 0; i <= NUM_TATAMIS; i++) {
        //g_print("update_match_table columns[%d] == %p\n", i, columns[i]);
        if (columns[i])
            gtk_tree_view_column_set_visible(columns[i], i <= number_of_tatamis);
    }
}

static void target_drag_leave(GtkWidget	     *widget,
                              GdkDragContext *context,
                              guint           time)
{
    have_drag = FALSE;
    g_print("%s\n", __FUNCTION__);
}

static gboolean target_drag_motion(GtkWidget	  *widget,
                                   GdkDragContext *context,
                                   gint            x,
                                   gint            y,
                                   guint           time)
{
    gint allocw = gtk_widget_get_allocated_width(match_scrolled_window);
    gint alloch = gtk_widget_get_allocated_height(match_scrolled_window);
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(match_scrolled_window));
    GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(match_scrolled_window));
    adj_x = (gint)gtk_adjustment_get_value(hadj);
    adj_y = (gint)gtk_adjustment_get_value(vadj);
    gboolean xupdate = FALSE, yupdate = FALSE;
    
    if (x < 100) {
        adj_x -= 100;
        xupdate = TRUE;
    } else if (x > allocw - 100) {
        adj_x += 100;
        xupdate = TRUE;
    }
    if (y < 100) {
        adj_y -= 100;
        yupdate = TRUE;
    } else if (y > alloch - 100) {
        adj_y += 100;
        yupdate = TRUE;
    }
    if (xupdate) {
        gtk_adjustment_set_value(hadj, adj_x);
        adj_x = (gint)gtk_adjustment_get_value(hadj);
    }
    if (yupdate) {
        gtk_adjustment_set_value(vadj, adj_y);
        adj_y = (gint)gtk_adjustment_get_value(vadj);
    }
    
    drag_x = x;
    drag_y = y;
    g_print("%s: x,y=%d,%d adj=%d,%d\n", __FUNCTION__, x, y, adj_x, adj_y);
    
    if (!have_drag) {
        have_drag = TRUE;
        //drag_data = find_match_data_by_xy(x, y);
        g_print("%s x=%d y=%d data=%x\n", __FUNCTION__, x, y, drag_data);
#if 0
        struct match *m = get_match_by_match_data(drag_data);
        if (m) {
            gchar buf[64];
            gint w, h;
            PangoFontDescription *desc;
            cairo_text_extents_t extents;
            struct category_data *catdata = avl_get_category(m->category);
            if (catdata)
                SNPRINTF_UTF8(buf, "%s:%d", catdata->category, m->number);
            else
                SNPRINTF_UTF8(buf, "?");

            cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 50, 20);
            cairo_t *c = cairo_create(cs);
            cairo_text_extents(c, buf, &extents);
            PangoLayout *layout = pango_cairo_create_layout(c);
            desc = pango_font_description_from_string("Arial");
            pango_font_description_set_absolute_size(desc, extents.height*PANGO_SCALE);
            pango_layout_set_text(layout, buf, -1);
            pango_layout_set_font_description(layout, desc);
            pango_layout_get_size(layout, &w, &h);
            w /= PANGO_SCALE;
            h /= PANGO_SCALE;

            cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
            cairo_rectangle(c, 0, 0, 50, 20);
            cairo_fill(c);
            cairo_set_source_rgb(c, 0, 0, 0);
            cairo_rectangle(c, 0, 0, 50, 20);
            cairo_stroke(c);

            cairo_move_to(c, 25, 10);
            pango_cairo_show_layout(c, layout);
            g_object_unref(layout);
            pango_font_description_free(desc);

            //GdkCursor *cur = gdk_cursor_new_from_surface(GDK_DISPLAY(GTK_WIDGET(main_window)), cs, 0, 0);
            cairo_destroy(c);
            cairo_surface_destroy(cs);
            gdk_window_set_cursor(gtk_widget_get_window(main_window), gdk_cursor_new(GDK_WATCH));
        }
#endif

        //gtk_image_set_from_pixbuf (GTK_IMAGE (widget), trashcan_open);
    }

    find_match_data_by_xy_update(x, y, TRUE);
    


#if 0
    GList *tmp_list;
    tmp_list = gdk_drag_context_list_targets (context);
    while (tmp_list) {
        char *name = gdk_atom_name (GDK_POINTER_TO_ATOM (tmp_list->data));
        g_print ("%s\n", name);
        g_free (name);
      
        tmp_list = tmp_list->next;
    }
#endif
    gdk_drag_status (context, gdk_drag_context_get_suggested_action (context), time);

    return TRUE;
}

static gboolean target_drag_drop(GtkWidget	*widget,
                                 GdkDragContext *context,
                                 gint            x,
                                 gint            y,
                                 guint           time)
{
    g_print("%s\n", __FUNCTION__);
    have_drag = FALSE;
    return TRUE;
}

static void target_drag_data_received(GtkWidget        *widget,
                                      GdkDragContext   *context,
                                      gint              x,
                                      gint              y,
                                      GtkSelectionData *selection_data,
                                      guint             info,
                                      guint             time)
{
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(match_scrolled_window));
    GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(match_scrolled_window));
    adj_x = (gint)gtk_adjustment_get_value(hadj);
    adj_y = (gint)gtk_adjustment_get_value(vadj);

    g_print("%s x=%d y=%d adjx=%d adjy=%d\n", __FUNCTION__, x, y,
            adj_x, adj_y);

    if (gtk_selection_data_get_length (selection_data) >= 0 &&
        gtk_selection_data_get_format (selection_data) == 8) {
        g_print ("Received \"%s\" in label\n", (gchar *) gtk_selection_data_get_data (selection_data));
        gtk_drag_finish (context, TRUE, FALSE, time);

        guint match_data = find_match_data_by_xy(x + adj_x, y);
        gint tatami = MATCH_DATA_TO_TATAMI(match_data);
        gint pos = MATCH_DATA_TO_POSITION(match_data);
        struct match *m = get_match_by_match_data(drag_data);

        g_print("Match %d:%d to T=%d pos=%d\n", m->category, m->number, tatami, pos);
        if (pos > 0 && m)
            db_change_freezed(m->category,
                              m->number,
                              tatami,
                              pos, drag_after);
        
        return;
    }
  
    gtk_drag_finish (context, FALSE, FALSE, time);
}

static void source_drag_data_get(GtkWidget        *widget,
                                 GdkDragContext   *context,
                                 GtkSelectionData *selection_data,
                                 guint             info,
                                 guint             time,
                                 gpointer          data)
{
    g_print("%s\n", __FUNCTION__);
    if (info == TARGET_ROOTWIN)
        g_print ("I was dropped on the rootwin\n");
    else
        gtk_selection_data_set (selection_data,
                                gtk_selection_data_get_target (selection_data),
                                8, (guchar *) "I'm Data!", 9);
}

static void source_drag_data_delete(GtkWidget      *widget,
                                    GdkDragContext *context,
                                    gpointer        data)
{
    g_print("%s\n", __FUNCTION__);
    g_print ("Delete the data!\n");
}

static GtkTargetEntry target_table[] = {
  { "STRING",     0, TARGET_STRING },
  { "text/plain", 0, TARGET_STRING },
  { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

#if 0
static gboolean do_invalidation(gpointer data)
{
    if (invalidate) {
        invalidate = FALSE;
        //gtk_widget_queue_draw(match_table_view);
    }
    return TRUE;
}
#endif

gboolean scroll_entries(GtkWidget * widget, GdkEvent * event, gpointer data) {
    invalidate = TRUE;
    return FALSE; // Let other handlers respond to this event
}

static guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

void set_match_table_page(GtkWidget *nb)
{
    match_table_clear();

    match_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(match_scrolled_window), 10);
    create_view_and_model();
    gtk_container_add(GTK_CONTAINER(match_scrolled_window), match_table_view);
    //gtk_widget_show_all(match_scrolled_window);

    match_table_label = gtk_label_new (_("Matches"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), match_scrolled_window, match_table_label);

    gtk_drag_dest_set (match_table_view,
                       GTK_DEST_DEFAULT_MOTION|GTK_DEST_DEFAULT_DROP,
                       //GTK_DEST_DEFAULT_ALL,
                       target_table, n_targets - 1, /* no rootwin */
                       /*GDK_ACTION_COPY |*/ GDK_ACTION_MOVE);

    gtk_drag_source_set (match_table_view, GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
                         target_table, n_targets, 
                         /*GDK_ACTION_COPY |*/ GDK_ACTION_MOVE);

    //gtk_drag_source_set_icon_name(match_table_view, "document-save");

    g_signal_connect (match_table_view, "drag_leave",
                      G_CALLBACK (target_drag_leave), NULL);

    g_signal_connect (match_table_view, "drag_motion",
                      G_CALLBACK (target_drag_motion), NULL);

    g_signal_connect (match_table_view, "drag_drop",
                      G_CALLBACK (target_drag_drop), NULL);

    g_signal_connect (match_table_view, "drag_data_received",
                      G_CALLBACK (target_drag_data_received), NULL);

    g_signal_connect (match_table_view, "drag_data_get",
                      G_CALLBACK (source_drag_data_get), NULL);
    g_signal_connect (match_table_view, "drag_data_delete",
                      G_CALLBACK (source_drag_data_delete), NULL);

    g_signal_connect(match_scrolled_window, "scroll-event", G_CALLBACK(scroll_entries), NULL);

    //g_timeout_add(300, do_invalidation, NULL);
}

void set_match_table_titles(void)
{
    if (match_table_label)
        gtk_label_set_text(GTK_LABEL(match_table_label), _("Matches"));
}
