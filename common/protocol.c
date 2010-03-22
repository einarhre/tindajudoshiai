/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2010 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

#include <string.h>
#include <gtk/gtk.h>

#include "comm.h"

#define put8(_n) *p++ = _n; if (p > end) goto out

#define put16(_n) *p++ = (_n >> 8)&0xff; *p++ = _n & 0xff; if (p > end) goto out

#define put32(_n) *p++ = (_n >> 24)&0xff; \
    *p++ = (_n >> 16)&0xff; *p++ = (_n >> 8)&0xff; *p++ = _n&0xff; if (p > end) goto out

#define putstr(_s) memcpy(p, &_s, sizeof(_s)); p += sizeof(_s); if (p > end) goto out

#define get8(_n) _n = *p++; if (p > end) goto out

#define get16(_n) _n = (p[0]<<8) | p[1]; p += 2; if (p > end) goto out

#define get32(_n) _n = (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3]; p += 4; \
    if (p > end) goto out

#define getstr(_s) memcpy(&_s, p, sizeof(_s)); p += sizeof(_s); if (p > end) goto out

gint encode_msg(struct message *m, guchar *buf, gint buflen)
{
    guchar *p = buf, *end = buf + buflen;

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
	putstr(m->u.update_label.expose);
	putstr(m->u.update_label.text);
	putstr(m->u.update_label.x);
	putstr(m->u.update_label.y);
	putstr(m->u.update_label.w);
	putstr(m->u.update_label.h);
	putstr(m->u.update_label.fg_r);
	putstr(m->u.update_label.fg_g);
	putstr(m->u.update_label.fg_b);
	putstr(m->u.update_label.bg_r);
	putstr(m->u.update_label.bg_g);
	putstr(m->u.update_label.bg_b);
	putstr(m->u.update_label.size);
	put32(m->u.update_label.label_num);
	put32(m->u.update_label.xalign);
	break;
    }

    gint len = (gint)(p - buf);
    p = buf + 2;
    put16(len);

    return len;

 out:
    g_print("%s:%d: Too short buffer %d\n",
	    __FUNCTION__, __LINE__, buflen);
    return -1;
}

gint decode_msg(struct message *m, guchar *buf, gint buflen)
{
    guchar *p = buf, *end = buf + buflen;
    gint len, ver;

    get8(ver);

    if (ver != COMM_VERSION) {
	g_print("%s:%d: Wrong message version %d (%d expected)\n",
		__FUNCTION__, __LINE__, ver, COMM_VERSION);
	return -1;
    }

    get8(m->type);
    get16(len);

    if (len > buflen) {
	g_print("%s:%d: Buffer length too small %d (needed %d)\n",
		__FUNCTION__, __LINE__, buflen, len);
	return -1;
    }

    get32(m->sender);

    switch (m->type) {
    case MSG_NEXT_MATCH:
	get32(m->u.next_match.tatami);
	get32(m->u.next_match.category);
	get32(m->u.next_match.match);
	get32(m->u.next_match.minutes);
	get32(m->u.next_match.match_time);
	get32(m->u.next_match.gs_time);
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
	getstr(m->u.update_label.x);
	getstr(m->u.update_label.y);
	getstr(m->u.update_label.w);
	getstr(m->u.update_label.h);
	getstr(m->u.update_label.fg_r);
	getstr(m->u.update_label.fg_g);
	getstr(m->u.update_label.fg_b);
	getstr(m->u.update_label.bg_r);
	getstr(m->u.update_label.bg_g);
	getstr(m->u.update_label.bg_b);
	getstr(m->u.update_label.size);
	get32(m->u.update_label.label_num);
	get32(m->u.update_label.xalign);
	break;
    }

    if ((gint)(p - buf) != len) {
	g_print("%s:%d: Wrong message length %d (expected %d bytes)\n",
		__FUNCTION__, __LINE__, (gint)(p - buf), len);
	return -1;
    }

    return len;

 out:
    g_print("%s:%d: Too short buffer %d\n",
	    __FUNCTION__, __LINE__, buflen);
    return -1;
}
