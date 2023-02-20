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

    struct wsmsg amsg;
    int n;

    if (!g_vhd || !g_vhd->pss_list)
        return 0;

    int len = websock_encode_msg(msg, buf, sizeof(buf));
    //int len = sprintf(buf, "{\"msg\":[1, 2, 3]}");
    //printf("SEND WS MSG g_vhd=%p %s\n", g_vhd, buf);

    pthread_mutex_lock(&g_vhd->lock_ring);

    n = (int)lws_ring_get_count_free_elements(g_vhd->ring);
    if (!n) {
        lwsl_user("dropping 1!\n");
        goto wait_unlock;
    }

    amsg.payload = malloc(LWS_PRE + len);
    if (!amsg.payload) {
        lwsl_user("OOM: dropping 2\n");
        goto wait_unlock;
    }
    amsg.len = len;
    memcpy(amsg.payload + LWS_PRE, buf, len);
    
    n = lws_ring_insert(g_vhd->ring, &amsg, 1);
    if (n != 1) {
        __minimal_destroy_message(&amsg);
        lwsl_user("dropping 3!\n");
    } else
        /*
         * This will cause a LWS_CALLBACK_EVENT_WAIT_CANCELLED
         * in the lws service thread context.
         */
        lws_cancel_service(g_vhd->context);

wait_unlock:
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
    char buf[32];
    int m;

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
        /* remove our closing pss from the list of live pss */
        lws_ll_fwd_remove(struct per_session_data__minimal, pss_list,
                          pss, vhd->pss_list);
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        pthread_mutex_lock(&vhd->lock_ring);

        pmsg = lws_ring_get_element(vhd->ring, &pss->tail);
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

        lws_ring_consume_and_update_oldest_tail(
            vhd->ring,	/* lws_ring object */
            struct per_session_data__minimal, /* type of objects with tails */
            &pss->tail,	/* tail of guy doing the consuming */
            1,		/* number of payload objects being consumed */
            vhd->pss_list,	/* head of list of objects with tails */
            tail,		/* member name of tail in objects with tails */
            pss_list	/* member name of next object in objects with tails */
            );

        /* more to do? */
        if (lws_ring_get_element(vhd->ring, &pss->tail))
            /* come back as soon as we can write more */
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
        
	json = cJSON_Parse((char *)in);
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


