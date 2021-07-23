/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2021 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>

#include <cairo.h>
#include <cairo-pdf.h>
#include <librsvg/rsvg.h>

#include "judoshiai.h"
#include "minilisp.h"

#define WRITE2(_s, _l)                                                  \
    do {if (dfile) fwrite(_s, 1, _l, dfile);                            \
        if (!rsvg_handle_write(handle, (guchar *)_s, _l, &err)) {       \
            g_print("\nERROR %s: %s %d\n",                              \
                    err->message, __FUNCTION__, __LINE__); err = NULL; return TRUE; } } while (0)

static FILE *dfile = NULL;

void read_svg_file_lisp(void);

static gboolean nomarg = FALSE;
static gboolean scale = TRUE;

static gchar *svg_data = NULL;
static gchar *svg_end = NULL;
static gsize  svg_datalen = 0;
static gint   svg_width;
static gint   svg_height;
static gboolean svg_ok = FALSE;
static RsvgHandle *handle;
static gchar *svg_lisp_dir_name = NULL;
static cairo_surface_t *surface = NULL;
static GtkWidget *darea1;

static void svg_read_lisp_files(gchar *dirname, GtkTextBuffer *buffer)
{
    gchar *fullname;

    if (dirname == NULL)
        return;

    g_print("scanning dir %s for lisp files\n", dirname);
    GDir *dir = g_dir_open(dirname, 0, NULL);
    if (dir) {
        const gchar *fname = g_dir_read_name(dir);
        while (fname) {
            fullname = g_build_filename(dirname, fname, NULL);
	    gchar *a = strstr(fname, ".lisp");
            if (a && a[5] == 0) {
		gchar *contents;
		gsize len;
		if (!g_file_get_contents(fullname, &contents, &len, NULL))
		    g_print("CANNOT OPEN '%s'\n", fullname);
		else  {
		    gchar *r;
		    g_print("reading lisp file %s\n", fullname);
		    if ((r = lisp_exe(contents))) {
			if (buffer) {
			    gtk_text_buffer_insert_at_cursor(buffer, r, strlen(r));
			    gtk_text_buffer_insert_at_cursor(buffer, "\n", 1);
			}
		    }
		    g_free(contents);
		}
	    } // if lisp

            g_free(fullname);
            fname = g_dir_read_name(dir);
	} // while

	g_dir_close(dir);
    } // dir
}

static void svg_lisp_read_file(gchar *filename, GtkTextBuffer *buffer)
{
    gchar *p;

    g_free(svg_data);
    svg_data = NULL;
    svg_end = NULL; // last </g>
    svg_datalen = 0;
    svg_ok = FALSE;

    if (filename == NULL || filename[0] == 0)
        return;

    gchar * dname = g_path_get_dirname(filename);
    if (!svg_lisp_dir_name ||
	strcmp(svg_lisp_dir_name, dname)) {
	g_free(svg_lisp_dir_name);
	svg_lisp_dir_name = dname;
	svg_read_lisp_files(svg_lisp_dir_name, buffer);
    } else
	g_free(dname);

    if (!g_file_get_contents(filename, &svg_data, &svg_datalen, NULL)) {
        g_print("CANNOT OPEN '%s'\n", filename);
	return;
    }

    if (svg_datalen > 40) {
	p = &(svg_data[svg_datalen - 5]);
	while (strncmp(p, "</svg", 5) && p > svg_data)
	    p--;
	if (p > svg_data) {
	    p -= 4;
	    while (strncmp(p, "</g", 3) && p > svg_data)
		p--;
	    if (p > svg_data)
		svg_end = p;
	}
    }

    RsvgHandle *h = rsvg_handle_new_from_data((guchar *)svg_data, svg_datalen, NULL);
    if (h) {
	RsvgDimensionData dim;
	rsvg_handle_get_dimensions(h, &dim);
	svg_width = dim.width;
	svg_height = dim.height;
	g_object_unref(h);
	svg_ok = TRUE;
    } else {
	g_print("Cannot open SVG file %s\n", filename);
    }
}

gint paint_svg_lisp(struct paint_data *pd)
{
    GError *err = NULL;

    if (svg_ok == FALSE)
        return FALSE;

    if (pd->c) {
	cairo_set_source_rgb(pd->c, 1.0, 1.0, 1.0);
	cairo_rectangle(pd->c, 0.0, 0.0, pd->paper_width, pd->paper_height);
	cairo_fill(pd->c);
    }

    handle = rsvg_handle_new();

    guchar *p = (guchar *)svg_data;
    guchar *limit = p + svg_datalen;
    gchar *lisp_end_code_b = NULL;
    gchar *lisp_end_code_e = NULL;

    //g_print("Open SVG debug file\n");
    //dfile = fopen("debug.svg", "w");

    while (*p && p < limit) {
        if ((*p == '%' && p[1] == '(') ||
	    (*p == '%' && p[1] == '!' && p[2] == '(')) {
	    gboolean put_to_end = FALSE;
	    if (p[1] == '!') {
		put_to_end = TRUE;
		p += 2;
	    } else {
		p++;
	    }

	    gchar *lisp_code = (gchar *)p;
	    gint par = 1;
	    p++;
	    while (p < limit && par) {
		if (*p == '(') par++;
		else if (*p == ')') par--;
		p++;
	    }
	    if (p < limit) {
		if (put_to_end) { // execute lisp in the end
		    lisp_end_code_b = lisp_code;
		    lisp_end_code_e = (gchar *)p;
		} else { // execute lisp now
		    guchar saved = *p;
		    gchar *r;
		    *p = 0;
		    lisp_print_script = 1;
		    if ((r = lisp_exe(lisp_code))) {
			if (pd->closure) {
			    gtk_text_buffer_insert_at_cursor(pd->closure, r, strlen(r));
			    gtk_text_buffer_insert_at_cursor(pd->closure, "\n", 1);
			}
		    }
		    lisp_print_script = 0;
		    *p = saved;
		}
	    }
        } else { // *p != %
	    if ((gchar *)p == svg_end && lisp_end_code_b) {
		guchar saved = *lisp_end_code_e;
		gchar *r;
		*lisp_end_code_e = 0;
		lisp_print_script = 1;
		if ((r = lisp_exe(lisp_end_code_b))) {
		    if (pd->closure) {
			gtk_text_buffer_insert_at_cursor(pd->closure, r, strlen(r));
			gtk_text_buffer_insert_at_cursor(pd->closure, "\n", 1);
		    }
		}
		lisp_print_script = 0;
		*lisp_end_code_e = saved;
	    }
            WRITE2(p, 1);
            p++;
        }
    } // while

    rsvg_handle_close(handle, NULL);

    if (scale && pd->c) {
        cairo_save(pd->c);
        cairo_scale(pd->c, pd->paper_width/svg_width, pd->paper_height/svg_height);
        rsvg_handle_render_cairo(handle, pd->c);
        cairo_restore(pd->c);
    } else if (pd->c) {
        rsvg_handle_render_cairo(handle, pd->c);
    }

    g_object_unref(handle);

    if (dfile) {
        fclose(dfile);
        dfile = NULL;
        g_print("SVG debug file closed\n");
    }

    return TRUE;
}

int lisp_write_script(char *txt, int len)
{
    GError *err = NULL;
    WRITE2(txt, len);
    return 0;
}

#define SIZEX 630
#define SIZEY 891

static gboolean svg_lisp_expose(GtkWidget *widget, GdkEventExpose *event, gpointer userdata)
{
    struct paint_data *pd = userdata;
    static cairo_surface_t *print_icon = NULL;

    if (print_icon == NULL) {
        gchar *file = g_build_filename(installation_dir, "etc", "png", "print.png", NULL);
        print_icon = cairo_image_surface_create_from_png(file);
        g_free(file);
    }

    if (pd->scroll) {
        GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(pd->scroll));
        gtk_adjustment_set_upper(GTK_ADJUSTMENT(adj), pd->paper_width*SIZEY/SIZEX);
        gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(pd->scroll), adj);
    }

    cairo_t *c = (cairo_t *)event;
    cairo_set_source_surface(c, surface, 0, 0);
    cairo_paint(c);

    cairo_set_source_surface(c, print_icon, 0, 0);
    cairo_paint(c);
    return FALSE;
}

static gboolean svg_lisp_delete_event( GtkWidget *widget,
				       GdkEvent  *event,
				       gpointer   data )
{
    return FALSE;
}

static void svg_lisp_destroy( GtkWidget *widget,
			      gpointer   data )
{
    if (surface)
	cairo_surface_destroy(surface);
    surface = NULL;
    g_free(data);
}

static void draw_page(GtkPrintOperation *operation,
		      GtkPrintContext   *context,
		      gint               page_nr,
		      gpointer           user_data)
{
    struct paint_data *pd = user_data;
    GtkPageSetup *setup = gtk_print_context_get_page_setup(context);

    extern void lisp_set_page(int p);
    lisp_set_page(page_nr);

    if (nomarg) {
        gtk_page_setup_set_top_margin(setup, 0.0, GTK_UNIT_POINTS);
        gtk_page_setup_set_bottom_margin(setup, 0.0, GTK_UNIT_POINTS);
        gtk_page_setup_set_left_margin(setup, 0.0, GTK_UNIT_POINTS);
        gtk_page_setup_set_right_margin(setup, 0.0, GTK_UNIT_POINTS);
    }
    pd->c = gtk_print_context_get_cairo_context(context);
    pd->paper_width = gtk_print_context_get_width(context);
    pd->paper_height = gtk_print_context_get_height(context);
    paint_svg_lisp(pd);
 }

static void begin_print(GtkPrintOperation *operation,
                        GtkPrintContext   *context,
                        gpointer           user_data)
{
    extern int lisp_get_pages(void);
    gtk_print_operation_set_n_pages(operation, lisp_get_pages());
}

static void init_display(GtkWidget *widget, struct paint_data *pd)
{
    if (!surface) return;
    pd->c = cairo_create(surface);
    paint_svg_lisp(pd);
    cairo_show_page(pd->c);
    cairo_destroy(pd->c);
    pd->c = NULL;
    gdk_window_invalidate_rect(gtk_widget_get_window(widget), NULL, TRUE);
}

static gboolean svg_lisp_print(GtkWidget *window,
			       GdkEventButton *event,
			       gpointer userdata)
{
    struct paint_data *pd = userdata;

    if (event->type == GDK_BUTTON_PRESS  &&
        (event->button == 1 || event->button == 3)) {
        gint x = event->x, y = event->y;

        if (x < 32 && y < 32) {
	    GtkPrintOperation *print;

	    print = gtk_print_operation_new();

	    g_signal_connect (print, "begin_print", G_CALLBACK (begin_print), pd);
	    g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), pd);

	    gtk_print_operation_set_use_full_page(print, FALSE);
	    gtk_print_operation_set_unit(print, GTK_UNIT_POINTS);

	    gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				    GTK_WINDOW (main_window), NULL);

	    g_object_unref(print);
            return TRUE;
        } else  {
	    extern void lisp_increment_page(void);
	    lisp_increment_page();

            GtkWidget *widget;
            widget = GTK_WIDGET(window);
	    init_display(widget, pd);

            return TRUE;
        }
    }

    return FALSE;
}

void svg_lisp_window(gchar *filename, GtkTextBuffer *buffer)
{
    GdkScreen *scr = gdk_screen_get_default();
    gint scr_height = gdk_screen_get_height(scr);
    gint height_req = 800, width_req = 600;
    gboolean landscape = FALSE;

    svg_lisp_read_file(filename, buffer);
    height_req = svg_height;
    width_req = svg_width;
    scale = TRUE;

    if (height_req > scr_height && scr_height > 100) {
        height_req = scr_height - 20;
	width_req = (svg_width*height_req)/svg_height;
	scale = TRUE;
    }

    landscape = width_req > height_req;

    GtkWindow *window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_title(GTK_WINDOW(window), "Lisp");
    gtk_widget_set_size_request(GTK_WIDGET(window), 600 /*width_req+20*/, 600 /*height_req+20*/);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);

    darea1 = gtk_drawing_area_new();
    gtk_widget_set_size_request(GTK_WIDGET(darea1), width_req, height_req);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
    gtk_container_add(GTK_CONTAINER(scrolled_window), darea1);
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);
    gtk_widget_show_all(GTK_WIDGET(window));

    struct paint_data *pd = g_malloc(sizeof(*pd));
    memset(pd, 0, sizeof(*pd));
    pd->scroll = scrolled_window;
    pd->filename = filename;
    pd->landscape = landscape;
    pd->closure = buffer;

    pd->paper_width = gtk_widget_get_allocated_width(darea1);
    pd->paper_height = gtk_widget_get_allocated_height(darea1);

    gtk_widget_add_events(darea1,
                          GDK_BUTTON_PRESS_MASK);

    extern void lisp_set_page(int p);
    lisp_set_page(0);

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_req, height_req);
    init_display(darea1, pd);
    //gtk_window_present(GTK_WINDOW(window));

    g_signal_connect(G_OBJECT(darea1),
                     "button-press-event", G_CALLBACK(svg_lisp_print), pd);
    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (svg_lisp_delete_event), pd);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (svg_lisp_destroy), pd);
    g_signal_connect(G_OBJECT(darea1),
                     "draw", G_CALLBACK(svg_lisp_expose), pd);
}
