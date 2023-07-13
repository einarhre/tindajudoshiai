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

#define QUEUE_LEN 256

#include <string.h>

#include "comm.h"
#include "unqlite.h"

#undef lwsl_user
#define lwsl_user g_print

int x_key_callback(
    unqlite_kv_cursor *pCursor,
    const void *pKey, int nKeyLen,
    void *pUserData
);


static unqlite *pDb;


/* one of these created for each message */

struct wsmsg {
    void *payload; /* is malloc'd */
    size_t len;
};

/* one of these is created for each client connecting to us */

struct per_session_data__minimal {
    struct per_session_data__minimal *pss_list;
    struct lws *wsi;
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

    struct lws_ring *ring; /* ringbuffer holding unsent messages */
    pthread_mutex_t lock_ring; /* serialize access to the ring buffer */

    struct wsmsg amsg; /* the one pending message... */
    int current; /* the current message number we are caching */

    int *interrupted;
    /*
     * b0 = 1: test compressible text, = 0: test uncompressible binary
     * b1 = 1: send as a single blob, = 0: send as fragments
     */
    int *options;
};

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
        g_print("pss=%p QLEN ws-unqlite = %d put=%d get=%d\n",
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

/* destroys the message when everyone has had a copy of it */

static void
__minimal_destroy_message(void *_msg)
{
    struct wsmsg *msg = _msg;

    free(msg->payload);
    msg->payload = NULL;
    msg->len = 0;
}

static void pri(char *p, int len) {
    int i;
    for (i = 0; i < len; i++) putchar(p[i]);
}

static void tell_others(struct per_vhost_data__minimal *vhd,
                        struct per_session_data__minimal *pss,
                        char *data, size_t len) {
    if (!vhd->pss_list)
        return;

    /*
     * let every subscriber know we want to write something
     * on them as soon as they are ready
     */

    pthread_mutex_lock(&vhd->lock_ring);

    lws_start_foreach_llp(struct per_session_data__minimal **,
                          ppss, vhd->pss_list) {
        if (pss != *ppss) {
            put_data(*ppss, data, len);
            lws_callback_on_writable((*ppss)->wsi);
        }
    } lws_end_foreach_llp(ppss, pss_list);

    pthread_mutex_unlock(&vhd->lock_ring);
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
    char *data = NULL;
    const struct wsmsg *pmsg;
    struct wsmsg amsg;
    char buf[64];
    int m;

    switch (reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
        vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
                                          lws_get_protocol(wsi),
                                          sizeof(struct per_vhost_data__minimal));
        vhd->context = lws_get_context(wsi);
        vhd->protocol = lws_get_protocol(wsi);
        vhd->vhost = lws_get_vhost(wsi);
        pthread_mutex_init(&vhd->lock_ring, NULL);
        break;

    case LWS_CALLBACK_PROTOCOL_DESTROY:
        pthread_mutex_destroy(&vhd->lock_ring);
        break;

    case LWS_CALLBACK_ESTABLISHED:
        g_print("Establisged pss=%p\n", pss);
        memset(&pss->queue, 0, sizeof(pss->queue));
        pss->indata_space = 8*1024;
        pss->indata_p = malloc(pss->indata_space);
        pss->position_rx = 0;
        pss->wsi = wsi;
        pss->qlen = 0;

        lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
        break;

    case LWS_CALLBACK_CLOSED:
        g_print("Closed %p\n", pss);
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

        /* notice we allowed for LWS_PRE in the payload already */
        m = lws_write(wsi, ((unsigned char *)pmsg->payload) + LWS_PRE,
                      pmsg->len, LWS_WRITE_TEXT);
        if (m < (int)pmsg->len) {
            lwsl_err("ERROR %d writing to ws socket\n", m);
            pthread_mutex_unlock(&vhd->lock_ring);
            return -1;
        }

        rm_data(pss);
        if (pss->qget != pss->qput)
             lws_callback_on_writable(pss->wsi);

        pthread_mutex_unlock(&vhd->lock_ring);
        break;

    case LWS_CALLBACK_RECEIVE:
        g_print("LWS_CALLBACK_RECEIVE: %4d (pss->pos=%d, rpp %5d, last %d) qlenmax=%d\n",
                (int)len, (int)pss->position_rx, (int)lws_remaining_packet_payload(wsi),
                lws_is_final_fragment(wsi), qlenmax);
        pri(in, len); printf("\n\n");
        
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

        if (data[0] == 'P') {
            int val = 0;
            int i;
            for (i = 1; i < len; i++) {
                if (data[i] == '=') {
                    val = i;
                    break;
                }
            }
            if (val == 0) return -1;

            //printf("SAVING "); pri(data+1, val-1); printf(" "); pri(data + val + 1, len - val - 1); printf("\n");
            int rc = unqlite_kv_store(pDb, data+1, val-1, data + val + 1, len - val - 1);
            if (rc != UNQLITE_OK){
                  const char *zBuf;
                  int iLen;
                  g_print("PUT ERROR\n");
                  /* Something goes wrong, extract database error log */
                  unqlite_config(pDb, UNQLITE_CONFIG_ERR_LOG, &zBuf, &iLen);
                  if (iLen > 0) {
                      puts(zBuf);
                  }
                  if (rc != UNQLITE_BUSY && rc != UNQLITE_NOTIMPLEMENTED) {
                      /* Rollback */
                      unqlite_rollback(pDb);
                  }
            } else {
                unqlite_commit(pDb);
                tell_others(vhd, pss, data, len);
            }
        } else if (data[0] == 'G') {
            unqlite_int64 nBytes;  //Data length
            char *zBuf;     //Dynamically allocated buffer

            //Extract data size first
            int rc = unqlite_kv_fetch(pDb, data+1, len - 1, NULL, &nBytes);
            if (rc != UNQLITE_OK) {
                nBytes = 0;
            }

            //Allocate a buffer big enough to hold the record content
            zBuf = (char *)malloc(nBytes + LWS_SEND_BUFFER_PRE_PADDING + len + 1);
            if (zBuf == NULL) return -1;
            //Copy record content in our buffer
            if (nBytes > 0)
                unqlite_kv_fetch(pDb, data+1, len - 1, &zBuf[LWS_SEND_BUFFER_PRE_PADDING + len + 1], &nBytes);
            memcpy(&zBuf[LWS_SEND_BUFFER_PRE_PADDING], data, len);
            zBuf[LWS_SEND_BUFFER_PRE_PADDING + len] = '=';
            lws_write(wsi, (unsigned char *)(&zBuf[LWS_SEND_BUFFER_PRE_PADDING]),
                      nBytes + len + 1, LWS_WRITE_TEXT);
            free(zBuf);
        } else if (data[0] == 'D') {
            int rc = unqlite_kv_delete(pDb, data+1, len - 1);
            tell_others(vhd, pss, in, len);
        } else if (data[0] == 'X') {
            int keylen;
            unqlite_kv_cursor *pCursor;
            int rc = unqlite_kv_cursor_init(pDb, &pCursor);
            if( rc != UNQLITE_OK || len < 2)
                return;

            for( unqlite_kv_cursor_first_entry(pCursor) ;
                 unqlite_kv_cursor_valid_entry(pCursor) ;
                 unqlite_kv_cursor_next_entry(pCursor) ){
                keylen = sizeof(buf) - 1;
                unqlite_kv_cursor_key(pCursor, buf+1, &keylen);

                if (memcmp(buf + 1, in + 1, len - 1) == 0) {
                    rc = unqlite_kv_delete(pDb, buf+1, keylen);
                    buf[0] = 'D';
                    tell_others(vhd, pss, buf, keylen+1);
                }
            }

            unqlite_kv_cursor_release(pDb, pCursor);
        } else if (data[0] == 'L') {
            int keylen;
            unqlite_kv_cursor *pCursor;
            int rc = unqlite_kv_cursor_init(pDb, &pCursor);
            if( rc != UNQLITE_OK)
                return;
            printf("ALL KEYS:\n");
            for( unqlite_kv_cursor_first_entry(pCursor) ;
                 unqlite_kv_cursor_valid_entry(pCursor) ;
                 unqlite_kv_cursor_next_entry(pCursor) ){
                keylen = sizeof(buf) - 1;
                unqlite_kv_cursor_key(pCursor, buf, &keylen);
                printf(" "); pri(buf, keylen); printf("\n");
            }
            printf("-----\n");
            unqlite_kv_cursor_release(pDb, pCursor);
        }
        break;

    default:
        break;
    }

    return 0;
}

int x_key_callback(
    unqlite_kv_cursor *pCursor,
    const void *pKey, int nKeyLen,
    void *pUserData
    )
{
    printf("KEY (%d)=", nKeyLen); pri(pKey, nKeyLen); printf("\n");

}

#define LWS_PLUGIN_PROTOCOL_MINIMAL \
	{ \
		"unqlite", \
		callback_minimal, \
		sizeof(struct per_session_data__minimal), \
		2048, \
		0, NULL, 0 \
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

static void sigint_handler(int sig)
{
    interrupted = 1;
}

gpointer ws_unqlite_thread(gpointer args)
{
    struct lws_context_creation_info info;
    struct lws_context *context;
    int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
        /* for LLL_ verbosity above NOTICE to be built into lws,
         * lws must have been configured and built with
         * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
        /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
        /* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
        /* | LLL_DEBUG */;

    // Open our database;
    int rc;
    char *dbfile = g_build_filename(g_get_user_data_dir(), "unqlite.db", NULL);
    rc = unqlite_open(&pDb, dbfile, UNQLITE_OPEN_CREATE);
    if (rc != UNQLITE_OK) return NULL;

    signal(SIGINT, sigint_handler);

    lws_set_log_level(logs, NULL);
    lwsl_user("LWS minimal ws broker | visit http://localhost:7681\n");

    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
    info.port = UNQLITE_PORT;
    info.mounts = &mount;
    info.protocols = protocols;
    info.options =
        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
        LWS_SERVER_OPTION_DISABLE_IPV6;

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return NULL;
    }

    while (n >= 0 && !interrupted) {
        n = lws_service(context, 0);
    }

    lws_context_destroy(context);
    unqlite_close(pDb);
    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}


