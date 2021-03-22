/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2020 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <fcntl.h>
#include <signal.h>

#endif /* WIN32 */

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include "judoshiai.h"
#include "common-utils.h"
#include "cJSON.h"

#include "microhttpd.h"

#define httpp_get_query_param(_parser, _str) \
    MHD_lookup_connection_value(_parser, MHD_GET_ARGUMENT_KIND, _str)

#define http_parser_t struct MHD_Connection
#define param_t struct MHD_Connection

#define FSTART                                                  \
    string s;                                                   \
    string_init(&s)

#define FEND return www_send(parser, &s, "text/html; charset=utf-8")
#define FEND_PNG return www_send(parser, &s, "image/png")
#define FEND_404 return www_send(parser, &s, NULL)
#define FEND_MIME(_mime) return www_send(parser, &s, _mime)

#define SCC(_a...) string_concat(&s, _a)

extern void get_websock(http_parser_t *parser, gint connum);

int index_html(http_parser_t *parser, gchar *txt);
void send_html_top(string *s, http_parser_t *parser, gchar *bodyattr);
void send_html_bottom(string *s, http_parser_t *parser);

#define PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define AUTHPAGE "<html><head><title>libmicrohttpd demo</title></head><body>Access granted</body></html>"
#define AUTHDENIED "<html><head><title>libmicrohttpd demo</title></head><body>Access denied</body></html>"
#define AUTHOPAQUE "11733b200778ce33060f31c9af70a870ba96ddd4"
#define LOGOUTPAGE "<html><head><title>libmicrohttpd demo</title></head><body>Logout</body></html>"
#define INTERNAL_ERROR_PAGE "<html><head><title>Internal error</title></head><body>Internal error</body></html>"
#define REQUEST_REFUSED_PAGE "<html><head><title>Request refused</title></head><body>Request refused (file exists?)</body></html>"
#define DUMMY_PAGE "<html><head><title></title></head><body></body></html>"

/* System-dependent definitions */
#ifndef WIN32
#define closesocket     close
#define SOCKET          int
#define HANDLE          int
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define INVALID_HANDLE_VALUE -1
#define SOCKADDR_BTH    struct sockaddr_rc
#define AF_BTH          PF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM
#define int_to_handle(_x) (_x)
#define handle_to_int(_x) (_x)
#else // WIN32
#define O_RDONLY S_IREAD
#define int_to_handle(_x) gint_to_ptr(_x)
#define handle_to_int(_x) ptr_to_gint(_x)
#endif

int sigreceived = 0;

struct judoka unknown_judoka = {
    .index = 0,
    .last = "?",
    .first = "?",
    .club = "?",
    .country = "",
    .regcategory = "?",
    .category = "?",
    .id = "",
    .comment = "",
    .coachid = ""
};

void sighandler(int sig)
{
    printf("sig received\n");
    sigreceived = 1;
}

int reply_web(http_parser_t *parser, struct msg_web_resp *resp);

gint mysend(SOCKET s, const gchar *p, gint len)
{
    gint n;
#if 0
    g_print("-->");
    for (n = 0; n < len; n++)
        g_print("%c", p[n]);
    g_print("<--\n");
#endif
    while (len > 0) {
        n = send(s, p, len, 0);
        if (n < 0) {
            g_print("mysend: cannot send (%d)\n", n);
            return n;
        }
        len -= n;
        p += n;
        if (len) {
            g_print("mysend: only %d bytes sent\n", n);
        }
    }
    return len;
}

gint sendf(SOCKET s, gchar *fmt, ...)
{
    gint ret;
    gchar *r = NULL;
    va_list ap;
    va_start(ap, fmt);
    gint n = g_vasprintf(&r, fmt, ap);
    va_end(ap);
    ret = mysend(s, r, n);
    g_free(r);
    return ret;
}

gint www_send(http_parser_t *parser, string *s, const gchar *mime)
{
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(s->len, s->buf, MHD_RESPMEM_MUST_FREE);
    if (!response) {
	g_printerr("ERROR: response\n");
	string_free(s);
	return MHD_NO;
    }
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, mime);
    int ret = MHD_queue_response(parser, mime ? MHD_HTTP_OK : MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    
    return ret;
}

#define NUM_ADDRESSES 24
static gulong ipaddresses[NUM_ADDRESSES] = {0};

static void remove_accepted(gulong addr)
{
    gint i;
    for (i = 0; i < NUM_ADDRESSES; i++)
        if (ipaddresses[i] == addr)
            ipaddresses[i] = FALSE;
}

static void add_accepted(gulong addr)
{
    gint i;

    remove_accepted(addr);

    for (i = 0; i < NUM_ADDRESSES; i++)
        if (ipaddresses[i] == 0) {
            ipaddresses[i] = addr;
            return;
        }
    g_print("ERROR: No more space for accepted IP addresses!\n");
}

static gboolean is_accepted(http_parser_t *parser)
{
#if 1
    return TRUE;
    //struct sockaddr_in *a = (struct sockaddr_in *)parser->addr;
    //gulong addr = a ? a->sin_addr.s_addr : 0;
#else
    gulong addr = parser->address;
    gint i;
    if (webpwcrc32 == 0)
        return TRUE;
    for (i = 0; i < NUM_ADDRESSES; i++)
        if (ipaddresses[i] == addr) {
            return TRUE;
        }
    return FALSE;
#endif
}

#define HIDDEN(x) do {							\
        if (x) h_##x = x;                                               \
        if (h_##x) {                                                    \
            string_concat(s, "<input type=\"hidden\" name=\"h_" #x "\" value=\"%s\">\r\n", h_##x); \
        }                                                               \
    } while (0)

void send_point_buttons(string *s, gint catnum, gint matchnum, gboolean blue)
{
    gchar m[64];
    gchar c[64];

    sprintf(m, "M%c-%d-%d", blue?'B':'W', catnum, matchnum);
    if (blue)
        sprintf(c, "style=\"background:#0000ff none; color:#ffffff; \"");
    else
        sprintf(c, "style=\"background:#ffffff none; color:#000000; \"");

    string_concat(s, 
          "<table><tr>"
          "<td><input type=\"submit\" name=\"%s-10\" value=\"10\" %s></td>"
          "<td><input type=\"submit\" name=\"%s-7\" value=\"7\" %s></td>"
          "<td><input type=\"submit\" name=\"%s-5\" value=\"5\" %s></td>"
          "<td><input type=\"submit\" name=\"%s-3\" value=\"3\" %s></td>"
          "<td><input type=\"submit\" name=\"%s-1\" value=\"1\" %s></td>"
          "</tr></table>",
          m, c, m, c, m, c, m, c, m, c);
}

int get_categories(http_parser_t *parser)
{
    FSTART;
#if 0
    gchar buf[1024];
    gint row;

    gchar *tatami = NULL;
    gchar *h_tatami = NULL;
    gchar *id = NULL;
    const gchar *h_id = NULL;
    gchar *win = NULL;

    avl_node *node;
    http_var_t *var;
    node = avl_get_first(parser->queryvars);
    while (node) {
        var = (http_var_t *)node->key;
        if (var) {
            //printf("Query variable: '%s'='%s'\n", var->name, var->value);
            if (var->name[0] == 'X')
                id = var->name+1;
            else if (var->name[0] == 'M')
                win = var->name+1;
            if (!strcmp(var->name, "h_id"))
                h_id = var->value;
            else if (!strncmp(var->name, "tatami", 6))
                tatami = var->name+6;
            else if (!strcmp(var->name, "h_tatami"))
                h_tatami = var->value;
        }
        node = avl_get_next(node);
    }

    if (win && h_tatami) {
        gchar catbuf[64];
        gint cat, num, pts;
        gint t = atoi(h_tatami) - 1;
        sscanf(win+1, "-%d-%d-%d", &cat, &num, &pts);
        //g_print("%d %d %d\n", cat, num, pts);

        if (win[0] == 'R') {
            db_set_comment(next_matches_info[t][1].catnum,
                           next_matches_info[t][1].matchnum,
                           COMMENT_EMPTY);
            db_set_comment(next_matches_info[t][0].catnum,
                           next_matches_info[t][0].matchnum,
                           COMMENT_MATCH_2);
            db_set_comment(cat, num, COMMENT_MATCH_1);
        }

        struct message msg;
        memset(&msg, 0, sizeof(msg));

        msg.type = MSG_SET_POINTS;
        msg.u.set_points.category     = cat;
        msg.u.set_points.number       = num;
        msg.u.set_points.sys          = 0;
        msg.u.set_points.blue_points  = (win[0] == 'B') ? pts : 0;
        msg.u.set_points.white_points = (win[0] == 'W') ? pts : 0;

        struct message *msg2 = put_to_rec_queue(&msg);
        time_t start = time(NULL);
        while ((time(NULL) < start + 5) && (msg2->type != 0))
            g_usleep(10000);

        if (win[0] == 'R')
            next_matches_info[t][0].won_catnum = 0;

        sprintf(catbuf, "%d", cat);
        httpp_set_query_param(parser, "h_id", catbuf);
        h_id = httpp_get_query_param(parser, "h_id");
    }

    send_html_top(&s, parser, "");

    sendf(s, "<form name=\"catform\" action=\"categories\" method=\"get\">");

    HIDDEN(id);
    HIDDEN(tatami);

    gchar *style0 = "style=\"width:6em; \"";
    gchar *style1 = "style=\"width:6em; font-weight:bold; \"";

    sendf(s, "<table><tr>"
          "<td><input type=\"submit\" name=\"tatami1\" value=\"Tatami 1\" %s></td>\r\n"
          "<td><input type=\"submit\" name=\"tatami2\" value=\"Tatami 2\" %s></td>\r\n"
          "<td><input type=\"submit\" name=\"tatami3\" value=\"Tatami 3\" %s></td>\r\n"
          "<td><input type=\"submit\" name=\"tatami4\" value=\"Tatami 4\" %s></td>\r\n"
          "<td><input type=\"submit\" name=\"tatami5\" value=\"Tatami 5\" %s></td>\r\n"
          "<td><input type=\"submit\" name=\"tatami6\" value=\"Tatami 6\" %s></td>\r\n"
          "<td><input type=\"submit\" name=\"tatami7\" value=\"Tatami 7\" %s></td>\r\n"
          "<td><input type=\"submit\" name=\"tatami8\" value=\"Tatami 8\" %s></td>\r\n"
          "</tr></table>\r\n",
          (h_tatami && h_tatami[0]=='1') ? style1 : style0,
          (h_tatami && h_tatami[0]=='2') ? style1 : style0,
          (h_tatami && h_tatami[0]=='3') ? style1 : style0,
          (h_tatami && h_tatami[0]=='4') ? style1 : style0,
          (h_tatami && h_tatami[0]=='5') ? style1 : style0,
          (h_tatami && h_tatami[0]=='6') ? style1 : style0,
          (h_tatami && h_tatami[0]=='7') ? style1 : style0,
          (h_tatami && h_tatami[0]=='8') ? style1 : style0);

    gint numrows = 0, numcols = 0;
    gchar **tablecopy = NULL;

    sendf(s, "<table border=\"1\" valign=\"top\">"
          "<tr><th>%s</th><th>%s</th><th>%s</th></tr>"
          "<tr><td valign=\"top\">\r\n", _("Matches"), _("Categories"), _("Sheet"));
	
    /* next matches */
    if (h_tatami) {
        gint i = atoi(h_tatami) - 1, k;
        gchar *xf, *xl, *xc;

        sendf(s, "<table class=\"nextmatches\">");

        if (next_matches_info[i][0].won_catnum) {
            xf = next_matches_info[i][0].won_first;
            xl = next_matches_info[i][0].won_last;
            xc = next_matches_info[i][0].won_cat;
        } else {
            xf = "";
            xl = "";
            xc = "";
        }

        sendf(s, "<tr><td class=\"cpl\">%s:<br>", _("Previous winner"));
        if (is_accepted(parser))
            sendf(s, "<input type=\"submit\" name=\"MR-%d-%d-0\" value=\"%s\">",
                  next_matches_info[i][0].won_catnum, next_matches_info[i][0].won_matchnum,
                  _("Cancel the match"));
        sendf(s, "</td><td class=\"cpr\">%s<br>%s %s<br>&nbsp;</td>", xc, xf, xl);

        //g_static_mutex_lock(&next_match_mutex);
        struct match *m = get_cached_next_matches(i+1);

        for (k = 0; m[k].number != 1000 && k < INFO_MATCH_NUM; k++) {
            gchar *class_ul = (k & 1) ? "class=\"cul1\"" : "class=\"cul2\"";
            gchar *class_ur = (k & 1) ? "class=\"cur1\"" : "class=\"cur2\"";
            gchar *class_dl = (k & 1) ? "class=\"cdl1\"" : "class=\"cdl2\"";
            gchar *class_dr = (k & 1) ? "class=\"cdr1\"" : "class=\"cdr2\"";
            gchar *class_cl = (k & 1) ? "class=\"ccl1\"" : "class=\"ccl2\"";
            gchar *class_cr = (k & 1) ? "class=\"ccr1\"" : "class=\"ccr2\"";
            struct judoka *blue, *white, *cat;
            blue = get_data(m[k].blue);
            white = get_data(m[k].white);
            cat = get_data(m[k].category);

            if (!blue)
                blue = &unknown_judoka;

            if (!white)
                white = &unknown_judoka;

            if (!cat)
                cat = &unknown_judoka;

            sendf(s, "<tr><td %s><b>%s %d:</b></td><td %s><b>%s</b></td></tr>", 
                  class_ul, _("Match"), m[k].number, class_ur, cat->last);

            if (k == 0 && is_accepted(parser)) {
                sendf(s, "<tr><td %s>", class_cl);
                send_point_buttons(s, m[k].category, m[k].number, TRUE);
                sendf(s, "</td><td %s>", class_cr);
                send_point_buttons(s, m[k].category, m[k].number, FALSE);
                sendf(s, "</td></tr>");
            }

            sendf(s, "<tr><td %s>%s %s<br>%s<br>&nbsp;</td><td %s>%s %s<br>%s<br>&nbsp;</td></tr>", 
                  class_dl, blue->first, blue->last, get_club_text(blue, 0), 
		  class_dr, white->first, white->last, get_club_text(white, 0));
			
            if (blue != &unknown_judoka)
                free_judoka(blue);
            if (white != &unknown_judoka)
                free_judoka(white);
            if (cat != &unknown_judoka)
                free_judoka(cat);
        }
        //g_static_mutex_unlock(&next_match_mutex);
        sendf(s, "</table>\r\n");
    } else { //!h_tatami
        sendf(s, _("Select a tatami"));
    }

    sendf(s, "</td><td valign=\"top\">");

    /* category list */
    if (h_tatami) {
        sprintf(buf, "select * from categories where \"tatami\"=%s and "
                "\"deleted\"=0 order by \"category\" asc", 
                h_tatami);
        tablecopy = db_get_table_copy(buf, &numrows, &numcols);

        if (tablecopy == NULL)
            return;

        sendf(s, "<table>\r\n");

        for (row = 0; row < numrows; row++) {
            gchar *cat = db_get_col_data(tablecopy, numcols, row, "category");
            gchar *id1 = db_get_col_data(tablecopy, numcols, row, "index");
            sendf(s, "<tr><td><input type=\"submit\" name=\"X%s\" "
                  "value=\"%s\" %s></td></tr>\r\n",
                  id1, cat, (h_id && strcmp(h_id, id1)==0) ? style1 : style0);
        }
        sendf(s, "</table>\r\n");

        db_close_table_copy(tablecopy);
    }
    sendf(s, "</td><td valign=\"top\">");

    /* picture */
    sendf(s, "<img src=\"web?op=5&c=%s&p=0\">", h_id ? h_id : "0");
    sendf(s, "<img src=\"web?op=5&c=%s&p=1\">", h_id ? h_id : "0");
    sendf(s, "<img src=\"web?op=5&c=%s&p=2\">", h_id ? h_id : "0");
    sendf(s, "</td></tr></table></form>\r\n");

    send_html_bottom(&s, parser);

#endif
    FEND;
}

static struct bracket_cache {
    gchar *sheet;
    gint   len;
    gint   size;
    gint   cat;
    gint   svg;
} sheet_cache[NUM_TATAMIS];

static GMutex cache_lock;

const gchar *svg_start =
    "HTTP/1.0 200 OK\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Content-Type: image/svg+xml\r\n\r\n";
const gchar *png_start =
    "HTTP/1.0 200 OK\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Content-Type: image/png\r\n\r\n";

gboolean is_png(const guchar *png)
{
    return png[0] != '<';
    return (png[0] == 137 && png[1] == 80 &&
	    png[2] == 78 && png[3] == 71);
}

static gchar *get_bracket_file_name(gint catid, gint type)
{
    gchar buf[200];
    const gchar *dir = g_get_tmp_dir();

    switch (type) {
    case 0:
        snprintf(buf, sizeof(buf), "%d.png", catid);
        break;
    case 1:
        snprintf(buf, sizeof(buf), "%d.svg", catid);
        break;
    case 2:
        snprintf(buf, sizeof(buf), "%d.pdf", catid);
        break;
    default:
        return NULL;
    }
    gchar *filename = g_build_filename(dir, buf, NULL);
    return filename;
}

void set_bracket_status(gint catid, gint c)
{
    GStatBuf st;
    gint i, j;
    for (i = 0; i <= 2; i++) {
        gchar *filename = get_bracket_file_name(catid, 0);
        if (g_stat(filename, &st) == 0) {
            j = 0;
            while (j < 5 && g_unlink(filename)) {
                // cannot rm, wait
                j++;
                g_print("Cannot unlink %s, wait (%d)\n", filename, j);
                g_usleep(100000);
            }
        }
        g_free(filename);
    }
}

gint get_bracket_status(gint catid, gint type)
{
    GStatBuf st;
    gint r = 0;
    gchar *filename = get_bracket_file_name(catid, type);
    if (g_stat(filename, &st) == 0) {
        struct timespec mtim = st.st_mtim;
        time_t now = time(NULL);
        g_print("%s: time st=%d now=%d diff=%d\n", filename, (int)mtim.tv_sec, (int)now, (int)(now - mtim.tv_sec));
        if (mtim.tv_sec > now - 10)
            r = 1;
    }
    g_free(filename);
    return r;
}

void get_bracket_2(gint tatami, gint catid, gint svg, gint page, struct msg_web_resp *resp)
{
    tatami--;

    g_print("get_bracket t=%d cat=%d svg=%d\n", tatami, catid, svg);
    if (catid == 0 && tatami >= 0 && tatami < NUM_TATAMIS)
	catid = next_matches_info[tatami][0].catnum;

    if (catid == 0)
        return;

    if (page == -1 && tatami >= 0 && tatami < NUM_TATAMIS) {
        page = match_on_page(catid, next_matches_info[tatami][0].matchnum);
    }
    
    const gchar *dir = g_get_tmp_dir();
    gchar prefix[64];
    snprintf(prefix, sizeof(prefix), "%d", catid);

    if (get_bracket_status(catid, svg) == 0) {
        g_print("*** WRITE %s/%s\n", dir, prefix);
        if (svg == 2)
            pdf_file(catid, dir, prefix);
        else
            png_file(catid, dir, prefix);
    }

    gchar buf[200];
    gchar *pngname;
    if (page <= 0) {
        snprintf(buf, sizeof(buf), "%s.%s", prefix, svg == 2 ? "pdf" : "png");
        pngname = g_build_filename(dir, buf, NULL);
        g_strlcpy(resp->u.get_bracket_resp.filename, pngname, sizeof(resp->u.get_bracket_resp.filename));
        g_free(pngname);
    } else {
        snprintf(buf, sizeof(buf), "%s-%d.%s", prefix, page, svg == 2 ? "pdf" : "png");
        pngname = g_build_filename(dir, buf, NULL);
        g_strlcpy(resp->u.get_bracket_resp.filename, pngname, sizeof(resp->u.get_bracket_resp.filename));
        g_free(pngname);
    }

    resp->u.get_bracket_resp.svg = svg;

#if 0    
    g_mutex_lock(&cache_lock);

    if (tatami >= 0 && tatami < NUM_TATAMIS) {
	if (sheet_cache[tatami].cat != catid ||
	    sheet_cache[tatami].sheet == NULL ||
	    sheet_cache[tatami].svg != svg ||
	    sheet_cache[tatami].len < 100) {
	    sheet_cache[tatami].len = 0;
	    sheet_cache[tatami].cat = catid;
	    sheet_cache[tatami].svg = svg;
	    write_sheet_to_stream(catid, write_to_http, &closure);
	}

	//mysend(s, sheet_cache[tatami].sheet, sheet_cache[tatami].len);
    } else {
	write_sheet_to_stream(catid, write_to_http, &closure);
    }

    g_mutex_unlock(&cache_lock);
#endif
}

void clear_cache_by_cat(gint cat)
{
    gint i;

    g_mutex_lock(&cache_lock);

    for (i = 0; i < NUM_TATAMIS; i++)
	if (cat == 0 ||
	    sheet_cache[i].cat == cat) {
	    sheet_cache[i].cat = 0;
	    sheet_cache[i].len = 0;
	}

    g_mutex_unlock(&cache_lock);
}

static gint pts2int(const gchar *p)
{
    gint r = 0;
    gint x = p[0] - 'A';

    if (x >= 2)
        r = 0x10000;
    else if (x)
        r = 0x01000;

    r |= ((p[1]-'A') << 8) | ((p[2]-'A') << 4) | (p[3]-'A');

    return r;
}

int check_password(http_parser_t *parser)
{
    FSTART;
#if 0
    const gchar *auth = httpp_getvar(parser, "authorization");
    gboolean accepted = FALSE;
    static time_t last_time = 0;
    time_t now;
    gchar *p = NULL;
    gchar n, bufin[64];
    gint cnt = 0, ch[4], eqcnt = 0, inpos = 0;

    now = time(NULL);
    if (now > last_time + 20)
        goto verdict;

    if (webpwcrc32 == 0) {
        accepted = TRUE;
        goto verdict;
    }

    if (!auth)
        goto verdict;

    p = strstr(auth, "Basic ");
    if (!p)
        goto verdict;

    p += 6;

    while ((n = *p++) && inpos < sizeof(bufin) - 6) {
        if ((n >= 'a' && n <= 'z') || (n >= 'A' && n <= 'Z') || 
            (n >= '0' && n <= '9') || n == '+' || n == '/' || n == '=') {
            gint x;
            if (n >= 'a' && n <= 'z')
                x = n - 'a' + 26;
            else if (n >= 'A' && n <= 'Z')
                x = n - 'A';
            else if (n >= '0' && n <= '9')
                x = n - '0' + 52;
            else if (n == '+')
                x = 62;
            else if (n == '/')
                x = 63;
            else { /* n == '=' */
                x = 0;
                eqcnt++;
            }
            ch[cnt++] = x;
            if (cnt == 4) {
                bufin[inpos++] = (ch[0] << 2) | (ch[1] >> 4);
                if (eqcnt <= 1) {
                    bufin[inpos++] = (ch[1] << 4) | (ch[2] >> 2);
                }
                if (eqcnt == 0) {
                    bufin[inpos++] = (ch[2] << 6) | ch[3];
                }
                cnt = 0;
                eqcnt = 0;
            }
        }
    }
    bufin[inpos] = 0;

    p = strchr(bufin, ':');
    if (!p)
        goto verdict;

    *p = 0;
    p++;
    if (pwcrc32((guchar *)p, strlen(p)) == webpwcrc32)
        accepted = TRUE;

verdict:
    last_time = now;

    if (accepted) {
        add_accepted(parser->address);
        index_html(parser, _("Password accepted.<br>"));
#if 0
        sendf(s, "HTTP/1.0 200 OK\r\n");
        sendf(s, "Content-Type: text/html\r\n\r\n");
        sendf(s, "<html><head><title>JudoShiai</title></head><body>\r\n");
        sendf(s, "<p>%s: %s. %s OK. <a href=\"index.html\">%s.</a></p></body></html>\r\n\r\n", 
              _("User"), bufin, _("Password"), _("Back"));
#endif
    } else {
        remove_accepted(parser->address);
        //index_html(parser, "<b>Salasana virheellinen!</b><br>");
#if 1
        sendf(s, "HTTP/1.0 401 Unauthorized\r\n");
        sendf(s, "WWW-Authenticate: Basic realm=\"JudoShiai\"\r\n");
        sendf(s, "Content-Type: text/html\r\n\r\n");
        sendf(s, "<html><head><title>JudoShiai</title></head><body>\r\n");
        sendf(s, "<p>%s. <a href=\"index.html\">%s.</a></p></body></html>\r\n\r\n",
              _("Wrong password"), _("Back"));
#endif
    }
#endif
    FEND;
}

void send_html_top(string *s, http_parser_t *parser, gchar *bodyattr)
{
    string_concat(s, "<html><head>"
	  "<meta name=\"viewport\" content=\"width=device-width, "
	  "target-densitydpi=device-dpi\">\r\n"
	  "<link rel=\"stylesheet\" type=\"text/css\" href=\"bootstrap.css\">"
	  "<link rel=\"stylesheet\" type=\"text/css\" href=\"jquery.treetable.theme.default.css\" />\r\n"
	  "<link rel=\"stylesheet\" type=\"text/css\" href=\"jquery.treetable.css\" />\r\n"
          "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
 	  "<script language=\"JavaScript\" type=\"text/javascript\" src=\"jquery.js\"></script>"
	  "<script language=\"JavaScript\" type=\"text/javascript\" src=\"bootstrap.min.js\"></script>"
	  "<script language=\"JavaScript\" type=\"text/javascript\" src=\"jquery.treetable.js\"></script>\r\n"
          "<title>JudoShiai</title></head><body %s>\r\n", bodyattr);

    if (webpwcrc32) {
	if (is_accepted(parser))
	    string_concat(s, "<a href=\"logout\">%s</a>\r\n", _("Logout"));
	else
	    string_concat(s, "<a href=\"login\">%s</a>\r\n", _("Login"));
    }

    string_concat(s, "<a href=\"timer.html\"><img src=\"judotimer.png\"></a>\r\n");
    string_concat(s, "<a href=\"info.html\"><img src=\"judoinfo.png\"></a>\r\n");
    string_concat(s, "<a href=\"weight.html\"><img src=\"judoweight.png\"></a>\r\n");
    string_concat(s, "<p><a href=\"https://sourceforge.net/projects/judoshiai/\">"
	  "<img src=\"auto-update.png\"><b>Download JudoShiai from Sourceforge.</b></a></p>");
    string_concat(s, "<p><a href=\"judoshiai-remote-setup-" SHIAI_VERSION ".exe\">"
	  "<img src=\"auto-update.png\"><b>Install JudoShiai from communication node.</b></a>");
    string_concat(s, "<br>This is a partially installation intended to be used in JudoTimer and JudoInfo computers."
	  "<br><tt>.shi</tt> files are not associated with JudoShiai program.");
}

void send_html_bottom(string *s, http_parser_t *parser)
{
    string_concat(s, "</body></html>\r\n\r\n");
}

int logout(http_parser_t *parser)
{
    FSTART;
#if 0
    remove_accepted(parser->address);
    index_html(parser, "");
    return;
    string_concat(s, "HTTP/1.0 200 OK\r\n");
    string_concat(s, "Content-Type: text/html\r\n\r\n");
    string_concat(s, "<html><head><title>JudoShiai</title></head><body>\r\n");
    string_concat(s, "<p>OK. <a href=\"index.html\">%s.</a></p></body></html>\r\n\r\n", _("Back"));
#endif
    FEND;
}

int index_html(http_parser_t *parser, gchar *txt)
{
    FSTART;
    send_html_top(&s, parser, "");
    string_concat(&s, "%s", txt);
    send_html_bottom(&s, parser);
    FEND;
}

int run_judotimer(http_parser_t *parser)
{
    FSTART;

    gint tatami = 0, cat = 0, match = 0, bvote = 0, wvote = 0, bhkm = 0, whkm = 0, tmo = 0;
    const gchar *bpts = NULL, *wpts = NULL, *tmp;
    gint b = 0, w = 0;

    tmp = httpp_get_query_param(parser, "tatami");
    if (tmp) tatami = atoi(tmp);

    tmp = httpp_get_query_param(parser, "cat");
    if (tmp) cat = atoi(tmp);

    tmp = httpp_get_query_param(parser, "match");
    if (tmp) match = atoi(tmp);

    tmp = httpp_get_query_param(parser, "bvote");
    if (tmp) bvote = atoi(tmp);

    tmp = httpp_get_query_param(parser, "wvote");
    if (tmp) wvote = atoi(tmp);

    tmp = httpp_get_query_param(parser, "bhkm");
    if (tmp) bhkm = atoi(tmp);

    tmp = httpp_get_query_param(parser, "whkm");
    if (tmp) whkm = atoi(tmp);

    tmp = httpp_get_query_param(parser, "time");
    if (tmp) tmo = atoi(tmp);

    bpts = httpp_get_query_param(parser, "bpts");
    if (bpts) b = pts2int(bpts);
    wpts = httpp_get_query_param(parser, "wpts");
    if (wpts) w = pts2int(wpts);

    if ((b != w || bvote != wvote || bhkm != whkm) && tatami && is_accepted(parser)) {
        struct message msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_RESULT;
        msg.u.result.tatami = tatami;
        msg.u.result.category = cat;
        msg.u.result.match = match;
        msg.u.result.minutes = tmo;
        msg.u.result.blue_score = b;
        msg.u.result.white_score = w;
        msg.u.result.blue_vote = bvote;
        msg.u.result.white_vote = wvote;
        msg.u.result.blue_hansokumake = bhkm;
        msg.u.result.white_hansokumake = whkm;

        struct message *msg2 = put_to_rec_queue(&msg);
        time_t start = time(NULL);
        while ((time(NULL) < start + 5) && (msg2->type != 0))
            g_usleep(10000);
    }

    if (tatami == 0)
        FEND_MIME("text/plain");

    //g_static_mutex_lock(&next_match_mutex);
    struct match *m = get_cached_next_matches(tatami);
    int k;

    if (!m) {
        //g_static_mutex_unlock(&next_match_mutex);
        FEND_MIME("text/plain");
    }

    string_concat(&s, "%d %d\r\n", m[0].category, m[0].number);

    for (k = 0; m[k].number != 1000 && k < 2; k++) {
        struct judoka *blue, *white, *jcat;
        blue = get_data(m[k].blue);
        white = get_data(m[k].white);
        jcat = get_data(m[k].category);

        if (!blue)
            blue = &unknown_judoka;

        if (!white)
            white = &unknown_judoka;

        if (!jcat)
            jcat = &unknown_judoka;

        if (k == 0 && 
            strcmp(next_matches_info[tatami-1][0].blue_first, blue->first))
            g_print("NEXT MATCH ERR: %s %s\n", 
                    next_matches_info[tatami-1][0].blue_first, blue->first);

        string_concat(&s, "%s\r\n", jcat->last);
        if (k == 0 && !is_accepted(parser)) {
            string_concat(&s, "%s!\r\n", _("No rights to give results"));
            string_concat(&s, "%s.\r\n", _("Login"));
        } else {
            string_concat(&s, "%s %s %s\r\n", blue->first, blue->last, blue->club);
            string_concat(&s, "%s %s %s\r\n", white->first, white->last, white->club);
        }

        if (blue != &unknown_judoka)
            free_judoka(blue);
        if (white != &unknown_judoka)
            free_judoka(white);
        if (jcat != &unknown_judoka)
            free_judoka(jcat);
    }
    //g_static_mutex_unlock(&next_match_mutex);
    string_concat(&s, "\r\n");

    FEND_MIME("text/plain");
}

int get_match(http_parser_t *parser)
{
    gint tatami0 = 0, position = 0;
    struct MHD_Response *response;
    string s;
    
    gchar *tmp = httpp_get_query_param(parser, "t");
    if (tmp) tatami0 = atoi(tmp);
    tmp = httpp_get_query_param(parser, "p");
    if (tmp) position = atoi(tmp);
    
    get_html_match(&s, tatami0, position);
    FEND;
}

int get_web(http_parser_t *parser, gint connum)
{
    gint op;
    time_t start;
    struct message msg;
    struct msg_web_resp reply;
    struct msg_web_resp *resp = &reply;
    const gchar *tmp;
    const gchar *id = NULL;
    const gchar *wg = NULL;
    const gchar *t = NULL;
    const gchar *s1 = NULL;
    const gchar *c = NULL;
    const gchar *p = NULL;
    FSTART;
    
    //resp = &connections[connum].resp;
    resp->request = MSG_WEB_GET_ERR;
    resp->ready = 0;

    tmp = httpp_get_query_param(parser, "op");
    if (tmp) op = atoi(tmp);
    else goto err;

    id = httpp_get_query_param(parser, "id");
    wg = httpp_get_query_param(parser, "wg");

    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_WEB;
    msg.u.web.request = op;
    msg.u.web.resp = resp;

    resp->request = op;

    switch (op) {
    case MSG_WEB_SET_COMP_WEIGHT:
	if (wg)
	    msg.u.web.u.set_comp_weight.weight = atoi(wg);
	else goto err;

	if (id)
	    strncpy(msg.u.web.u.set_comp_weight.id, id,
		    sizeof(msg.u.web.u.get_comp_data.id)-1);
	else goto err;
	break;

    case MSG_WEB_GET_COMP_DATA:
	if (id)
	    strncpy(msg.u.web.u.get_comp_data.id, id,
		    sizeof(msg.u.web.u.get_comp_data.id)-1);
	else goto err;
	break;

    case MSG_WEB_GET_MATCH_CRC:
	break;

    case MSG_WEB_GET_MATCH_INFO:
	t = httpp_get_query_param(parser, "t");
	if (t)
	    msg.u.web.u.get_match_info.tatami = atoi(t);
	else goto err;
	break;

    case MSG_WEB_GET_BRACKET:
	t = httpp_get_query_param(parser, "t");
        msg.u.web.u.get_bracket.tatami = t ? atoi(t) : 0;
        s1 = httpp_get_query_param(parser, "s");
        msg.u.web.u.get_bracket.svg = s1 ? atoi(s1) : 0;
        c = httpp_get_query_param(parser, "c");
        msg.u.web.u.get_bracket.cat = c ? atoi(c) : 0;
        p = httpp_get_query_param(parser, "p");
        msg.u.web.u.get_bracket.page = p ? atoi(p) : -1;
	break;

    case MSG_WEB_GET_CAT_INFO:
	c = httpp_get_query_param(parser, "c");
	if (c)
	    msg.u.web.u.get_category_info.catix = atoi(c);
	else goto err;
	break;

    default:
        g_print("UNKNOWN WEB OP %d\n", op);
        goto err;
    }

    put_to_rec_queue(&msg);

    start = time(NULL);
    
    while (!g_atomic_int_get(&resp->ready) &&
           time(NULL) < start + 5) {
        g_usleep(50000);
    }
    if (time(NULL) >= start + 5 ||
        g_atomic_int_get(&resp->ready) != MSG_WEB_RESP_OK) {
        g_printerr("NO WEB REPLY\n");
        return reply_web(parser, NULL);
    }
    
    return reply_web(parser, resp);
err:
    return reply_web(parser, NULL);
}

int reply_web(http_parser_t *parser, struct msg_web_resp *resp)
{
    cJSON *root, *array, *member;
    FSTART;

    g_print("reply_web\n");
    if (!resp) {
        FEND_404;
    }
    
    gint op = resp->request, i;
    gint ready = g_atomic_int_get(&resp->ready);
    g_print("reply_web op=%d ready=%d\n", op, ready);

    resp->request = 0;

    if (op == MSG_WEB_GET_BRACKET) {
        GStatBuf st;
        struct MHD_Response *response;
        g_print("looking for %s\n", resp->u.get_bracket_resp.filename);
        HANDLE fd = int_to_handle(g_open(resp->u.get_bracket_resp.filename, O_RDONLY, 0));
        if (fd != INVALID_HANDLE_VALUE) {
            if (g_stat(resp->u.get_bracket_resp.filename, &st) || (!S_ISREG(st.st_mode))) {
#ifdef WIN32
                CloseHandle(fd);
#else
                g_close(fd, NULL);
#endif
                fd = INVALID_HANDLE_VALUE;
            }
        }

        if (fd == INVALID_HANDLE_VALUE) {
            g_print("WEB RESP: FILE %s NOT FOUND!\n", resp->u.get_bracket_resp.filename);
            response = MHD_create_response_from_buffer(strlen (PAGE),
                                                       (void *) PAGE,
                                                       MHD_RESPMEM_PERSISTENT);
            int ret = MHD_queue_response(parser, MHD_HTTP_NOT_FOUND, response);
            MHD_destroy_response(response);
            return ret;
        }

        response = MHD_create_response_from_fd64(st.st_size, handle_to_int(fd));
        if (!response) {
            g_printerr("response failed\n");
#ifdef WIN32
            CloseHandle(fd);
#else
            g_close(fd, NULL);
#endif
            return MHD_NO;
        }

        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE,
                                resp->u.get_bracket_resp.svg == 2 ? "application/pdf" : "image/png");
        int ret = MHD_queue_response(parser, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    switch (op) {
    case MSG_WEB_GET_COMP_DATA:
    case MSG_WEB_SET_COMP_WEIGHT:
#define CSTR(_s) _s ? _s : ""
        root = cJSON_CreateObject();	
	cJSON_AddNumberToObject(root, "req", resp->request);
	cJSON_AddNumberToObject(root, "ix", resp->u.get_comp_data_resp.index);
	cJSON_AddStringToObject(root, "last", CSTR(resp->u.get_comp_data_resp.last));
	cJSON_AddStringToObject(root, "first", CSTR(resp->u.get_comp_data_resp.first));
	cJSON_AddStringToObject(root, "club", CSTR(resp->u.get_comp_data_resp.club));
	cJSON_AddStringToObject(root, "rcat", CSTR(resp->u.get_comp_data_resp.regcategory));
	cJSON_AddStringToObject(root, "cat", CSTR(resp->u.get_comp_data_resp.category));
	cJSON_AddNumberToObject(root, "weight", resp->u.get_comp_data_resp.weight);
        cJSON_AddStringToObject(root, "ecat", CSTR(resp->u.get_comp_data_resp.estim_category));
        s.buf = cJSON_PrintUnformatted(root);
        s.len = strlen(s.buf);
        cJSON_Delete(root);
	break;

    case MSG_WEB_GET_MATCH_CRC:
	string_concat(&s, "\"crc\":[");
	for (i = 0; i < NUM_TATAMIS; i++)
	    string_concat(&s, "%c%d", i ? ',' : ' ', resp->u.get_match_crc_resp.crc[i]);
	string_concat(&s, "]\r\n");
	break;

    case MSG_WEB_GET_MATCH_INFO:
	root = cJSON_CreateObject();	
	array = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "match_info", array);

	for (i = 0; i <= INFO_MATCH_NUM; i++) {
            member = cJSON_CreateObject();
            cJSON_AddItemToArray(array, member);
	    cJSON_AddNumberToObject(member, "t", resp->u.get_match_info_resp[i].tatami);
	    cJSON_AddNumberToObject(member, "row", resp->u.get_match_info_resp[i].num);
	    cJSON_AddNumberToObject(member, "cat", resp->u.get_match_info_resp[i].match_category_ix);
	    cJSON_AddNumberToObject(member, "num", resp->u.get_match_info_resp[i].match_number);
	    cJSON_AddNumberToObject(member, "comp1", resp->u.get_match_info_resp[i].comp1);
	    cJSON_AddNumberToObject(member, "comp2", resp->u.get_match_info_resp[i].comp2);
	    cJSON_AddNumberToObject(member, "round", resp->u.get_match_info_resp[i].round);
        }
        s.buf = cJSON_PrintUnformatted(root);
        s.len = strlen(s.buf);
        cJSON_Delete(root);
	break;

    case MSG_WEB_GET_CAT_INFO:
	root = cJSON_CreateObject();	
	cJSON_AddNumberToObject(root, "cat", resp->u.get_category_info_resp.catix);
	cJSON_AddNumberToObject(root, "system", resp->u.get_category_info_resp.system);
	cJSON_AddNumberToObject(root, "numcomp", resp->u.get_category_info_resp.numcomp);
	cJSON_AddNumberToObject(root, "table", resp->u.get_category_info_resp.table);
	cJSON_AddNumberToObject(root, "wishsys", resp->u.get_category_info_resp.wishsys);
	cJSON_AddNumberToObject(root, "numpages", resp->u.get_category_info_resp.num_pages);
        s.buf = cJSON_PrintUnformatted(root);
        s.len = strlen(s.buf);
        cJSON_Delete(root);
	break;
    }

    FEND;
}

static gint my_atoi(const gchar *s) { return (s ? atoi(s) : 0); }

#define GET_INT(_x) gint _x = new_comp ? 0 : my_atoi(db_get_col_data(tablecopy, numcols, row, #_x))
#define GET_STR(_x) gchar *_x = new_comp ? "" : db_get_col_data(tablecopy, numcols, row, #_x)

#define SEARCH_FIELD "<form method=\"get\" action=\"getcompetitor\" name=\"search\">Search (bar code):<input type=\"text\" name=\"index\"></form>\r\n"

int get_competitor(http_parser_t *parser)
{
    FSTART;
    gchar buf[1024];
    gint row;
    gint i;
    gboolean new_comp = FALSE;
    const gchar *ix = NULL;

    ix = httpp_get_query_param(parser, "index");
    if (!strcmp(ix, "0"))
        new_comp = TRUE;

    send_html_top(&s, parser, "onLoad=\"document.valtable.weight.focus()\"");

    snprintf(buf, sizeof(buf), "select * from competitors where \"id\"=\"%s\"", ix);
    gint numrows, numcols;
    gchar **tablecopy = db_get_table_copy(buf, &numrows, &numcols);

    if (tablecopy && numrows == 0) {
        db_close_table_copy(tablecopy);
        snprintf(buf, sizeof(buf), "select * from competitors where \"index\"=%s", ix);
        tablecopy = db_get_table_copy(buf, &numrows, &numcols);
    }

    if (!tablecopy) {
        string_concat(&s, "<h1>%s</h1>", _("No competitor found!"));
        send_html_bottom(&s, parser);
        FEND;
    } else if (numrows == 0)
        new_comp = TRUE;

    row = 0;

    GET_INT(index);
    GET_STR(last);
    GET_STR(first);
    GET_INT(birthyear);
    GET_STR(club);
    GET_STR(regcategory);
    GET_INT(belt);
    GET_INT(weight);
    //GET_INT(visible); xxx not used
    GET_STR(category);
    GET_INT(deleted);
    GET_STR(country);
    GET_STR(id);
    GET_INT(seeding);
    GET_INT(clubseeding);

#define HTML_ROW_STR(_h, _x)                                            \
    string_concat(&s, "<tr><td>%s:</td><td><input type=\"text\" name=\"%s\" value=\"%s\"/></td></tr>\r\n", \
          _h, #_x, _x)
#define HTML_ROW_INT(_h, _x)                                            \
    string_concat(&s, "<tr><td>%s:</td><td><input type=\"text\" name=\"%s\" value=\"%d\"/></td></tr>\r\n", \
          _h, #_x, _x)

    string_concat(&s, "<form method=\"get\" action=\"setcompetitor\" name=\"valtable\">"
          "<input type=\"hidden\" name=\"index\" value=\"%d\">"
          "<table class=\"competitor\">\r\n", 
          index, category);

    //          "<table class=\"competitor\" border=\"1\" frame=\"hsides\" rules=\"rows\">\r\n", 

    HTML_ROW_STR("Last Name", last);
    HTML_ROW_STR("First Name", first);
    HTML_ROW_INT("Year of Birth", birthyear);

    string_concat(&s, "<tr><td>%s:</td><td><select name=\"belt\" value=\"\">", "Grade");
    for (i = 0; belts[i]; i++)
        string_concat(&s, "<option %s>%s</option>", belt == i ? "selected" : "", belts[i]);
    string_concat(&s, "</select></td></tr>\r\n");

    HTML_ROW_STR("Club", club);
    HTML_ROW_STR("Country", country);
    HTML_ROW_STR("Reg.Category", regcategory);

    // category list box
//    db_close_table();  //mutex problem.
    numrows = db_get_table("select category from categories order by category");
    string_concat(&s, "<tr><td>%s:</td><td><select name=\"category\" value=\"\">", "Category");
    for (row = 0; row < numrows; row++) {
    	gchar *cat = db_get_data(row, "category");
         string_concat(&s, "<option %s>%s</option>", !strcmp(cat,category) ? "selected" : "", cat);
    }
    db_close_table();
    string_concat(&s, "</select></td></tr>\r\n");

    string_concat(&s, "<tr><td>%s:</td><td><input type=\"text\" name=\"weight\" value=\"%d.%02d\"/></td></tr>\r\n", \
          "Weight", weight/1000, (weight%1000)/10);

    string_concat(&s, "<tr><td>%s:</td><td><select name=\"seeding\" value=\"\">", "Seeding");
    for (i = 0; i <= 4; i++)
        string_concat(&s, "<option %s>%d</option>", seeding == i ? "selected" : "", i);
    string_concat(&s, "</select></td></tr>\r\n");

    string_concat(&s, "<tr><td>%s:</td><td><input type=\"text\" name=\"clubseeding\" value=\"%d\"/></td></tr>\r\n", \
          "Club Seeding", clubseeding);

    HTML_ROW_STR("ID", id);

    string_concat(&s, "<tr><td>%s:</td><td><select name=\"gender\" value=\"\">", "Gender");
    string_concat(&s, "<option %s>%s</option>", (deleted & (GENDER_MALE | GENDER_FEMALE)) == 0 ? "selected" : "", "?");
    string_concat(&s, "<option %s>%s</option>", deleted & GENDER_MALE ? "selected" : "", "Male");
    string_concat(&s, "<option %s>%s</option>", deleted & GENDER_FEMALE ? "selected" : "", "Female");
    string_concat(&s, "</select></td></tr>\r\n");

    string_concat(&s, "<tr><td>%s:</td><td><select name=\"judogi\" value=\"\">", "Control");
    string_concat(&s, "<option %s>%s</option>", (deleted & (JUDOGI_OK | JUDOGI_NOK)) == 0 ? "selected" : "", "Not checked");
    string_concat(&s, "<option %s>%s</option>", deleted & JUDOGI_OK ? "selected" : "", "OK");
    string_concat(&s, "<option %s>%s</option>", deleted & JUDOGI_NOK ? "selected" : "", "NOK");
    string_concat(&s, "</select></td></tr>\r\n");

    string_concat(&s, "<tr><td>%s:</td><td><input type=\"checkbox\" name=\"hansokumake\" "
          "value=\"yes\" %s/></td></tr>\r\n", "Hansoku-make", deleted&2 ? "checked" : "");

    if (category[0] == '?')
        string_concat(&s, "<tr><td>%s:</td><td><input type=\"checkbox\" name=\"delete\" "
              "value=\"yes\" %s/></td></tr>\r\n", "Delete", deleted&1 ? "checked" : "");

    string_concat(&s, "</table>\r\n");

    if (is_accepted(parser))
        string_concat(&s, "<input type=\"submit\" value=\"%s\">", new_comp ? _("Add") : _("OK"));

    string_concat(&s, "</form><br><a href=\"competitors\">Competitors</a>"
          SEARCH_FIELD);

    send_html_bottom(&s, parser);

    db_close_table_copy(tablecopy);

    FEND;
}

#define GET_HTML_STR(_x) _x = httpp_get_query_param(parser, #_x)
#define GET_HTML_INT(_x) _x = my_atoi(httpp_get_query_param(parser, #_x))
#define DEF_STR(_x) const gchar *_x = ""
#define DEF_INT(_x) gint _x = 0

int set_competitor(http_parser_t *parser)
{
    FSTART;

    DEF_INT(index);
    DEF_STR(last);
    DEF_STR(first);
    DEF_INT(birthyear);
    DEF_STR(club);
    DEF_STR(regcategory);
    DEF_STR(belt);
    DEF_STR(weight);
    //DEF_INT(visible);
    DEF_STR(category);
    DEF_INT(deleted);
    DEF_INT(seeding);
    DEF_INT(clubseeding);
    DEF_STR(country);
    DEF_STR(hansokumake);
    DEF_STR(id);
    DEF_STR(delete);
    DEF_STR(judogi);
    DEF_STR(gender);

    GET_HTML_INT(index);
    GET_HTML_STR(last);
    GET_HTML_STR(first);
    GET_HTML_INT(birthyear);
    GET_HTML_STR(club);
    GET_HTML_STR(regcategory);
    GET_HTML_STR(belt);
    GET_HTML_STR(weight);
    //GET_HTML_INT(visible);
    GET_HTML_STR(category);
    //GET_HTML_INT(deleted);
    GET_HTML_INT(seeding);
    GET_HTML_INT(clubseeding);
    GET_HTML_STR(country);
    GET_HTML_STR(hansokumake);
    GET_HTML_STR(id);
    GET_HTML_STR(delete);
    GET_HTML_STR(judogi);
    GET_HTML_STR(gender);

    deleted = 0;

    if (!strcmp(delete, "yes"))
        deleted |= DELETED;

    if (!strcmp(hansokumake, "yes"))
        deleted |= HANSOKUMAKE;

    if (!strcmp(gender, "Male"))
        deleted |= GENDER_MALE;
    else if (!strcmp(gender, "Female"))
        deleted |= GENDER_FEMALE;

    if (!strcmp(judogi, "OK"))
        deleted |= JUDOGI_OK;
    else if (!strcmp(judogi, "NOK"))
        deleted |= JUDOGI_NOK;

    gint beltval;
    const gchar *b = belt;
    gboolean kyu = strchr(b, 'k') != NULL || strchr(b, 'K') != NULL;
    gboolean mon = strchr(b, 'm') != NULL || strchr(b, 'M') != NULL;
    while (*b && (*b < '0' || *b > '9'))
        b++;

    gint grade = atoi(b);
    if (grade && mon)
        beltval = 21 - grade;
    else if (grade && kyu)
        beltval = 7 - grade;
    else
    	beltval = 6 + grade;

    if (beltval < 0 || beltval > 20) beltval = 0;

#undef STRCPY
#define STRCPY(_x)                                                      \
    strncpy(msg.u.edit_competitor._x, _x, sizeof(msg.u.edit_competitor._x)-1)

#undef STRCPY_UTF8
#define STRCPY_UTF8(_x)                                                 \
    do { gsize _sz;                                                     \
        gchar *_s = g_convert(_x, -1, "UTF-8", "ISO-8859-1", NULL, &_sz, NULL); \
        strncpy(msg.u.edit_competitor._x, _s, sizeof(msg.u.edit_competitor._x)-1); \
        g_free(_s);                                                     \
    } while (0)

    struct message msg;
    memset(&msg, 0, sizeof(msg));

    msg.type = MSG_EDIT_COMPETITOR;
    msg.u.edit_competitor.operation = EDIT_OP_SET;
    msg.u.edit_competitor.index = index;
    STRCPY(last);
    STRCPY(first);
    msg.u.edit_competitor.birthyear = birthyear;
    STRCPY(club);
    STRCPY(regcategory);
    msg.u.edit_competitor.belt = beltval;
    msg.u.edit_competitor.weight = weight_grams(weight);
    msg.u.edit_competitor.visible = TRUE;
    STRCPY(category);
    msg.u.edit_competitor.deleted = deleted;
    STRCPY(country);
    STRCPY(id);
    msg.u.edit_competitor.seeding = seeding;
    msg.u.edit_competitor.clubseeding = clubseeding;

    struct message *msg2 = put_to_rec_queue(&msg);
    time_t start = time(NULL);
    while ((time(NULL) < start + 5) && (msg2->type != 0))
        g_usleep(10000);

    send_html_top(&s, parser, "");

    string_concat(&s, "<h1>OK</h1>"
          "%s %s saved.<br>"
          "<a href=\"competitors\">Show competitors</a>\r\n"
          SEARCH_FIELD, first, last);

    send_html_bottom(&s, parser);

    FEND;
}

int get_competitors(http_parser_t *parser, gboolean show_deleted)
{
    FSTART;
    gint row;
    const gchar *order = "country";
    
    order = httpp_get_query_param(parser, "order");

    gint numrows, numcols;
    gchar **tablecopy;

    if (!strcmp(order, "club"))
        tablecopy = db_get_table_copy("select * from competitors order by \"category\",\"club\",\"last\",\"first\" asc", 
                                      &numrows, &numcols);
    else if (!strcmp(order, "last"))
        tablecopy = db_get_table_copy("select * from competitors order by \"category\",\"last\",\"first\" asc", 
                                      &numrows, &numcols);
    else if (!strcmp(order, "regcat"))
		tablecopy = db_get_table_copy("select * from competitors order by  \"category\",\"deleted\",\"regcategory\",\"last\",\"first\" asc",
                &numrows, &numcols);
//       tablecopy = db_get_table_copy("select * from competitors order by \"regcategory\",\"last\",\"first\" asc",
//                                      &numrows, &numcols);
    else if (!strcmp(order, "category"))
    	tablecopy = db_get_table_copy("select * from competitors order by \"category\",\"last\",\"first\" asc",
                &numrows, &numcols);
    else if (!strcmp(order, "weight"))
    	tablecopy = db_get_table_copy("select * from competitors order by \"category\",\"weight\",\"last\",\"first\" asc",
                &numrows, &numcols);
    else if (!strcmp(order, "ID"))
    	tablecopy = db_get_table_copy("select * from competitors order by \"category\",\"id\",\"last\",\"first\" asc",
                &numrows, &numcols);
    else if (!strcmp(order, "Index"))
    	tablecopy = db_get_table_copy("select * from competitors order by \"category\",\"index\",\"last\",\"first\" asc",
                &numrows, &numcols);
    else
        tablecopy = db_get_table_copy("select * from competitors order by \"category\",\"country\",\"club\",\"last\",\"first\" asc", 
                                      &numrows, &numcols);

    if (!tablecopy)
        FEND;

    send_html_top(&s, parser, "onLoad=\"document.search.index.focus()\"");

    string_concat(&s, SEARCH_FIELD);

    string_concat(&s, "<table id=\"competitors\">\r\n");
    string_concat(&s, "<tr>"
          "<th><a href=\"#\" onclick=\"sort(0);\"><b>%s</b></a></th>"
          "<th><b>%s</b></th>"
          "<th><a href=\"#\" onclick=\"sort(2);\"><b>%s</b></a></th>"
	  "<th><a href=\"#\" onclick=\"sort(3);\"><b>%s</b></a></th>"
          "<th><a href=\"#\" onclick=\"sort(4);\"><b>%s</b></a></th>"
	  "<th><a href=\"#\" onclick=\"sort(5);\"><b>%s</b></a></th>"
	  "<th><a href=\"#\" onclick=\"sort(6);\"><b>%s/%s</b></a></th>"
          "<th></th></tr>\r\n",
          _("Surname"), _("Name"), _("Club"), _("Country"), _("Reg.Cat"), _("Weight"), _("ID"), _("Index"));

    gchar lastcat[16];
    lastcat[0] = 0;
    gint catid = 7000;

    for (row = 0; row < numrows; row++) {

        gchar *rcat = db_get_col_data(tablecopy, numcols, row, "regcategory");
        gint weight = my_atoi(db_get_col_data(tablecopy, numcols, row, "weight"));
        gchar *idx = db_get_col_data(tablecopy, numcols, row, "index");

        gchar *last = db_get_col_data(tablecopy, numcols, row, "last");
        gchar *first = db_get_col_data(tablecopy, numcols, row, "first");
        gchar *club = db_get_col_data(tablecopy, numcols, row, "club");
        gchar *country = db_get_col_data(tablecopy, numcols, row, "country");
        gchar *cat = db_get_col_data(tablecopy, numcols, row, "category");
        gchar *id = db_get_col_data(tablecopy, numcols, row, "id");
        gint   deleted = my_atoi(db_get_col_data(tablecopy, numcols, row, "deleted"));

	if (strcmp(lastcat, cat)) {
	    catid++;
            string_concat(&s, "<tr data-tt-id=\"%d\"><td>%s</td><td></td><td></td><td></td>"
		  "<td></td><td></td><td></td></tr>\r\n", catid, cat);
	    strcpy(lastcat, cat);
	}

        if (((deleted&1) && show_deleted) ||
            ((deleted&1) == 0 && show_deleted == FALSE)) {
            string_concat(&s, "<tr %s data-tt-id=\"%s\" data-tt-parent-id=\"%d\">"
		  "<td><a href=\"getcompetitor?index=%s\">%s</a></td><td>%s</td><td>%s</td><td>%s</td>"
                  "<td>%s</td><td>%d.%02d</td>"
                  "<td><a href=\"getcompetitor?index=%s\">%s/%s</a></td></tr>\r\n",
                  (deleted & JUDOGI_OK) ? "class=\"judogiok\"" : ((deleted & JUDOGI_NOK) ? "class=\"judoginok\"" : ""),
		  idx, catid, idx,
                  last, first, club, country, rcat, weight/1000, (weight%1000)/10,
		  idx, id, idx);
        }
    }

    string_concat(&s, "</table>\r\n");
    string_concat(&s, "<script> $(\"#competitors\").treetable({ expandable: true }); </script>\r\n");
    string_concat(&s, "<script> function sort(col) {"
	  "$(\"#competitors tr:not(:first-child)\").each(function() {"
	  "var node = $(\"#competitors\").treetable(\"node\", $(this).attr('data-tt-id'));"
	  "$(\"#competitors\").treetable(\"sortBranch\", node, col); });"
	  "} </script>\r\n");

    send_html_bottom(&s, parser);

    db_close_table_copy(tablecopy);

    FEND;
}

int get_match_info(http_parser_t *parser, gchar *txt)
{
    FSTART;
    int t = 0, i;

    t = my_atoi(httpp_get_query_param(parser, "t"));

    if (t < 1 || t > NUM_TATAMIS) {
	/* abstract */
	for (t = 1; t <= NUM_TATAMIS; t++) {
	    string_concat(&s, "%d,%d\r\n", t, match_crc[t]);
	}
	string_concat(&s, "\r\n");
        FEND_MIME("text/plain");
    }

    struct match *m = get_cached_next_matches(t);
    if (!m) {
        FEND_404;
    }

    string_concat(&s, "%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
	  t, 0,
	  next_matches_info[t-1][0].won_catnum,
	  next_matches_info[t-1][0].won_matchnum,
	  next_matches_info[t-1][0].won_ix,
	  0, 0, 0, 0);

    for (i = 0; i < NEXT_MATCH_NUM; i++) {
	string_concat(&s, "%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
	      t, i+1, m[i].category, m[i].number,
	      m[i].blue, m[i].white,
	      0, 0, m[i].round);
    }

    string_concat(&s, "\r\n");

    FEND_MIME("text/plain");
}

int get_name_info(http_parser_t *parser, gchar *txt)
{
    FSTART;
    int comp = 0;

    comp = my_atoi(httpp_get_query_param(parser, "c"));

    struct judoka *j = get_data(comp);
    if (!j) {
        FEND_404;
    }

    string_concat(&s, "%d\t%s\t%s\t%s\r\n\r\n",
	  comp, j->last, j->first, get_club_text(j, CLUB_TEXT_ABBREVIATION));
    free_judoka(j);

    FEND_MIME("text/plain");
}

int set_result(http_parser_t *parser, gchar *txt)
{
    FSTART;
    struct message msg;
    const char *tmp;

    if (!is_accepted(parser)) {
	string_concat(&s, "%s!\r\n", _("No rights to give results"));
	string_concat(&s, "%s.\r\n", _("Login"));
        FEND;
    }
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_RESULT;

    tmp = httpp_get_query_param(parser, "t");
    if (tmp) msg.u.result.tatami = atoi(tmp);
    tmp = httpp_get_query_param(parser, "c");
    if (tmp) msg.u.result.category = atoi(tmp);
    tmp = httpp_get_query_param(parser, "m");
    if (tmp) msg.u.result.match = atoi(tmp);
    tmp = httpp_get_query_param(parser, "min");
    if (tmp) msg.u.result.minutes = atoi(tmp);
    tmp = httpp_get_query_param(parser, "s1");
    if (tmp) msg.u.result.blue_score = atoi(tmp);
    tmp = httpp_get_query_param(parser, "s2");
    if (tmp) msg.u.result.white_score = atoi(tmp);
    tmp = httpp_get_query_param(parser, "v1");
    if (tmp) msg.u.result.blue_vote = atoi(tmp);
    tmp = httpp_get_query_param(parser, "v2");
    if (tmp) msg.u.result.white_vote = atoi(tmp);
    tmp = httpp_get_query_param(parser, "h1");
    if (tmp) msg.u.result.blue_hansokumake = atoi(tmp);
    tmp = httpp_get_query_param(parser, "h2");
    if (tmp) msg.u.result.white_hansokumake = atoi(tmp);
    tmp = httpp_get_query_param(parser, "l");
    if (tmp) msg.u.result.legend = atoi(tmp);

    if (msg.u.result.tatami &&
	(msg.u.result.blue_score != msg.u.result.white_score ||
	 msg.u.result.blue_vote != msg.u.result.white_vote ||
	 msg.u.result.blue_hansokumake != msg.u.result.white_hansokumake)) {
	put_to_rec_queue(&msg);
        FEND_MIME("text/plain");
    }

    FEND_404;
}

int cancel_rest(http_parser_t *parser, gchar *txt)
{
    FSTART;
    struct message msg;
    const char *tmp;

    if (!is_accepted(parser)) {
	string_concat(&s, "%s!\r\n", _("No rights to give results"));
	string_concat(&s, "%s.\r\n", _("Login"));
        FEND;
    }

    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_CANCEL_REST_TIME;

    tmp = httpp_get_query_param(parser, "c");
    if (tmp) msg.u.cancel_rest_time.category = atoi(tmp);
    tmp = httpp_get_query_param(parser, "m");
    if (tmp) msg.u.cancel_rest_time.number = atoi(tmp);
    tmp = httpp_get_query_param(parser, "b");
    if (tmp) msg.u.cancel_rest_time.blue = atoi(tmp);
    tmp = httpp_get_query_param(parser, "w");
    if (tmp) msg.u.cancel_rest_time.white = atoi(tmp);

    string_concat(&s, "HTTP/1.0 200 OK\r\n");
    string_concat(&s, "Content-Type: text/plain\r\n\r\nOK\r\n\r\n");
    put_to_rec_queue(&msg);
    FEND_MIME("text/plain");
}

int get_judoshiai(http_parser_t *parser, gchar *txt)
{
    FSTART;
    int t = 0, x = 0, y = 0;
    static gint tab = 0;
    const char *tmp;
    struct message msg;

    tmp = httpp_get_query_param(parser, "x");
    if (tmp) x = atoi(tmp);
    tmp = httpp_get_query_param(parser, "y");
    if (tmp) y = atoi(tmp);
    g_print("tab=%d x=%d y=%d\n", tab, x, y);

    if (y > 44 && y < 76) {
	t = (x-14)/125;
	if (t != tab) {
	    x = y = 0;
	    memset(&msg, 0, sizeof(msg));
	    msg.type = MSG_EVENT;
	    msg.u.event.event = MSG_EVENT_SELECT_TAB;
	    msg.u.event.tab = t;
	    struct message *msg2 = put_to_rec_queue(&msg);
	    time_t start = time(NULL);
	    while ((time(NULL) < start + 5) && (msg2->type != 0))
		g_usleep(10000);

	    tab = t;
	}
    } else if (tab == 0 && y > 77) {
	    memset(&msg, 0, sizeof(msg));
	    msg.type = MSG_EVENT;
	    msg.u.event.event = MSG_EVENT_CLICK_COMP;
	    msg.u.event.x = x - 25;
	    msg.u.event.y = y - 110;
	    struct message *msg2 = put_to_rec_queue(&msg);
	    time_t start = time(NULL);
	    while ((time(NULL) < start + 5) && (msg2->type != 0))
		g_usleep(10000);
    } else if (tab == 1) {
	    memset(&msg, 0, sizeof(msg));
	    msg.type = MSG_EVENT;
	    msg.u.event.event = MSG_EVENT_CLICK_SHEET;
	    msg.u.event.x = x - 14;
	    msg.u.event.y = y - 77;
	    struct message *msg2 = put_to_rec_queue(&msg);
	    time_t start = time(NULL);
	    while ((time(NULL) < start + 5) && (msg2->type != 0))
		g_usleep(10000);
    }

    g_usleep(200000);

    /* Take a snapshot. */
    gchar buf[128];
    snprintf(buf, sizeof(buf),
	     "import -window '%s' /tmp/js.png",
	     gtk_window_get_title(GTK_WINDOW(main_window)));
    system(buf);

    send_html_top(&s, parser, "");
    string_concat(&s,
	  "<form action=\"judoshiai\">\r\n"
	  "<input type=\"image\" id=\"jsImg\" src=\"js.png\" alt=\"Submit\" />\r\n"
	  "</form>\r\n", t);
    send_html_bottom(&s, parser);
    FEND;
}

int get_js_png(http_parser_t *parser, gchar *txt)
{
    gchar *img;
    gsize len;
    FSTART;

    if (g_file_get_contents("/tmp/js.png", &img, &len, NULL)) {
        s.buf = img;
        s.len = len;
        FEND_PNG;
    } else {
        FEND_404;
    }
}

int get_next_match(http_parser_t *parser, gchar *txt)
{
    FSTART;
    int t = 0;
    struct message *msg;
    struct msg_next_match *nm;

    t = my_atoi(httpp_get_query_param(parser, "t"));

    if (t <= 0 || t > NUM_TATAMIS) {
	g_print("next match t=%d\n", t);
        FEND_404;
    }

    msg = get_next_match_msg(t);
    nm = &msg->u.next_match;
    gint rest_time = 0;
    time_t now = time(NULL);
    if (now < msg->u.next_match.rest_time)
	rest_time = msg->u.next_match.rest_time - now;

    string_concat(&s, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
	  nm->tatami, nm->category, nm->match,  nm->minutes, nm->match_time,
	  nm->gs_time, nm->rep_time, rest_time, nm->pin_time_ippon,
	  nm->pin_time_wazaari, nm->pin_time_yuko, nm->pin_time_koka,
	  nm->flags);
    string_concat(&s, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n",
	  nm->cat_1, nm->blue_1, nm->white_1,
	  nm->cat_2, nm->blue_2, nm->white_2);
    FEND_MIME("text/plain");
}

int sql_cmd(http_parser_t *parser, gchar *txt)
{
    FSTART;
    gchar **tablecopy;
    gint numrows, numcols, row, col;
    const char *sql;

    if (!is_accepted(parser)) {
	string_concat(&s, "%s!\r\n", "No access rights!");
	string_concat(&s, "%s.\r\n\r\n", _("Login"));
        FEND_404;
    }

    sql = httpp_get_query_param(parser, "sql");
    tablecopy = db_get_table_copy((char *)(uintptr_t)sql, &numrows, &numcols);

    if (!tablecopy) {
	string_concat(&s, "Error!\r\n\r\n");
	FEND_404;
    }

    for (row = 0; row < numrows; row++) {
	for (col = 0; col < numcols; col++) {
	    string_concat(&s, "%s", tablecopy[col + numcols*row]);
	    if (col < numcols - 1)
		string_concat(&s, "\t");
	}
	string_concat(&s, "\r\n");
    }
    string_concat(&s, "\r\n");

    db_close_table_copy(tablecopy);
    FEND_MIME("text/plain");
}

int get_file(struct MHD_Connection *connection, const char *url)
{
    gint i;
    struct MHD_Response *response;
    const char *p = url + 1;

    char *mime = "application/octet-stream";
    if (*p == 0)
        p = "index.html";

    gint off = 2;
    char *slash = strchr(p, '/');
    char *dir = NULL;
    char *p2 = strrchr(p, '.');
    if (p2) {
        p2++;
        if (!strcmp(p2, "html")) {
	    mime = "text/html";
	    dir = "html";
        } else if (!strcmp(p2, "htm")) {
	    mime = "text/html";
	    dir = "html";
        } else if (!strcmp(p2, "svg")) {
	    mime = "image/svg+xml";
	    dir = "html";
        } else if (!strcmp(p2, "css")) {
	    mime = "text/css";
	    dir = "css";
        } else if (!strcmp(p2, "txt")) {
	    mime = "text/plain";
        } else if (!strcmp(p2, "png")) {
	    mime = "image/png";
	    dir = "png";
        } else if (!strcmp(p2, "ico")) {
	    mime = "image/x-icon";
	    dir = "png";
        } else if (!strcmp(p2, "jpg")) {
	    mime = "image/jpg";
        } else if (!strcmp(p2, "class")) {
	    mime = "application/x-java-applet";
        } else if (!strcmp(p2, "jar")) {
	    mime = "application/java-archive";
        } else if (!strcmp(p2, "pdf")) {
	    mime = "application/pdf";
        } else if (!strcmp(p2, "swf")) {
	    mime = "application/x-shockwave-flash";
        } else if (!strcmp(p2, "js")) {
	    mime = "text/javascript";
	    dir = "js";
        } else if (!strcmp(p2, "map")) {
	    dir = "js";
        } else if (!strcmp(p2, "ttf")) {
	    mime = "application/octet-stream";
        } else if (!strcmp(p2, "mp3")) {
	    mime = "audio/mpeg3";
	    dir = "mp3";
        } else if (!strcmp(p2, "exe")) {
	    mime = "application/vnd.microsoft.portable-executable";
	    dir = "bin";
	}
    }

    gchar *path[8], **path1;
    path[0] = installation_dir;
    path[1] = "etc";
    if (!slash && dir) {
	path[2] = dir;
	off = 3;
    }
    path1 = g_strsplit(p, "/", 5);

    for (i = 0; i < 4 && path1[i]; i++)
	path[i+off] = path1[i];
    path[i+off] = NULL;

    gchar *docfile = g_build_filenamev(path);
    g_print("GET %s\n", docfile);

    //FILE *f = fopen(docfile, "rb");
    HANDLE fd = int_to_handle(g_open(docfile, O_RDONLY, 0));
    
    GStatBuf st;
    if (fd != INVALID_HANDLE_VALUE) {
	if (g_stat(docfile, &st) || (!S_ISREG(st.st_mode))) {
	    /* not a regular file, refuse to serve */
            g_printerr("%s is not a regular file %x\n", docfile, st.st_mode);
#ifdef WIN32
            CloseHandle(fd);
#else
	    g_close(fd, NULL);
#endif
	    fd = INVALID_HANDLE_VALUE;
	}
    }

    g_strfreev(path1);
    g_free(docfile);

    if (fd == INVALID_HANDLE_VALUE) {
	response = MHD_create_response_from_buffer(strlen (PAGE),
						   (void *) PAGE,
						   MHD_RESPMEM_PERSISTENT);
	int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
	MHD_destroy_response(response);
        return ret;
    }

    response = MHD_create_response_from_fd64(st.st_size, handle_to_int(fd));
    if (!response) {
        g_printerr("response failed\n");
#ifdef WIN32
        CloseHandle(fd);
#else
        g_close(fd, NULL);
#endif
        return MHD_NO;
    }

    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, mime);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

#define INTERNAL_ERROR_PAGE "<html><head><title>Internal error</title></head><body>Internal error</body></html>"

#define REQUEST_REFUSED_PAGE "<html><head><title>Request refused</title></head><body>Request refused (file exists?)</body></html>"

#define DUMMY_PAGE "<html><head><title></title></head><body></body></html>"

/**
 * Context we keep for an upload.
 */
struct UploadContext
{
    char *data;
    int   len;
    int   space;
};

#define httpp_get_query_param(_parser, _str) \
    MHD_lookup_connection_value(_parser, MHD_GET_ARGUMENT_KIND, _str)

#define log g_printerr

int analyze_http(void *cls,
		 struct MHD_Connection *connection,
		 const char *url,
		 const char *method,
		 const char *version,
		 const char *upload_data,
		 size_t *upload_data_size,
		 void **ptr)
{
    static int aptr;

    (void)version;           /* Unused. Silent compiler warning. */
    (void)upload_data;       /* Unused. Silent compiler warning. */
    (void)upload_data_size;  /* Unused. Silent compiler warning. */

    g_printerr("%s %s conn=%p *ptr=%p upload_data=%p upload_data_size=%d\n", method, url, connection, *ptr,
               upload_data, (int)*upload_data_size);

    if (!strcmp(method, MHD_HTTP_METHOD_POST)) {
        struct UploadContext *uc = *ptr;

        if (strcmp("/json", url))
            return MHD_NO;

        if (NULL == uc) {
            if (NULL == (uc = malloc (sizeof (struct UploadContext))))
                return MHD_NO; /* out of memory, close connection */
            memset(uc, 0, sizeof (struct UploadContext));
            uc->space = 1024;
            uc->data = malloc(uc->space);
            *ptr = uc;
            return MHD_YES;
        }

        if (*upload_data_size != 0) {
            if (*upload_data_size >= uc->space - uc->len) {
                while (*upload_data_size >= uc->space - uc->len)
                    uc->space *= 2;
                uc->data = realloc(uc->data, uc->space);
                //g_printerr("Realloc space=%d\n", uc->space);
            }
            memcpy(uc->data + uc->len, upload_data, *upload_data_size);
            uc->len += *upload_data_size;
            //g_printerr("Got %d bytes\n", (int)*upload_data_size);
            //g_printerr("-- data:\n  %s\n", upload_data);
            *upload_data_size = 0;
            return MHD_YES;
        }

        g_printerr("End of %d bytes of data %s\n", uc->len, uc->data);
        cJSON *json = cJSON_Parse(uc->data);
        free(uc->data);
        free(uc);
        if (!json)
            return MHD_NO;
        
        struct message msg;
        struct msg_web_resp resp;

        resp.request = MSG_WEB_GET_ERR;
        resp.ready = 0;
        resp.request = MSG_WEB_JSON;
        resp.u.json.json = NULL;
        
        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_WEB;
        msg.u.web.request = MSG_WEB_JSON;
        msg.u.web.resp = &resp;
        msg.u.web.u.json.json = json;

        put_to_rec_queue(&msg);

        time_t start = time(NULL);
    
        while (!g_atomic_int_get(&resp.ready) &&
               time(NULL) < start + 10) {
            g_usleep(50000);
        }
        if (time(NULL) >= start + 10 ||
            g_atomic_int_get(&resp.ready) != MSG_WEB_RESP_OK) {
            g_printerr("NO JSON REPLY resp.ready=%d\n", resp.ready);
            return MHD_NO;
        }

        struct MHD_Response *response = NULL;
        if (resp.u.json.json) {
            char *s = cJSON_PrintUnformatted(resp.u.json.json);
            cJSON_Delete(resp.u.json.json);
            response = MHD_create_response_from_buffer(strlen(s), s, MHD_RESPMEM_MUST_FREE);
        } else {
            response = MHD_create_response_from_buffer(2, "{}", MHD_RESPMEM_PERSISTENT);
        }
        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
        int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    } // POST

    if (strcmp(method, MHD_HTTP_METHOD_GET) && strcmp(method, MHD_HTTP_METHOD_HEAD))
	return MHD_NO;              /* unexpected method */

    if (&aptr != *ptr) {
	/* do never respond on first call */
	*ptr = &aptr;
	return MHD_YES;
    }
    *ptr = NULL;                  /* reset when done */

    if (!strcmp(url, "/competitors"))
        return get_competitors(connection, FALSE);
    else if (!strcmp(url, "/delcompetitors"))
        return get_competitors(connection, TRUE);
    else if (!strcmp(url, "/getcompetitor"))
        return get_competitor(connection);
    else if (!strcmp(url, "/setcompetitor"))
        return set_competitor(connection);
    else if (!strcmp(url, "/categories"))
        return get_categories(connection);
    else if (!strcmp(url, "/judotimer"))
        return run_judotimer(connection);
    else if (!strcmp(url, "/password"))
        return check_password(connection);
    else if (!strcmp(url, "/login"))
        return check_password(connection);
    else if (!strcmp(url, "/logout"))
        return logout(connection);
    else if (!strcmp(url, "/matchinfo"))
        return get_match_info(connection, "");
    else if (!strcmp(url, "/nameinfo"))
        return get_name_info(connection, "");
    else if (!strcmp(url, "/nextmatch"))
        return get_next_match(connection, "");
    else if (!strcmp(url, "/matchresult"))
        return set_result(connection, "");
    else if (!strcmp(url, "/restcancel"))
        return cancel_rest(connection, "");
    else if (!strcmp(url, "/sqlcmd"))
        return sql_cmd(connection, "");
    else if (!strcmp(url, "/judoshiai"))
        return get_judoshiai(connection, "");
    else if (!strcmp(url, "/web") ||
             !strcmp(url, "/www/web"))
        return get_web(connection, 0/*connum*/);
    else if (!strcmp(url, "/getmatch"))
        get_match(connection);
    else if (!strcmp(url, "/js.png"))
        return get_js_png(connection, "");
    else if (!strcmp(url, "/index.html"))
        return index_html(connection, "");
    else if (!strcmp(url, "/"))
        return index_html(connection, "");
    else
        return get_file(connection, url);

    return MHD_YES;
}

void *httpd_thread(void *arg)
{
    struct MHD_Daemon *d;

    g_printerr("HTTPD thread started\n");

    do {
	d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | MHD_ALLOW_UPGRADE,
			      8088,
			      NULL, NULL, &analyze_http, PAGE, MHD_OPTION_END);
	if (!d) g_usleep(2000000);
    } while (!d);
    
    while (1)   /* exit loop when flag is cleared */
    {
	g_usleep(100000);
    }

    MHD_stop_daemon(d);

    g_printerr("httpd exit\n");
    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}
