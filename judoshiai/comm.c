/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2021 by Hannu Jokinen
 * Full copyright text is included in the software package.
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

/* System-dependent definitions */
#ifndef WIN32
#define SOCKET          gint
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define SOCKADDR_BTH    struct sockaddr_rc
#define AF_BTH          PF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM
#endif

#include <sys/time.h>
#include <gtk/gtk.h>

#include "sqlite3.h"
#include "judoshiai.h"
#include "sha.h"
#include "cJSON.h"

extern gint webpwcrc32;
extern gint json_edit_or_create_judoka(cJSON *root);

#define JSON_CHECK(_a) do { if (!(_a)) { ret = MSG_WEB_RESP_ERR; goto json_end; }} while (0)
#define JSON_GET_VAL(_root, _item) cJSON *_item = cJSON_GetObjectItem(_root, #_item); JSON_CHECK(_item)
#define JSON_GET_STR(_root, _item)              \
    const gchar *_item;                                                 \
    do { cJSON *tmp = cJSON_GetObjectItem(_root, #_item); JSON_CHECK(tmp); \
        _item = tmp->valuestring; } while (0)
#define JSON_GET_INT(_root, _item)              \
    int _item;                                                          \
    do { cJSON *tmp = cJSON_GetObjectItem(_root, #_item); JSON_CHECK(tmp); \
        _item = tmp->valueint; } while (0)
#define JSON_GET_LIST(_root, _item)              \
    struct cJSON *_item;                                                          \
    do { cJSON *tmp = cJSON_GetObjectItem(_root, #_item); JSON_CHECK(tmp); \
        _item = tmp->child; } while (0)
#define JSON_OP(_op) (!strcmp(op, #_op))
    

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

#define NUM_CONNECTIONS 32
static struct jsconn connections[NUM_CONNECTIONS];

static struct {
    gulong addr;
    gint id;
    gchar ssdp_info[SSDP_INFO_LEN];
} noconnections[NUM_CONNECTIONS];

static SOCKET mcast_out_fd;


//static volatile struct message message_queue[MSG_QUEUE_LEN];
//static volatile gint msg_put = 0, msg_get = 0;
//static GStaticMutex msg_mutex = G_STATIC_MUTEX_INIT;

static int closesock(int s)
{
#if defined(__WIN32__) || defined(WIN32)
    shutdown(s, SD_SEND);
    return closesocket(s);
#else
    return close(s);
#endif
}

struct message *get_next_match_msg(int tatami)
{
    return &next_match_messages[tatami-1];
}

void send_info_packet(struct message *msg)
{
    gint i;

    for (i = 0; i < NUM_CONNECTIONS; i++) {
	if (connections[i].fd > 0 &&
	    connections[i].conn_type == APPLICATION_TYPE_INFO) {
	    if (connections[i].websock) {
		if (connections[i].websock_ok)
		    websock_send_msg(connections[i].fd, msg);
	    } else
		send_msg(connections[i].fd, msg, 0);
	}
    }

    send_msg(mcast_out_fd, msg, MULTICAST_PORT_FROM_JS);
}

void add_client_ssdp_info(gchar *rec, struct sockaddr_in *client)
{
    gint i, id;

    gchar *p1, *p = strstr(rec, "UPnP/1.0 Judo");
    if (!p)
        return;
    p += 9;

    p1 = strchr(p, '\r');
    if (!p1)
        return;

    *p1 = 0;
    p1++;
    p1 = strstr(p1, "uuid:");
    if (!p1)
        return;

    p1 += 5;
    sscanf(p1, "%x", &id);

    // clear data
    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (client->sin_addr.s_addr == noconnections[i].addr &&
            id == noconnections[i].id) {
            noconnections[i].addr = 0;
            noconnections[i].id = 0;
        }
    }

    // first option: connection exists
    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (client->sin_addr.s_addr == connections[i].addr &&
            id == connections[i].id) {
            snprintf(connections[i].ssdp_info, SSDP_INFO_LEN, "%s", p);
            return;
        }
    }

    // second option: no connection yet (wrong config)
    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (noconnections[i].addr == 0) {
            noconnections[i].addr = client->sin_addr.s_addr;
            noconnections[i].id = id;
            snprintf(noconnections[i].ssdp_info, SSDP_INFO_LEN, "%s", p);
            return;
        }
    }
}

gchar *other_info(gint num)
{
    guint addr = ntohl(others[num].addr);
    static gchar buf[256];

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
    case MSG_EDIT_COMPETITOR:
    case MSG_SCALE:
    case MSG_EVENT:
    case MSG_WEB:
    case MSG_LANG:
    case MSG_LOOKUP_COMP:
        return TRUE;
    }
    return FALSE;
}

static gchar *find_estim_cat(struct judoka *j)
{
    // free returned value
    if (j->regcategory == NULL || j->regcategory[0] == 0)
        return find_correct_category(current_year - j->birthyear, j->weight,
                                     (j->deleted & GENDER_FEMALE) ? IS_FEMALE : IS_MALE,
                                     NULL, TRUE);
    return find_correct_category(0, j->weight, 0, j->regcategory, FALSE);
}

static void json_add_judoka_data(cJSON *out, struct judoka *j)
{
    cJSON_AddNumberToObject(out, "index", j->index);
    cJSON_AddStringToObject(out, "last", j->last);
    cJSON_AddStringToObject(out, "first", j->first);
    cJSON_AddStringToObject(out, "club", j->club);
    cJSON_AddStringToObject(out, "country", j->country);
    cJSON_AddStringToObject(out, "regcat", j->regcategory);
    cJSON_AddStringToObject(out, "cat", j->category);
    cJSON_AddNumberToObject(out, "weight", j->weight);
    gchar *estim = find_estim_cat(j);
    if (estim) {
        cJSON_AddStringToObject(out, "ecat", estim);
        g_free(estim);
    }
}

static void json_add_category_data(cJSON *out, struct category_data *c)
{
    cJSON_AddNumberToObject(out, "index", c->index);
    cJSON_AddStringToObject(out, "category", c->category);
    cJSON_AddNumberToObject(out, "tatami", c->tatami);
    cJSON_AddNumberToObject(out, "group", c->group);
    cJSON_AddNumberToObject(out, "system", c->system.system);
    cJSON_AddNumberToObject(out, "numcomp", c->system.numcomp);
    cJSON_AddNumberToObject(out, "table", c->system.table);
    cJSON_AddNumberToObject(out, "wishsys", c->system.wishsys);
    cJSON_AddNumberToObject(out, "status", c->match_status);
    cJSON_AddNumberToObject(out, "count", c->match_count);
    cJSON_AddNumberToObject(out, "matchedcnt", c->matched_matches_count);
    cJSON_AddStringToObject(out, "sysdescr", get_system_description(c->index, c->system.numcomp));
}

static void json_add_match_data(cJSON *obj, struct match *m)
{
    if (m->number == 1000)
        return;

    struct category_data *cat = avl_get_category(m->category);
    if (!cat)
        return;
    cJSON_AddStringToObject(obj, "category", cat->category);
    cJSON_AddNumberToObject(obj, "number", m->number);

    struct judoka *j1 = avl_get_competitor(m->blue);
    if (j1) {
        cJSON *comp = cJSON_CreateObject();
        cJSON_AddStringToObject(comp, "last", j1->last);
        cJSON_AddStringToObject(comp, "first", j1->first);
        cJSON_AddStringToObject(comp, "club", j1->club);
        cJSON_AddStringToObject(comp, "country", j1->country);
        cJSON_AddItemToObject(obj, "comp1", comp);
    }
        
    struct judoka *j2 = avl_get_competitor(m->white);
    if (j2) {
        cJSON *comp = cJSON_CreateObject();
        cJSON_AddStringToObject(comp, "last", j2->last);
        cJSON_AddStringToObject(comp, "first", j2->first);
        cJSON_AddStringToObject(comp, "club", j2->club);
        cJSON_AddStringToObject(comp, "country", j2->country);
        cJSON_AddItemToObject(obj, "comp2", comp);
    }
}

static struct judoka *find_judoka_by_id(const gchar *id)
{
    gboolean coach;
    struct judoka *j = NULL;
    gint indx = db_get_index_by_id(id, &coach);
    if (indx) j = get_data(indx);
    if (!j) j = get_data(atoi(id));
    return j;
}

static void fill_msg_edit_competitor(struct message *msg, struct judoka *j)
{
#define CP2MSG_INT(_dst) msg->u.edit_competitor._dst = j->_dst
#define CP2MSG_STR(_dst) strncpy(msg->u.edit_competitor._dst, j->_dst, sizeof(msg->u.edit_competitor._dst)-1)
    CP2MSG_INT(index);
    CP2MSG_STR(last);
    CP2MSG_STR(first);
    CP2MSG_INT(birthyear);
    CP2MSG_STR(club);
    CP2MSG_STR(regcategory);
    CP2MSG_INT(belt);
    CP2MSG_INT(weight);
    CP2MSG_INT(visible);
    CP2MSG_STR(category);
    CP2MSG_INT(deleted);
    CP2MSG_STR(country);
    CP2MSG_STR(id);
    CP2MSG_INT(seeding);
    CP2MSG_INT(clubseeding);
    CP2MSG_STR(comment);
    CP2MSG_STR(coachid);
    strncpy(msg->u.edit_competitor.beltstr, belts[j->belt], sizeof(msg->u.edit_competitor.beltstr)-1);
    msg->u.edit_competitor.matchflags = get_judogi_status(j->index);
}

static void fill_judoka_edit_competitor(struct message *msg, struct judoka *j)
{
#define SET_J(_x) j->_x = msg->u.edit_competitor._x
    SET_J(index);
    SET_J(last);
    SET_J(first);
    SET_J(birthyear);
    SET_J(belt);
    SET_J(club);
    SET_J(regcategory);
    SET_J(weight);
    SET_J(visible);
    SET_J(category);
    SET_J(deleted);
    SET_J(country);
    SET_J(id);
    SET_J(seeding);
    SET_J(clubseeding);
    SET_J(comment);
    SET_J(coachid);
}

// curl -X POST -H 'Content-Type: application/json' -d '{"op": "getcomp", "pw": "PASSWD", "ix":23}' http://localhost:8088/json
void handle_json(struct message *input_msg)
{
    struct judoka *j = NULL;
    int ret = MSG_WEB_RESP_OK;
    cJSON *root = input_msg->u.web.u.json.json;
    struct msg_web_resp *resp;
    resp = input_msg->u.web.resp;
    resp->request = input_msg->u.web.request;

    if (webpwcrc32) {
        JSON_GET_STR(root, pw);
        gint crc = pwcrc32((unsigned char *)pw, strlen(pw));
        if (crc != webpwcrc32) {
            ret = MSG_WEB_RESP_ERR;
            goto json_end;
        }
    }
    
    JSON_GET_STR(root, op);
    j = NULL;

    if (JSON_OP(getcomp) || JSON_OP(setweight)) {
        JSON_GET_INT(root, ix);
        JSON_CHECK(j = get_data(ix));
        if (JSON_OP(setweight)) {
            JSON_GET_INT(root, weight);
            j->weight = weight;
            db_update_judoka(j->index, j);
            display_one_judoka(j);
        }
        cJSON *out = cJSON_CreateObject();
        json_add_judoka_data(out, j);
        resp->u.json.json = out;
    } else if (JSON_OP(lang)) {
        JSON_GET_LIST(root, words);
        cJSON *out = cJSON_CreateArray();
        while (words) {
            cJSON_AddItemToArray(out, cJSON_CreateString(_(words->valuestring)));
            words = words->next;
        }
        resp->u.json.json = out;
    } else if (JSON_OP(setcomp)) {
        gint index = json_edit_or_create_judoka(root);
        cJSON *out = cJSON_CreateObject();
        cJSON_AddNumberToObject(out, "ix", index);
        resp->u.json.json = out;
    } else if (JSON_OP(sql)) {
        gint row;
        int numrows, numcols;
        JSON_GET_STR(root, cmd);

        const char **tablecopy = db_get_table_copy(cmd, &numrows, &numcols);
        if (tablecopy == NULL)
            goto json_end;

        cJSON *out = cJSON_CreateArray();

        for (row = 0; row <= numrows; row++) {
            cJSON *jsonrow = cJSON_CreateStringArray(&tablecopy[row*numcols], numcols);
            cJSON_AddItemToArray(out, jsonrow);
        }        

        db_close_table_copy(tablecopy);
        resp->u.json.json = out;
    } else if (JSON_OP(matches)) {
        JSON_GET_INT(root, tatami);
        cJSON *out = cJSON_CreateArray();
        struct match *m = get_cached_next_matches(tatami);
        int i;

        for (i = 0; i < NEXT_MATCH_NUM; i++) {
            if (m[i].number == 1000)
                break;
            cJSON *a = cJSON_CreateObject();
            json_add_match_data(a, &m[i]);
            cJSON_AddItemToArray(out, a);
        }

        resp->u.json.json = out;
    } else if (JSON_OP(draw)) {
        GtkTreeIter iter;

        JSON_GET_LIST(root, cat);
        while (cat) {
            if (find_iter(&iter, cat->valueint)) {
                gint n = gtk_tree_model_iter_n_children(current_model, &iter);
                if (n >= 1 && n <= NUM_COMPETITORS)
                    draw_one_category(&iter, n);
            }
            cat = cat->next;
        }
        cJSON *out = cJSON_CreateObject();
        resp->u.json.json = out;
    } else if (JSON_OP(validate)) {
        extern gchar *db_validation(GtkWidget *w, gpointer data);
        gchar *r = db_validation(NULL, NULL);
        cJSON *out = cJSON_CreateObject();
        if (r) {
            cJSON_AddStringToObject(out, "validate", r);
            g_free(r);
        }
        resp->u.json.json = out;
    } else if (JSON_OP(accrcard)) {
        GSList *complist = NULL;
        JSON_GET_LIST(root, comps);
        JSON_GET_INT(root, what);
        while (comps) {
            complist = g_slist_append(complist, gint_to_ptr(comps->valueint));
            comps = comps->next;
        }
        get_accr_cards(complist, what, resp);
        g_slist_free(g_steal_pointer (&complist));
    } else if (JSON_OP(movcat)) {
        cJSON *out = cJSON_CreateArray();
        resp->u.json.json = out;

        JSON_GET_LIST(root, comps);
        JSON_GET_STR(root, dest);
        while (comps) {
            struct judoka *j1 = get_data(comps->valueint);
            if (j1) {
                if (db_competitor_match_status(j1->index) & MATCH_EXISTS) {
                    cJSON *msg = cJSON_CreateObject();
                    cJSON_AddStringToObject(msg, "first", j1->first);
                    cJSON_AddStringToObject(msg, "last", j1->last);
                    cJSON_AddStringToObject(msg, "msg", _("Remove drawing first"));
                    cJSON_AddItemToArray(out, msg);
                } else {
                    if (j1->category)
                        g_free((void *)j1->category);
                    j1->category = strdup(dest);
                    display_one_judoka(j1);
                    db_update_judoka(j1->index, j1);
                }
                free_judoka(j1);
            }
            comps = comps->next;
        }
    } else if (JSON_OP(categories)) {
        gint i;
        cJSON *out = cJSON_CreateArray();
        resp->u.json.json = out;

        for (i = 0; i <= number_of_tatamis; i++) {
            struct category_data *cd = category_queue[i].next;
            while (cd) {
                cJSON *catobj = cJSON_CreateObject();
                json_add_category_data(catobj, cd);
                cJSON_AddItemToArray(out, catobj);
                cd = cd->next;
            }
        }
    }

json_end:
    if (j) free_judoka(j);
    cJSON_Delete(root);
    g_atomic_int_set(&resp->ready, ret);
}

void msg_received(struct message *input_msg)
{
    struct message output_msg;
    guint now, i;
    gboolean newentry = FALSE;
    gulong addr = input_msg->src_ip_addr;
    struct judoka *j, j2;
    gchar  buf[16];

#if 0
    if (input_msg->type != MSG_HELLO)
        g_print("msg type = %d from %lx (my addr = %lx)\n", input_msg->type, addr, my_address);
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
            if (input_msg->u.name_req.index & 0xff000000) {
                gint ageix = find_age_index(j->last);
                guint n = (guint)input_msg->u.name_req.index >> 24;
                //g_print("IX=0x%x CAT=%s AGEIX=%d SUB=%d\n", input_msg->u.name_req.index, j->last, ageix, n);
                if (ageix >= 0 && n > 0 && n <= NUM_CAT_DEF_WEIGHTS) {
                    snprintf(output_msg.u.name_info.last, sizeof(output_msg.u.name_info.last),
                             "%s (%s)", j->last, 
                             category_definitions[ageix].weights[n - 1].weighttext);
                }
            } else
                strncpy(output_msg.u.name_info.last, j->last, sizeof(output_msg.u.name_info.last)-1);
            strncpy(output_msg.u.name_info.first, j->first, sizeof(output_msg.u.name_info.first)-1);
            strncpy(output_msg.u.name_info.club, get_club_text(j, CLUB_TEXT_ABBREVIATION),
		    sizeof(output_msg.u.name_info.club)-1);
            send_packet(&output_msg);
            free_judoka(j);
        }
        break;

    case MSG_ALL_REQ:
        g_print("REQUESTING ALL\n");
        for (i = 1; i <= NUM_TATAMIS; i++)
            send_matches(i);
        break;

    case MSG_CANCEL_REST_TIME:
        db_reset_last_match_times(input_msg->u.cancel_rest_time.category,
                                  input_msg->u.cancel_rest_time.number,
                                  input_msg->u.cancel_rest_time.blue,
                                  input_msg->u.cancel_rest_time.white);
        update_matches(input_msg->u.cancel_rest_time.category, (struct compsys){0,0,0,0}, 0);
        break;

    case MSG_EDIT_COMPETITOR:
	if (input_msg->u.edit_competitor.operation == EDIT_OP_GET_BY_ID) {
            j = find_judoka_by_id(input_msg->u.edit_competitor.id);

	    memset(&output_msg, 0, sizeof(output_msg));
	    output_msg.type = MSG_EDIT_COMPETITOR;
	    output_msg.u.edit_competitor.operation = EDIT_OP_GET;

	    if (j) {
                fill_msg_edit_competitor(&output_msg, j);
		free_judoka(j);
	    }

	    send_packet(&output_msg);
	} else if (input_msg->u.edit_competitor.operation == EDIT_OP_SET_WEIGHT) {
            j = get_data(input_msg->u.edit_competitor.index);
	    if (j) {
                j->weight = input_msg->u.edit_competitor.weight;
                j->deleted = input_msg->u.edit_competitor.deleted;
                if (j->visible) {
                    db_update_judoka(j->index, j);
                    make_backup();

                    memset(&output_msg, 0, sizeof(output_msg));
                    output_msg.type = MSG_EDIT_COMPETITOR;
                    output_msg.u.edit_competitor.operation = EDIT_OP_CONFIRM;
                    fill_msg_edit_competitor(&output_msg, j);

                    // find estimated category
                    gchar *estim = find_estim_cat(j);
                    strncpy(output_msg.u.edit_competitor.estim_category,
                            estim ? estim : "", sizeof(output_msg.u.edit_competitor.estim_category)-1);
                    g_free(estim);

                    send_packet(&output_msg);
                }
                display_one_judoka(j);
                free_judoka(j);
            }
	} else if (input_msg->u.edit_competitor.operation == EDIT_OP_SET_JUDOGI) {
            set_judogi_status(input_msg->u.edit_competitor.index, input_msg->u.edit_competitor.matchflags);
	} else if (input_msg->u.edit_competitor.operation == EDIT_OP_SET) {
	    memset(&j2, 0, sizeof(j2));
            fill_judoka_edit_competitor(input_msg, &j2);

	    if (j2.index) { // edit old competitor
		j = get_data(j2.index);
		if (j) {
//		    j2.category = j->category;
		    db_update_judoka(j2.index, &j2);
		    if ((j->deleted & 1) == 0 && (j2.deleted & 1)) {
			GtkTreeIter iter;
			if (find_iter(&iter, j2.index))
			    gtk_tree_store_remove((GtkTreeStore *)current_model, &iter);
		    } else {
			display_one_judoka(&j2);
			update_competitors_categories(j2.index);
		    }
		    free_judoka(j);
		} else if ((j2.deleted & 1) == 0) {
		    j2.category = "?";
		    db_update_judoka(j2.index, &j2);

		    comp_index_set(j2.index);
		    if (j2.index >= current_index)
			current_index = j2.index + 1;

		    display_one_judoka(&j2);

		    avl_set_competitor(j2.index, &j2);
		    //avl_set_competitor_status(j2.index, j2.deleted);
		}
	    } else { // add new competitor
		j2.index = comp_index_get_free();//current_index++;
		j2.category = "?";
		db_add_judoka(j2.index, &j2);
		display_one_judoka(&j2);
		update_competitors_categories(j2.index);
	    }
	}
        break;

    case MSG_SCALE:
        g_snprintf(buf, sizeof(buf), "%d.%02d", input_msg->u.scale.weight/1000, (input_msg->u.scale.weight%1000)/10);
        if (weight_entry)
            gtk_button_set_label(GTK_BUTTON(weight_entry), buf);
        break;

    case MSG_EVENT:
	if (input_msg->u.event.event == MSG_EVENT_SELECT_TAB) {
	    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),
					  input_msg->u.event.tab);
	    if (input_msg->u.event.tab == 0)
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(current_view));
	} else if (input_msg->u.event.event == MSG_EVENT_CLICK_COMP) {
	    GtkTreePath *path;
	    GtkTreeViewColumn *col;
	    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(current_view),
					      input_msg->u.event.x,
					      input_msg->u.event.y,
					      &path, &col, NULL, NULL)) {
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(current_view));
		gtk_tree_view_expand_row(GTK_TREE_VIEW(current_view),
					 path, FALSE);
	    }
	} else if (input_msg->u.event.event == MSG_EVENT_CLICK_SHEET) {
	    GdkEventButton event;
	    event.type = GDK_BUTTON_PRESS;
	    event.button = 1;
	    event.x = input_msg->u.event.x;
	    event.y = input_msg->u.event.y;
	    change_current_page(NULL, &event, NULL);
	}
	break;

    case MSG_WEB: {
	struct msg_web_resp *resp;

	resp = input_msg->u.web.resp;
	resp->request = input_msg->u.web.request;

	if (input_msg->u.web.request == MSG_WEB_GET_COMP_DATA ||
	    input_msg->u.web.request == MSG_WEB_SET_COMP_WEIGHT) {
	    if (input_msg->u.web.request == MSG_WEB_GET_COMP_DATA)
                j = find_judoka_by_id(input_msg->u.web.u.get_comp_data.id);
	    else
                j = find_judoka_by_id(input_msg->u.web.u.set_comp_weight.id);

	    if (!j) {
		g_atomic_int_set(&resp->ready, MSG_WEB_RESP_ERR);
		return;
	    }

	    if (input_msg->u.web.request == MSG_WEB_SET_COMP_WEIGHT) {
		j->weight = input_msg->u.web.u.set_comp_weight.weight;
		db_update_judoka(j->index, j);
		display_one_judoka(j);
	    }

#define CP2WEB_INT(_dst) resp->u.get_comp_data_resp._dst = j->_dst
#define CP2WEB_STR(_dst) strncpy(resp->u.get_comp_data_resp._dst, j->_dst, sizeof(resp->u.get_comp_data_resp._dst)-1)
	    //resp->u.get_comp_data_resp.index = indx;
	    CP2WEB_INT(index);
	    CP2WEB_STR(last);
	    CP2WEB_STR(first);
	    CP2WEB_STR(club);
	    CP2WEB_STR(country);
	    CP2WEB_STR(regcategory);
	    CP2WEB_STR(category);
	    CP2WEB_INT(weight);

	    // find estimated category
            gchar *estim = find_estim_cat(j);
	    strncpy(resp->u.get_comp_data_resp.estim_category,
		    estim ? estim : "", sizeof(resp->u.get_comp_data_resp.estim_category)-1);
	    g_free(estim);

	    free_judoka(j);
	} else 	if (input_msg->u.web.request == MSG_WEB_GET_MATCH_CRC) {
	    for (i = 0; i < NUM_TATAMIS; i++)
		resp->u.get_match_crc_resp.crc[i] = match_crc[i+1];
	} else 	if (input_msg->u.web.request == MSG_WEB_GET_MATCH_INFO) {
	    gint t = input_msg->u.web.u.get_match_info.tatami;
	    if (t < 1 || t > NUM_TATAMIS) {
		g_atomic_int_set(&resp->ready, MSG_WEB_RESP_ERR);
		return;
	    }

	    struct match *m = get_cached_next_matches(t);
	    memset(&resp->u.get_match_info_resp[0], 0,
		   sizeof(resp->u.get_match_info_resp[0]));
	    resp->u.get_match_info_resp[0].tatami = t;
	    resp->u.get_match_info_resp[0].match_category_ix =
		next_matches_info[t-1][0].won_catnum;
	    resp->u.get_match_info_resp[0].match_number =
		next_matches_info[t-1][0].won_matchnum;
	    resp->u.get_match_info_resp[0].comp1 =
		next_matches_info[t-1][0].won_ix;

	    for (i = 0; i < INFO_MATCH_NUM; i++) {
		resp->u.get_match_info_resp[i+1].tatami = t;
		resp->u.get_match_info_resp[i+1].num = i+1;
		resp->u.get_match_info_resp[i+1].match_category_ix = m[i].category;
		resp->u.get_match_info_resp[i+1].match_number = m[i].number;
		resp->u.get_match_info_resp[i+1].comp1 = m[i].blue;
		resp->u.get_match_info_resp[i+1].comp2 = m[i].white;
		resp->u.get_match_info_resp[i+1].round = m[i].round;
	    }
	} else	if (input_msg->u.web.request == MSG_WEB_GET_BRACKET) {
            get_bracket_2(input_msg->u.web.u.get_bracket.tatami,
                input_msg->u.web.u.get_bracket.cat,
                input_msg->u.web.u.get_bracket.svg,
                input_msg->u.web.u.get_bracket.page,
                resp);
	} else 	if (input_msg->u.web.request == MSG_WEB_GET_CAT_INFO) {
	    gint catix = input_msg->u.web.u.get_category_info.catix;
	    struct compsys sys = get_cat_system(catix);
	    resp->u.get_category_info_resp.catix = catix;
	    resp->u.get_category_info_resp.system = sys.system;
	    resp->u.get_category_info_resp.numcomp = sys.numcomp;
	    resp->u.get_category_info_resp.table = sys.table;
	    resp->u.get_category_info_resp.wishsys = sys.wishsys;
	    resp->u.get_category_info_resp.num_pages = num_pages(sys);
	} else 	if (input_msg->u.web.request == MSG_WEB_JSON) {
            handle_json(input_msg);
            return;
	}

	g_atomic_int_set(&resp->ready, MSG_WEB_RESP_OK);
	break;
    }
    case MSG_LANG: {
	char *trans = _(input_msg->u.lang.english);
	memset(&output_msg, 0, sizeof(output_msg));
	output_msg.type = MSG_LANG;
	strcpy(output_msg.u.lang.english, input_msg->u.lang.english);

	if (trans)
	    strncpy(output_msg.u.lang.translation, trans,
		    sizeof(output_msg.u.lang.translation)-1);
	else
	    strncpy(output_msg.u.lang.translation, input_msg->u.lang.english,
		    sizeof(output_msg.u.lang.translation)-1);
	send_packet(&output_msg);
	break;
    }
    case MSG_LOOKUP_COMP: {
	memset(&output_msg, 0, sizeof(output_msg));
	output_msg.type = MSG_LOOKUP_COMP;
	strcpy(output_msg.u.lookup_comp.name, input_msg->u.lookup_comp.name);
	lookup_competitor(&output_msg.u.lookup_comp);
	send_packet(&output_msg);
	break;
    }
    } // switch
}

gint timeout_callback(gpointer data)
{
    int i;

    for (i = 0; i < number_of_tatamis; i++) {
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
    gint tatami = ptr_to_gint(data);

#if (GTKVER == 3)
    tatami_state[tatami-1] = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item));
    sprintf(buf, "tatami%d", tatami);
    g_key_file_set_integer(keyfile, "preferences", buf,
                           gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)));
#else
    tatami_state[tatami-1] = GTK_CHECK_MENU_ITEM(menu_item)->active;
    sprintf(buf, "tatami%d", tatami);
    g_key_file_set_integer(keyfile, "preferences", buf,
                           GTK_CHECK_MENU_ITEM(menu_item)->active)a;
#endif
}


/* Which message is sent to who? Avoid sending unnecessary messages. */
static gboolean send_message_to_application[NUM_MESSAGES][NUM_APPLICATION_TYPES] = {
    // ALL  SHIAI  TIMER  INFO   WEIGHT JUDOGI PROXY  SERVER
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // 0,
    {TRUE,  FALSE, TRUE , FALSE, FALSE, FALSE, FALSE,  TRUE}, // MSG_NEXT_MATCH = 1,
    {TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_RESULT,
    {TRUE,  FALSE, TRUE , FALSE, FALSE, FALSE, FALSE,  TRUE}, // MSG_ACK,
    {TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_SET_COMMENT,
    {TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_SET_POINTS,
    {TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_HELLO,
    {TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_DUMMY,
    {TRUE,  FALSE, FALSE, TRUE , FALSE, TRUE , FALSE,  TRUE}, // MSG_MATCH_INFO,
    {TRUE,  FALSE, FALSE, TRUE , FALSE, TRUE , FALSE,  TRUE}, // MSG_NAME_INFO,
    {TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_NAME_REQ,
    {TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_ALL_REQ,
    {TRUE,  FALSE, FALSE, TRUE , FALSE, TRUE , FALSE,  TRUE}, // MSG_CANCEL_REST_TIME,
    {TRUE,  FALSE, TRUE , FALSE, FALSE, FALSE, FALSE,  TRUE}, // MSG_UPDATE_LABEL,
    {FALSE, FALSE, FALSE, FALSE, TRUE , TRUE , FALSE, FALSE}, // MSG_EDIT_COMPETITOR,
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_SCALE,
    {TRUE,  FALSE, FALSE, TRUE , FALSE, TRUE , FALSE,  TRUE}, // MSG_11_MATCH_INFO,
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_EVENT,
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, // MSG_WEB,
    {TRUE,  FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE}, // MSG_LANG,
    {FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE}, // MSG_LOOKUP_COMP,
    {FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE}, // MSG_INPUT,
    {FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE}, // MSG_LABELS,
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
    SOCKET node_fd, tmp_fd, websock_fd, mcast_in_fd;
    socklen_t alen;
    struct sockaddr_in my_addr, caller;
    gint reuse = 1;
    fd_set read_fd, fds;
    gint xmllen = strlen(xml);
    struct message msg_out;
    gboolean msg_out_ready;

#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    /* Node socket */
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

    /* WebSock socket */
    if ((websock_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("serv socket");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (setsockopt(websock_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt (SO_REUSEADDR)");
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(WEBSOCK_PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(websock_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) {
        perror("websock bind");
        g_print("CANNOT BIND websock!\n");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    /* Multicast out socket */
    if ((mcast_out_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        perror("multicast out socket");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    /* Multicast in socket */
    if ((mcast_in_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        perror("multicast in socket");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (setsockopt(mcast_in_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt (SO_REUSEADDR)");
    }

    struct sockaddr_in mc_addr;
    struct ip_mreq mreq;

    memset(&mc_addr, 0, sizeof(mc_addr));
    mc_addr.sin_family      = AF_INET;
    mc_addr.sin_port        = htons(MULTICAST_PORT_TO_JS);
    mc_addr.sin_addr.s_addr = htonl(MULTICAST_ADDR);
    if (bind(mcast_in_fd, (struct sockaddr *)&mc_addr, sizeof(mc_addr)) == -1) {
	perror("Multicast addr bind");
    }

    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = htonl(MULTICAST_ADDR);
    mreq.imr_interface.s_addr = 0;
    if (setsockopt(mcast_in_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq)) == -1) {
	perror("Multicast setsockopt");
    }

    /***/

    listen(node_fd, 5);
    listen(websock_fd, 5);

    FD_ZERO(&read_fd);
    FD_SET(node_fd, &read_fd);
    FD_SET(websock_fd, &read_fd);
    FD_SET(mcast_in_fd, &read_fd);

    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
        struct timeval timeout;
        gint r, i;

        fds = read_fd;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        r = select(64, &fds, NULL, NULL, &timeout);

        /* messages to send */

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

        if (msg_out_ready) {
	    struct timeval cpu_start, cpu_end;
	    int cpu_diff, ret;
	    gettimeofday(&cpu_start, NULL);

            ws_send_msg(&msg_out);
            
            for (i = 0; i < NUM_CONNECTIONS; i++) {
                if (connections[i].fd == 0)
                    continue;

                // don't send unnecessary messages
                if (msg_out.type < 1 ||
                    msg_out.type >= NUM_MESSAGES ||
                    connections[i].conn_type < 0 ||
                    connections[i].conn_type >= NUM_APPLICATION_TYPES ||
                    (send_message_to_application[(gint)msg_out.type][connections[i].conn_type] == FALSE))
                    continue;

                extern time_t msg_out_start_time;
                extern gulong msg_out_addr;
                msg_out_addr = connections[i].addr;
                msg_out_start_time = time(NULL);

		if (connections[i].websock) {
		    ret = 0;
		    if (connections[i].websock_ok)
			ret = websock_send_msg(connections[i].fd, &msg_out);
		} else
		    ret = send_msg(connections[i].fd, &msg_out, 0);

                if (ret < 0) {
                    perror("sendto");
                    g_print("Node cannot send: conn=%d fd=%d\n", i, connections[i].fd);

		    closesock(connections[i].fd);
		    FD_CLR(connections[i].fd, &read_fd);
		    connections[i].fd = 0;
		    connections[i].ssdp_info[0] = 0;
                }
                msg_out_start_time = 0;
            }

	    send_msg(mcast_out_fd, &msg_out, MULTICAST_PORT_FROM_JS);

	    gettimeofday(&cpu_end, NULL);
	    cpu_diff = cpu_end.tv_usec - cpu_start.tv_usec;
	    if (cpu_diff < 0) cpu_diff += 1000000;
	    /*
	    fprintf(loki, "msg %02d.%03d-%02d.%03d type=%d\r\n",
		    cpu_start.tv_sec%100, cpu_start.tv_usec/1000,
		    cpu_end.tv_sec%100, cpu_end.tv_usec/1000,
		    msg_out.type);
	    fflush(loki);
	    */
        }

        if (r <= 0)
            continue;

        /* messages to receive */

        if (FD_ISSET(node_fd, &fds)) {
            alen = sizeof(caller);
            if ((tmp_fd = accept(node_fd, (struct sockaddr *)&caller, &alen)) < 0) {
                perror("serv accept");
		g_usleep(1000000);
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
                closesock(tmp_fd);
                continue;
            }

            connections[i].fd = tmp_fd;
            connections[i].addr = caller.sin_addr.s_addr;
            connections[i].id = 0;
            connections[i].conn_type = 0;
            connections[i].websock = FALSE;
            connections[i].websock_ok = FALSE;
            g_print("Node: new connection[%d]: fd=%d addr=%s\n",
                    i, (int)tmp_fd, inet_ntoa(caller.sin_addr));
            FD_SET(tmp_fd, &read_fd);
        }

        if (FD_ISSET(websock_fd, &fds)) {
            alen = sizeof(caller);
            if ((tmp_fd = accept(websock_fd, (struct sockaddr *)&caller, &alen)) < 0) {
                perror("websock accept");
		g_print("websock=%d tmpfd=%d\n", (int)websock_fd, (int)tmp_fd);
		g_usleep(1000000);
                continue;
            }

            for (i = 0; i < NUM_CONNECTIONS; i++)
                if (connections[i].fd == 0)
                    break;

            if (i >= NUM_CONNECTIONS) {
                g_print("Node cannot accept new connections!\n");
                closesock(tmp_fd);
                continue;
            }

            connections[i].fd = tmp_fd;
            connections[i].addr = caller.sin_addr.s_addr;
            connections[i].id = 0;
            connections[i].conn_type = 0;
            connections[i].websock = TRUE;
            g_print("Node: new websock connection[%d]: fd=%d addr=%s\n",
                    i, (int)tmp_fd, inet_ntoa(caller.sin_addr));
            FD_SET(tmp_fd, &read_fd);
        }

	if (FD_ISSET(mcast_in_fd, &fds)) {
	    struct message msg;
            r = recv(mcast_in_fd, (void *)&msg, sizeof(msg), 0);
            if (r > 0) {
		put_to_rec_queue(&msg);
	    }
	}

	for (i = 0; i < NUM_CONNECTIONS; i++) {
            static guchar inbuf[2000];

            if (connections[i].fd == 0)
                continue;

            if (!(FD_ISSET(connections[i].fd, &fds)))
                continue;

            r = recv(connections[i].fd, (char *)inbuf, sizeof(inbuf), 0);
            if (r > 0) {
		if (connections[i].websock) {
		    handle_websock(&connections[i], (gchar *)inbuf, r, NULL);
		} else {
		    guchar *p = connections[i].buf;
		    gint j, blen = sizeof(connections[i].buf);
		    struct message msg;

		    if (strncmp((gchar *)inbuf, "<policy-file-request/>", 10) == 0) {
			send(connections[i].fd, xml, xmllen+1, 0);
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
					connections[i].id = msg.sender;
					g_print("Node: conn=%d type=%d id=%08x\n",
						i, connections[i].conn_type, connections[i].id);
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
		}
            } else {
                g_print("Node: connection %d fd=%d closed (r=%d, err=%s)\n",
			i, connections[i].fd, r, strerror(errno));
                closesock(connections[i].fd);
                FD_CLR(connections[i].fd, &read_fd);
                connections[i].fd = 0;
                connections[i].ssdp_info[0] = 0;
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
    socklen_t alen;
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
            perror("serv accept 2");
	    g_usleep(1000000);
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
        closesock(tmp_fd);
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
        closesock(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&server, sizeof(server))) {
        perror("client connect");
        closesock(client_fd);
        fclose(f);
        return -1;
    }

    while ((n = recv(client_fd, buf, sizeof(buf), 0)) > 0) {
        fwrite(buf, 1, n, f);
        ntot += n;
    }

    fclose(f);
    closesock(client_fd);

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
    gint i, line = 0;

    dialog = gtk_dialog_new_with_buttons (_("Connections to this node"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);

#if (GTKVER == 3)
    vbox = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(vbox), 5);
    gtk_grid_set_column_spacing(GTK_GRID(vbox), 5);
#else
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
#endif
    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (connections[i].fd == 0)
            continue;
        gchar *t = "?";
        switch (connections[i].conn_type) {
        case APPLICATION_TYPE_SHIAI: t = "JudoShiai"; break;
        case APPLICATION_TYPE_TIMER: t = "JudoTimer"; break;
        case APPLICATION_TYPE_INFO:  t = "JudoInfo";  break;
        case APPLICATION_TYPE_WEIGHT:t = "JudoWeight";  break;
        case APPLICATION_TYPE_JUDOGI:t = "JudoJudogi";  break;
        case APPLICATION_TYPE_SERVER:t = "JudoServer";  break;
        }

        myaddr = ntohl(connections[i].addr);
        snprintf(addrstr, sizeof(addrstr), "%ld.%ld.%ld.%ld",
                (myaddr>>24)&0xff, (myaddr>>16)&0xff,
                 (myaddr>>8)&0xff, (myaddr)&0xff);
        label = gtk_label_new(addrstr);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_grid_attach(GTK_GRID(vbox), label, 0, line, 1, 1);

        label = gtk_label_new(t);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_grid_attach(GTK_GRID(vbox), label, 1, line, 1, 1);

        label = gtk_label_new(connections[i].ssdp_info);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_grid_attach(GTK_GRID(vbox), label, 2, line++, 1, 1);
    }

    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (noconnections[i].addr == 0)
            continue;

        myaddr = ntohl(noconnections[i].addr);
        snprintf(addrstr, sizeof(addrstr), "%ld.%ld.%ld.%ld",
                (myaddr>>24)&0xff, (myaddr>>16)&0xff,
                 (myaddr>>8)&0xff, (myaddr)&0xff);
        label = gtk_label_new(addrstr);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_grid_attach(GTK_GRID(vbox), label, 0, line, 1, 1);

        if (my_address == noconnections[i].id)
            label = gtk_label_new(_("THIS APPLICATION"));
        else
            label = gtk_label_new(_("NOT CONNECTED"));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_grid_attach(GTK_GRID(vbox), label, 1, line, 1, 1);

        label = gtk_label_new(noconnections[i].ssdp_info);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_grid_attach(GTK_GRID(vbox), label, 2, line++, 1, 1);
    }

#if (GTKVER == 3)
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       vbox, FALSE, FALSE, 0);
#else
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox);
#endif
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

static gint read_all_files(SOCKET s, const gchar *subdirname, gint sendnow, gint level)
{
    gchar *dirname = g_build_filename(installation_dir, subdirname, NULL);
    GDir *dir = g_dir_open(dirname, 0, NULL);
    if (dir) {
	const gchar *fname = g_dir_read_name(dir);
	while (fname) {
	    gchar *fullname = g_build_filename(dirname, fname, NULL);

	    gchar *subname;
	    if (subdirname) subname = g_build_filename(subdirname, fname, NULL);
	    else subname = g_strdup(fname);

	    if (g_file_test(fullname, G_FILE_TEST_IS_DIR)) {
		if (read_all_files(s, subname, sendnow, level+1) < 0)
		    return -1;
	    } else {
		gchar *contents;
		gsize length;
		if (g_file_get_contents(fullname, &contents, &length, NULL)) {
		    gint i, n = 0;
		    gchar buf[256];
		    SHA_CTX context;
		    sha1_byte digest[SHA1_DIGEST_LENGTH];
		    memset(digest, 0, sizeof(digest));
		    gchar *contcopy = g_malloc(length);
		    memcpy(contcopy, contents, length);
		    SHA1_Init(&context);
		    SHA1_Update(&context, (guchar *)contcopy, length);
		    SHA1_Final(digest, &context);
		    g_free(contcopy);

		    gchar *p = "get:";
		    g_print(" %s", p); p = &buf[n];
		    n = snprintf(buf, sizeof(buf), sendnow ? "FILE" : "NAME");
		    buf[n++] = 0;
		    g_print(" %s", p); p = &buf[n];
		    for (i = 0; i < SHA1_DIGEST_LENGTH; i++)
			n += snprintf(buf+n, sizeof(buf)-n, "%02x", digest[i]);
		    buf[n++] = 0;
		    g_print(" %s", p); p = &buf[n];
		    n += snprintf(buf+n, sizeof(buf)-n, "%ld", length);
		    buf[n++] = 0;
		    g_print(" %s", p); p = &buf[n];
		    n += snprintf(buf+n, sizeof(buf)-n, "%s", subname);
		    buf[n++] = 0;
		    g_print(" %s\n", p); p = &buf[n];

		    if (send(s, buf, n, 0) < 0) {
			g_free(contents);
			g_print("auto-update send");
			return -1;
		    }

		    if (sendnow) {
			gchar *p1 = contents;
			while (length) {
			    if ((n = send(s, p1, length, 0)) < 0) {
				g_free(contents);
				g_print("auto-update send 2");
				return -1;
			    }
			    p1 += n;
			    length -= n;
			    if (n == 0) {
				g_print("slow down\n");
				g_usleep(100000);
			    }
			}
		    }

		    if ((n = recv(s, buf, 1, 0)) < 0) {
			g_free(contents);
			g_print("auto-update recv");
			return -1;
		    }

		    g_free(contents);
		} // if g_file_get_contents
	    } // if (g_file_test(fullname != DIR
	    g_free(subname);
            g_free(fullname);
            fname = g_dir_read_name(dir);
	} // while (fname)
        g_dir_close(dir);
    } // if (dir)
    g_free(dirname);

    return 0;
}

gpointer auto_update_thread(gpointer args)
{
    SOCKET serv_fd, tmp_fd;
    socklen_t alen;
    struct sockaddr_in my_addr, caller;
    gint reuse = 1;

    if ((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("auto-update socket");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("auto-update (SO_REUSEADDR)");
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SHIAI_PORT-1);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serv_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) {
        perror("auto-update bind");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (listen(serv_fd, 1) < 0)
        perror("auto-update listen");

    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
        alen = sizeof(caller);
        if ((tmp_fd = accept(serv_fd, (struct sockaddr *)&caller, &alen)) < 0) {
            perror("auto-update accept");
	    g_usleep(1000000);
            continue;
        }

	read_all_files(tmp_fd, NULL, 0, 0);

        closesock(tmp_fd);
    }

    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}

gpointer get_file_thread(gpointer args)
{
    SOCKET serv_fd, tmp_fd;
    socklen_t alen;
    struct sockaddr_in my_addr, caller;
    gint reuse = 1;
    gchar *contents;
    gsize length;

    if ((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("get-file socket");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)) < 0) {
        perror("get-file (SO_REUSEADDR)");
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SHIAI_PORT-2);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serv_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) {
        perror("get-file bind");
        g_thread_exit(NULL);    /* not required just good pratice */
        return NULL;
    }

    if (listen(serv_fd, 1) < 0)
        perror("get-file listen");

    for ( ; *((gboolean *)args) ; )   /* exit loop when flag is cleared */
    {
        gint n;
        static gchar buf[500];

        alen = sizeof(caller);
        if ((tmp_fd = accept(serv_fd, (struct sockaddr *)&caller, &alen)) < 0) {
            perror("get-file accept");
	    g_usleep(1000000);
            continue;
        }

	if ((n = recv(tmp_fd, buf, sizeof(buf), 0)) <= 0) {
            perror("get-file recv");
	    closesock(tmp_fd);
	    continue;
	}

	buf[n] = 0;
	gchar *p = strchr(buf, '\n');
	if (p) *p = 0;
	p = strchr(buf, '\r');
	if (p) *p = 0;

	if (!strcmp(buf, "VER")) {
	    g_print("Get VER\n");
	    p = full_version();
	    send(tmp_fd, p, strlen(p), 0);
	} else if (!strcmp(buf, "ALL")) {
	    g_print("Get ALL\n");
	    read_all_files(tmp_fd, NULL, 1, 0);
	} else {
	    gchar *fname = g_build_filename(installation_dir, buf, NULL);
	    if (g_file_get_contents(fname, &contents, &length, NULL)) {
		p = contents;
		while (length > 0) {
		    n = send(tmp_fd, p, length, 0);
		    if (n <= 0) {
			perror("get-file send");
			break;
		    }
		    length -= n;
		    p += n;
		}
		g_free(contents);
	    } else perror(fname);
	    g_free(fname);
	}

        closesock(tmp_fd);
    }

    g_thread_exit(NULL);    /* not required just good pratice */
    return NULL;
}
