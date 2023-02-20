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

#include <stdio.h>
#include <stdint.h>

#include "round.h"

struct cJSON;

#ifdef MYDEBUG
#define mylog(_f, _a...) g_print(_f, ##_a)
#define mylogerr(_f, _a...) g_printerr(_f, ##_a)
#else
#define mylog(_f, _a...) do {} while (0)
#define mylogerr(_f, _a...) do {} while (0)
#endif

#define ptr_to_gint( p ) ((gint)(uintptr_t) (p) )
#define gint_to_ptr( p ) ((void*)(uintptr_t) (p) )


#if (GTKVER == 3)
#define GdkRegion	cairo_region_t
#define GdkRectangle	cairo_rectangle_int_t
#define gdk_region_new	cairo_region_create
#define gdk_region_copy	cairo_region_copy
#define gdk_region_destroy	cairo_region_destroy
#define gdk_region_rectangle	cairo_region_create_rectangle
#define gdk_region_get_clipbox	cairo_region_get_extents
#define gdk_region_get_rectangles	cairo_region_num_rectangles_xxx
//   and cairo_region_get_rectangle
#define gdk_region_empty	cairo_region_is_empty
#define gdk_region_equal	cairo_region_equal
#define gdk_region_point_in	cairo_region_contains_point
#define gdk_region_rect_in	cairo_region_contains_rectangle
#define gdk_region_offset	cairo_region_translate
#define gdk_region_union_with_rect	cairo_region_union_rectangle
#define gdk_region_intersect	cairo_region_intersect
#define gdk_region_union	cairo_region_union
#define gdk_region_subtract	cairo_region_subtract
#define gdk_region_xor	cairo_region_xor

#undef GTK_STOCK_CANCEL
#define GTK_STOCK_CANCEL "_Cancel"
#undef GTK_STOCK_PRINT
#define GTK_STOCK_PRINT "_Print"
#undef GTK_STOCK_OK
#define GTK_STOCK_OK "_OK"
#undef GTK_STOCK_OPEN
#define GTK_STOCK_OPEN "_Open"
#undef GTK_STOCK_SAVE
#define GTK_STOCK_SAVE "_Save"
#undef GTK_STOCK_APPLY
#define GTK_STOCK_APPLY "_Apply"
#endif


#define SPRINTF(_buf, _fmt...) do {int _n = snprintf(_buf, sizeof(_buf), _fmt); \
        if (_n >= sizeof(_buf)) g_print("Buffer overflow! %s:%d\n", __FILE__, __LINE__); } while (0)

/* Messages */

#ifndef _COMM_H_
#define _COMM_H_

#ifndef NUM_TATAMIS
#define NUM_TATAMIS 20
#endif

#ifndef INFO_MATCH_NUM
#define INFO_MATCH_NUM 10
#endif

#define SHIAI_PORT          2310
#define JUDOTIMER_PORT      2311
#define WEBSOCK_PORT        2315
#define SERIAL_PORT         2316
#define BROKER_PORT         2317
#define WS_COMM_PORT        2318
#define JUDOSERVER_PORT     5000
#define WEBSOCK_INFO_PORT   5030
#define WEBSOCK_TIMER_PORT  5001

#define MULTICAST_ADDR 0xe0013a01 // 244.1.58.1
#define MULTICAST_PORT_FROM_JS 12310
#define MULTICAST_PORT_TO_JS   12311

#define COMM_VERSION 4

#define APPLICATION_TYPE_UNKNOWN 0
#define APPLICATION_TYPE_SHIAI   1
#define APPLICATION_TYPE_TIMER   2
#define APPLICATION_TYPE_INFO    3
#define APPLICATION_TYPE_WEIGHT  4
#define APPLICATION_TYPE_JUDOGI  5
#define APPLICATION_TYPE_PROXY   6
#define APPLICATION_TYPE_SERVER  7
#define NUM_APPLICATION_TYPES    8

#define COMM_ESCAPE 0xff
#define COMM_FF     0xfe
#define COMM_BEGIN  0xfd
#define COMM_END    0xfc

enum message_types {
    MSG_NEXT_MATCH = 1,
    MSG_RESULT,
    MSG_ACK,
    MSG_SET_COMMENT,
    MSG_SET_POINTS,  // 5
    MSG_HELLO,
    MSG_DUMMY,
    MSG_MATCH_INFO,
    MSG_NAME_INFO,
    MSG_NAME_REQ,    // 10
    MSG_ALL_REQ,
    MSG_CANCEL_REST_TIME,
    MSG_UPDATE_LABEL,
    MSG_EDIT_COMPETITOR,
    MSG_SCALE,       // 15
    MSG_11_MATCH_INFO,
    MSG_EVENT,
    MSG_WEB,
    MSG_LANG,
    MSG_LOOKUP_COMP, // 20
    MSG_INPUT,
    MSG_LABELS,
    NUM_MESSAGES
};

#define START_BIG           128
#define STOP_BIG            129
#define START_ADVERTISEMENT 130
#define START_COMPETITORS   131
#define STOP_COMPETITORS    132
#define START_WINNER        133
#define STOP_WINNER         134
#define SAVED_LAST_NAMES    135
#define SHOW_MSG            136
#define SET_SCORE           137
#define SET_POINTS          138
#define SET_OSAEKOMI_VALUE  139
#define SET_TIMER_VALUE     140
#define SET_TIMER_OSAEKOMI_COLOR 141
#define SET_TIMER_RUN_COLOR 142
#define MAX_LABEL_NUMBER    143

#define MATCH_FLAG_BLUE_DELAYED  0x0001
#define MATCH_FLAG_WHITE_DELAYED 0x0002
#define MATCH_FLAG_REST_TIME     0x0004
#define MATCH_FLAG_BLUE_REST     0x0008
#define MATCH_FLAG_WHITE_REST    0x0010
#define MATCH_FLAG_SEMIFINAL_A   0x0020
#define MATCH_FLAG_SEMIFINAL_B   0x0040
#define MATCH_FLAG_BRONZE_A      0x0080
#define MATCH_FLAG_BRONZE_B      0x0100
#define MATCH_FLAG_GOLD          0x0200
#define MATCH_FLAG_SILVER        0x0400
#define MATCH_FLAG_JUDOGI1_OK    0x0800
#define MATCH_FLAG_JUDOGI1_NOK   0x1000
#define MATCH_FLAG_JUDOGI2_OK    0x2000
#define MATCH_FLAG_JUDOGI2_NOK   0x4000
#define MATCH_FLAG_JUDOGI_MASK   (MATCH_FLAG_JUDOGI1_OK | MATCH_FLAG_JUDOGI1_NOK | \
                                  MATCH_FLAG_JUDOGI2_OK | MATCH_FLAG_JUDOGI2_NOK)
#define MATCH_FLAG_REPECHAGE     0x8000
#define MATCH_FLAG_TEAM_EVENT    0x10000

struct msg_next_match {
    int tatami;
    int category;
    int match;
    int minutes;
    int match_time;
    int gs_time;
    int rep_time;
    int rest_time;
    char pin_time_ippon;
    char pin_time_wazaari;
    char pin_time_yuko;
    char pin_time_koka;
    char cat_1[32];
    char blue_1[64];
    char white_1[64];
    char cat_2[32];
    char blue_2[64];
    char white_2[64];
    int  flags;
    int  round;
    char layout[8];
};

struct msg_result {
    int tatami;
    int category;
    int match;
    int minutes;
    int blue_score;
    int white_score;
    char blue_vote;
    char white_vote;
    char blue_hansokumake;
    char white_hansokumake;
    int legend;
};

struct msg_set_comment {
    int data;
    int category;
    int number;
    int cmd;
    int sys;
};

struct msg_set_points {
    int category;
    int number;
    int sys;
    int blue_points;
    int white_points;
};

struct msg_ack {
    int tatami;
};

struct msg_hello {
    char          info_competition[30];
    char          info_date[30];
    char          info_place[30];
};

struct msg_dummy {
    int application_type;
    int tatami;
};

enum info_types {
    INFO_RUN_COLOR,
    INFO_OSAEKOMI_COLOR,
    INFO_TIMER,
    INFO_OSAEKOMI,
    INFO_POINTS,
    INFO_SCORE,
    NUM_INFOS
};

struct msg_match_info {
    int tatami;
    int position;
    int category;
    int number;
    int blue;
    int white;
    int flags;
    int rest_time;
    int round;
};

struct msg_11_match_info {
    struct msg_match_info info[11];
};

struct msg_name_info {
    int index;
    char last[64];
    char first[64];
    char club[64];
};

struct msg_name_req {
    int index;
};

struct msg_cancel_rest_time {
    int category;
    int number;
    int blue;
    int white;
};

struct msg_update_label {
    union {
	struct {
	    char expose[64]; // total 208 bytes
	    char text[64];
	    char text2[64];
	    char text3[16];
	};
	struct {
	    char cat_a[8];
	    char comp1_a[48];
	    char comp2_a[48];
	    char cat_b[8];
	    char comp1_b[48];
	    char comp2_b[48];
	};
	struct {
	    int pts1[5];
	    int pts2[5];
	};
	struct {
	    int i1, i2, i3, i4, i5;
	};
    };
    double x, y;
    double w, h;
    double fg_r, fg_g, fg_b;
    double bg_r, bg_g, bg_b;
    double size;
    /* special label nums */
    int label_num;
    int xalign;
    int round;
};

#define EDIT_OP_GET        0
#define EDIT_OP_SET        1
#define EDIT_OP_SET_WEIGHT 2
#define EDIT_OP_SET_FLAGS  4
#define EDIT_OP_GET_BY_ID  8
#define EDIT_OP_SET_JUDOGI 16
#define EDIT_OP_CONFIRM    32

struct msg_edit_competitor {
    int operation;
    int index; // == 0 -> new competitor
    char last[32];
    char first[32];
    int birthyear;
    char club[32];
    char regcategory[20];
    int belt;
    int weight;
    int visible;
    char category[20];
    int deleted;
    char country[20];
    char id[20];
    int seeding;
    int clubseeding;
    int matchflags;
    char comment[20];
    char coachid[20];
    char beltstr[16];
    char estim_category[20];
};

struct msg_scale {
    int weight;
    char config[64];
};

struct msg_event {
#define MSG_EVENT_SELECT_TAB  1
#define MSG_EVENT_CLICK_COMP  2
#define MSG_EVENT_CLICK_SHEET 3
    int event;
    int tab;
    int x;
    int y;
};

/* Replies from main thread to httpd.*/
struct msg_web_resp {
#define MSG_WEB_RESP_OK       1
#define MSG_WEB_RESP_ERR      2
#define MSG_WEB_RESP_OK_SENT  3
    volatile int ready;
    int request;
    union {
	struct msg_web_get_comp_data_resp {
	    int index;
	    char last[32];
	    char first[32];
	    int birthyear;
	    char club[32];
	    char country[20];
	    int belt;
	    int weight;
	    char regcategory[20];
	    char category[20];
	    char estim_category[20];
	} get_comp_data_resp;

	struct msg_web_get_match_crc_resp {
	    int crc[NUM_TATAMIS];
	} get_match_crc_resp;

	struct msg_web_get_match_info_resp {
	    int tatami;
	    int num;
	    int match_category_ix;
	    int match_number;
	    int comp1;
	    int comp2;
	    int round;
	} get_match_info_resp[INFO_MATCH_NUM+1];

	struct msg_web_get_bracket_resp {
            int tatami;
            int svg;
            char filename[160];
        } get_bracket_resp;

	struct msg_web_get_category_info_resp {
	    int catix;
	    int system;
	    int numcomp;
	    int table;
	    int wishsys;
	    int num_pages;
	} get_category_info_resp;

	struct msg_web_json_resp {
            struct cJSON *json;
            int file;
        } json;
    } u;
};

/* Messages from httpd to main thread. */
struct msg_web_req {
#define MSG_WEB_GET_ERR        -1
#define MSG_WEB_GET_COMP_DATA   1
#define MSG_WEB_SET_COMP_WEIGHT 2
#define MSG_WEB_GET_MATCH_CRC   3
#define MSG_WEB_GET_MATCH_INFO  4
#define MSG_WEB_GET_BRACKET     5
#define MSG_WEB_GET_CAT_INFO    6
#define MSG_WEB_JSON            7
    int request;
    /* httpd allocates space for resp. */
    struct msg_web_resp *resp;
    union {
	struct msg_web_get_comp_data {
	    char id[20];
	} get_comp_data;

	struct msg_web_set_weight {
	    char id[20];
	    int  weight;
	} set_comp_weight;

	struct msg_web_get_match_info {
	    int tatami;
	} get_match_info;

	struct msg_web_get_bracket {
            int tatami;
            int svg;
            int cat;
	    int page;
            int connum;
	} get_bracket;

	struct msg_web_get_category_info {
	    int catix;
	} get_category_info;

	struct msg_web_json {
            struct cJSON *json;
        } json;
    } u;
};

struct msg_lang {
    char english[64];
    char translation[64];
};

#define NUM_LOOKUP 8
struct msg_lookup_comp {
    char name[16];
    struct {
	int index;
	char fullname[80];
    } result[NUM_LOOKUP];
};

struct msg_input {
    int tatami;
    int key;
#define INPUT_CTL   4
#define INPUT_SHIFT 1
    int shift;
#define INPUT_LEFT_BUTTON   1
#define INPUT_MIDDLE_BUTTON 2
#define INPUT_RIGHT_BUTTON  3
#define INPUT_SCROLL_UP     4
#define INPUT_SCROLL_DOWN   5
    int button;
    int label;
};

struct msg_labels {
    int longtxt;
    union {
	struct {
	    int  lbl;
	    char text[32];
	    char fg[8];
	    char bg[8];
	} labels[4];
	struct {
	    int lbl;
	    char text[128];
	    char fg[8];
	    char bg[8];
	} longlabel;
    };
};

struct message {
    long  src_ip_addr; // Source address of the packet. Added by the comm node (network byte order).
    char  type;
    int   sender;
    union {
        struct msg_next_match  next_match;
        struct msg_result      result;
        struct msg_ack         ack;
        struct msg_set_comment set_comment;
        struct msg_set_points  set_points;
        struct msg_hello       hello;
        struct msg_match_info  match_info;
        struct msg_11_match_info  match_info_11;
        struct msg_name_info   name_info;
        struct msg_name_req    name_req;
        struct msg_cancel_rest_time cancel_rest_time;
        struct msg_update_label update_label;
        struct msg_edit_competitor edit_competitor;
        struct msg_scale       scale;
        struct msg_dummy       dummy;
        struct msg_event       event;
	struct msg_web_req     web;
	struct msg_lang        lang;
	struct msg_lookup_comp lookup_comp;
	struct msg_input       input;
	struct msg_labels      labels;
    } u;
};

#define SSDP_INFO_LEN 48
enum {
    WEBSOCK_NO = 0,
    WEBSOCK_TIMER,
    WEBSOCK_INFO
};
struct jsconn {
    int fd;
    int listen_port;
    uint32_t addr;
    int id;
    uint8_t buf[512];
    int ri;
    int escape;
    int conn_type;
    char ssdp_info[SSDP_INFO_LEN];
    int websock;
    int websock_ok;
};

#define MSG_QUEUE_LEN 2048
extern volatile int msg_queue_put, msg_queue_get;
extern struct message msg_to_send[MSG_QUEUE_LEN];
#ifndef EMSCRIPTEN
extern GStaticMutex send_mutex;
#endif
extern int ssdp_notify;
extern char ssdp_id[64];

/* comm.c */
extern void open_comm_socket(void);
extern void send_packet(struct message *msg);
extern int send_msg(int fd, struct message *msg, gint mcastport);
extern int msg_accepted(struct message *m);
extern int keep_connection(void);
extern int get_port(void);
extern int application_type(void);
extern int pwcrc32(const unsigned char *str, int len);
extern int encode_msg(struct message *m, unsigned char *buf, int buflen);
extern int decode_msg(struct message *m, unsigned char *buf, int buflen);
extern void set_ssdp_id(void);
extern void handle_ssdp_packet(char *p);

/* common.c */
extern const char *round_to_str(int round);

/* websocket-protocol.c */
extern int websock_encode_msg(struct message *m, unsigned char *buf, int buflen);
extern int websock_decode_msg(struct message *m, struct cJSON *json, gint crc32);

/* ip.c */
extern struct message *put_to_rec_queue(volatile struct message *m);
extern void msg_to_queue(struct message *msg);
extern struct message *get_rec_msg(void);
extern void set_preferences(void);
extern uint32_t host2net(uint32_t a);
extern gchar *msg_name(gint msg);

static inline char *full_version(void)
{
    static char buf[64];
    int n = 0;

    n = snprintf(buf, sizeof(buf), "%s", SHIAI_VERSION);
#ifdef __arm__
    n += snprintf(buf+n, sizeof(buf)-n, " arm");
#else
    if (sizeof(void *) == 8)
	n += snprintf(buf+n, sizeof(buf)-n, " x86_64");
    else
	n += snprintf(buf+n, sizeof(buf)-n, " x86");
#endif
#ifdef WIN32
    n += snprintf(buf+n, sizeof(buf)-n, " Windows%s", TGTEXT_S);
#else
    n += snprintf(buf+n, sizeof(buf)-n, " Linux");
#endif
    return buf;
}


static inline void swap32(uint32_t *d) {
    uint32_t x = *d;
    uint8_t *p = (uint8_t *)d;
    p[0] = (x >> 24) & 0xff;
    p[1] = (x >> 16) & 0xff;
    p[2] = (x >> 8) & 0xff;
    p[3] = x & 0xff;
}

static inline uint32_t hton32(uint32_t x) {
    uint32_t a = x;
    swap32(&a);
    return a;
}
#define ntoh32 hton32

#endif
