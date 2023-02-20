#include "svg.h"

gchar *get_svg_file_name(svg_handle *svg)
{
    GtkWidget *dialog;
    static gchar *last_dir = NULL;
    gint response;

    dialog = gtk_file_chooser_dialog_new (_("Open file"),
                                          GTK_WINDOW(main_window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    if (last_dir)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        g_free(svg->svg_file);
        svg->svg_file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        g_free(last_dir);
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (dialog));
        g_key_file_set_string(keyfile, "preferences", "svgfile", svg->svg_file);
    } else if (response == GTK_RESPONSE_CLOSE) {
        svg->svg_ok = FALSE;
        g_free(svg->svg_file);
        svg->svg_file = NULL;
        g_key_file_set_string(keyfile, "preferences", "svgfile", "");
    }

    gtk_widget_destroy (dialog);

    return svg->svg_file;
}

void read_svg_file(svg_handle *svg)
{
    g_free(svg->svg_data);
    svg->svg_data = NULL;
    svg->svg_datalen = 0;
    svg->svg_ok = FALSE;
    memset(&svg->images, 0, sizeof(svg->images));
    svg->page_mask = 1;
    
    if (svg->svg_file == NULL || svg->svg_file[0] == 0)
        return;

    if (!g_file_get_contents(svg->svg_file, &svg->svg_data, &svg->svg_datalen, NULL))
        mylog("CANNOT OPEN '%s'\n", svg_file);
    else  {
        datamax = svg->svg_data + svg->svg_datalen;
        RsvgHandle *h = rsvg_handle_new_from_data((guchar *)svg->svg_data, svg->svg_datalen, NULL);
        if (h) {
            RsvgDimensionData dim;
            rsvg_handle_get_dimensions(h, &dim);
            svg->svg_width = dim.width;
            svg->svg_height = dim.height;

            if (svg->win.height < 100) {
                gint width;
                gint height;
                gtk_window_get_size(GTK_WINDOW(main_window), &width, &height);
                svg->win.height = height;
            }
            svg->win.width = svg->win.height*dim.width/dim.height;

            gint page;
            for (page = 0; page < NUM_SVG_PAGES; page++) {
                gchar buf[16];
                gint n;

                snprintf(buf, sizeof(buf), "#page%d", page+1);
                if (rsvg_handle_has_sub(h, buf))
                    svg->page_mask |= (1 << page);
                else if (page > 0)
                    break;
                
                for (n = 0; n < NUM_SVG_IMAGES; n++) {
                    if (page == 0) {
                        snprintf(buf, sizeof(buf), "#%s", svg->imagename[n]);
                        if (!rsvg_handle_has_sub(h, buf))
                            snprintf(buf, sizeof(buf), "#%s_%d", svg->imagename[n], page+1);
                    } else
                        snprintf(buf, sizeof(buf), "#%s_%d", svg->imagename[n], page+1);
                    
                    if (rsvg_handle_has_sub(h, buf)) {
                        RsvgPositionData pos;
                        rsvg_handle_get_dimensions_sub (h, &dim, buf);
                        rsvg_handle_get_position_sub(h, &pos, buf);
                        svg->images[page][n].rect.x = (gdouble)pos.x;
                        svg->images[page][n].rect.y = (gdouble)pos.y;
                        svg->images[page][n].rect.width = dim.em;
                        svg->images[page][n].rect.height = dim.ex;

                        /***
                        rsvg_handle_get_geometry_for_layer(h, buf, &viewport, &out_ink_rect,
                                                           &images[page][n].rect, NULL);
                        ***/
                        svg->images[page][n].exists = TRUE;
                    }
                }
            }

            g_object_unref(h);
            svg->svg_ok = TRUE;
        } else {
            mylog("Cannot open SVG file %s\n", svg->svg_file);
        }
    }
}
