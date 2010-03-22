/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2010 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

#if defined(__WIN32__) || defined(WIN32)

#define  __USE_W32_SOCKETS
//#define Win32_Winsock

#include <windows.h>
#include <stdio.h>
#include <initguid.h>
#include <winsock2.h>

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

/* System-dependent definitions */
#ifndef WIN32
#define closesocket     close
#define SOCKET          gint
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define SOCKADDR_BTH    struct sockaddr_rc
#define AF_BTH          PF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM
#endif

#include <gtk/gtk.h>

#include "sqlite3.h"
#include "judoshiai.h"

static void send_packet_1(struct message *msg);
void send_packet(struct message *msg);

static struct message next_match_messages[NUM_TATAMIS];
static gboolean tatami_state[NUM_TATAMIS];

struct message hello_message;
static struct {
    guint addr;
    guint time;
    gchar info_competition[30];
    gchar info_date[30];
    gchar info_place[30];
} others[NUM_OTHERS];

#define NUM_CONNECTIONS 16
static struct {
    guint fd;
    gulong addr;
    guchar buf[512];
    gint ri;
    gboolean escape;
    gint conn_type;
} connections[NUM_CONNECTIONS];

//static volatile struct message message_queue[MSG_QUEUE_LEN];
//static volatile gint msg_put = 0, msg_get = 0;
//static GStaticMutex msg_mutex = G_STATIC_MUTEX_INIT;

gchar *other_info(gint num)
{
    guint addr = ntohl(others[num].addr);
    static gchar buf[100];

    if (others[num].time + 10 > time(NULL)) {
        snprintf(buf, sizeof(buf), "%s - %s - %s (%d.%d.%d.%d)",
                 others[num].info_competition,
                 others[num].info_date,
                 others[num].info_place,
                 (addr >> 24) & 0xff,
                 (addr >> 16) & 0xff,
                 (addr >> 8) & 0xff,
                 addr & 0xff);
    } else {
        snprintf(buf, sizeof(buf), "-");
    }
    return buf;
}

gboolean msg_accepted(struct message *m)
{
    switch (m->type) {
    case MSG_RESULT:
    case MSG_SET_COMMENT:
    case MSG_SET_POINTS:
    case MSG_HELLO:
    case MSG_NAME_REQ:
    case MSG_ALL_REQ:
    case MSG_CANCEL_REST_TIME:
        return TRUE;
    }
    return FALSE;
}

void msg_received(struct message *input_msg)
{
    struct message output_msg;
    guint now, i;
    gboolean newentry = FALSE;
    gulong addr = input_msg->src_ip_addr;
    struct judoka *j;

#if 0
    if (input_msg->type != MSG_HELLO)
        g_print("msg type = %d from %lx\n", input_msg->type, addr);
#endif

    if (input_msg->sender == my_address)
        return;

    switch (input_msg->type) {
    case MSG_RESULT:
        if (TRUE /*tatami_state[input_msg->u.result.tatami-1]*/) {
            output_msg.type = MSG_ACK;
            output_msg.u.ack.tatami = input_msg->u.result.tatami;
            send_packet_1(&output_msg);
        }

        set_points_and_score(input_msg);
        break;

    case MSG_SET_COMMENT:
        set_comment_from_net(input_msg);
        break;

    case MSG_SET_POINTS:
        set_points_from_net(input_msg);
        break;

    case MSG_HELLO:
        now = time(NULL);

        for (i = 0; i < NUM_OTHERS; i++)
            if (others[i].addr == addr &&
                (others[i].time + 10) >= now)
                break;

        if (i >= NUM_OTHERS) {
            newentry = TRUE;
            for (i = 0; i < NUM_OTHERS; i++)
                if (others[i].addr == 0 ||
                    (others[i].time + 10) < now)
                    break;
        }

        if (i < NUM_OTHERS) {
            others[i].addr = addr;
            others[i].time = now;
            if (newentry || strcmp(others[i].info_date, input_msg->u.hello.info_date)) {
                strcpy(others[i].info_competition, input_msg->u.hello.info_competition);
                strcpy(others[i].info_date,        input_msg->u.hello.info_date);
                strcpy(others[i].info_place,       input_msg->u.hello.info_place);
            }
        }
        break;

    case MSG_NAME_REQ:
        j = get_data(input_msg->u.name_req.index);
        if (j) {
            memset(&output_msg, 0, sizeof(output_msg));
            output_msg.type = MSG_NAME_INFO;
            output_msg.u.name_info.index = input_msg->u.name_req.index;
            strncpy(output_msg.u.name_info.last, j->last, sizeof(output_msg.u.name_info.last)-1);
            strncpy(output_msg.u.name_info.first, j->first, sizeof(output_msg.u.name_info.first)-1);
            strncpy(output_msg.u.name_info.club, 
		    j->country && j->country[0] ? j->country : j->club, 
		    sizeof(output_msg.u.name_info.club)-1);
            send_packet(&output_msg);
            free_judoka(j);
        }
        break;

    case MSG_ALL_REQ:
        for (i = 1; i <= NUM_TATAMIS; i++)
            send_matches(i);
        break;

    case MSG_CANCEL_REST_TIME:
        db_reset_last_match_times(input_msg->u.cancel_rest_time.category, 
                                  input_msg->u.cancel_rest_time.number,
                                  input_msg->u.cancel_rest_time.blue,
                                  input_msg->u.cancel_rest_time.white);
        update_matches(input_msg->u.cancel_rest_time.category, 0, 0);
        break;
    }
}

gint timeout_callback(gpointer data)
{
    int i;

    for (i = 0; i < NUM_TATAMIS; i++) {
        /* DON'T CARE
           if (!tatami_state[i])
           continue;
        */

        if (next_match_messages[i].u.next_match.tatami > 0) {
            //g_print("next match message sent\n");
            send_packet_1(&next_match_messages[i]);
        }
    }

    hello_message.type = MSG_HELLO;
    send_packet_1(&hello_message);

    return TRUE;
}

void copy_packet(struct message *msg)
{
#if 0
    if (msg->type != MSG_HELLO && msg->type != MSG_DUMMY) {
        if (msg->type < 1 || msg->type >= NUM_MESSAGES) {
            g_print("CORRUPTED MESSAGE %d\n", msg->type);
            return;
        }
#if 0
        if (msg->type == MSG_NEXT_MATCH && msg->sender != my_address)
            g_print("next match: sender=%d src_ip_addr=%lx\n",
                    msg->sender, msg->src_ip_addr);
#endif
    }

    if (msg->type == MSG_DUMMY)
        return;

    g_static_mutex_lock(&msg_mutex);
    message_queue[msg_put++] = *msg;
    if (msg_put >= MSG_QUEUE_LEN)
        msg_put = 0;

    if (msg_put == msg_get)
        g_print("MSG QUEUE FULL!\n");
    g_static_mutex_unlock(&msg_mutex);
#endif
}

static void send_packet_1(struct message *msg)
{
    gint old_val;

    if (msg->type == MSG_NEXT_MATCH) {
        /* Update rest time. rest_time has an absolute value 
         * while we send the remaining time. */
        old_val = msg->u.next_match.rest_time;
        time_t now = time(NULL);
        if (now >= old_val)
            msg->u.next_match.rest_time = 0;
        else
            msg->u.next_match.rest_time = old_val - now;
    }

    msg->sender = my_address;
    msg_to_queue(msg);

    if (msg->type == MSG_NEXT_MATCH)
        msg->u.next_match.rest_time = old_val;
}

void send_packet(struct message *msg)
{
    if (msg->type == MSG_NEXT_MATCH &&
        msg->u.next_match.tatami > 0 &&
        msg->u.next_match.tatami <= NUM_TATAMIS) {
        next_match_messages[msg->u.next_match.tatami-1] = *msg;

        /* DON'T CARE
           if (!tatami_state[msg->u.next_match.tatami-1])
           return;
        */
    }

    send_packet_1(msg);
}

void set_tatami_state(GtkWidget *menu_item, gpointer data)
{
    gchar buf[32];
    gint tatami = (gint)data;

    tatami_state[tatami-1] = GTK_CHECK_MENU_ITEM(menu_item)->active;

    sprintf(buf, "tatami%d", tatami);
    g_key_file_set_integer(keyfile, "preferences", buf, 
                           GTK_CHECK_MENU_ITEM(menu_item)->active);
}


/* Which message is sent to who? Avoid sending unnecessary messages. */
static gboolean send_message_to_application[NUM_MESSAGES][NUM_APPLICATION_TYPES] = {
    // ALL  SHIAI  TIMER  INFO
    {FALSE, FALSE, FALSE, FALSE}, // 0,
    {TRUE,  FALSE, TRUE , FALSE}, // MSG_NEXT_MATCH = 1,
    {TRUE,  FALSE, FALSE, FALSE}, // MSG_RESULT,
    {TRUE,  FALSE, TRUE , FALSE}, // MSG_ACK,
    {TRUE,  FALSE, FALSE, FALSE}, // MSG_SET_COMMENT,
    {TRUE,  FALSE, FALSE, FALSE}, // MSG_SET_POINTS,
    {TRUE,  FALSE, FALSE, FALSE}, // MSG_HELLO,
    {TRUE,  FALSE, FALSE, FALSE}, // MSG_DUMMY,
    {TRUE,  FALSE, FALSE, TRUE }, // MSG_MATCH_INFO,
    {TRUE,  FALSE, FALSE, TRUE }, // MSG_NAME_INFO,
    {TRUE,  FALSE, FALSE, FALSE}, // MSG_NAME_REQ,
    {TRUE,  FALSE, FALSE, FALSE}, // MSG_ALL_REQ,
    {TRUE,  FALSE, FALSE, TRUE }, // MSG_CANCEL_REST_TIME,
    {TRUE,  FALSE, TRUE , FALSE}  // MSG_UPDATE_LABEL,
};

/*
 * Does not need gdk_threads_enter/leave() wrapping
 *      if no GTK/GDK apis are called
 */

extern void sighandler(int sig);

gchar *xml = "<?xml version=\"1.0\"?>\n"
        "<cross-domain-policy>\n"
        "    <allow-access-from domain=\"*\" to-ports=\"2310\"/>\n"
        "</cross-domain-policy>";

gpointer node_thread(gpointer args)
{
    SOCKET node_fd, tmp_fd;
    guint alen;
    struct sockaddr_in my_addr, caller;
    gint reuse = 1;
    fd_set read_fd, fds;

#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    if ((node_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("serv socket");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (setsockopt(node_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt (SO_REUSEADDR)");
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SHIAI_PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(node_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) {
        perror("serv bind");
        g_print("CANNOT BIND!\n");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    listen(node_fd, 5);

    FD_ZERO(&read_fd);
    FD_SET(node_fd, &read_fd);

    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
        struct timeval timeout;
        gint r, i;
                
        fds = read_fd;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;

        r = select(32, &fds, NULL, NULL, &timeout);

        /* messages to send */

        g_static_mutex_lock(&send_mutex);
        if (msg_queue_get != msg_queue_put) {
            for (i = 0; i < NUM_CONNECTIONS; i++) {
                if (connections[i].fd == 0)
                    continue;

                // don't send unnecessary messages
                if (msg_to_send[msg_queue_get].type < 1 ||
                    msg_to_send[msg_queue_get].type >= NUM_MESSAGES ||
                    connections[i].conn_type < 0 ||
                    connections[i].conn_type >= NUM_APPLICATION_TYPES ||
                    send_message_to_application
                    [(gint)msg_to_send[msg_queue_get].type]
                    [connections[i].conn_type] == FALSE)
                    continue;

                if (send_msg(connections[i].fd, &msg_to_send[msg_queue_get]) < 0) {
                    perror("sendto");
                    g_print("Node cannot send: conn=%d fd=%d\n", i, connections[i].fd);
                }
            }

            msg_queue_get++;
            if (msg_queue_get >= MSG_QUEUE_LEN)
                msg_queue_get = 0;
        }
        g_static_mutex_unlock(&send_mutex);

        if (r <= 0)
            continue;

        /* messages to receive */

        if (FD_ISSET(node_fd, &fds)) {
            alen = sizeof(caller);
            if ((tmp_fd = accept(node_fd, (struct sockaddr *)&caller, &alen)) < 0) {
                perror("serv accept");
                continue;
            }
#if 0
            const int nodelayflag = 1;
            if (setsockopt(tmp_fd, IPPROTO_TCP, TCP_NODELAY, 
                           (const void *)&nodelayflag, sizeof(nodelayflag))) {
                g_print("CANNOT SET TCP_NODELAY (2)\n");
            }
#endif
            for (i = 0; i < NUM_CONNECTIONS; i++)
                if (connections[i].fd == 0)
                    break;

            if (i >= NUM_CONNECTIONS) {
                g_print("Node cannot accept new connections!\n");
                closesocket(tmp_fd);
                continue;
            }

            connections[i].fd = tmp_fd;
            connections[i].addr = caller.sin_addr.s_addr;
            connections[i].conn_type = 0;
            g_print("Node: new connection[%d]: fd=%d addr=%x\n", 
                    i, tmp_fd, caller.sin_addr.s_addr);
            FD_SET(tmp_fd, &read_fd);
        }

        for (i = 0; i < NUM_CONNECTIONS; i++) {
            static guchar inbuf[2000];
			
            if (connections[i].fd == 0)
                continue;

            if (!(FD_ISSET(connections[i].fd, &fds)))
                continue;

            r = recv(connections[i].fd, inbuf, sizeof(inbuf), 0);
            if (r > 0) {
                guchar *p = connections[i].buf;
                gint j, blen = sizeof(connections[i].buf);
		struct message msg;

                if (strncmp(inbuf, "<policy-file-request/>", 10) == 0) {
                    send(connections[i].fd, xml, strlen(xml)+1, 0);
                    g_print("policy file sent to %d\n", i);
                }

                for (j = 0; j < r; j++) {
                    guchar c = inbuf[j];
                    if (c == COMM_ESCAPE) {
                        connections[i].escape = TRUE;
                    } else if (connections[i].escape) {
                        if (c == COMM_FF) {
                            if (connections[i].ri < blen)
                                p[connections[i].ri++] = COMM_ESCAPE;
                        } else if (c == COMM_BEGIN) {
                            connections[i].ri = 0;
                        } else if (c == COMM_END) {
			    decode_msg(&msg, p, connections[i].ri);
                            msg.src_ip_addr = connections[i].addr;

                            if (msg.type == MSG_DUMMY) {
                                if (msg.u.dummy.application_type !=
                                    connections[i].conn_type) {
                                    connections[i].conn_type =
                                        msg.u.dummy.application_type;
                                    g_print("Node: conn %d type %d\n",
                                            i, connections[i].conn_type);
                                }
                            } else {
                                put_to_rec_queue(&msg); // XXX
                            }
                        } else {
                            g_print("Node: conn %d has wrong char 0x%02x after esc!\n", i, c);
                        }
                        connections[i].escape = FALSE;
                    } else if (connections[i].ri < blen) {
                        p[connections[i].ri++] = c;
                    }
                }
            } else {
                g_print("Node: connection %d fd=%d closed\n", i, connections[i].fd);
                closesocket(connections[i].fd);
                FD_CLR(connections[i].fd, &read_fd);
                connections[i].fd = 0;
            }
        }
    }

    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}

G_LOCK_EXTERN(db);

gpointer server_thread(gpointer args)
{
    SOCKET serv_fd, tmp_fd;
    guint alen;
    struct sockaddr_in my_addr, caller;
    gint reuse = 1;

    if ((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("serv socket");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt (SO_REUSEADDR)");
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SHIAI_PORT+1);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serv_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) {
        perror("serv bind");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    listen(serv_fd, 1);

    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
        gint n;
        FILE *db_fd;
        static gchar buf[500];

        alen = sizeof(caller);
        if ((tmp_fd = accept(serv_fd, (struct sockaddr *)&caller, &alen)) < 0) {
            perror("serv accept");
            continue;
        }

        G_LOCK(db);
        if ((db_fd = fopen(database_name, "rb"))) {
            while ((n = fread(buf, 1, sizeof(buf), db_fd)) > 0) {
                gint w = send(tmp_fd, buf, n, 0);
#ifdef WIN32
                if (w < 0)
                    g_print("server write: %d\n", WSAGetLastError());
#else
                if (w < 0)
                    perror("server write");
#endif

                if (w != n) g_print("send problem: %d of %d sent\n", w, n);
            }
            fclose(db_fd);
        }
        G_UNLOCK(db);
        closesocket(tmp_fd);
    }

    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}

gint read_file_from_net(gchar *filename, gint num)
{
    FILE *f;
    SOCKET client_fd;
    gint n;
    struct sockaddr_in server;
    gchar buf[500];
    gint ntot = 0;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("client socket");
        return -1;
    }
        
    memset(&server, 0, sizeof(server));
    server.sin_family      = AF_INET;
    server.sin_port        = htons(SHIAI_PORT+1);
    server.sin_addr.s_addr = others[num].addr;

    f = fopen(filename, "wb");
    if (f == NULL) {
        perror("file open");
        closesocket(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&server, sizeof(server))) {
        perror("client connect");
        closesocket(client_fd);
        fclose(f);
        return -1;
    }

    while ((n = recv(client_fd, buf, sizeof(buf), 0)) > 0) {
        fwrite(buf, 1, n, f);
        ntot += n;
    }

    fclose(f);
    closesocket(client_fd);

    if (ntot > 10)
        return 0;
    else
        return -1;
}

void show_node_connections( GtkWidget *w,
                            gpointer   data )
{
    gchar addrstr[128];
    gulong myaddr;
    GtkWidget *dialog, *vbox, *label;
    gint i;

    dialog = gtk_dialog_new_with_buttons (_("Connections to this node"),
                                          NULL,
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (connections[i].fd == 0)
            continue;
        gchar *t = "?";
        switch (connections[i].conn_type) {
        case APPLICATION_TYPE_SHIAI: t = "JudoShiai"; break;
        case APPLICATION_TYPE_TIMER: t = "JudoTimer"; break;
        case APPLICATION_TYPE_INFO:  t = "JudoInfo";  break;
        }

        myaddr = ntohl(connections[i].addr);
        sprintf(addrstr, "%ld.%ld.%ld.%ld (%s)", 
                (myaddr>>24)&0xff, (myaddr>>16)&0xff, 
                (myaddr>>8)&0xff, (myaddr)&0xff, t);
        label = gtk_label_new(addrstr);
        gtk_box_pack_start_defaults(GTK_BOX(vbox), label);
    }

    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox);
    gtk_widget_show_all(dialog);

    g_signal_connect(G_OBJECT(dialog), "response",
                     G_CALLBACK(gtk_widget_destroy), NULL);
}

gboolean keep_connection(void)
{
    return TRUE;
}

gint get_port(void)
{
    return SHIAI_PORT;
}
