#ifndef _MATCH_TABLE_H_
#define _MATCH_TABLE_H_

#include <gtk/gtk.h>

/* Some boilerplate GObject type check and type cast macros. */

#define CUSTOM_TYPE_CELL_RENDERER_MATCH             (custom_cell_renderer_match_get_type())
G_DECLARE_DERIVABLE_TYPE(CustomCellRendererMatch, custom_cell_renderer_match, CUSTOM, CELL_RENDERER_MATCH, GtkCellRenderer)


struct _CustomCellRendererMatchClass
{
  GtkCellRendererClass  parent_class;
};


GType                custom_cell_renderer_match_get_type (void);

GtkCellRenderer     *custom_cell_renderer_match_new (void);


#endif /* _MATCH_TABLE_H_ */
