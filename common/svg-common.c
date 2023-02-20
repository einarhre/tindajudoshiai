#if defined(__WIN32__) || defined(WIN32)

#define  __USE_W32_SOCKETS
//#define Win32_Winsock

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <initguid.h>
#include <ws2tcpip.h>

#else /* UNIX */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#endif /* WIN32 */

#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <gdk/gdkkeysyms-compat.h>
#include <curl/curl.h>

#include <libintl.h>
#define _(String) gettext(String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif

#include "svg-common.h"
#include "comm.h"

extern const char *round_to_str(int round);

gchar *get_svg_file_name_common(GtkWindow *mainwin, GKeyFile *keyfile, const gchar *keyname)
{
    GtkWidget *dialog;
    static gchar *last_dir = NULL;
    gint response;
    gchar *svg_file = NULL;

    dialog = gtk_file_chooser_dialog_new (_("Open file"),
                                          GTK_WINDOW(mainwin),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

    if (last_dir)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), last_dir);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        svg_file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        g_free(last_dir);
        last_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (dialog));
        g_key_file_set_string(keyfile, "preferences", keyname, svg_file);
    } else if (response == GTK_RESPONSE_CLOSE) {
        svg_file = NULL;
        g_key_file_set_string(keyfile, "preferences", keyname, "");
    }

    gtk_widget_destroy (dialog);

    return svg_file;
}

void read_svg_file_common(svg_handle *svg, GtkWindow *mainwin)
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
        mylog("CANNOT OPEN SVG '%s'\n", svg->svg_file);
    else  {
        svg->datamax = svg->svg_data + svg->svg_datalen;
        RsvgHandle *h = rsvg_handle_new_from_data((guchar *)svg->svg_data, svg->svg_datalen, NULL);
        if (h) {
            RsvgDimensionData dim;
            rsvg_handle_get_dimensions(h, &dim);
            svg->svg_width = dim.width;
            svg->svg_height = dim.height;

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
                    if (!svg->imagename || !svg->imagename[n])
                        break;
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

void draw_image_common(svg_handle *pd, RsvgRectangle *rect, cairo_surface_t *image, gboolean fit)
{
    if (!image || cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
        return;

    gint w = cairo_image_surface_get_width(image);
    gint h = cairo_image_surface_get_height(image);
    gdouble scale_h = rect->height/h;
    gdouble scale_w = fit ? rect->width/w : scale_h;
    cairo_save(pd->c);
    cairo_scale(pd->c, scale_w, scale_h);
    cairo_set_source_surface(pd->c, image, rect->x/scale_w, rect->y/scale_h);
    cairo_paint(pd->c);
    cairo_restore(pd->c);
}

void draw_image_file_common(svg_handle *pd, RsvgRectangle *rect, const gchar *file, gboolean fit)
{
    cairo_surface_t *image = cairo_image_surface_create_from_png(file);
    draw_image_common(pd, rect, image, fit);
    cairo_surface_destroy(image);
}

void draw_flag_common(svg_handle *pd, RsvgRectangle *rect, const gchar *country)
{
    extern gchar *installation_dir;
    gchar name[16];
    snprintf(name, sizeof(name), "%s.png", country);
    gchar *file = g_build_filename(installation_dir, "etc", "flags-ioc", name, NULL);
    draw_image_file_common(pd, rect, file, FALSE);
    g_free(file);
}

gint paint_svg_common(svg_handle *svg)
{
    if (svg->svg_ok == FALSE || svg->svg_data == NULL)
        return FALSE;

    RsvgHandle *handle = rsvg_handle_new();
    //rsvg_handle_set_dpi(handle, 90.0);
    svg->handle = handle;
    
    guchar *p = (guchar *)svg->svg_data;
    while (p < (guchar *)svg->datamax && *p) {
        if (*p == '#' && p[1] == 'a' && p[2] == 'b' && p[3] == 'c' && p[4] == 'd') {
            svg->cnt = 1;
            memcpy(svg->attr[0].code, p, 7);
            svg->attr[0].code[7] = 0;
            p += 7;

            if (svg->svg_cb)
                svg->svg_cb(svg);
        }
        else if (*p == '%' && IS_LABEL_CHAR(p[1])) {
            memset(svg->attr, 0, sizeof(svg->attr));
            svg->cnt = 0;
            p++;
            while (IS_LABEL_CHAR(*p) || IS_VALUE_CHAR(*p) || *p == '-' || *p == '\'' || *p == '|' || *p == '!') {
                while (IS_LABEL_CHAR(*p))
                    svg->attr[svg->cnt].code[svg->attr[svg->cnt].codecnt++] = *p++;

                if (*p == '-') p++;

                while (IS_VALUE_CHAR(*p))
                    svg->attr[svg->cnt].value = svg->attr[svg->cnt].value*10 + *p++ - '0';

                if (*p == '-') p++;

                if (*p == '\'' || *p == '|') {
                    svg->cnt++;
                    p++;
                    svg->attr[svg->cnt].code[0] = '\'';
                    svg->attr[svg->cnt].codecnt = 1;
                    while (*p && *p != '\'' && *p != '|') {
                        svg->attr[svg->cnt].code[svg->attr[svg->cnt].codecnt] = *p++;
                        if (svg->attr[svg->cnt].codecnt < CODE_LEN_COMMON)
                            svg->attr[svg->cnt].codecnt++;
                    }
                    if (*p == '\'' || *p == '|')
                        p++;
                }

                svg->cnt++;

                if (*p == '!') {
                    p++;
                    break;
                }
            } // while IS_LABEL

            if (svg->svg_cb)
                svg->svg_cb(svg);
        } // *p = %
        else {
            WRITE2(p, 1);
            p++;
        }
    }

    rsvg_handle_close(handle, NULL);

    if (svg->c) {
        cairo_save(svg->c);
        cairo_scale(svg->c, svg->paper_width/svg->svg_width, svg->paper_height/svg->svg_height);

        if (svg->page_mask > 1) {
            gchar buf[16];
            snprintf(buf, sizeof(buf), "#page%d", svg->current_page+1);
            rsvg_handle_render_cairo_sub(handle, svg->c, buf);
        } else
            rsvg_handle_render_cairo(handle, svg->c);

        if (svg->img_cb)
            svg->img_cb(svg);
        cairo_restore(svg->c);
    }
    
    g_object_unref(handle);

    return TRUE;
}

static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct svg_memory *mem = (struct svg_memory *)userp;
 
    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if(ptr == NULL)
        return 0;  /* out of memory! */
 
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;
 
    return realsize;
}
 
gint ask_json_common(const gchar *s, struct svg_memory *chunk)
{
    extern gulong node_ip_addr, ssdp_ip_addr;
    gulong addr = node_ip_addr ? node_ip_addr : ssdp_ip_addr;
    struct in_addr inaddr;
    CURL *curl;
    CURLcode res;

    inaddr.s_addr = addr;
    
    curl = curl_easy_init();
    if (curl) {
        gchar buf[32];
        snprintf(buf, sizeof(buf), "http://%s:8088/json", inet_ntoa(inaddr));
        //mylog("CURL: %s\n%s\n", buf, s);
        curl_easy_setopt(curl, CURLOPT_URL, buf);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s);
        /* example.com is redirected, so we tell libcurl to follow redirection */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        /* Define our callback to get called when there's data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
        
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
 
        /* always cleanup */
        curl_easy_cleanup(curl);

        if (chunk->response) {
            chunk->json = cJSON_Parse(chunk->response);
        }
    }

    return 0;
}

void free_svg_memory_common(struct svg_memory *m)
{
    if (m->response)
        free(m->response);
    m->response = NULL;
    m->size = 0;
    if (m->json)
        cJSON_Delete(m->json);
    m->json = NULL;
}

struct cJSON *get_array_1_common(struct cJSON *lst, gint ix1)
{
    if (!lst) return NULL;
    return cJSON_GetArrayItem(lst, ix1);
}

struct cJSON *get_array_2_common(struct cJSON *lst, gint ix1, gint ix2)
{
    if (!lst) return NULL;
    struct cJSON *e1 = cJSON_GetArrayItem(lst, ix1);
    if (!e1) return NULL;
    return cJSON_GetArrayItem(e1, ix2);
}

gchar *get_array_1_str_common(struct cJSON *lst, gint ix1)
{
    if (!lst) return NULL;
    struct cJSON *e1 = cJSON_GetArrayItem(lst, ix1);
    if (!e1) return NULL;
    if (e1->type == cJSON_String) return e1->valuestring;
    return NULL;
}

gint get_array_1_int_common(struct cJSON *lst, gint ix1)
{
    if (!lst) return 0;
    struct cJSON *e1 = cJSON_GetArrayItem(lst, ix1);
    if (!e1) return 0;
    if (e1->type == cJSON_String) return atoi(e1->valuestring);
    if (e1->type == cJSON_Number) return e1->valueint;
    return 0;
}

gchar *get_array_2_str_common(struct cJSON *lst, gint ix1, gint ix2)
{
    if (!lst) return NULL;
    return get_array_1_str_common(cJSON_GetArrayItem(lst, ix1), ix2);
}

gint get_array_2_int_common(struct cJSON *lst, gint ix1, gint ix2)
{
    if (!lst) return 0;
    return get_array_1_int_common(cJSON_GetArrayItem(lst, ix1), ix2);
}
