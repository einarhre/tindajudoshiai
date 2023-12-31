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

#if 0
#if defined(__WIN32__) || defined(WIN32)

#define  __USE_W32_SOCKETS

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
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef EMSCRIPTEN
#include <glib.h>
#else
#define gint int
#define gchar char
#endif
#include "comm.h"
#include "cJSON.h"



static int putstr_esc(char *p, int len, char *s)
{
    int a = 0;

    p[a++] = '"';
    while (a < len - 2) {
	if (*s == 0)
	    break;
	if (*s == '"' || *s == '\t') {
	    p[a++] = '\\';
	}
	if (*s == '\t') {
	    p[a++] = 't';
	    s++;
	} else {
	    p[a++] = *s++;
	}
    }
    p[a++] = '"';
    p[a++] = ',';
    return a;
}

static int putdbl1(char *b, int l, double d)
{
    int n = snprintf(b, l, "%f#", d);
    char *c = strchr(b, ',');
    if (c) *c = '.'; // change comma to full stop
    c = strchr(b, '#');
    if (c) *c = ','; // change comma to full stop
    return n;
}

#define put8(_n) len += snprintf(p+len, buflen-len, "%d,", _n)
#define put16(_n) len += snprintf(p+len, buflen-len, "%d,", _n)
#define put32(_n) len += snprintf(p+len, buflen-len, "%d,", _n)
#define put32n(_n) len += snprintf(p+len, buflen-len, "%d,", ntoh32(_n))
#define putdbl(_n) len += putdbl1(p+len, buflen-len, _n)
//#define putdbl(_n) len += snprintf(p+len, buflen-len, "%f,", _n)
#define putstr(_s) len += putstr_esc(p+len, buflen-len, _s)

int websock_encode_msg(struct message *m, unsigned char *buf, int buflen)
{
    char *p = (char *)buf;
    int i;
    int len = 0;

    len += snprintf(p+len, buflen-len, "{\"msg\":[");
    put8(COMM_VERSION);
    put8(m->type);
    put16(0); // length
    put32(m->sender);

    switch (m->type) {
    case MSG_NEXT_MATCH:
	put32(m->u.next_match.tatami);
	put32(m->u.next_match.category);
	put32(m->u.next_match.match);
	put32(m->u.next_match.minutes);
	put32(m->u.next_match.match_time);
	put32(m->u.next_match.gs_time);
	put32(m->u.next_match.rep_time);
	put32(m->u.next_match.rest_time);
	put8(m->u.next_match.pin_time_ippon);
	put8(m->u.next_match.pin_time_wazaari);
	put8(m->u.next_match.pin_time_yuko);
	put8(m->u.next_match.pin_time_koka);
	putstr(m->u.next_match.cat_1);
	putstr(m->u.next_match.blue_1);
	putstr(m->u.next_match.white_1);
	putstr(m->u.next_match.cat_2);
	putstr(m->u.next_match.blue_2);
	putstr(m->u.next_match.white_2);
	put32(m->u.next_match.flags);
	put32(m->u.next_match.round);
	putstr(m->u.next_match.layout);
	break;
    case MSG_RESULT:
	put32(m->u.result.tatami);
	put32(m->u.result.category);
	put32(m->u.result.match);
	put32(m->u.result.minutes);
	put32(m->u.result.blue_score);
	put32(m->u.result.white_score);
	put8(m->u.result.blue_vote);
	put8(m->u.result.white_vote);
	put8(m->u.result.blue_hansokumake);
	put8(m->u.result.white_hansokumake);
	put32(m->u.result.legend);
	break;
    case MSG_ACK:
	put32(m->u.ack.tatami);
	break;
    case MSG_SET_COMMENT:
	put32(m->u.set_comment.data);
	put32(m->u.set_comment.category);
	put32(m->u.set_comment.number);
	put32(m->u.set_comment.cmd);
	put32(m->u.set_comment.sys);
	break;
    case MSG_SET_POINTS:
	put32(m->u.set_points.category);
	put32(m->u.set_points.number);
	put32(m->u.set_points.sys);
	put32(m->u.set_points.blue_points);
	put32(m->u.set_points.white_points);
	break;
    case MSG_HELLO:
	putstr(m->u.hello.info_competition);
	putstr(m->u.hello.info_date);
	putstr(m->u.hello.info_place);
	break;
    case MSG_DUMMY:
	put32(m->u.dummy.application_type);
	put32(m->u.dummy.tatami);
	break;
    case MSG_MATCH_INFO:
	put32(m->u.match_info.tatami);
	put32(m->u.match_info.position);
	put32(m->u.match_info.category);
	put32(m->u.match_info.number);
	put32(m->u.match_info.blue);
	put32(m->u.match_info.white);
	put32(m->u.match_info.flags);
	put32(m->u.match_info.rest_time);
	put32(m->u.match_info.round);
	break;
    case MSG_11_MATCH_INFO:
	for (i = 0; i < 11; i++) {
	    put32(m->u.match_info_11.info[i].tatami);
	    put32(m->u.match_info_11.info[i].position);
	    put32(m->u.match_info_11.info[i].category);
	    put32(m->u.match_info_11.info[i].number);
	    put32(m->u.match_info_11.info[i].blue);
	    put32(m->u.match_info_11.info[i].white);
	    put32(m->u.match_info_11.info[i].flags);
	    put32(m->u.match_info_11.info[i].rest_time);
	    put32(m->u.match_info_11.info[i].round);
	}
	break;
    case MSG_NAME_INFO:
	put32(m->u.name_info.index);
	putstr(m->u.name_info.last);
	putstr(m->u.name_info.first);
	putstr(m->u.name_info.club);
	break;
    case MSG_NAME_REQ:
	put32(m->u.name_req.index);
	break;
    case MSG_ALL_REQ:
	break;
    case MSG_CANCEL_REST_TIME:
	put32(m->u.cancel_rest_time.category);
	put32(m->u.cancel_rest_time.number);
	put32(m->u.cancel_rest_time.blue);
	put32(m->u.cancel_rest_time.white);
	break;
    case MSG_UPDATE_LABEL:
	put32(m->u.update_label.label_num);
	switch (m->u.update_label.label_num) {
	case START_COMPETITORS:
	    putstr(m->u.update_label.expose);
	    putstr(m->u.update_label.text);
	    putstr(m->u.update_label.text2);
	    putstr(m->u.update_label.text3);
	    put32(m->u.update_label.round);
	    break;
	case SET_TIMER_RUN_COLOR:
	case SET_OSAEKOMI_VALUE:
	    put32n(m->u.update_label.i1);
	    put32n(m->u.update_label.i2);
	    break;
	case SET_TIMER_OSAEKOMI_COLOR:
	    put32n(m->u.update_label.i1);
	    put32n(m->u.update_label.i2);
	    put32n(m->u.update_label.i3);
	    break;
	case SET_TIMER_VALUE:
	    put32n(m->u.update_label.i1);
	    put32n(m->u.update_label.i2);
	    put32n(m->u.update_label.i3);
	    put32n(m->u.update_label.i4);
	    put32n(m->u.update_label.i5);
	    break;
	case SET_POINTS:
	    for (i = 0; i < 5; i++)
		put32n(m->u.update_label.pts1[i]);
	    for (i = 0; i < 5; i++)
		put32n(m->u.update_label.pts2[i]);
	    break;
	case SET_SCORE:
	    put32(m->u.update_label.xalign);
	    break;
	case SHOW_MSG:
	    putstr(m->u.update_label.cat_a);
	    putstr(m->u.update_label.comp1_a);
	    putstr(m->u.update_label.comp2_a);
	    putstr(m->u.update_label.cat_b);
	    putstr(m->u.update_label.comp1_b);
	    putstr(m->u.update_label.comp2_b);
	    put32(m->u.update_label.xalign);
	    put32(m->u.update_label.round);
	    break;
	case SAVED_LAST_NAMES:
	    putstr(m->u.update_label.text);
	    putstr(m->u.update_label.text2);
	    putstr(m->u.update_label.text3);
	    break;
	case START_WINNER:
	    putstr(m->u.update_label.text);
	    putstr(m->u.update_label.text2);
	    put32(m->u.update_label.text3[0]);
	    break;
	case STOP_BIG:
	case STOP_WINNER:
	    break;
	case START_BIG:
	    putstr(m->u.update_label.text);
	    break;
	case STOP_COMPETITORS:
	    put32(m->u.update_label.xalign);
	    break;
	default:
	    putstr(m->u.update_label.expose);
	    putstr(m->u.update_label.text);
	    putstr(m->u.update_label.text2);
	    putstr(m->u.update_label.text3);
	    putdbl(m->u.update_label.x);
	    putdbl(m->u.update_label.y);
	    putdbl(m->u.update_label.w);
	    putdbl(m->u.update_label.h);
	    putdbl(m->u.update_label.fg_r);
	    putdbl(m->u.update_label.fg_g);
	    putdbl(m->u.update_label.fg_b);
	    putdbl(m->u.update_label.bg_r);
	    putdbl(m->u.update_label.bg_g);
	    putdbl(m->u.update_label.bg_b);
	    putdbl(m->u.update_label.size);
	    put32(m->u.update_label.label_num);
	    put32(m->u.update_label.xalign);
	    put32(m->u.update_label.round);
	}
	break;
    case MSG_EDIT_COMPETITOR:
	put32(m->u.edit_competitor.operation);
	put32(m->u.edit_competitor.index);
	putstr(m->u.edit_competitor.last);
	putstr(m->u.edit_competitor.first);
	put32(m->u.edit_competitor.birthyear);
	putstr(m->u.edit_competitor.club);
	putstr(m->u.edit_competitor.regcategory);
	put32(m->u.edit_competitor.belt);
	put32(m->u.edit_competitor.weight);
	put32(m->u.edit_competitor.visible);
	putstr(m->u.edit_competitor.category);
	put32(m->u.edit_competitor.deleted);
	putstr(m->u.edit_competitor.country);
	putstr(m->u.edit_competitor.id);
	put32(m->u.edit_competitor.seeding);
	put32(m->u.edit_competitor.clubseeding);
	put32(m->u.edit_competitor.matchflags);
	putstr(m->u.edit_competitor.comment);
	putstr(m->u.edit_competitor.coachid);
	putstr(m->u.edit_competitor.beltstr);
	putstr(m->u.edit_competitor.estim_category);
	break;
    case MSG_SCALE:
	put32(m->u.scale.weight);
	putstr(m->u.scale.config);
	break;
    case MSG_LANG:
	putstr(m->u.lang.english);
	putstr(m->u.lang.translation);
	break;
    case MSG_LOOKUP_COMP:
	putstr(m->u.lookup_comp.name);
	for (i = 0; i < NUM_LOOKUP; i++) {
	    put32(m->u.lookup_comp.result[i].index);
	    putstr(m->u.lookup_comp.result[i].fullname);
	}
	break;
    case MSG_INPUT:
	put32(m->u.input.tatami);
	put32(m->u.input.key);
	put32(m->u.input.shift);
	put32(m->u.input.button);
	put32(m->u.input.label);
	break;
    case MSG_LABELS:
	put32(m->u.labels.longtxt);
	if (m->u.labels.longtxt) {
	    put32(m->u.labels.longlabel.lbl);
	    putstr(m->u.labels.longlabel.text);
	    putstr(m->u.labels.longlabel.fg);
	    putstr(m->u.labels.longlabel.bg);
	} else {
	    for (i = 0; i < 4; i++) {
		put32(m->u.labels.labels[i].lbl);
		putstr(m->u.labels.labels[i].text);
		putstr(m->u.labels.labels[i].fg);
		putstr(m->u.labels.labels[i].bg);
	    }
	}
	break;
    }

    len += snprintf(p+len, buflen-len, "0]}");
    return len;
}

#define getbool    if (!val) return -1; \
    if (val->type == cJSON_False) _n = FALSE; \
    else if (val->type == cJSON_True) _n = TRUE; \
    else printf("E:%d\n", __LINE__); \
    val = val->next  
#define get8(_n)   if (!val || val->type != cJSON_Number) {printf("E:%d\n", __LINE__); return -1;} _n = val->valueint; val = val->next
#define get16(_n)  if (!val || val->type != cJSON_Number) {printf("E:%d\n", __LINE__); return -1;} _n = val->valueint; val = val->next
#define get32(_n)  if (!val || val->type != cJSON_Number) {printf("E:%d\n", __LINE__); return -1;} _n = val->valueint; val = val->next
#define getdbl(_n) if (!val || val->type != cJSON_Number) {printf("E:%d\n", __LINE__); return -1;} _n = val->valuedouble; val = val->next
#define getstr(_s) if (!val || val->type != cJSON_String) {printf("E:%d\n", __LINE__); return -1;} strncpy((char *)_s, val->valuestring, sizeof(_s)-1); val = val->next

int websock_decode_msg(struct message *m, cJSON *json, gint crc32)
{
    int ver, len, i;
    //cJSON *msg = json->child;
    cJSON *msg = cJSON_GetObjectItem(json, "msg");
    
    char *txt = cJSON_Print(json);
    mylog("JSON=%s\n", txt);
    free(txt);
    
    if (!msg || msg->type != cJSON_Array)
	return -1;

    /***
    if (crc32) {
        cJSON *pw = cJSON_GetObjectItem(json, "pw");
        if (!pw || pw->type != cJSON_String) return -1;
        gint crc = pwcrc32((unsigned char *)pw->valuestring, strlen(pw->valuestring));
        if (crc != crc32) {
            return -1;
        }
    }
    ***/
    
    cJSON *val = msg->child;

    get8(ver);

    if (ver != COMM_VERSION) {
	return -1;
    }
    get8(m->type);
    get16(len);
    (void)len;
    get32(m->sender);

    switch (m->type) {
    case MSG_NEXT_MATCH:
	get32(m->u.next_match.tatami);
	get32(m->u.next_match.category);
	get32(m->u.next_match.match);
	get32(m->u.next_match.minutes);
	get32(m->u.next_match.match_time);
	get32(m->u.next_match.gs_time);
	get32(m->u.next_match.rep_time);
	get32(m->u.next_match.rest_time);
	get8(m->u.next_match.pin_time_ippon);
	get8(m->u.next_match.pin_time_wazaari);
	get8(m->u.next_match.pin_time_yuko);
	get8(m->u.next_match.pin_time_koka);
	getstr(m->u.next_match.cat_1);
	getstr(m->u.next_match.blue_1);
	getstr(m->u.next_match.white_1);
	getstr(m->u.next_match.cat_2);
	getstr(m->u.next_match.blue_2);
	getstr(m->u.next_match.white_2);
	get32(m->u.next_match.flags);
	get32(m->u.next_match.round);
	getstr(m->u.next_match.layout);
	break;
    case MSG_RESULT:
	get32(m->u.result.tatami);
	get32(m->u.result.category);
	get32(m->u.result.match);
	get32(m->u.result.minutes);
	get32(m->u.result.blue_score);
	get32(m->u.result.white_score);
	get8(m->u.result.blue_vote);
	get8(m->u.result.white_vote);
	get8(m->u.result.blue_hansokumake);
	get8(m->u.result.white_hansokumake);
	get32(m->u.result.legend);
	break;
    case MSG_ACK:
	get32(m->u.ack.tatami);
	break;
    case MSG_SET_COMMENT:
	get32(m->u.set_comment.data);
	get32(m->u.set_comment.category);
	get32(m->u.set_comment.number);
	get32(m->u.set_comment.cmd);
	get32(m->u.set_comment.sys);
	break;
    case MSG_SET_POINTS:
	get32(m->u.set_points.category);
	get32(m->u.set_points.number);
	get32(m->u.set_points.sys);
	get32(m->u.set_points.blue_points);
	get32(m->u.set_points.white_points);
	break;
    case MSG_HELLO:
	getstr(m->u.hello.info_competition);
	getstr(m->u.hello.info_date);
	getstr(m->u.hello.info_place);
	break;
    case MSG_DUMMY:
	get32(m->u.dummy.application_type);
	get32(m->u.dummy.tatami);
	break;
    case MSG_MATCH_INFO:
	get32(m->u.match_info.tatami);
	get32(m->u.match_info.position);
	get32(m->u.match_info.category);
	get32(m->u.match_info.number);
	get32(m->u.match_info.blue);
	get32(m->u.match_info.white);
	get32(m->u.match_info.flags);
	get32(m->u.match_info.rest_time);
	get32(m->u.match_info.round);
	break;
    case MSG_11_MATCH_INFO:
	for (i = 0; i < 11; i++) {
	    get32(m->u.match_info_11.info[i].tatami);
	    get32(m->u.match_info_11.info[i].position);
	    get32(m->u.match_info_11.info[i].category);
	    get32(m->u.match_info_11.info[i].number);
	    get32(m->u.match_info_11.info[i].blue);
	    get32(m->u.match_info_11.info[i].white);
	    get32(m->u.match_info_11.info[i].flags);
	    get32(m->u.match_info_11.info[i].rest_time);
	    get32(m->u.match_info_11.info[i].round);
	}
	break;
    case MSG_NAME_INFO:
	get32(m->u.name_info.index);
	getstr(m->u.name_info.last);
	getstr(m->u.name_info.first);
	getstr(m->u.name_info.club);
	break;
    case MSG_NAME_REQ:
	get32(m->u.name_req.index);
	break;
    case MSG_ALL_REQ:
	break;
    case MSG_CANCEL_REST_TIME:
	get32(m->u.cancel_rest_time.category);
	get32(m->u.cancel_rest_time.number);
	get32(m->u.cancel_rest_time.blue);
	get32(m->u.cancel_rest_time.white);
	break;
    case MSG_UPDATE_LABEL:
	getstr(m->u.update_label.expose);
	getstr(m->u.update_label.text);
	getstr(m->u.update_label.text2);
	getstr(m->u.update_label.text3);
	getdbl(m->u.update_label.x);
	getdbl(m->u.update_label.y);
	getdbl(m->u.update_label.w);
	getdbl(m->u.update_label.h);
	getdbl(m->u.update_label.fg_r);
	getdbl(m->u.update_label.fg_g);
	getdbl(m->u.update_label.fg_b);
	getdbl(m->u.update_label.bg_r);
	getdbl(m->u.update_label.bg_g);
	getdbl(m->u.update_label.bg_b);
	getdbl(m->u.update_label.size);
	get32(m->u.update_label.label_num);
	get32(m->u.update_label.xalign);
	get32(m->u.update_label.round);
	break;
    case MSG_EDIT_COMPETITOR:
	get32(m->u.edit_competitor.operation);
	get32(m->u.edit_competitor.index);
	getstr(m->u.edit_competitor.last);
	getstr(m->u.edit_competitor.first);
	get32(m->u.edit_competitor.birthyear);
	getstr(m->u.edit_competitor.club);
	getstr(m->u.edit_competitor.regcategory);
	get32(m->u.edit_competitor.belt);
	get32(m->u.edit_competitor.weight);
	get32(m->u.edit_competitor.visible);
	getstr(m->u.edit_competitor.category);
	get32(m->u.edit_competitor.deleted);
	getstr(m->u.edit_competitor.country);
	getstr(m->u.edit_competitor.id);
	get32(m->u.edit_competitor.seeding);
	get32(m->u.edit_competitor.clubseeding);
	get32(m->u.edit_competitor.matchflags);
	getstr(m->u.edit_competitor.comment);
	getstr(m->u.edit_competitor.coachid);
	getstr(m->u.edit_competitor.beltstr);
	getstr(m->u.edit_competitor.estim_category);
	break;
    case MSG_SCALE:
	get32(m->u.scale.weight);
	getstr(m->u.scale.config);
	break;
    case MSG_LANG:
	getstr(m->u.lang.english);
	getstr(m->u.lang.translation);
	break;
    case MSG_LOOKUP_COMP:
	getstr(m->u.lookup_comp.name);
	for (i = 0; i < NUM_LOOKUP; i++) {
	    get32(m->u.lookup_comp.result[i].index);
	    getstr(m->u.lookup_comp.result[i].fullname);
	}
	break;
    case MSG_INPUT:
	get32(m->u.input.tatami);
	get32(m->u.input.key);
	get32(m->u.input.shift);
	get32(m->u.input.button);
	get32(m->u.input.label);
	break;
    case MSG_LABELS:
	get32(m->u.labels.longtxt);
	if (m->u.labels.longtxt) {
	    get32(m->u.labels.longlabel.lbl);
	    getstr(m->u.labels.longlabel.text);
	    getstr(m->u.labels.longlabel.fg);
	    getstr(m->u.labels.longlabel.bg);
	} else {
	    for (i = 0; i < 4; i++) {
		get32(m->u.labels.labels[i].lbl);
		getstr(m->u.labels.labels[i].text);
		getstr(m->u.labels.labels[i].fg);
		getstr(m->u.labels.labels[i].bg);
	    }
	}
	break;
    }

    return 0;
}
