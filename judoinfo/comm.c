/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright 2006-2023 Hannu Jokinen
 * 
 * This file is part of JudoShiai.
 *
 * JudoShiai is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * JudoShiai is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with JudoShiai. If not, see
 * <https://www.gnu.org/licenses/>.
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#endif /* WIN32 */

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#ifdef WIN32
#if (GTKVER != 3)
#include <glib/gwin32.h>
#endif
#endif

#include "judoinfo.h"
#include "comm.h"

/* System-dependent definitions */
#ifndef WIN32
#define closesocket     close
#define SOCKET  int
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define SOCKADDR_BTH struct sockaddr_rc
#define AF_BTH PF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM
#endif

static void get_bracket(gint tatami, gint last_category, gint last_number);
void send_packet(struct message *msg);

G_LOCK_DEFINE_STATIC(req_mutex);

time_t traffic_last_rec_time;

void copy_packet(struct message *msg)
{
    /* dummy function to make linker happy (defined in comm.c) */
}

gint timeout_callback(gpointer data)
{
    struct message msg;
    extern gint my_address;

    msg.type = MSG_DUMMY;
    msg.sender = my_address;
    msg.u.dummy.application_type = application_type();
    send_packet(&msg);

    return TRUE;
}

#define ASK_TABLE_LEN (4*1024)
static gint ask_table[ASK_TABLE_LEN];
static gint put_ptr = 0, answers = 0;

static gint remove_pos(gint p)
{
    gint i, r = 0;
    G_LOCK(req_mutex);
    r = ask_table[p];
    for (i = p; i < put_ptr-1; i++)
	ask_table[i] = ask_table[i+1];
    put_ptr--;
    G_UNLOCK(req_mutex);
    return r;
}

gint timeout_ask_for_data(gpointer data)
{
    if (put_ptr == 0)
	return TRUE;

    // Slow down if lots of answers since most probably
    // other infos are asking, too.
    if (answers > 10) {
	if (rand() & 7)	return TRUE;
    } else if (answers > 5) {
	if (rand() & 3) return TRUE;
    } else if (answers > 2) {
	if (rand() & 1) return TRUE;
    }
    answers = 0;

    gint ix = remove_pos(rand() % put_ptr);

    if (ix < 10)
	return TRUE;

    struct name_data *j = avl_get_data(ix);
    if (j)
	return TRUE;

    extern gint my_address;
    struct message msg;
    msg.type = MSG_NAME_REQ;
    msg.sender = my_address;
    msg.u.name_req.index = ix;
    send_packet(&msg);

    return TRUE;
}

static void remove_from_req_queue(gint ix)
{
    gint i;
    G_LOCK(req_mutex);
    for (i = 0; i < put_ptr; i++) {
	if (ask_table[i] == ix)
	    ask_table[i] = 0;
    }
    G_UNLOCK(req_mutex);
}

void ask_for_data(gint index)
{
    // already requested?
    gint i;
    G_LOCK(req_mutex);
    for (i = 0; i < put_ptr; i++) {
	if (ask_table[i] == index) {
	    G_UNLOCK(req_mutex);
	    return;
	}
    }

    if (put_ptr < ASK_TABLE_LEN)
	ask_table[put_ptr++] = index;
    else mylog("%s:%d queue overflow!\n", __FUNCTION__, __LINE__);
    G_UNLOCK(req_mutex);
}

gboolean msg_accepted(struct message *m)
{
    switch (m->type) {
    case MSG_MATCH_INFO:
    case MSG_11_MATCH_INFO:
    case MSG_NAME_INFO:
    case MSG_CANCEL_REST_TIME:
        return TRUE;
    }
    return FALSE;
}

static void handle_info_msg(struct msg_match_info *input_msg)
{
    gint tatami;
    gint position;

    tatami = input_msg->tatami - 1;
    position = input_msg->position;
    if (tatami < 0 || tatami >= NUM_TATAMIS)
	return;
    if (position < 0 || position >= NUM_LINES)
	return;

    match_list[tatami][position].category = input_msg->category;
    match_list[tatami][position].number   = input_msg->number;
    match_list[tatami][position].blue     = input_msg->blue;
    match_list[tatami][position].white    = input_msg->white;
    match_list[tatami][position].flags    = input_msg->flags;
    match_list[tatami][position].round    = input_msg->round;
    match_list[tatami][position].rest_end = input_msg->rest_time + time(NULL);

#if 0    
	mylog("match info %d:%d b=%d w=%d\n",
	match_list[tatami][position].category,
	match_list[tatami][position].number,
	match_list[tatami][position].blue,
	match_list[tatami][position].white);
#endif

#if 0 // let avl search initiate data request (need to know basis)
    j = avl_get_data(match_list[tatami][position].category);
    if (j == NULL) {
	ask_for_data(match_list[tatami][position].category);
    }

    j = avl_get_data(match_list[tatami][position].blue);
    if (j == NULL) {
	ask_for_data(match_list[tatami][position].blue);
    }

    if (position > 0) {
	j = avl_get_data(match_list[tatami][position].white);
	if (j == NULL) {
	    ask_for_data(match_list[tatami][position].white);
	}
    }
#endif

    write_matches();
    //refresh_window();

    if (show_tatami[tatami] == FALSE &&
	number_of_conf_tatamis() == 0 &&
	match_list[tatami][position].blue &&
	match_list[tatami][position].white)
	show_tatami[tatami] = TRUE;

    if (show_tatami[tatami] && position == 1 && show_bracket()) {
        static gint last_category = 0, last_number = 0;
        if (input_msg->category != last_category ||
            input_msg->number != last_number) {
            last_category = input_msg->category;
            last_number = input_msg->number;
	    get_bracket(tatami+1, last_category, last_number);
	}
    }
}

void msg_received(struct message *input_msg)
{
    gint i;

    /**
    if (input_msg->sender >= 0 && input_msg->sender < 10)
        return;
    **/
    
    traffic_last_rec_time = time(NULL);
#if 0
    mylog("msg type = %d from %d\n",
            input_msg->type, input_msg->sender);
#endif
    switch (input_msg->type) {
    case MSG_MATCH_INFO:
	handle_info_msg(&input_msg->u.match_info);
        g_atomic_int_set(&refresh_now, 1);
        break;

    case MSG_11_MATCH_INFO:
	for (i = 0; i < 11; i++) {
	    handle_info_msg(&input_msg->u.match_info_11.info[i]);
	}
        g_atomic_int_set(&refresh_now, 1);
        break;

    case MSG_NAME_INFO:
	remove_from_req_queue(input_msg->u.name_info.index);

        avl_set_data(input_msg->u.name_info.index,
                     input_msg->u.name_info.first,
                     input_msg->u.name_info.last,
                     input_msg->u.name_info.club);
	answers++;
        //refresh_window();
#if 0
        mylog("name info %d: %s %s, %s\n",
                input_msg->u.name_info.index,
                input_msg->u.name_info.first,
                input_msg->u.name_info.last,
                input_msg->u.name_info.club);
#endif
        break;
#if 0
    case MSG_CANCEL_REST_TIME:
        for (tatami = 0; tatami < NUM_TATAMIS; tatami++) {
            for (position = 0; position < NUM_LINES; position++) {
                if (match_list[tatami][position].category == input_msg->u.cancel_rest_time.category &&
                    match_list[tatami][position].number == input_msg->u.cancel_rest_time.number) {
                    match_list[tatami][position].rest_end = 0;
                    match_list[tatami][position].flags = 0;
                    //refresh_window();
                    return;
                }
            }
        }
        break;
#endif
    }
}


void send_packet(struct message *msg)
{
#if 0
    if (msg->type == MSG_RESULT) {
        printf("tatami=%d cat=%d match=%d min=%d blue=%x white=%x bv=%d wv=%d\n",
               msg->u.result.tatami,
               msg->u.result.category,
               msg->u.result.match,
               msg->u.result.minutes,
               msg->u.result.blue_score,
               msg->u.result.white_score,
               msg->u.result.blue_vote,
               msg->u.result.white_vote);
    }
#endif

    msg_to_queue(msg);
}

gboolean keep_connection(void)
{
    return TRUE;
}

gint get_port(void)
{
    return SHIAI_PORT;
}

#define BRACKET_LEN 1000000
guchar bracket[BRACKET_LEN];
gint bracket_len = 0;
gboolean bracket_ok = FALSE;
guchar *bracket_start = NULL;
guchar png_start[] = {137, 80, 78, 71, 13, 10, 26, 10};
gint bracket_pos = 0;

gint bracket_type(void)
{
    if (!bracket_start || !bracket_ok)
        return BRACKET_TYPE_ERR;

    if (memcmp(bracket_start, png_start, 8) == 0)
        return BRACKET_TYPE_PNG;

    return BRACKET_TYPE_SVG;
}

static void get_bracket(gint tatami, gint category, gint number)
{
    SOCKET fd;
    gint n, k;
    struct sockaddr_in node;
    char out[64];

    bracket_ok = FALSE;
    bracket_start = NULL;
    bracket_len = 0;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("bracket socket");
        mylog("CANNOT CREATE SOCKET (%s:%d)!\n", __FUNCTION__, __LINE__);
        return;
    }

    memset(&node, 0, sizeof(node));
    node.sin_family      = AF_INET;
    node.sin_port        = htons(8088);
    node.sin_addr.s_addr = node_ip_addr;

    if (connect(fd, (struct sockaddr *)&node, sizeof(node))) {
        closesocket(fd);
        return;
    }

#if 0
    snprintf(out, sizeof(out), "GET /bracket?t=%d&s=1 HTTP/1.0\r\n\r\n", tatami-1);
#else
    snprintf(out, sizeof(out), "GET /web?op=%d&t=%d&s=1 HTTP/1.0\r\n\r\n",
             MSG_WEB_GET_BRACKET, tatami);
#endif
    k = strlen(out);
    if ((n = send(fd, (char *)out, k, 0)) != k) {
        mylog("%s: send error: sent %d/%d octets\n", __FUNCTION__, n, k);
        closesocket(fd);
        return;
    }

    while ((n = recv(fd, (void *)&bracket[bracket_len], sizeof(bracket) - bracket_len - 1, 0)) > 0) {
        bracket_len += n;
    }

    closesocket(fd);

    bracket[bracket_len] = 0;
    bracket_start = (guchar *)strstr((gchar *)bracket, "\r\n\r\n");
    if (bracket_start)
        bracket_start += 4;
    else
        return;
    bracket_len -= (gint)(bracket_start - bracket);
    bracket_ok = TRUE;
}

cairo_status_t
bracket_read(void *closure,
             unsigned char *data,
             unsigned int length)
{
    if (!bracket_ok || !bracket_start)
        return CAIRO_STATUS_READ_ERROR;

    if (length > bracket_len)
        length = bracket_len;

    memcpy(data, bracket_start + bracket_pos, length);
    bracket_pos += length;

    return CAIRO_STATUS_SUCCESS;
}
