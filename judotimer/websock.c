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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <fcntl.h>
#include <signal.h>

#endif /* WIN32 */

#include <glib.h>
#include <glib/gprintf.h>

#include "sha.h"
#include "comm.h"
#include "httpp.h"
#include "cJSON.h"

#define g_print(_a...) do { } while (0)

/* System-dependent definitions */
#ifndef WIN32
#define closesocket     close
#define SOCKET          int
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define SOCKADDR_BTH    struct sockaddr_rc
#define AF_BTH          PF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM
#endif

#define WEBSOCK_STR "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

static void encodeblock( unsigned char *in, unsigned char *out, int len )
{
    out[0] = (unsigned char) cb64[ (int)(in[0] >> 2) ];
    out[1] = (unsigned char) cb64[ (int)(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ (int)(((in[1] & 0x0f) << 2) |
						    (len > 2 ? ((in[2] & 0xc0) >> 6) : 0)) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ (int)(in[2] & 0x3f) ] : '=');
}

#define OP_CODE_CONT_FRAME 0
#define OP_CODE_TEXT_FRAME 1
#define OP_CODE_BIN_FRAME  2
#define OP_CODE_CONN_CLOSE 8
#define OP_CODE_PING       9
#define OP_CODE_PONG      10

static void websock_message(struct jsconn *conn, unsigned char *p,
			    gint length, struct message *msg_out)
{
    SOCKET s = conn->fd;
    gint i;
    //gboolean fin = (p[0] & 0x80) != 0;
    gboolean masked = (p[1] & 0x80) != 0;
    gint opcode = p[0] & 0x0f;
    gint len = p[1] & 0x7f;
    gint data = 2;
    gint mask[4];
    gchar out[1024];

    if (len == 126) {
	len = (p[2] << 8) | p[3];
	data = 4;
    } else if (len == 127) {
	len = 0;
	for (i = 0; i < 8; i++)
	    len = (len << 8) | p[2+i];
	data = 10;
    }

    if (masked) {
	mask[0] = p[data];
	mask[1] = p[data+1];
	mask[2] = p[data+2];
	mask[3] = p[data+3];
	data += 4;
	for (i = 0; i < len; i++) {
	    p[data+i] ^= mask[i&3];
	}
    }

    //mylog("OP CODE %d\n", opcode);

    if (opcode == OP_CODE_PING) {
	out[0] = 0x80 | OP_CODE_PONG;
	out[1] = 0x00;
	send(s, out, 2, 0);
	return;
    }

    if (opcode == OP_CODE_CONN_CLOSE) {
	out[0] = 0x80 | OP_CODE_CONN_CLOSE;
	out[1] = 0x00;
	send(s, out, 2, 0);
	return;
    }

    if (opcode == OP_CODE_TEXT_FRAME) {
	struct message msg;
	cJSON *json;

	p[length] = 0;

	json = cJSON_Parse((char *)p + data);
	if (!json) {
	    mylog("json err: %s\n", p+data);
	    return;
	}

	int r = websock_decode_msg(&msg, json, 0);
	cJSON_Delete(json);
	if (r < 0) {
	    mylog("decode err: %s\n", p);
	    return;
	}

	if (msg.type == MSG_DUMMY) {
	    if (msg.u.dummy.application_type != conn->conn_type) {
		conn->conn_type = msg.u.dummy.application_type;
		conn->id = msg.sender;
		mylog("Node: conn=%d type=%d id=%08x\n",
			i, conn->conn_type, conn->id);
	    }
	} else if (msg_out) {
	    *msg_out = msg;
	} else {
	    put_to_rec_queue(&msg);
	}
#if 0
	mylog("Text len=%d:\n  ", len);
	for (i = 0; i < len; i++)
	    mylog("%c", p[data+i]);
	mylog("\n");
#endif
    }
}

static void websock_handshake(struct jsconn *conn, char *in, gint length)
{
    SOCKET s = conn->fd;
    gchar buf[1024];
    gint n;
    http_parser_t *parser;

    parser = httpp_create_parser();
    httpp_initialize(parser, NULL);

    if (httpp_parse(parser, in, length) == 0) {
	mylog("websock parse error\n");
	httpp_destroy(parser);
	return;
    }

    //const gchar *ug = httpp_getvar(parser, "upgrade");
    const gchar *key = httpp_getvar(parser, "sec-websocket-key");
    const gchar *pro = httpp_getvar(parser, "sec-websocket-protocol");

    if (key) {
	SHA_CTX context;
	sha1_byte digest[SHA1_DIGEST_LENGTH];
	guchar out[4];
	gint i;
	SHA1_Init(&context);

	memset(buf, 0, sizeof(buf));
	memset(digest, 0, sizeof(digest));
	strcpy(buf, key);
	strcat(buf, WEBSOCK_STR);
	SHA1_Update(&context, (guchar *)buf, strlen(buf));
	SHA1_Final(digest, &context);

	n = 0;
	n += snprintf(buf+n, sizeof(buf)-n, "HTTP/1.1 101 Switching Protocols\r\n");
	n += snprintf(buf+n, sizeof(buf)-n, "Upgrade: websocket\r\nConnection: Upgrade\r\n");
	if (pro)
	    n += snprintf(buf+n, sizeof(buf)-n,
			  "Sec-WebSocket-Protocol: %s\r\n", pro);
	n += snprintf(buf+n, sizeof(buf)-n, "Sec-WebSocket-Accept: ");

	for (i = 0; i < SHA1_DIGEST_LENGTH; i++)
		mylog(" %02x", digest[i]);
	mylog("\n");

	for (i = 0; i < SHA1_DIGEST_LENGTH; i += 3) {
	    encodeblock(&digest[i], out, SHA1_DIGEST_LENGTH - i);
	    n += snprintf(buf+n, sizeof(buf)-n, "%c%c%c%c", out[0], out[1], out[2], out[3]);
	}
	n += snprintf(buf+n, sizeof(buf)-n, "\r\n\r\n");

	mylog("KEY=%s REPLY %d bytes:\n%s\n", key, n, buf);
	send(s, buf, n, 0);
	conn->websock_ok = TRUE;

#if (APP_NUM == APPLICATION_TYPE_SERVER)
	extern void notify_conn(struct jsconn *conn);
	notify_conn(conn);
#endif
    }

    httpp_destroy(parser);
}

void handle_websock(struct jsconn *conn, char *in, gint length, struct message *msg)
{
    if (in[0] == 'G' && in[1] == 'E' && in[2] == 'T') {
	websock_handshake(conn, in, length);
    } else {
	websock_message(conn, (unsigned char *)in, length, msg);
    }
}

gint websock_send_msg(gint fd, struct message *msg)
{
    guchar buf[1024], *p = buf + 16;
    gint len = websock_encode_msg(msg, p, sizeof(buf)-16);

    //mylog("%s: %s to web\n", __FUNCTION__, msg_name(msg->type));

    if (len < 0) {
	return 0;
    } else if (len < 126) {
	p = buf + 14;
	p[0] = 0x80 | OP_CODE_TEXT_FRAME;
	p[1] = len;
	len += 2;
    } else {
	p = buf + 12;
	p[0] = 0x80 | OP_CODE_TEXT_FRAME;
	p[1] = 126;
	p[2] = len >> 8;
	p[3] = len & 0xff;
	len += 4;
    }

    send(fd, (gchar *)p, len, 0);
    return 0;
}
