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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#endif /* WIN32 */

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) gettext(String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain)
#define bind_textdomain_codeset(Domain,Codeset) (Codeset)
#endif /* ENABLE_NLS */

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

#include "comm.h"

extern gint my_address;
extern gboolean this_is_shiai(void);
extern void copy_packet(struct message *msg);
extern int pwcrc32(const unsigned char *str, int len);

gulong my_ip_address = 0, node_ip_addr = 0, ssdp_ip_addr = 0;
static gulong tmp_node_addr;
gchar  my_hostname[100];
gboolean connection_ok = FALSE;
#if (GTKVER == 3)
G_LOCK_DEFINE(send_mutex);
G_LOCK_DEFINE_STATIC(rec_mutex);
#else
GStaticMutex send_mutex = G_STATIC_MUTEX_INIT;
static GStaticMutex rec_mutex = G_STATIC_MUTEX_INIT;
#endif

static struct {
    gint rx;
    gint tx;
    guint start;
    guint stop;
    gint rxtype[NUM_MESSAGES];
    gint txtype[NUM_MESSAGES];
} msg_stat;

uint32_t host2net(uint32_t a)
{
    return htonl(a);
}

static gint unblock_socket(gint sd) {
#ifdef WIN32
    u_long one = 1;
    if(sd != 501) // Hack related to WinIP Raw Socket support
        ioctlsocket (sd, FIONBIO, &one);
#else
    int options;
    /* Unblock our socket to prevent recvfrom from blocking forever
       on certain target ports. */
    options = O_NONBLOCK | fcntl(sd, F_GETFL);
    fcntl(sd, F_SETFL, options);
#endif //WIN32
    return 1;
}

#ifdef WIN32
gint nodescan(guint network) // addr in network byte order
{
    SOCKET sd;
    guint last, addr;
    struct sockaddr_in dst;
    gint r;
    fd_set xxx;
    gint ret;
    struct timeval timeout;
    (void)r;

    addr = ntohl(network) & 0xffffff00;

    for (last = 1; last <= 254 && node_ip_addr == 0; last++) {
        sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sd < 0)
            return -1;

        unblock_socket(sd);

        struct linger l;
        l.l_onoff = 1;
        l.l_linger = 0;
        setsockopt(sd, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(struct linger));

        memset(&dst, 0, sizeof(dst));
        dst.sin_family = AF_INET;
        dst.sin_port = htons(SHIAI_PORT);
        dst.sin_addr.s_addr = htonl(addr + last);

        r = connect(sd, (const struct sockaddr *)&dst, sizeof(dst));

        g_usleep(500000);

        FD_ZERO(&xxx);
        FD_SET(sd, &xxx);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        ret = select(0, NULL, &xxx, NULL, &timeout);
        closesocket(sd);

        if (ret > 0) {
            dst.sin_addr.s_addr = htonl(addr + last);
            mylog("%s: node found\n", inet_ntoa(dst.sin_addr));
            return dst.sin_addr.s_addr;
        }

        g_usleep(1000000);
    }

    return 0;
}

#else // !WIN32

gint nodescan(guint network) // addr in network byte order
{
    guint i, sd[16], block;
    guint last, addr;
    struct sockaddr_in dst;
    guint found = 0;

    addr = ntohl(network) & 0xffffff00;

    for (block = 0; block < 16 && node_ip_addr == 0; block++) {
        for (i = 0; i < 16; i++) {
            last = (block << 4) | i;
            if (last == 0 || last > 254)
                continue;

            memset(&dst, 0, sizeof(dst));
            dst.sin_family = AF_INET;
            dst.sin_port = htons(SHIAI_PORT);
            dst.sin_addr.s_addr = htonl(addr + last);

            sd[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sd < 0)
                return -1;

            unblock_socket(sd[i]);

            connect(sd[i], (const struct sockaddr *)&dst, sizeof(dst));
        }

        g_usleep(1000000);

        for (i = 0; i < 16; i++) {
            last = (block << 4) | i;
            if (last == 0 || last > 254)
                continue;

            memset(&dst, 0, sizeof(dst));
            dst.sin_family = AF_INET;
            dst.sin_port = htons(SHIAI_PORT);
            dst.sin_addr.s_addr = htonl(addr + last);

            int ret = connect(sd[i], (const struct sockaddr *)&dst, sizeof(dst));

            closesocket(sd[i]);

            if (ret >= 0 && found == 0)
                found = dst.sin_addr.s_addr;
        }

        if (found) {
            dst.sin_addr.s_addr = found;
            mylog("%s: node found\n", inet_ntoa(dst.sin_addr));
            return found;
        }
    }

    return 0;
}
#endif //WIN32


static gulong addrs[16];
static gint   addrcnt = 0;

gulong get_addr(gint i) {
    return addrs[i];
}
gint get_addrcnt(void) {
    return addrcnt;
}

static gulong test_address(gulong a, gulong mask, gboolean is_not)
{
    gint i;
    for (i = 0; i < addrcnt; i++) {
        if (is_not == FALSE) {
            if ((addrs[i] & mask) == (a & mask))
                return htonl(addrs[i]);
        } else {
            if ((addrs[i] & mask) != (a & mask))
                return htonl(addrs[i]);
        }
    }
    return 0;
}

gulong get_my_address()
{
    gulong found;

    addrcnt = 0;

#ifdef WIN32
    gint i;

    SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
    if (sd == SOCKET_ERROR) {
        return 0;
    }

    INTERFACE_INFO InterfaceList[20];
    unsigned long nBytesReturned;
    if (WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
                 sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR) {
        closesocket(sd);
        return 0;
    }

    gint nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);

    for (i = 0; i < nNumInterfaces; ++i) {
        struct sockaddr_in *pAddress;
        pAddress = (struct sockaddr_in *) & (InterfaceList[i].iiAddress);
        mylog("Interface %s\n", inet_ntoa(pAddress->sin_addr));

        //u_long nFlags = InterfaceList[i].iiFlags;
        if (/*(nFlags & IFF_UP) &&*/ (addrcnt < 16))
            addrs[addrcnt++] = ntohl(pAddress->sin_addr.s_addr);
    }

    closesocket(sd);

#else // !WIN32

    struct if_nameindex *ifnames, *ifnm;
    struct ifreq ifr;
    struct sockaddr_in sin;
    gint   fd;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return 0;

    ifnames = if_nameindex();
    for (ifnm = ifnames; ifnm && ifnm->if_name && ifnm->if_name[0]; ifnm++) {
        strncpy (ifr.ifr_name, ifnm->if_name, IFNAMSIZ);
        if (ioctl (fd, SIOCGIFADDR, &ifr) == 0) {
            memcpy (&sin, &(ifr.ifr_addr), sizeof (sin));
            if (addrcnt < 16)
                addrs[addrcnt++] = ntohl(sin.sin_addr.s_addr);
            g_print ("Interface %s = IP %s\n", ifnm->if_name, inet_ntoa (sin.sin_addr));
        }
    }
    if_freenameindex (ifnames);
    close(fd);

#endif // WIN32

    if ((found = test_address(0xc0000000, 0xff000000, FALSE)) != 0) // 192.x.x.x
        return found;
    if ((found = test_address(0xac000000, 0xff000000, FALSE)) != 0) // 172.x.x.x
        return found;
    if ((found = test_address(0x0a000000, 0xff000000, FALSE)) != 0) // 10.x.x.x
        return found;
    if ((found = test_address(0x7f000000, 0xff000000, TRUE)) != 0) // 127.x.x.x
        return found;
    if ((found = test_address(0x00000000, 0x00000000, FALSE)) != 0) // any address
        return found;
    return 0;
}

extern GKeyFile *keyfile;

static void node_ip_address_callback(GtkWidget *widget,
                                     GdkEvent *event,
                                     GtkWidget *data)
{
    gulong a,b,c,d;

    if (ptr_to_gint(event) == GTK_RESPONSE_OK) {
        sscanf(gtk_entry_get_text(GTK_ENTRY(data)), "%ld.%ld.%ld.%ld", &a, &b, &c, &d);
        node_ip_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
        g_key_file_set_string(keyfile, "preferences", "nodeipaddress",
                              gtk_entry_get_text(GTK_ENTRY(data)));
    }
    gtk_widget_destroy(widget);
}

static void multicast_cb(GtkWidget *w, gpointer arg)
{
    GtkWidget *address = (GtkWidget *)arg;
    gulong myaddr;
    gchar addrstr[64];
    gboolean mc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    if (mc) {
	myaddr = MULTICAST_ADDR;
    } else if (ntohl(node_ip_addr) == MULTICAST_ADDR) {
	myaddr = 0;
    } else {
	myaddr = ntohl(node_ip_addr);
    }

    sprintf(addrstr, "%ld.%ld.%ld.%ld",
	    (myaddr>>24)&0xff, (myaddr>>16)&0xff,
	    (myaddr>>8)&0xff, (myaddr)&0xff);
    gtk_entry_set_text(GTK_ENTRY(address), addrstr);
}

void ask_node_ip_address( GtkWidget *w,
                          gpointer   data )
{
    gchar addrstr[64];
    gulong myaddr = ntohl(node_ip_addr);
    GtkWidget *dialog, *hbox, *label, *address;

    sprintf(addrstr, "%ld.%ld.%ld.%ld",
            (myaddr>>24)&0xff, (myaddr>>16)&0xff,
            (myaddr>>8)&0xff, (myaddr)&0xff);

    dialog = gtk_dialog_new_with_buttons (_("Address of the communication node"),
                                          NULL,
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          NULL);

    label = gtk_label_new(_("IP address:"));
    address = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(address), addrstr);
    hbox = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_grid_attach(GTK_GRID(hbox), label, 0, 0, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(hbox), address, label, GTK_POS_RIGHT, 1, 1);

    if (!this_is_shiai()) {
	gtk_grid_attach(GTK_GRID(hbox), gtk_label_new(_("Multicast")), 0, 1, 1, 1);
	GtkWidget *multicast = gtk_check_button_new();
	gtk_grid_attach(GTK_GRID(hbox), multicast, 1, 1, 1, 1);
	if (myaddr == MULTICAST_ADDR) {
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multicast), TRUE);
	}
	g_signal_connect(G_OBJECT(multicast), "clicked",
			 G_CALLBACK(multicast_cb), address);
    }
    
    if (connection_ok && node_ip_addr == 0) {
        gulong myaddr1 = ntohl(tmp_node_addr);
        g_snprintf(addrstr, sizeof(addrstr), "%s: %ld.%ld.%ld.%ld", _("(Connection OK)"),
                   (myaddr1>>24)&0xff, (myaddr1>>16)&0xff,
                   (myaddr1>>8)&0xff, (myaddr1)&0xff);
        label = gtk_label_new(addrstr);
    } else if (connection_ok)
        label = gtk_label_new(_("(Connection OK)"));
    else
        label = gtk_label_new(_("(Connection broken)"));
    gtk_grid_attach_next_to(GTK_GRID(hbox), label, address, GTK_POS_RIGHT, 1, 1);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
 		      hbox, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    g_signal_connect(G_OBJECT(dialog), "response",
                     G_CALLBACK(node_ip_address_callback), address);
}

void show_my_ip_addresses( GtkWidget *w,
                           gpointer   data )
{
    gchar addrstr[40];
    gulong myaddr;
    GtkWidget *dialog, *vbox, *label;
    gint i;

    my_ip_address = get_my_address();

    dialog = gtk_dialog_new_with_buttons (_("Own IP addresses"),
                                          NULL,
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);
#if (GTKVER == 3)
    vbox = gtk_grid_new();
#else
    vbox = gtk_vbox_new(FALSE, 5);
#endif
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    for (i = 0; i < addrcnt; i++) {
        myaddr = addrs[i];
        if (myaddr == ntohl(my_ip_address))
            sprintf(addrstr, "<b>%ld.%ld.%ld.%ld</b>",
                    (myaddr>>24)&0xff, (myaddr>>16)&0xff,
                    (myaddr>>8)&0xff, (myaddr)&0xff);
        else
            sprintf(addrstr, "%ld.%ld.%ld.%ld",
                    (myaddr>>24)&0xff, (myaddr>>16)&0xff,
                    (myaddr>>8)&0xff, (myaddr)&0xff);

        label = gtk_label_new(addrstr);
        g_object_set(label, "use-markup", TRUE, NULL);
#if (GTKVER == 3)
        gtk_grid_attach_next_to(GTK_GRID(vbox), label, NULL, GTK_POS_BOTTOM, 1, 1);
#else
        gtk_box_pack_start_defaults(GTK_BOX(vbox), label);
#endif
    }

#if (GTKVER == 3)
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
 		      vbox, TRUE, TRUE, 0);
#else
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox);
#endif
    gtk_widget_show_all(dialog);

    g_signal_connect(G_OBJECT(dialog), "response",
                     G_CALLBACK(gtk_widget_destroy), NULL);
}

extern void msg_received(struct message *input_msg);
extern gint timeout_callback(gpointer data);

volatile gint msg_queue_put = 0, msg_queue_get = 0;
struct message msg_to_send[MSG_QUEUE_LEN];

static volatile gint rec_msg_queue_put = 0, rec_msg_queue_get = 0;
static struct message rec_msgs[MSG_QUEUE_LEN];

static gboolean already_in_rec_queue(volatile struct message *msg)
{
    if (msg->type != MSG_RESULT)
        return FALSE;

    gint i = rec_msg_queue_get;

    while (i != rec_msg_queue_put) {
        if (memcmp((struct message *)&msg->u.result, &rec_msgs[i].u.result, sizeof(msg->u.result)) == 0) {
            mylog("resend: tatami=%d cat=%d num=%d\n",
                    msg->u.result.tatami, msg->u.result.category, msg->u.result.match);
            return TRUE;
        }

        i++;
        if (i >= MSG_QUEUE_LEN)
            i = 0;
    }
    return FALSE;
}

void msg_to_queue(struct message *msg)
{
#if (GTKVER == 3)
    G_LOCK(send_mutex);
#else
    g_static_mutex_lock (&send_mutex);
#endif
    msg_to_send[msg_queue_put] = *msg;
    msg_queue_put++;
    if (msg_queue_put >= MSG_QUEUE_LEN)
        msg_queue_put = 0;
#if (GTKVER == 3)
    G_UNLOCK(send_mutex);
#else
    g_static_mutex_unlock (&send_mutex);
#endif

    if (msg->type > 0 && msg->type < NUM_MESSAGES)
        msg_stat.txtype[(gint)msg->type]++;
    msg_stat.tx++;
}

struct message *get_rec_msg(void)
{
    struct message *msg;

#if (GTKVER == 3)
    G_LOCK(rec_mutex);
#else
    g_static_mutex_lock(&rec_mutex);
#endif
    if (rec_msg_queue_put == rec_msg_queue_get) {
#if (GTKVER == 3)
        G_UNLOCK(rec_mutex);
#else
        g_static_mutex_unlock(&rec_mutex);
#endif
        return NULL;
    }

    msg = &rec_msgs[rec_msg_queue_get];
    rec_msg_queue_get++;
    if (rec_msg_queue_get >= MSG_QUEUE_LEN)
        rec_msg_queue_get = 0;

#if (GTKVER == 3)
    G_UNLOCK(rec_mutex);
#else
    g_static_mutex_unlock(&rec_mutex);
#endif

    return msg;
}

gboolean check_for_input(gpointer data)
{
    struct message *msg = get_rec_msg();

    if (msg) {
        if (msg->type > 0 && msg->type < NUM_MESSAGES)
            msg_stat.rxtype[(gint)msg->type]++;
        msg_stat.rx++;

        msg_received(msg);
        msg->type = 0;
    }

    return TRUE;
}

extern void check_for_update(gulong addr, gchar *app);
extern gchar *program_path;

static gint conn_info_callback(gpointer data)
{
    static gboolean last_ok = FALSE;
    static gboolean update_checked = FALSE;

    if (connection_ok == last_ok)
        return TRUE;

    last_ok = connection_ok;

    if (connection_ok && !update_checked) {
	gulong addr = node_ip_addr;
	if (addr == 0 && this_is_shiai() == FALSE)
            addr = ssdp_ip_addr;
	if (addr) {
	    if (!this_is_shiai())
		check_for_update(addr, program_path);
	    update_checked = TRUE;
	}
    }

    return TRUE;
}

void open_comm_socket(void)
{
    g_mutex_init(&G_LOCK_NAME(send_mutex));

#ifdef WIN32
    WSADATA WSAData = {0};

    if (0 != WSAStartup(MAKEWORD(2, 2), &WSAData)) {
        printf("-FATAL- | Unable to initialize Winsock version 2.2\n");
        return;
    }
#endif
    my_ip_address = get_my_address();
    mylog("My address = %ld.%ld.%ld.%ld\n",
            (my_ip_address)&0xff,
            (my_ip_address>>8)&0xff,
            (my_ip_address>>16)&0xff,
            (my_ip_address>>24)&0xff);

    g_timeout_add(4000, timeout_callback, NULL);
    g_timeout_add(500, conn_info_callback, NULL);
    g_timeout_add(20, check_for_input, NULL);
//        g_idle_add(check_for_input, NULL);

    msg_stat.start = time(NULL);
}

struct message *put_to_rec_queue(volatile struct message *m)
{
    struct message *ret;

    if (msg_accepted((struct message *)m) == FALSE)
        return NULL;

#if (GTKVER == 3)
    G_LOCK(rec_mutex);
#else
    g_static_mutex_lock(&rec_mutex);
#endif
    if (already_in_rec_queue(m)) {
#if (GTKVER == 3)
        G_UNLOCK(rec_mutex);
#else
        g_static_mutex_unlock(&rec_mutex);
#endif
        return NULL;
    }

    rec_msgs[rec_msg_queue_put] = *m;
    ret = &rec_msgs[rec_msg_queue_put];
    rec_msg_queue_put++;
    if (rec_msg_queue_put >= MSG_QUEUE_LEN)
        rec_msg_queue_put = 0;
#if (GTKVER == 3)
    G_UNLOCK(rec_mutex);
#else
    g_static_mutex_unlock(&rec_mutex);
#endif
    return ret;
}

gint send_msg(gint fd, struct message *msg, gint mcastport)
{
    gint i = 0, err = 0, k = 0, len;
    guchar out[2048], buf[2048];

    if (mcastport) {
	struct sockaddr_in mcast_out;

	memset((gchar*)&mcast_out, 0, sizeof(mcast_out));
	mcast_out.sin_family = AF_INET;
	mcast_out.sin_port = htons(mcastport);
	mcast_out.sin_addr.s_addr = htonl(MULTICAST_ADDR);

	if (sendto(fd, (void *)msg, sizeof(*msg), 0,
		   (struct sockaddr *)(&mcast_out), sizeof(mcast_out)) < 0)
	    perror("multicast send");
	return sizeof(*msg);
    }

    len = encode_msg(msg, buf, sizeof(buf));
    if (len < 0)
	return -1;

    out[k++] = COMM_ESCAPE;
    out[k++] = COMM_BEGIN;

    for (i = 0; i < len; i++) {
        guchar c = buf[i];

        if (k > sizeof(out) - 4)
            return -1;

        if (c == COMM_ESCAPE) {
            out[k++] = COMM_ESCAPE;
            out[k++] = COMM_FF;
        } else {
            out[k++] = c;
        }
    }

    out[k++] = COMM_ESCAPE;
    out[k++] = COMM_END;

    if ((err = send(fd, (char *)out, k, 0)) != k) {
        mylog("send_msg error: sent %d/%d octets\n", err, k);
        return -1;
    }

    return len;
}

void print_stat(void)
{
    gint i;

    msg_stat.stop = time(NULL);
    guint elapsed = msg_stat.stop - msg_stat.start;
    if (elapsed == 0)
        elapsed = 1;

    mylog("Messages in %d s: rx: %d (%d/s) tx: %d (%d/s)\n",
            elapsed, msg_stat.rx, msg_stat.rx/elapsed,
            msg_stat.tx, msg_stat.tx/elapsed);

    for (i = 1; i < NUM_MESSAGES; i++) {
        mylog("  %2d: rx: %d (%d/s) tx: %d (%d/s)\n", i,
                msg_stat.rxtype[i], msg_stat.rxtype[i]/elapsed,
                msg_stat.txtype[i], msg_stat.txtype[i]/elapsed);
    }
}

gpointer client_thread(gpointer args)
{
    SOCKET comm_fd;
    gint n;
    struct sockaddr_in node;
    struct message input_msg;
    static guchar inbuf[2048];
    fd_set read_fd, fds;
    gint old_port = get_port();
    struct message msg_out;
    gboolean msg_out_ready, multicast;

    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
        if (node_ip_addr == 0 && this_is_shiai() == FALSE) {
            tmp_node_addr = ssdp_ip_addr;//nodescan(my_ip_address);
            if (tmp_node_addr == 0) {
                g_usleep(1000000);
                continue;
            }
            //node_ip_addr = tmp_node_addr;
        } else {
            tmp_node_addr = node_ip_addr;
        }

	if (ntohl(tmp_node_addr) == MULTICAST_ADDR)
	    multicast = TRUE;
	else
	    multicast = FALSE;
	
	if (multicast) {
	    if ((comm_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
		perror("client udp socket");
		mylog("CANNOT CREATE SOCKET (%s:%d)!\n", __FUNCTION__, __LINE__);
		g_thread_exit(NULL);    /* not required just good pratice */
		return NULL;
	    }
	} else {
	    if ((comm_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		perror("client tcp socket");
		mylog("CANNOT CREATE SOCKET (%s:%d)!\n", __FUNCTION__, __LINE__);
		g_thread_exit(NULL);    /* not required just good pratice */
		return NULL;
	    }
	}

        memset(&node, 0, sizeof(node));
        node.sin_family      = AF_INET;
        node.sin_port        = htons(get_port());
        node.sin_addr.s_addr = tmp_node_addr;

	if (multicast) {
	    struct ip_mreq mreq;
	    gint reuse = 1;

	    if (setsockopt(comm_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
		perror("setsockopt (SO_REUSEADDR)");
	    }

	    node.sin_port        = htons(MULTICAST_PORT_FROM_JS);
	    node.sin_addr.s_addr = 0;
	    if (bind(comm_fd, (struct sockaddr *)&node, sizeof(node)) == -1) {
		perror("Multicast bind");
		closesocket(comm_fd);
		g_usleep(1000000);
		continue;
	    }

	    memset(&mreq, 0, sizeof(mreq));
	    mreq.imr_multiaddr.s_addr = htonl(MULTICAST_ADDR);
	    mreq.imr_interface.s_addr = 0;
	    if (setsockopt(comm_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq)) == -1) {
		perror("Multicast setsockopt");
		closesocket(comm_fd);
		g_usleep(1000000);
		continue;
	    }
	} else {
	    if (connect(comm_fd, (struct sockaddr *)&node, sizeof(node))) {
		closesocket(comm_fd);
		g_usleep(1000000);
		continue;
	    }
	}

        mylog("Connection OK.\n");

	/* send something to keep the connection open */
	input_msg.type = MSG_DUMMY;
	input_msg.sender = my_address;
	input_msg.u.dummy.application_type = application_type();
	send_msg(comm_fd, &input_msg, multicast ? MULTICAST_PORT_TO_JS : 0);

#if (APP_NUM == APPLICATION_TYPE_INFO)	
	struct message msg;
	msg.type = MSG_ALL_REQ;
	msg.sender = my_address;
	send_packet(&msg);
	mylog("JudoInfo connection ok: requesting all\n");
#endif
	
        connection_ok = TRUE;

        FD_ZERO(&read_fd);
        FD_SET(comm_fd, &read_fd);

        while ((tmp_node_addr == ssdp_ip_addr && node_ip_addr == 0) ||
               (tmp_node_addr == node_ip_addr)) {
            struct timeval timeout;
            gint r;

            if (old_port != get_port()) {
                mylog("oldport=%d newport=%d\n", old_port, get_port());
                old_port = get_port();
                ssdp_ip_addr = 0;
                break;
            }

            fds = read_fd;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;

            r = select(comm_fd+1, &fds, NULL, NULL, &timeout);

            // Mutex may be locked for a long time if there are network problems
            // during send. Thus we send a copy to unlock the mutex immediatelly.
            msg_out_ready = FALSE;
            G_LOCK(send_mutex);
            if (msg_queue_get != msg_queue_put) {
                msg_out = msg_to_send[msg_queue_get];
                msg_queue_get++;
                if (msg_queue_get >= MSG_QUEUE_LEN)
                    msg_queue_get = 0;
                msg_out_ready = TRUE;
            }
            G_UNLOCK(send_mutex);

            if (msg_out_ready)
                send_msg(comm_fd, &msg_out, multicast ? MULTICAST_PORT_TO_JS : 0);

            if (r <= 0)
                continue;

            if (FD_ISSET(comm_fd, &fds)) {
                struct message m;
                static gint ri = 0;
                static gboolean escape = FALSE;
                static guchar p[2048];

		if (multicast) {
		    if ((n = recv(comm_fd, (void *)&m, sizeof(m), 0)) > 0) {
			if (msg_accepted(&m)) {
			    G_LOCK(rec_mutex);
			    rec_msgs[rec_msg_queue_put] = m;
			    rec_msg_queue_put++;
			    if (rec_msg_queue_put >= MSG_QUEUE_LEN)
				rec_msg_queue_put = 0;
			    G_UNLOCK(rec_mutex);
			}
		    }
		} else {
		    if ((n = recv(comm_fd, (char *)inbuf, sizeof(inbuf), 0)) > 0) {
			gint i;
			for (i = 0; i < n; i++) {
			    guchar c = inbuf[i];
			    if (c == COMM_ESCAPE) {
				escape = TRUE;
			    } else if (escape) {
				if (c == COMM_FF) {
				    if (ri < sizeof(p))
					p[ri++] = COMM_ESCAPE;
				} else if (c == COMM_BEGIN) {
				    ri = 0;
				    memset(p, 0, sizeof(p));
				} else if (c == COMM_END) {
				    decode_msg(&m, p, ri);
				    if (msg_accepted(&m)) {
					G_LOCK(rec_mutex);
					rec_msgs[rec_msg_queue_put] = m;
					rec_msg_queue_put++;
					if (rec_msg_queue_put >= MSG_QUEUE_LEN)
					    rec_msg_queue_put = 0;
					G_UNLOCK(rec_mutex);
				    }
				} else {
				    mylog("Wrong char 0x%02x after esc!\n", c);
				}
				escape = FALSE;
			    } else if (ri < sizeof(p)) {
				p[ri++] = c;
			    }
			} // for
		    } else
			break;
		}
            }
        } // while

        ssdp_ip_addr = 0;
        connection_ok = FALSE;

        closesocket(comm_fd);

        mylog("Connection NOK.\n");
    }

    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}

#define SSDP_MULTICAST      "239.255.255.250"
#define SSDP_PORT           1900
#define URN ":urn:judoshiai:service:all:" SHIAI_VERSION
#define ST "ST" URN
#define NT "NT" URN

static gchar *ssdp_req_data = NULL;
static gint   ssdp_req_data_len = 0;
gchar ssdp_id[64];
gboolean ssdp_notify = TRUE;

static void analyze_ssdp(gchar *rec, struct sockaddr_in *client)
{
#if (APP_NUM == APPLICATION_TYPE_PROXY)
    extern void report_to_proxy(gchar *rec, struct sockaddr_in *client);
    report_to_proxy(rec, client);
    return;
#endif

#if (APP_NUM == APPLICATION_TYPE_SHIAI)
    extern void add_client_ssdp_info(gchar *rec, struct sockaddr_in *client);
    add_client_ssdp_info(rec, client);
    return;
#endif

    if (connection_ok)
        return;

    gchar *p1, *p = strstr(rec, "UPnP/1.0 Judo");

    if (!p)
        return;

    p1 = strchr(p, '\r');
    if (!p1)
        return;

    *p1 = 0;

#if (APP_NUM == APPLICATION_TYPE_INFO) || (APP_NUM == APPLICATION_TYPE_WEIGHT) || (APP_NUM == APPLICATION_TYPE_JUDOGI)
    if (strncmp(p+9, "JudoShiai", 9) == 0) {
        ssdp_ip_addr = client->sin_addr.s_addr;
        //mylog("SSDP %s: judoshiai addr=%lx\n", APPLICATION, ssdp_ip_addr);
    }
#elif (APP_NUM == APPLICATION_TYPE_TIMER)
#define MODE_SLAVE  2
    extern gint tatami, mode;

    if (mode == MODE_SLAVE) {
        if (strncmp(p+9, "JudoTimer", 9) == 0 && strstr(p+18, "master")) {
            p1 = strstr(p+18, "tatami=");
            if (p1) {
                gint t = atoi(p1+7);
                if (t == tatami) {
                    ssdp_ip_addr = client->sin_addr.s_addr;
                    //mylog("SSDP %s: master timer addr=%lx\n", APPLICATION, ssdp_ip_addr);
                }
            }
        }
    } else if (strncmp(p+9, "JudoShiai", 9) == 0) {
        ssdp_ip_addr = client->sin_addr.s_addr;
        //mylog("SSDP %s: judoshiai addr=%lx\n", APPLICATION, ssdp_ip_addr);
    }
#endif
}

gpointer ssdp_thread(gpointer args)
{
    SOCKET sock_in, sock_out;
#if (APP_NUM == APPLICATION_TYPE_SHIAI)
    SOCKET sock_mdns;
    struct sockaddr_in mdns_local, mdns_remote;
#endif
    gint ret;
    struct sockaddr_in name_out, name_in;
    struct sockaddr_in clientsock;
    static gchar inbuf[2048];
    fd_set read_fd, fds;
    socklen_t socklen;
    struct ip_mreq mreq;
    gint reuse = 1;
    gint ttl = 3;

#ifdef WIN32
    const gchar *os = "Windows";
#else
    const gchar *os = "Linux";
#endif

    ssdp_req_data = g_strdup_printf("M-SEARCH * HTTP/1.1\r\n"
                                    "HOST: 239.255.255.250:1900\r\n"
                                    "MAN: \"ssdp:discover\"\r\n"
                                    ST "\r\n"
                                    "MX:3\r\n"
                                    "\r\n");
    ssdp_req_data_len = strlen(ssdp_req_data);

    // receiving socket

    if ((sock_in = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        perror("SSDP socket");
        goto out;
    }

    if (setsockopt(sock_in, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt (SO_REUSEADDR)");
    }

    memset((gchar *)&name_in, 0, sizeof(name_in));
    name_in.sin_family = AF_INET;
    name_in.sin_port = htons(SSDP_PORT);
    name_in.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock_in, (struct sockaddr *)&name_in, sizeof(name_in)) == -1) {
        perror("SSDP bind");
        goto out;
    }

    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(SSDP_MULTICAST);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock_in, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq)) == -1) {
        perror("SSDP setsockopt");
    }

#if (APP_NUM == APPLICATION_TYPE_SHIAI)
    // mDNS

    if ((sock_mdns = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        perror("mDNS socket");
        goto out;
    }

    if (setsockopt(sock_mdns, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt (SO_REUSEADDR)");
    }

    memset((gchar *)&mdns_local, 0, sizeof(mdns_local));
    mdns_local.sin_family = AF_INET;
    mdns_local.sin_port = htons(5353);
    mdns_local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock_mdns, (struct sockaddr *)&mdns_local, sizeof(mdns_local)) == -1) {
        perror("mDNS bind");
        goto out;
    }

    memset((gchar *)&mdns_remote, 0, sizeof(mdns_remote));
    mdns_remote.sin_family = AF_INET;
    mdns_remote.sin_port = htons(5353);
    mdns_remote.sin_addr.s_addr = inet_addr("224.0.0.251");
    
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock_mdns, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq)) == -1) {
        perror("mDNS setsockopt");
    }
#endif
    
    // transmitting socket

    if ((sock_out = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        perror("SSDP socket");
        goto out;
    }

    if (setsockopt(sock_out, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl, sizeof(ttl)) < 0)
        perror("ERROR: sockopt IP_MULTICAST_TTL\n");

    memset((gchar*)&name_out, 0, sizeof(name_out));
    name_out.sin_family = AF_INET;
    name_out.sin_port = htons(SSDP_PORT);
    name_out.sin_addr.s_addr = inet_addr(SSDP_MULTICAST);

#if 0
    ret = sendto(sock_out, ssdp_req_data, ssdp_req_data_len, 0, (struct sockaddr*) &name_out,
                 sizeof(struct sockaddr_in));
    if (ret != ssdp_req_data_len) {
        perror("SSDP send req");
        goto out;
    }
#endif

    FD_ZERO(&fds);
    FD_SET(sock_in, &fds);
    FD_SET(sock_out, &fds);
#if (APP_NUM == APPLICATION_TYPE_SHIAI)
    FD_SET(sock_mdns, &fds);
#endif
    
    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
	struct timeval timeout;
        gint len;

        timeout.tv_sec=3;
        timeout.tv_usec=0;
        read_fd = fds;

        if (select(sock_out+2, &read_fd, NULL, NULL, &timeout) < 0) {
            perror("SSDP select");
            continue;
        }

        if (FD_ISSET(sock_out, &read_fd)) {
            socklen = sizeof(clientsock);
            if ((len = recvfrom(sock_out, inbuf, sizeof(inbuf)-1, 0,
                                (struct sockaddr *)&clientsock, &socklen)) == (size_t)-1) {
                perror("SSDP recvfrom");
                goto out;
            }

            if (len > 0) {
                inbuf[len]='\0';

                if (strncmp(inbuf, "HTTP/1.1 200 OK", 12) == 0)
                    analyze_ssdp(inbuf, &clientsock);
            } // len > 0
        }

        if (FD_ISSET(sock_in, &read_fd)) {
            socklen = sizeof(clientsock);
            if ((len = recvfrom(sock_in, inbuf, sizeof(inbuf)-1, 0,
                                (struct sockaddr *)&clientsock, &socklen)) == (size_t)-1) {
                perror("SSDP recvfrom");
                goto out;
            }

            if (len > 0) {
                inbuf[len]='\0';

                if (strncmp(inbuf, "NOTIFY", 6) == 0)
                    analyze_ssdp(inbuf, &clientsock);
                else if (strncmp(inbuf, "M-SEARCH", 8) == 0 &&
                         strstr(inbuf, "ST:urn:judoshiai:service:all:")) {
                    //mylog("SSDP %s rec: '%s'\n\n", APPLICATION, inbuf);
                    struct sockaddr_in addr;
                    addr.sin_addr.s_addr = my_ip_address;

                    gchar *resp = g_strdup_printf("HTTP/1.1 200 OK\r\n"
                                                  "DATE:Mon, 25 Feb 2013 12:22:23 GMT\r\n"
                                                  "CACHE-CONTROL: max-age=1800\r\n"
                                                  "EXT:\r\n"
                                                  "LOCATION: http://%s/UPnP/desc.xml\r\n"
                                                  "SERVER:%s/1 UPnP/1.0 %s/%s\r\n"
                                                  ST "\r\n"
                                                  "USN: uuid:%08x-cafe-babe-737%d-%012x\r\n"
                                                  "\r\n",
                                                  inet_ntoa(addr.sin_addr), os,
                                                  ssdp_id,
                                                  SHIAI_VERSION, my_address,
                                                  application_type(), (int)(htonl(my_ip_address)));

                    //mylog("SSDP %s send: '%s'\n", APPLICATION, resp);
                    ret = sendto(sock_out, resp, strlen(resp), 0, (struct sockaddr *)&clientsock, socklen);
                    (void)ret; // make compiler happy
                    g_free(resp);
                }
            } // len > 0
        }

#if (APP_NUM == APPLICATION_TYPE_SHIAI)
        if (FD_ISSET(sock_mdns, &read_fd)) {
            socklen = sizeof(clientsock);
            if ((len = recvfrom(sock_mdns, inbuf, sizeof(inbuf)-1, 0,
                                (struct sockaddr *)&clientsock, &socklen)) == (size_t)-1) {
                perror("mDNS recvfrom");
                goto out;
            }

            if (len >= 12) {
#define GET_SHORT(_i) ((inbuf[_i]<<8) | inbuf[_i+1])
#define SET_SHORT(_i, _v) inbuf[_i] = (_v >> 8); inbuf[_i+1] = (_v & 0xff)
                //guint16 id = GET_SHORT(0);
                guint16 flags = GET_SHORT(2);
                guint16 questions = GET_SHORT(4);
                //guint16 answer = GET_SHORT(6);
                //guint16 auth = GET_SHORT(8);
                //guint16 addit = GET_SHORT(10);

                if (questions == 1 && (flags & 0xf000) == 0x0000) {
                    guchar *names = NULL;
                    gint n = 0, k = 12;
                    guchar strlen = inbuf[k++];
                    while (strlen && k < len) {
                        if (n == 0)
                            names = (guchar *)&inbuf[k];
                        n++;
                        k += strlen;
                        strlen = inbuf[k];
                        k++;
                    }

                    guint16 type = GET_SHORT(k); 
                    guint16 klass = GET_SHORT(k + 2);
                    if (type == 0x0001 && klass == 0x0001 && n > 0 &&
                        names && strncmp((char *)names, "judoshiai", 9) == 0) {
                        SET_SHORT(2, 0x8400); // flags
                        SET_SHORT(4, 0x0000); // questions
                        SET_SHORT(6, 0x0001); // answers
                        SET_SHORT(8, 0x0000); // auth
                        SET_SHORT(10, 0x0000); // addit
                        SET_SHORT(k + 2, 0x8001); // class
                        SET_SHORT(k + 4, 0x0000); // time to live msb
                        SET_SHORT(k + 6, 0x0078); // time to live lsb, 2 min
                        SET_SHORT(k + 8, 0x0004); // data length
                        inbuf[k + 13] = (my_ip_address >> 24) & 0xff;
                        inbuf[k + 12] = (my_ip_address >> 16) & 0xff;
                        inbuf[k + 11] = (my_ip_address >> 8) & 0xff;
                        inbuf[k + 10] = (my_ip_address >> 0) & 0xff;
                        
                        ret = sendto(sock_mdns, inbuf, len + 10, 0,
                                     (struct sockaddr *)&mdns_remote, sizeof(mdns_remote));
                    }
                }
            } // len > 0
        }
#endif

        time_t now = time(NULL);
        static time_t last_notify = 0;

        if (ssdp_notify || now > last_notify + 10) {
            static gchar *resp = NULL;
            static gint   resplen = 0;

            if (ssdp_notify) {
                g_free(resp);
                resp = NULL;
            }

            if (resp == NULL) {
                struct sockaddr_in addr;
                addr.sin_addr.s_addr = my_ip_address;

                resp = g_strdup_printf("NOTIFY * HTTP/1.1\r\n"
                                       "HOST: 239.255.255.250:1900\r\n"
                                       "CACHE-CONTROL: max-age=1800\r\n"
                                       "LOCATION: http://%s/UPnP/desc.xml\r\n"
                                       NT "\r\n"
                                       "NTS: ssdp-alive\r\n"
                                       "SERVER:%s/1 UPnP/1.0 %s/%s\r\n"
                                       "USN: uuid:%08x-cafe-babe-737%d-%012x\r\n"
                                       "\r\n",
                                       inet_ntoa(addr.sin_addr), os,
                                       ssdp_id,
                                       SHIAI_VERSION, my_address,
                                       application_type(), (int)(htonl(my_ip_address)));
                resplen = strlen(resp);
            }

            //mylog("SSDP %s send: '%s'\n", APPLICATION, resp);
            ret = sendto(sock_out, resp, resplen, 0, (struct sockaddr *)&name_out, sizeof(struct sockaddr_in));
            ssdp_notify = FALSE;
            last_notify = now;
        }

#if (APP_NUM != APPLICATION_TYPE_SHIAI)
        static time_t last_time = 0;
        if (!connection_ok && now > last_time + 5) {
            last_time = now;
            ret = sendto(sock_out, ssdp_req_data, ssdp_req_data_len, 0, (struct sockaddr*) &name_out,
                         sizeof(struct sockaddr_in));
            //mylog("SSDP %s REQ SEND by timeout\n\n", APPLICATION);
            if (ret != ssdp_req_data_len) {
                perror("SSDP send req");
            }
        }
#endif
    }

 out:
    mylog("SSDP OUT!\n");
    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}

gchar *menu_text_with_dots(gchar *text)
{
    static gchar buf[64];
    snprintf(buf, sizeof(buf), "%s...", text);
    return buf;
}
