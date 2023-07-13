/*
 * lws-minimal-ws-broker
 *
 * Written in 2010-2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This demonstrates the most minimal http server you can make with lws,
 * with an added publish / broker / subscribe ws server.
 *
 * To keep it simple, it serves stuff in the subdirectory "./mount-origin" of
 * the directory it was started in.
 * You can change that by changing mount.origin.
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <glib.h>
#include <pthread.h>


#define LWS_PLUGIN_STATIC

#include <string.h>

#include "comm.h"
#include "cJSON.h"

extern gint webpwcrc32;
extern int pwcrc32(const unsigned char *str, int len);

#define QUEUE_LEN 256

/* one of these created for each message */

struct wsmsg {
    char *payload; /* is malloc'd */
    size_t len;
};

/* one of these is created for each client connecting to us */

struct per_session_data__minimal {
    struct per_session_data__minimal *pss_list;
    struct lws *wsi;
    uint32_t tail;
    char publishing; /* nonzero: peer is publishing to us */
    int type;
    int tatami;
    int passwdcrc;
    struct wsmsg queue[QUEUE_LEN];
    int qget, qput, qlen;
    char *indata_p;
    int indata_space;
    int position_tx, position_rx;
};

/* one of these is created for each vhost our protocol is used with */

struct per_vhost_data__minimal {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    struct per_session_data__minimal *pss_list; /* linked-list of live pss*/

    pthread_mutex_t lock_ring; /* serialize access to the ring buffer */
    struct lws_ring *ring; /* ringbuffer holding unsent messages */
    uint32_t tail;
};

static struct per_vhost_data__minimal *g_vhd = NULL;

static int qlenmax = 0;

static void put_data(struct per_session_data__minimal *pss,
                     void *data, int len)
{
    struct wsmsg *m = &pss->queue[pss->qput];
    m->payload = malloc(LWS_PRE + len);
    memcpy(m->payload + LWS_PRE, data, len);
    m->len = len;

    int old = pss->qput;
    pss->qput++;
    if (pss->qput >= QUEUE_LEN) pss->qput = 0;
    if (pss->qput == pss->qget) {
        g_print("QUEUE FULL!\n");
        pss->qput = old;
    }
    pss->qlen++;
    if (pss->qlen > qlenmax) {
        qlenmax = pss->qlen;
        g_print("pss=%p QLEN ws-comm = %d put=%d get=%d\n",
                pss, pss->qlen, pss->qput, pss->qget);
    }
}

static struct wsmsg *get_data(struct per_session_data__minimal *pss)
{
    if (pss->qget == pss->qput) return NULL;
    return &pss->queue[pss->qget];
}

static void rm_data(struct per_session_data__minimal *pss)
{
    if (pss->qget == pss->qput) return;
    struct wsmsg *m = &pss->queue[pss->qget];

    pss->qget++;
    if (pss->qget >= QUEUE_LEN) pss->qget = 0;

    free(m->payload);
    m->payload = NULL;
    pss->qlen--;
}

static void rm_queue(struct per_session_data__minimal *pss)
{
    int i;
    for (i = 0; i < QUEUE_LEN; i++) {
        if (pss->queue[i].payload) {
            free(pss->queue[i].payload);
            pss->queue[i].payload = NULL;
        }
    }
    pss->qput = pss->qget = 0;
}


static void
__minimal_destroy_message(void *_msg)
{
	struct wsmsg *msg = _msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
}

gint ws_send_msg(struct message *msg)
{
    guchar buf[2048];

    if (!g_vhd || !g_vhd->pss_list)
        return 0;

    int len = websock_encode_msg(msg, buf, sizeof(buf));
    //int len = sprintf(buf, "{\"msg\":[1, 2, 3]}");
    //printf("SEND WS MSG g_vhd=%p %s\n", g_vhd, buf);

    pthread_mutex_lock(&g_vhd->lock_ring);

    lws_start_foreach_llp(struct per_session_data__minimal **,
                          ppss, g_vhd->pss_list) {
        put_data(*ppss, buf, len);
        lws_callback_on_writable((*ppss)->wsi);
    } lws_end_foreach_llp(ppss, pss_list);

    pthread_mutex_unlock(&g_vhd->lock_ring);

    return 0;
}


static int
callback_minimal(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
    struct per_session_data__minimal *pss =
        (struct per_session_data__minimal *)user;
    struct per_vhost_data__minimal *vhd =
        (struct per_vhost_data__minimal *)
        lws_protocol_vh_priv_get(lws_get_vhost(wsi),
                                 lws_get_protocol(wsi));
    const struct wsmsg *pmsg;
    char buf[64];
    int m;
    char *data = NULL;

    //printf("WS CB reason=%d vhd=%p\n", reason, vhd);
    switch (reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
        vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
                                          lws_get_protocol(wsi),
                                          sizeof(struct per_vhost_data__minimal));
        pthread_mutex_init(&vhd->lock_ring, NULL);
        vhd->context = lws_get_context(wsi);
        vhd->protocol = lws_get_protocol(wsi);
        vhd->vhost = lws_get_vhost(wsi);

        vhd->ring = lws_ring_create(sizeof(struct wsmsg), 64,
                                    __minimal_destroy_message);
        if (!vhd->ring) {
            lwsl_err("%s: failed to create ring\n", __func__);
            return 1;
        }

        g_vhd = vhd;
        mylog("CREATED VHD=%p\n", vhd);

        if (!vhd->ring)
            return 1;
        break;

    case LWS_CALLBACK_PROTOCOL_DESTROY:
        if (vhd->ring)
            lws_ring_destroy(vhd->ring);
        pthread_mutex_destroy(&vhd->lock_ring);
        break;

    case LWS_CALLBACK_ESTABLISHED:
        pss->tail = lws_ring_get_oldest_tail(vhd->ring);
        pss->wsi = wsi;
        pss->passwdcrc = 0;

        memset(&pss->queue, 0, sizeof(pss->queue));
        pss->indata_space = 8*1024;
        pss->indata_p = malloc(pss->indata_space);
        pss->position_rx = 0;
        pss->wsi = wsi;
        pss->qlen = 0;

        if (lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_GET_URI) > 0) {
            //printf("BROKER URL = %s\n", buf);
            if (!strncmp(buf, "/tm", 3)) {
                pss->type = APPLICATION_TYPE_TIMER;
                pss->tatami = atoi(buf + 3);
                pss->publishing = TRUE;
            } else if (!strncmp(buf, "/ts", 3)) {
                pss->type = APPLICATION_TYPE_TIMER;
                pss->tatami = atoi(buf + 3);
                pss->publishing = FALSE;
            } else if (!strncmp(buf, "/info", 5)) {
                pss->type = APPLICATION_TYPE_INFO;
            } else if (!strncmp(buf, "/weight", 7)) {
                pss->type = APPLICATION_TYPE_WEIGHT;
            } else if (!strncmp(buf, "/judogi", 7)) {
                pss->type = APPLICATION_TYPE_JUDOGI;
            }
            char *p = strstr(buf, "_pw_");
            if (p)
                pss->passwdcrc = pwcrc32((unsigned char *)(p+4), strlen(p+4));
        }

        /* add ourselves to the list of live pss held in the vhd */
        lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
        break;

    case LWS_CALLBACK_CLOSED:
        free(pss->indata_p);
        rm_queue(pss);
        /* remove our closing pss from the list of live pss */
        lws_ll_fwd_remove(struct per_session_data__minimal, pss_list,
                          pss, vhd->pss_list);
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        pthread_mutex_lock(&vhd->lock_ring);
        pmsg = get_data(pss);
        if (!pmsg) {
            pthread_mutex_unlock(&vhd->lock_ring);
            break;
        }

        if (webpwcrc32 == 0 || webpwcrc32 == pss->passwdcrc) {
            /* notice we allowed for LWS_PRE in the payload already */
            m = lws_write(wsi, ((unsigned char *)pmsg->payload) + LWS_PRE,
                          pmsg->len, LWS_WRITE_TEXT);
            if (m < (int)pmsg->len) {
                pthread_mutex_unlock(&vhd->lock_ring);
                lwsl_err("ERROR %d writing to ws socket\n", m);
                return -1;
            }
        }

        rm_data(pss);
        if (pss->qget != pss->qput)
             lws_callback_on_writable(pss->wsi);

        pthread_mutex_unlock(&vhd->lock_ring);
        break;

    case LWS_CALLBACK_RECEIVE:
    {
        struct message msg;
	cJSON *json;

        if (webpwcrc32 != 0 && webpwcrc32 != pss->passwdcrc) {
	    mylogerr("password error\n");
	    break;
        }

        if (pss->indata_space <= pss->position_rx + len) {
            pss->indata_space = 2*pss->indata_space;
            pss->indata_p = realloc(pss->indata_p, pss->indata_space);
        }
        memcpy(pss->indata_p + pss->position_rx, in, len);
        pss->position_rx += len;

        if (!lws_is_final_fragment(wsi))
            break;

        len = pss->position_rx;
        pss->position_rx = 0;
        data = pss->indata_p;

	json = cJSON_Parse((char *)data);
	if (!json) {
	    mylogerr("json err: %s\n", (char *)in);
	    break;
	}

	int r = websock_decode_msg(&msg, json, webpwcrc32);
	cJSON_Delete(json);
	if (r < 0) {
	    mylogerr("decode err: %s\n", (char *)in);
	    break;
	}
        put_to_rec_queue(&msg);

        break;
    }

    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        if (!vhd)
            break;
        /*
         * When the "spam" threads add a message to the ringbuffer,
         * they create this event in the lws service thread context
         * using lws_cancel_service().
         *
         * We respond by scheduling a writable callback for all
         * connected clients.
         */
        lws_start_foreach_llp(struct per_session_data__minimal **,
                              ppss, vhd->pss_list) {
            lws_callback_on_writable((*ppss)->wsi);
        } lws_end_foreach_llp(ppss, pss_list);
        break;

    default:
        break;
    }

    return 0;
}

#define LWS_PLUGIN_PROTOCOL_MINIMAL \
	{ \
		"js", \
		callback_minimal, \
		sizeof(struct per_session_data__minimal), \
		2048, \
		0, NULL, 2048 \
	}

static struct lws_protocols protocols[] = {
	{ "http", lws_callback_http_dummy, 0, 0 },
	LWS_PLUGIN_PROTOCOL_MINIMAL,
	{ NULL, NULL, 0, 0 } /* terminator */
};

static int interrupted;

static const struct lws_http_mount mount = {
	/* .mount_next */		NULL,		/* linked-list "next" */
	/* .mountpoint */		"/",		/* mountpoint URL */
	/* .origin */			"./mount-origin", /* serve from dir */
	/* .def */			"index.html",	/* default filename */
	/* .protocol */			NULL,
	/* .cgienv */			NULL,
	/* .extra_mimetypes */		NULL,
	/* .interpret */		NULL,
	/* .cgi_timeout */		0,
	/* .cache_max_age */		0,
	/* .auth_mask */		0,
	/* .cache_reusable */		0,
	/* .cache_revalidate */		0,
	/* .cache_intermediaries */	0,
	/* .origin_protocol */		LWSMPRO_FILE,	/* files in a dir */
	/* .mountpoint_len */		1,		/* char count */
	/* .basic_auth_login_file */	NULL,
};

gpointer ws_comm_thread(gpointer args)
{
    extern gchar *installation_dir;
    struct lws_context_creation_info info;
    struct lws_context *context;
    int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
        /* for LLL_ verbosity above NOTICE to be built into lws,
         * lws must have been configured and built with
         * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
        /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
        /* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
        /* | LLL_DEBUG */;

    const char *cert_filepath = g_build_filename(installation_dir, "etc", "certificate.pem", NULL);
    const char *private_key_filepath = g_build_filename(installation_dir, "etc", "privkey.pem", NULL);

    //signal(SIGINT, sigint_handler);

    lws_set_log_level(logs, NULL);
    lwsl_user("LWS minimal ws comm | visit http://localhost:7681\n");

    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
    info.port = WS_COMM_PORT;
    info.mounts = &mount;
    info.protocols = protocols;
    info.options =
        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
        LWS_SERVER_OPTION_DISABLE_IPV6;

    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.ssl_cert_filepath = cert_filepath;
    info.ssl_private_key_filepath = private_key_filepath;

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return NULL;
    }

    while (n >= 0 && !interrupted) {
        n = lws_service(context, 0);
    }

    lws_context_destroy(context);

    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}


