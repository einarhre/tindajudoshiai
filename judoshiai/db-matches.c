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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <assert.h>

#include "sqlite3.h"
#include "judoshiai.h"
#include "minilisp.h"
#include "cJSON.h"

static void db_print_one_match(struct match *m);

//#define TATAMI_DEBUG 1
#ifdef TATAMI_DEBUG
#define DEBUG_INSERT(_t) do { \
	if (m.tatami == TATAMI_DEBUG) {					\
	    mylog("DEBUG LINE %d: %s\n", __LINE__, _t);		\
	    struct category_data *cd = avl_get_category(next_match[i-1].category); \
	    if (cd) \
		mylog("  %d:   %s/%d [%04x]\n", i-1, cd->category,	\
			next_match[i-1].number, next_match[i-1].round); \
	    mylog("  this: %s/%d [%04x]\n", catdata->category, m.number, m.round);	\
	    cd = avl_get_category(next_match[i].category);		\
	    if (cd)							\
		mylog("  %d:   %s/%d [%04x]\n", i, cd->category,	\
			next_match[i].number, next_match[i].round);	\
	}								\
    } while (0)

#define DEBUG_LOG(_f, ...) do {						\
	if (m.tatami == TATAMI_DEBUG) {					\
	    mylog("DEBUG LINE %d: " _f "\n", __LINE__, ##__VA_ARGS__);		\
	}								\
    } while (0)
#else
#define DEBUG_INSERT(_t) do { } while (0)
#define DEBUG_LOG(_f, ...) do { } while (0)
#endif

gboolean auto_arrange = FALSE;
//gboolean use_weights = FALSE;
gint match_crc[NUM_TATAMIS+1];

static gboolean match_row_found;
static gboolean match_found;
static gboolean matched_match_found;
static gboolean unmatched_match_found;
static gboolean real_match_found;
static struct match *category_matches_p;
static struct match m_static;

#define NUM_SAVED_MATCHES 16
static gint num_saved_matches;
static struct match saved_matches[NUM_SAVED_MATCHES];

static struct match *next_match, next_matches[NUM_TATAMIS+1][NEXT_MATCH_NUM];
static struct match matches_waiting[WAITING_MATCH_NUM];
static gint num_matches_waiting = 0;
static gint next_match_num;
static gint next_match_tatami;

static gint match_count, matched_matches_count;
static struct {
    gint match_count, matched_matches_count;
} matches_by_tatami[NUM_TATAMIS+1];

static gint bluecomp, whitecomp, bluepts, whitepts;

static gint team1_wins, team2_wins, no_team_wins, team1_pts, team2_pts;
static gint team1_tot_time, team2_tot_time, team1_matches, team2_matches;
static gint team_matches_total;
static struct match team_last_match;
static gboolean team_extra_exists;
static gchar team_weight_classes[NUM_CAT_DEF_WEIGHTS];
static gint team_num_weight_classes;

/* Use mutex when calling db_next_match(). */
#if (GTKVER == 3)
G_LOCK_DEFINE(next_match_mutex);
#else
GStaticMutex next_match_mutex = G_STATIC_MUTEX_INIT;
#endif

// data for coach info
static struct {
    gint tatami;
    gint waittime;
    gint matchnum;
    gint round;
} coach_info[10000];
static gint current_round = 0;

struct match *get_cached_next_matches(gint tatami)
{
    return next_matches[tatami];
}

void json_add_coach_info(cJSON *c, struct judoka *j)
{
    gint ix = j->index;
    cJSON_AddNumberToObject(c, "tatami", coach_info[ix].tatami);
    cJSON_AddNumberToObject(c, "waittime", coach_info[ix].waittime);
    cJSON_AddNumberToObject(c, "matchnum", coach_info[ix].matchnum);
    cJSON_AddNumberToObject(c, "round", coach_info[ix].round);
}

int lisp_get_next_match(int tatami, int fight)
{
    if (tatami < 1 || tatami > NUM_TATAMIS) return -1;
    if (fight < 1 || fight > NEXT_MATCH_NUM) return -1;

    struct match *m = next_matches[tatami];
    fight--;
    lisp_set_match(m[fight].category, m[fight].number,
		   m[fight].blue, m[fight].white,
		   m[fight].blue_score, m[fight].white_score,
		   m[fight].blue_points, m[fight].white_points,
		   m[fight].match_time, m[fight].comment,
		   m[fight].tatami, m[fight].group, m[fight].flags,
		   m[fight].forcedtatami, m[fight].forcednumber,
		   m[fight].date, m[fight].legend, m[fight].round);
    return 0;
}

#define INSERT_SPACE(x)							\
    do {								\
        gint k; next_match_num++;                                       \
        if (next_match_num > NEXT_MATCH_NUM) next_match_num = NEXT_MATCH_NUM; \
        for (k = next_match_num-1; k > x; k--)                          \
            next_match[k] = next_match[k-1];                            \
    } while (0)

void db_matches_init(void)
{
    gint i, j;

#if (GTKVER == 3)
    g_mutex_init(&G_LOCK_NAME(next_match_mutex));
#endif

    memset(&next_matches, 0, sizeof(next_matches));

    for (i = 0; i <= NUM_TATAMIS; i++)
        for (j = 0; j < NEXT_MATCH_NUM; j++)
            next_matches[i][j].number = INVALID_MATCH;
}

static void change_team_competitors_to_real_persons(gint category, gint number, guint *comp1, guint *comp2)
{
    gboolean changed = FALSE;
    struct judoka *jc = get_data(category);
    if (jc) {
        gchar *wcname = get_weight_class_name(jc->last, number-1);
        free_judoka(jc);
        if (wcname) {
            GtkTreeIter iter;
            struct judoka *jb = get_data(*comp1);
            if (jb) {
                if (find_iter_name_2(&iter, NULL, NULL, jb->last, wcname)) {
                    guint ix;
                    gtk_tree_model_get(current_model, &iter,
                                       COL_INDEX, &ix, -1);
                    *comp1 = ix;
		    changed = TRUE;
                }
                free_judoka(jb);
            } // jb
            struct judoka *jw = get_data(*comp2);
            if (jw) {
                if (find_iter_name_2(&iter, NULL, NULL, jw->last, wcname)) {
                    guint ix;
                    gtk_tree_model_get(current_model, &iter,
                                       COL_INDEX, &ix, -1);
                    *comp2 = ix;
		    changed = TRUE;
                }
                free_judoka(jw);
            } // jw
	    (void)changed;
	    /* not sure if this works
	    if (changed)
		change_competitor_names(category, number, *comp1, *comp2);
	    */
        } // wcname
    } // jc
}

static int db_callback_matches(void *data, int argc, char **argv, char **azColName)
{
    guint flags = ptr_to_gint(data);
    gint i, a, b;
    gchar *p = NULL;
    
    for (i = 0; i < argc; i++) {
        gint val = argv[i] ? atoi(argv[i]) : 0;

        if (IS(category))
            m_static.category = val;
        else if (IS(number))
            m_static.number = val;
        else if (IS(blue))
            m_static.blue = val;
        else if (IS(white))
            m_static.white = val;
        else if (IS(blue_score))
            m_static.blue_score = val;
        else if (IS(white_score))
            m_static.white_score = val;
        else if (IS(blue_points))
            m_static.blue_points = val;
        else if (IS(white_points))
            m_static.white_points = val;
        else if (IS(time))
            m_static.match_time = val;
        else if (IS(comment))
            m_static.comment = val;
        else if (IS(forcedtatami))
            m_static.forcedtatami = val;
        else if (IS(forcednumber))
            m_static.forcednumber = val;
        else if (IS(date))
            m_static.date = val;
        else if (IS(legend))
            m_static.legend = val;
        else if (IS(deleted))
            m_static.deleted = val;
    }

    m_static.round = 0;

    /* Statistics */
    match_row_found = TRUE;

    if ((m_static.deleted & DELETED) == 0)
	match_found = TRUE;

    if ((m_static.deleted & DELETED) == 0 &&
	m_static.blue >= COMPETITOR && m_static.white >= COMPETITOR)
	real_match_found = TRUE;

    if ((m_static.deleted & DELETED) == 0 &&
	(m_static.blue_points || m_static.white_points) &&
	m_static.blue >= COMPETITOR && m_static.white >= COMPETITOR) {
	matched_match_found = TRUE;
	matched_matches_count++;
	matches_by_tatami[m_static.forcedtatami].matched_matches_count++;
    } else if ((m_static.deleted & DELETED) == 0 &&
	       m_static.blue >= COMPETITOR && m_static.white >= COMPETITOR &&
	       m_static.blue_points == 0 && m_static.white_points == 0 &&
	       ((avl_get_competitor_status(m_static.blue) & HANSOKUMAKE) ||
		(avl_get_competitor_status(m_static.white) & HANSOKUMAKE))) {
	matched_matches_count++;
	matches_by_tatami[m_static.forcedtatami].matched_matches_count++;
    }

    if ((m_static.deleted & DELETED) == 0 &&
	m_static.blue >= COMPETITOR && m_static.white >= COMPETITOR &&
	m_static.blue_points == 0 && m_static.white_points == 0 &&
	(avl_get_competitor_status(m_static.blue) & HANSOKUMAKE) == 0 &&
	(avl_get_competitor_status(m_static.white) & HANSOKUMAKE) == 0) {
	unmatched_match_found = TRUE;
    }

    if ((m_static.deleted & DELETED) == 0 &&
	m_static.blue != GHOST && m_static.white != GHOST) {
	match_count++;
	matches_by_tatami[m_static.forcedtatami].match_count++;
    }

    if (bluecomp == 0)
	bluecomp = m_static.blue;
    else if (bluecomp != m_static.blue)
	bluecomp = -1;
    if (whitecomp == 0)
	whitecomp = m_static.white;
    else if (whitecomp != m_static.white)
	whitecomp = -1;
    bluepts += m_static.blue_points;
    whitepts += m_static.white_points;

    /* Operation */
    if (flags & DB_NEXT_MATCH) {
        if (m_static.blue == GHOST || m_static.white == GHOST)
            return 0;

        if (m_static.blue_points || m_static.white_points)
            return 0;

        struct category_data *catdata = avl_get_category(m_static.category);
        struct category_data *catdata1 = NULL;

        if (catdata == NULL)
            return 0;

        gboolean team = (catdata->deleted & TEAM_EVENT) != 0;
	if (team)
	    DEBUG_LOG("TEAM cat=%d, bracket match=%d, weight class match=%d, round=0x%04x",
		      MATCH_CATEGORY_GET(m_static.category),
		      MATCH_CATEGORY_SUB_GET(m_static.category),
		      m_static.number,
		      round_number(catdata, MATCH_CATEGORY_SUB_GET(m_static.category)));

        if (team && (m_static.category & MATCH_CATEGORY_SUB_MASK) == 0)
            return 0;

	if (m_static.forcedtatami)
	    m_static.tatami = m_static.forcedtatami;
	else
	    m_static.tatami = catdata->tatami;

        if (m_static.tatami != next_match_tatami)
            return 0;

        m_static.group = catdata->group;
	if (team)
	    m_static.round = round_number(catdata, MATCH_CATEGORY_SUB_GET(m_static.category));
	else
	    m_static.round = round_number(catdata, m_static.number);

#ifdef TATAMI_DEBUG
        if (m_static.tatami == TATAMI_DEBUG) {
            struct judoka *cat = get_data(m_static.category);
            mylog("\nHANDLE MATCH: %s:%d ix=%d=0x%x comment %d\n",
                    cat ? cat->last : "?", m_static.number, m_static.category, m_static.category, m_static.comment);
            free_judoka(cat);
        }
#endif
        // team event may have named competitors
        if (team) {
            change_team_competitors_to_real_persons(m_static.category, m_static.number, &m_static.blue, &m_static.white);
        } // team

        // coach info
        if (m_static.blue > 0 && m_static.blue < 10000) {
            if (current_round != coach_info[m_static.blue].round) {
                coach_info[m_static.blue].matchnum = 0;
                coach_info[m_static.blue].round = current_round;
                coach_info[m_static.blue].waittime = -1;
            }
            if ((m_static.number < coach_info[m_static.blue].matchnum) || (coach_info[m_static.blue].matchnum == 0)) {
                coach_info[m_static.blue].matchnum = m_static.number;
                coach_info[m_static.blue].tatami = m_static.tatami;
            }
        }
        if (m_static.white > 0 && m_static.white < 10000) {
            if (current_round != coach_info[m_static.white].round) {
                coach_info[m_static.white].matchnum = 0;
                coach_info[m_static.white].round = current_round;
                coach_info[m_static.white].waittime = -1;
            }
            if ((m_static.number < coach_info[m_static.white].matchnum) || (coach_info[m_static.white].matchnum == 0)) {
                coach_info[m_static.white].matchnum = m_static.number;
                coach_info[m_static.white].tatami = m_static.tatami;
            }
        }

        if (m_static.comment == COMMENT_MATCH_1) {
	    DEBUG_LOG("COMMENT 1");
            INSERT_SPACE(0);
            next_match[0] = m_static;
            return 0;
        } else if (m_static.comment == COMMENT_MATCH_2) {
	    DEBUG_LOG("COMMENT 2");
            if (next_match_num > 0 && next_match[0].comment == COMMENT_MATCH_1) {
                INSERT_SPACE(1);
                next_match[1] = m_static;
            } else {
                INSERT_SPACE(0);
                next_match[0] = m_static;
            }
            return 0;
        } else if (m_static.comment == COMMENT_WAIT) {
	    DEBUG_LOG("COMMENT WAIT");
            return 0;
        }

        for (i = 0; i < next_match_num; i++) {
            gboolean insert = FALSE;

            if (next_match[i].comment == COMMENT_MATCH_1 ||
		next_match[i].comment == COMMENT_MATCH_2)
                continue;

            if (m_static.forcednumber) {
                if ((next_match[i].forcednumber == 0) ||
                    (next_match[i].forcednumber > m_static.forcednumber)) {
                    insert = TRUE;
		    goto do_insert;
		}
		continue;
            }

	    if (next_match[i].forcednumber)
		continue;

	    if (next_match[i].group > m_static.group) {
		insert = TRUE;
		goto do_insert;
	    }

	    if (next_match[i].group < m_static.group)
		continue;

	    if (team &&
		(next_match[i].category & MATCH_CATEGORY_MASK) ==
		(m_static.category & MATCH_CATEGORY_MASK)) {
		/* Team category match number */
		if ((next_match[i].category & MATCH_CATEGORY_SUB_MASK) ==
		    (m_static.category & MATCH_CATEGORY_SUB_MASK)) {
		    /* Weight class match */
		    if (next_match[i].number > m_static.number) {
			insert = TRUE;
			DEBUG_INSERT("Insert decision");
			goto do_insert;
		    }
		    continue;
		}
		if ((next_match[i].category & MATCH_CATEGORY_SUB_MASK) >
		    (m_static.category & MATCH_CATEGORY_SUB_MASK)) {
		    insert = TRUE;
		    DEBUG_INSERT("Insert decision");
		    goto do_insert;
		}
		continue;
	    }

	    if ((next_match[i].category & MATCH_CATEGORY_MASK) ==
		(m_static.category & MATCH_CATEGORY_MASK) &&
		next_match[i].number > m_static.number) {
		insert = TRUE;
		DEBUG_INSERT("Insert decision");
		goto do_insert;
	    }

	    /* Matches in round number order. */
	    catdata1 =
		avl_get_category(next_match[i].category);
	    if (!catdata1) continue;

	    a = ROUND_TYPE_NUMBER(m_static.round);
	    b = ROUND_TYPE_NUMBER(next_match[i].round);

	    if (i >= 1 && ROUND_TYPE(m_static.round) == ROUND_ROBIN) {
		if (prop_get_int_val(PROP_MIX_POOL_MATCHES_INTO_ROUNDS)) {
		    if (ROUND_TYPE(next_match[i-1].round) != ROUND_ROBIN &&
			ROUND_TYPE(next_match[i].round) != ROUND_ROBIN) {
			/* put round robins everywhere */
			insert = TRUE;
			DEBUG_INSERT("Insert decision");
			goto do_insert;
		    }
		    continue;
		}
		if (ROUND_TYPE(next_match[i].round) != ROUND_ROBIN) {
		    gint j;
		    gint prev_french_round = 0;
		    /* Find previous french round */
		    for (j = i-1; j >= 0; j--) {
			if (ROUND_TYPE(next_match[j].round) != ROUND_ROBIN) {
			    prev_french_round = ROUND_TYPE_NUMBER(next_match[j].round);
			    break;
			}
		    }
		    DEBUG_LOG("i=%d prev=%x next=%x", i, prev_french_round, ROUND_TYPE_NUMBER(next_match[i].round));
		    DEBUG_LOG("     cat-1=%d cat=%d   rnd-1=%04x rnd=%04x",
			      next_match[i-1].category, next_match[i].category,
			      next_match[i-1].round, m_static.round);

		    if ((prev_french_round != ROUND_TYPE_NUMBER(next_match[i].round) ||
			 next_match[i-1].category != next_match[i].category) &&
			(ROUND_TYPE(next_match[i-1].round) != ROUND_ROBIN ||
			 (ROUND_NUMBER(next_match[i-1].round) ==
			  ROUND_NUMBER(m_static.round)))) {
			/* put round robins after round */
			insert = TRUE;
			DEBUG_INSERT("Insert decision");
			goto do_insert;
		    }
		}
		continue;
	    }

	    if (b > a) {
		if ((next_match[i].category != m_static.category ||
		     (next_match[i].number > m_static.number))) {
		    insert = TRUE;
		    DEBUG_INSERT("Insert decision");
		    goto do_insert;
		}
		continue;
	    }

	    if (b < a)
		continue;

	    /* Lowest weight class first. */
	    p = strrchr(catdata->category, '-');
	    if (p) a = atoi(p+1);
	    else a = 1000;
	    p = strrchr(catdata1->category, '-');
	    if (p) b = atoi(p+1);
	    else b = 1000;

	    if (a < b) {
		insert = TRUE;
		DEBUG_INSERT("Insert decision");
		goto do_insert;
	    }
	    if (a == b &&
		next_match[i].category > m_static.category) {
		insert = TRUE;
		DEBUG_INSERT("Insert decision");
		goto do_insert;
	    }

	do_insert:
            if (insert) {
#ifdef TATAMI_DEBUG
		if (m_static.tatami == TATAMI_DEBUG && i > 0) {
		    mylog("Inserting %d/%d (%s) between %d/%d and %d/%d\n",
			    m_static.category, m_static.number, catdata->category,
			    next_match[i-1].category, next_match[i-1].number,
			    next_match[i].category, next_match[i].number);
		}
#endif
		/* Check that there are no matches with lower number. */
		gint j;
		for (j = i+1; j < next_match_num; j++) {
		    if (next_match[j].category == m_static.category &&
			next_match[j].number < m_static.number &&
			m_static.forcednumber == 0) {
			/* Cannot insert here. */
			i = j;
			DEBUG_LOG("Cannot insert");
			break;
		    }
		}

		/* OK to insert */
		if (j == next_match_num) {
		    INSERT_SPACE(i);
		    next_match[i] = m_static;
		    return 0;
		}
            }
        } /* for i */

        if (next_match_num < NEXT_MATCH_NUM) {
	    DEBUG_INSERT("Add last");
            next_match[next_match_num++] = m_static;
        }

        return 0;
    } else if ((flags & ADD_MATCH) && ((m_static.deleted & DELETED) == 0)) {
        set_match(&m_static);
    } else if ((flags & SAVE_MATCH) && ((m_static.deleted & DELETED) == 0)) {
        if (num_saved_matches < NUM_SAVED_MATCHES) {
            saved_matches[num_saved_matches++] = m_static;
        }
    } else if ((flags & DB_REMOVE_COMMENT) && ((m_static.deleted & DELETED) == 0)) {
        m_static.comment = COMMENT_EMPTY;
        set_match(&m_static);
    } else if ((flags & DB_READ_CATEGORY_MATCHES) && ((m_static.deleted & DELETED) == 0)) {
        category_matches_p[m_static.number] = m_static;
    } else if (flags & DB_MATCH_WAITING) {
        if (num_matches_waiting < WAITING_MATCH_NUM)
            matches_waiting[num_matches_waiting++] = m_static;
    } else if (flags & DB_UPDATE_LAST_MATCH_TIME) {
        if (m_static.blue > COMPETITOR)
            avl_set_competitor_last_match_time(m_static.blue);
        if (m_static.white > COMPETITOR)
            avl_set_competitor_last_match_time(m_static.white);
    } else if ((flags & DB_RESET_LAST_MATCH_TIME_B) || (flags & DB_RESET_LAST_MATCH_TIME_W)) {
        if (m_static.blue > COMPETITOR && (flags & DB_RESET_LAST_MATCH_TIME_B))
            avl_reset_competitor_last_match_time(m_static.blue);
        if (m_static.white > COMPETITOR && (flags & DB_RESET_LAST_MATCH_TIME_W))
            avl_reset_competitor_last_match_time(m_static.white);
    } else if (flags & DB_FIND_TEAM_WINNER) {
	if (m_static.blue_points != 11 && m_static.white_points != 11)
	    team_matches_total++; // do not count draws

	// find possible extra matches
	if ((m_static.blue_points || m_static.white_points) &&  // match has been fought
	    m_static.blue_points != 11 &&                       // not hikiwake == no competitors
	    m_static.white_points != 11 &&
	    m_static.number != 999 &&                           // skip extra match
	    (prop_get_int_val(PROP_TEAM_EVENT_DRAW_IF_ONE_COMP_MISSING) || // one competitor can be missing
	     m_static.match_time)) {                            // real match with two competitors
	    if (team_num_weight_classes < NUM_CAT_DEF_WEIGHTS)
		team_weight_classes[team_num_weight_classes++] = m_static.number;
	}

	if (m_static.blue_points && m_static.white_points == 0) {
            team1_wins++;
	    if (m_static.number != 999) {
		team1_pts += get_points_gint(m_static.blue_points, m_static.category);
		if (m_static.match_time) {
		    team1_tot_time += m_static.match_time;
		    team1_matches++;
		}
	    }
        }

        if (m_static.white_points && m_static.blue_points == 0) {
            team2_wins++;
	    if (m_static.number != 999) {
		team2_pts += get_points_gint(m_static.white_points, m_static.category);
		if (m_static.match_time) {
		    team2_tot_time += m_static.match_time;
		    team2_matches++;
		}
	    }
        }

        if (m_static.blue_points == 0 && m_static.white_points == 0)
	    no_team_wins++;

        if (m_static.blue > COMPETITOR && m_static.white > COMPETITOR)
	    team_last_match = m_static;

	if (m_static.number == 999)
	    team_extra_exists = TRUE;
    } else if (flags & DB_PRINT_CAT_MATCHES) {
        db_print_one_match(&m_static);
    }

    return 0;
}

void db_set_forced_tatami(gint tatami, gint category, gint number)
{
    /* Don't change tatami if forcednumber is set. */
    db_exec_str(NULL, NULL,
		"UPDATE matches SET \"forcedtatami\"=%d, \"comment\"=%d "
		"WHERE \"forcednumber\"=0 AND ("
		"(\"category\"=%d AND \"number\"=%d) OR "
		"\"category\"=%d)",
		tatami, COMMENT_EMPTY,
		category, number,
		category | (number << MATCH_CATEGORY_SUB_SHIFT));
}

void db_freeze_matches(gint tatami, gint category, gint number, gint arg)
{
    gint i, num = 1;

    switch (arg) {
    case FREEZE_MATCHES:
        for (i = 0; i < NEXT_MATCH_NUM; i++) {
            if (next_matches[tatami][i].number == INVALID_MATCH)
                continue;

            db_exec_str(NULL, NULL,
                        "UPDATE matches SET \"forcedtatami\"=%d, "
                        "\"forcednumber\"=%d "
                        "WHERE \"category\"&%u=%d AND \"number\"=%d",
                        tatami, num++,
                        ~MATCH_CATEGORY_CAT_MASK,
                        next_matches[tatami][i].category,
                        next_matches[tatami][i].number);
        }
        db_read_matches();
        update_matches(0, (struct compsys){0,0,0,0}, tatami);
        break;
    case UNFREEZE_EXPORTED:
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"forcedtatami\"=0, \"forcednumber\"=0, "
                    "\"comment\"=%d WHERE EXISTS(SELECT * FROM categories "
                    "WHERE categories.\"tatami\"=%d "
                    "AND categories.\"index\"=matches.\"category\"&%d)",
                    COMMENT_EMPTY, tatami, MATCH_CATEGORY_MASK);
        db_read_matches();
        for (i = 1; i <= number_of_tatamis; i++)
            update_matches(0, (struct compsys){0,0,0,0}, i);
	update_category_status_info_all();
        break;
    case UNFREEZE_IMPORTED:
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"forcedtatami\"=0, \"forcednumber\"=0, "
                    "\"comment\"=%d WHERE \"forcedtatami\"=%d",
                    COMMENT_EMPTY, tatami);
        db_read_matches();
        for (i = 1; i <= number_of_tatamis; i++)
            update_matches(0, (struct compsys){0,0,0,0}, i);
	update_category_status_info_all();
        break;
    case UNFREEZE_THIS:
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"forcedtatami\"=0, \"forcednumber\"=0, "
                    "\"comment\"=%d WHERE \"category\"&%u=%d AND \"number\"=%d",
                    COMMENT_EMPTY, ~MATCH_CATEGORY_CAT_MASK, category, number);
        db_read_match(category, number);
        update_matches(category, (struct compsys){0,0,0,0}, 0);
        update_matches(0, (struct compsys){0,0,0,0}, tatami);
	update_category_status_info_all();
        break;
    case UNFREEZE_THESE:
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"forcedtatami\"=0, \"forcednumber\"=0, "
                    "\"comment\"=%d WHERE \"category\"&%u=%d AND \"forcedtatami\"=%d",
                    COMMENT_EMPTY, ~MATCH_CATEGORY_CAT_MASK, category, tatami);
        db_read_matches();
        for (i = 1; i <= number_of_tatamis; i++)
            update_matches(0, (struct compsys){0,0,0,0}, i);
	update_category_status_info_all();
        break;
    }
}

static gboolean db_match_ready(gint cat, gint num)
{
    match_found = FALSE;

    db_exec_str(NULL, db_callback_matches,
                "SELECT * from matches WHERE \"category\"&%u=%d AND \"number\"=%d "
                "AND \"blue\">%d AND \"white\">%d",
                ~MATCH_CATEGORY_CAT_MASK, cat, num, COMPETITOR, COMPETITOR);

    return match_found;
}

static void db_set_comm(gint comm, gint cat, gint num)
{
    if (comm == COMMENT_MATCH_1 &&
        db_match_ready(cat, num) == FALSE)
        comm = COMMENT_EMPTY;

    db_exec_str(NULL, NULL,
                "UPDATE matches SET \"comment\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                comm, ~MATCH_CATEGORY_CAT_MASK, cat, num);

    db_read_match(cat, num);
}

void db_change_freezed(gint category, gint number,
		       gint tatami, gint position, gboolean after)
{
    gint comment = COMMENT_EMPTY, i;
    //gint default_tatami = db_get_tatami(category);
    gint current_tatami = db_find_match_tatami(category, number);
    gint forcedmax = 0;

    for (i = 0; i < NEXT_MATCH_NUM && i < position - (after ? 0 : 1); i++) {
        if (next_matches[tatami][i].forcednumber > forcedmax)
            forcedmax = next_matches[tatami][i].forcednumber;
    }
    forcedmax++;

    if (tatami == 0) {
        /* delay match */
        comment = COMMENT_WAIT;
    } else if (position == 0 || (position == 1 && after == FALSE)) {
        /* first match */
        comment = COMMENT_MATCH_1;
    } else if ((position == 1 && after == TRUE) || (position == 2 && after == FALSE)) {
        /* second match */
        comment = COMMENT_MATCH_2;
    }
#if 0
    if (tatami && comment == COMMENT_MATCH_1 &&
        db_match_ready(category, number) == FALSE)
        return;
#endif
    if (tatami) {
        db_exec_str(NULL, NULL, "UPDATE matches SET \"forcednumber\"=\"forcednumber\"+1 "
                    "WHERE \"forcednumber\">=%d AND \"forcedtatami\"=%d",
                    forcedmax, tatami);
    }

    db_exec_str(NULL, NULL, "UPDATE matches SET \"forcedtatami\"=%d, \"forcednumber\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                tatami, tatami ? forcedmax : 0, ~MATCH_CATEGORY_CAT_MASK, category, number);

#ifdef TATAMI_DEBUG
    mylog("%s: %d:%d, move to T%d P%d (after=%d), comment to %d\n",
            __FUNCTION__, category, number,
            tatami, position, after, comment);
#endif

    if (tatami && current_tatami == tatami) {
        gint current_pos;

        for (current_pos = 0;
             current_pos < NEXT_MATCH_NUM;
             current_pos++)
            if (next_matches[tatami][current_pos].category == category &&
                next_matches[tatami][current_pos].number == number)
                break;

        if (current_pos == 0) {
            db_set_comm(COMMENT_MATCH_1,
                        next_matches[tatami][1].category,
                        next_matches[tatami][1].number);

            if (comment != COMMENT_MATCH_2) {
                db_set_comm(COMMENT_MATCH_2,
                            next_matches[tatami][2].category,
                            next_matches[tatami][2].number);
            }
        } else if (current_pos == 1) {
            if (comment == COMMENT_MATCH_1) {
                db_set_comm(COMMENT_MATCH_2,
                            next_matches[tatami][0].category,
                            next_matches[tatami][0].number);
            } else {
                db_set_comm(COMMENT_MATCH_2,
                            next_matches[tatami][2].category,
                            next_matches[tatami][2].number);
            }
        } else {
            if (comment == COMMENT_MATCH_1 || comment == COMMENT_MATCH_2) {
                db_set_comm(COMMENT_EMPTY,
                            next_matches[tatami][1].category,
                            next_matches[tatami][1].number);
            }
            if (comment == COMMENT_MATCH_1) {
                db_set_comm(COMMENT_MATCH_2,
                            next_matches[tatami][0].category,
                            next_matches[tatami][0].number);
            }
        }
    } else if (tatami) {
        if (comment == COMMENT_MATCH_1 || comment == COMMENT_MATCH_2) {
            db_set_comm(COMMENT_EMPTY,
                        next_matches[tatami][1].category,
                        next_matches[tatami][1].number);
        }
        if (comment == COMMENT_MATCH_1) {
            db_set_comm(COMMENT_MATCH_2,
                        next_matches[tatami][0].category,
                        next_matches[tatami][0].number);
        }
    }


    db_set_comment(category, number, comment);
    //db_set_comm(comment, category, number);

    update_matches(0, (struct compsys){0,0,0,0}, current_tatami);
    if (tatami && tatami != current_tatami)
        update_matches(0, (struct compsys){0,0,0,0}, tatami);
}

void db_add_match(struct match *m)
{
    gchar buffer[400];

    assert(m->blue < 10000);
    assert(m->white < 10000);

    sprintf(buffer, "INSERT INTO matches VALUES ("
            VALINT ", " VALINT ", " VALINT ", " VALINT ", "
            VALINT ", " VALINT ", "
            VALINT ", " VALINT ", " VALINT ", " VALINT ", " VALINT ", "
            VALINT ", " VALINT ", " VALINT ", " VALINT " )",
            m->category, m->number,
            m->blue, m->white, m->blue_score, m->white_score,
            m->blue_points, m->white_points,
            m->match_time, m->comment, m->deleted, 0, 0, m->date, m->legend);

    //mylog("%s\n", buffer);
    db_exec(db_name, buffer, NULL, db_callback_matches);
}

void db_update_match(struct match *m)
{
    gchar buffer[400];

    assert(m->blue < 10000);
    assert(m->white < 10000);

    sprintf(buffer, "UPDATE matches SET "
            VARVAL(blue, %d) ", "
            VARVAL(white, %d) ", "
            VARVAL(blue_score, %d) ", "
            VARVAL(white_score, %d) ", "
            VARVAL(blue_points, %d) ", "
            VARVAL(white_points, %d) ", "
            VARVAL(deleted, %d) ", "
            VARVAL(time, %d) " WHERE "
            VARVAL(category, %d) " AND "
            VARVAL(number, %d),
            m->blue, m->white, m->blue_score, m->white_score,
            m->blue_points, m->white_points, m->deleted,
            m->match_time, m->category, m->number);

    //mylog("%s\n", buffer);
    db_exec(db_name, buffer, NULL, db_callback_matches);
}

gboolean db_match_exists(gint category, gint number, gint flags)
{
    gchar buffer[100];

    sprintf(buffer, "SELECT * FROM matches WHERE "
            VARVAL(category, %d) " AND "
            VARVAL(number, %d),
            category, number);

    match_found = match_row_found = FALSE;
    db_exec(db_name, buffer, NULL, db_callback_matches);

    return (flags & DB_MATCH_ROW) ? match_row_found : match_found;
}

struct match *db_get_match_data(gint category, gint number)
{
    if (db_match_exists(category, number, 0))
        return &m_static;
    return NULL;
}

gboolean db_matched_matches_exist(gint category)
{
    gchar buffer[100];

    sprintf(buffer, "SELECT * FROM matches WHERE "
            VARVAL(category, %d), category);

    match_found = match_row_found = matched_match_found = FALSE;
    db_exec(db_name, buffer, NULL, db_callback_matches);

    return matched_match_found;
}


gint db_category_get_match_status(gint category)
{
    struct category_data *catdata = avl_get_category(category);
    if (catdata)
	return catdata->match_status;
    return db_category_set_match_status(category);
}

gint db_category_set_match_status(gint category)
{
    gchar buffer[100];
    gint res = 0;

    match_count = matched_matches_count = bluecomp = whitecomp = bluepts = whitepts = 0;
    match_found = match_row_found = matched_match_found = unmatched_match_found = real_match_found = FALSE;
    memset(matches_by_tatami, 0, sizeof(matches_by_tatami));

    sprintf(buffer, "SELECT * FROM matches WHERE "
            VARVAL(category, %d), category);
    db_exec(db_name, buffer, NULL, db_callback_matches);

    if (matched_matches_count > match_count) {
        mylog("%s[%d]: ERROR %d %d %d\n", __FUNCTION__, __LINE__,
                category, matched_matches_count, match_count);
        matched_matches_count = match_count;
    }

    if (match_found)
        res |= MATCH_EXISTS;
    if (matched_match_found)
        res |= MATCH_MATCHED;
    if (real_match_found)
        res |= REAL_MATCH_EXISTS;

    if (match_count == 3 && matched_matches_count == 2 &&
        bluecomp > 0 && whitecomp > 0 &&
        ((bluepts && whitepts == 0) || (bluepts == 0 && whitepts))) {
        /* two's pool with three matches completed after two matches */
    } else if (unmatched_match_found)
        res |= MATCH_UNMATCHED;

    struct category_data *catdata = avl_get_category(category);
    if (catdata) {
	gint i;
        if (catdata->system.system)
            res |= SYSTEM_DEFINED;
        catdata->match_status = res;
        catdata->by_tatami[0].match_count = catdata->match_count = match_count;
        catdata->by_tatami[0].matched_matches_count =
	    catdata->matched_matches_count = matched_matches_count;
	for (i = 1; i <= NUM_TATAMIS; i++) {
	    catdata->by_tatami[i].match_count = matches_by_tatami[i].match_count;
	    catdata->by_tatami[i].matched_matches_count = matches_by_tatami[i].matched_matches_count;
	    /* pos 0 contains count for default tatami */
	    catdata->by_tatami[0].match_count -= catdata->by_tatami[i].match_count;
	    catdata->by_tatami[0].matched_matches_count -= catdata->by_tatami[i].matched_matches_count;
	}

        if (find_age_index(catdata->category) < 0)
            catdata->defined = FALSE;
        else
            catdata->defined = TRUE;

        // data for coach
        if (automatic_web_page_update) {
            gint n = 0;
            gchar buf[80];

            n = sprintf(buf, "c-");
            for (i = 0; i < (sizeof(catdata->category)-1) && catdata->category[i]; i++)
                n += sprintf(buf + n, "%02x", (guchar)catdata->category[i]);
            n += sprintf(buf + n, ".txt");

            gchar *file = g_build_filename(current_directory, buf, NULL);
            FILE *f = g_fopen(file, "wb");
            g_free(file);
            if (f) {
                fprintf(f, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
                        catdata->index, catdata->tatami, catdata->group,
                        catdata->system.system, catdata->system.numcomp, catdata->system.table, catdata->system.wishsys,
                        catdata->match_status, catdata->match_count, catdata->matched_matches_count);
                fclose(f);
            }
        }
    }

    return res;
}

gint db_competitor_match_status(gint competitor)
{
    gchar buffer[100];
    gint res = 0;

    sprintf(buffer, "SELECT * FROM matches WHERE \"blue\"=%d OR \"white\"=%d",
            competitor, competitor);

    match_found = match_row_found = matched_match_found = unmatched_match_found = FALSE;
    db_exec(db_name, buffer, NULL, db_callback_matches);

    if (match_found)
        res |= MATCH_EXISTS;
    if (matched_match_found)
        res |= MATCH_MATCHED;
    if (unmatched_match_found)
        res |= MATCH_UNMATCHED;

    return res;
}


void db_set_match(struct match *m1)
{
    memset(&m_static, 0, sizeof(m_static));

    if (db_match_exists(m1->category, m1->number, DB_MATCH_ROW)) {
        if (m_static.blue != m1->blue ||
            m_static.white != m1->white ||
            m_static.blue_score != m1->blue_score ||
            m_static.white_score != m1->white_score ||
            m_static.blue_points != m1->blue_points ||
            m_static.white_points != m1->white_points ||
            m_static.deleted != m1->deleted ||
            m_static.category != m1->category ||
            m_static.number != m1->number ||
            m_static.match_time != m1->match_time ||
            m_static.comment != m1->comment ||
            m_static.forcedtatami != m1->forcedtatami ||
            m_static.forcednumber != m1->forcednumber ||
            m_static.date != m1->date ||
            m_static.legend != m1->legend) {
            db_update_match(m1);

            // is this a team event?
            struct category_data *cat = avl_get_category(m1->category);
            if (cat && (cat->deleted & TEAM_EVENT) && (m1->category & MATCH_CATEGORY_SUB_MASK) == 0) {
                db_exec_str(NULL, NULL, "UPDATE matches SET \"blue\"=%d "
                            "WHERE \"category\"&%d=%d AND \"blue\"=0",
                            m1->blue, MATCH_CATEGORY_MASK | MATCH_CATEGORY_SUB_MASK,
                            m1->category | (m1->number << MATCH_CATEGORY_SUB_SHIFT));
                db_exec_str(NULL, NULL, "UPDATE matches SET \"white\"=%d "
                            "WHERE \"category\"&%d=%d AND \"white\"=0",
                            m1->white, MATCH_CATEGORY_MASK | MATCH_CATEGORY_SUB_MASK,
                            m1->category | (m1->number << MATCH_CATEGORY_SUB_SHIFT));
            }
        }
    } else {
        db_add_match(m1);

        // is this a team event?
        struct category_data *cat = avl_get_category(m1->category);
        if (cat && (cat->deleted & TEAM_EVENT) &&
	    (m1->category & MATCH_CATEGORY_SUB_MASK) == 0) {
            struct match subm;
            memset(&subm, 0, sizeof(subm));
            subm.category = m1->category | (m1->number << MATCH_CATEGORY_SUB_SHIFT);
            gint i = find_age_index(cat->category);
            if (i >= 0) {
                gint j;
                for (j = 0; j < NUM_CAT_DEF_WEIGHTS && category_definitions[i].weights[j].weighttext[0]; j++) {
                    subm.number = j + 1;
                    subm.blue = m1->blue;
                    subm.white = m1->white;
                    //change_team_competitors_to_real_persons(subm.category, j+1, &subm.blue, &subm.white);
                    db_add_match(&subm);
                }
            }
        }
    }

    // clear coach info
    if (m1->blue > 0 && m1->blue < 10000) {
        coach_info[m1->blue].tatami = 0;
        coach_info[m1->blue].waittime = -1;
        coach_info[m1->blue].matchnum = 0;
    }
    if (m1->white > 0 && m1->white < 10000) {
        coach_info[m1->white].tatami = 0;
        coach_info[m1->white].waittime = -1;
        coach_info[m1->white].matchnum = 0;
    }
}

void db_read_category_matches(gint category, struct match *m)
{
    gchar buffer[100];

    sprintf(buffer, "SELECT * FROM matches WHERE \"category\"&%u=%d", ~MATCH_CATEGORY_CAT_MASK, category);

    category_matches_p = m;
    db_exec(db_name, buffer, (gpointer)DB_READ_CATEGORY_MATCHES, db_callback_matches);
}

void db_read_matches(void)
{
    db_exec(db_name, "SELECT * FROM matches",
            (gpointer)ADD_MATCH, db_callback_matches);
}

void db_remove_matches(guint category)
{
    gchar buffer[256];

    sprintf(buffer, "DELETE FROM matches WHERE \"category\"&%d=%d", MATCH_CATEGORY_MASK, category);

    db_exec(db_name, buffer, NULL, db_callback_matches);
}

void db_set_points(gint category, gint number, gint minutes,
                   gint blue, gint white, gint blue_score, gint white_score, gint legend)
{
    gchar buffer[256];

    if (blue_score || white_score || minutes) // full information available
        sprintf(buffer,
                "UPDATE matches SET \"blue_points\"=%d, \"white_points\"=%d, "
                "\"blue_score\"=%d, \"white_score\"=%d, \"time\"=%d, \"date\"=%ld, \"legend\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                blue, white, blue_score, white_score, minutes, time(NULL), legend,
                ~MATCH_CATEGORY_CAT_MASK, category, number);
    else // correct points but leave other info as is
        sprintf(buffer,
                "UPDATE matches SET \"blue_points\"=%d, \"white_points\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                blue, white, ~MATCH_CATEGORY_CAT_MASK, category, number);

    db_exec(db_name, buffer, NULL, db_callback_matches);

    sprintf(buffer,
            "SELECT * FROM matches "
            "WHERE \"category\"&%u=%d AND \"number\"=%d",
            ~MATCH_CATEGORY_CAT_MASK, category, number);

    db_exec(db_name, buffer, (gpointer)DB_UPDATE_LAST_MATCH_TIME, db_callback_matches);

    if (blue == 0 && white == 0) {
        sprintf(buffer,
                "UPDATE matches SET \"blue\"=0 "
                "WHERE \"category\"&%u=%d AND \"number\"=%d AND \"blue\"=%d",
                ~MATCH_CATEGORY_CAT_MASK, category, number, GHOST);
        db_exec(db_name, buffer, NULL, db_callback_matches);
        sprintf(buffer,
                "UPDATE matches SET \"white\"=0 "
                "WHERE \"category\"&%u=%d AND \"number\"=%d AND \"white\"=%d",
                ~MATCH_CATEGORY_CAT_MASK, category, number, GHOST);
        db_exec(db_name, buffer, NULL, db_callback_matches);
    }
}

gint db_set_score(gint category, gint number, gint score, gboolean is_blue, gboolean hikiwake)
{
    gint maxshido = 4;
    gint winscore = 0, losescore = 0;
    gint points = 0;
    gint blue_score, white_score, blue_points, white_points;

    if (prop_get_int_val_cat(PROP_RULES_2017, category) ||
	prop_get_int_val_cat(PROP_RULES_2018, category))
	maxshido = 3;

    // find current match data
    db_exec_str(NULL, db_callback_matches,
                "SELECT * FROM matches "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                ~MATCH_CATEGORY_CAT_MASK, category, number);

    blue_score = m_static.blue_score;
    white_score = m_static.white_score;
    blue_points = m_static.blue_points;
    white_points = m_static.white_points;

    if (is_blue) blue_score = score & 0xfffff;
    else white_score = score & 0xfffff;

    if (hikiwake) {
	blue_points = white_points = 11;
    } else {
	if ((blue_score & 7) >= maxshido) {
	    blue_score &= 0xffff;
	    white_score |= 0x10000;
	} else if ((white_score & 7) >= maxshido) {
	    white_score &= 0xffff;
	    blue_score |= 0x10000;
	}

	if ((blue_score & 0xffff0) > (white_score & 0xffff0)) {
	    winscore = blue_score & 0xffff0;
	    losescore = white_score & 0xffff0;
	} else if ((blue_score & 0xffff0) < (white_score & 0xffff0)) {
	    winscore = white_score & 0xffff0;
	    losescore = blue_score & 0xffff0;
	} else if ((blue_score & 0x7) != (white_score & 0x7)) {
	    if ((blue_score & 0xf) > (white_score & 0xf)) {
		winscore = white_score;
		losescore = blue_score;
	    } else {
		winscore = blue_score;
		losescore = white_score;
	    }
	}

	if ((winscore & 0xffff0) != (losescore & 0xffff0)) {
	    if ((winscore & 0xf0000) && (losescore & 0xf0000) == 0) points = 10;
	    else if ((winscore & 0xf000) > (losescore & 0xf000)) points = 7;
	    else if ((winscore & 0xf00) > (losescore & 0xf00)) points = 5;
	    else if ((winscore & 0xf0) > (losescore & 0xf0)) points = 3;

	    // After golden score only one point to winner if 20th bit set (Slovakia).
	    if (prop_get_int_val_cat(PROP_GS_WIN_GIVES_1_POINT, category) &&
		(score & 0x100000))
		points = 2;

	    if ((blue_score) > (white_score)) {
		blue_points = points;
		white_points = 0;
	    } else {
		blue_points = 0;
		white_points = points;
	    }
	} else if (winscore != losescore) {
	    if ((blue_score & 0xf) > (white_score & 0xf)) {
		blue_points = 0;
		white_points = 1;
	    } else {
		blue_points = 1;
		white_points = 0;
	    }
	} else {
	    blue_points = white_points = 0;
	}
    }

    db_exec_str(NULL, db_callback_matches,
                "UPDATE matches SET \"blue_score\"=%d, \"white_score\"=%d, "
		"\"blue_points\"=%d, \"white_points\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
		blue_score, white_score,
		blue_points, white_points,
                ~MATCH_CATEGORY_CAT_MASK, category, number);

    return (blue_points << 8) | white_points;
}

void db_set_time(gint category, gint number, gint tim)
{
    db_exec_str(NULL, db_callback_matches,
                "UPDATE matches SET \"time\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                tim, ~MATCH_CATEGORY_CAT_MASK, category, number);
}

void db_set_legend(gint category, gint number, gint legend)
{
    db_exec_str(NULL, db_callback_matches,
                "UPDATE matches SET \"legend\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                legend, ~MATCH_CATEGORY_CAT_MASK, category, number);
}

void db_reset_last_match_times(gint category, gint number, gboolean blue, gboolean white)
{
    gint x = 0;
    if (blue)
        x = DB_RESET_LAST_MATCH_TIME_B;
    if (white)
        x |= DB_RESET_LAST_MATCH_TIME_W;

    db_exec_str(gint_to_ptr(x), db_callback_matches,
                "SELECT * FROM matches "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                ~MATCH_CATEGORY_CAT_MASK, category, number);
}

void db_read_match(gint category, gint number)
{
    gchar buffer[200];

    sprintf(buffer, "SELECT * FROM matches WHERE \"category\"&%u=%d AND \"number\"=%d",
            ~MATCH_CATEGORY_CAT_MASK, category, number);

    db_exec(db_name, buffer, (gpointer)ADD_MATCH, db_callback_matches);
}

void db_read_matches_of_category(gint category)
{
    gchar buffer[200];

    sprintf(buffer, "SELECT * FROM matches WHERE \"category\"&%u=%d",
            MATCH_CATEGORY_MASK, category);

    db_exec(db_name, buffer, (gpointer)ADD_MATCH, db_callback_matches);
}

static gint competitor_cannot_match(gint index, gint tatami, gint num)
{
    if (index == 0)
        return 0;

    gint i, j, result = 0;
    gint hash = avl_get_competitor_hash(index);
    time_t last_time = avl_get_competitor_last_match_time(index);
    gint rest_time = 180;
    struct category_data *catdata =
        avl_get_category(next_matches[tatami][num].category);

    if (catdata)
        rest_time = catdata->rest_time;

    if (time(NULL) < last_time + rest_time) {
        //mylog("T%d:%d rested %ld s\n", tatami, num, time(NULL) - last_time);
        return MATCH_FLAG_REST_TIME;
    }

    for (i = 1; i <= NUM_TATAMIS; i++) {
        for (j = 0; j <= 1; j++) {
            if (i == tatami && j == num)
                continue;
            if (next_matches[i][j].number >= INVALID_MATCH)
                continue;

            result = 0;
            if (next_matches[i][j].blue &&
                (hash == avl_get_competitor_hash(next_matches[i][j].blue)))
                result |= MATCH_FLAG_BLUE_DELAYED;
            if (next_matches[i][j].white &&
                (hash == avl_get_competitor_hash(next_matches[i][j].white)))
                result |= MATCH_FLAG_WHITE_DELAYED;

            if (result) {
                //mylog("T%d:%d matching in T%d:%d\n", tatami, num, i, j);
                return result;
            }
        }
    }

    return 0;
}

struct match *db_next_match(gint category, gint tatami)
{
    gint i;

    PROF_START;
    current_round++;

    if (category) {
        tatami = db_get_tatami(category);
        /*** BUG
             sprintf(buffer, "SELECT * FROM categories WHERE \"category\"=%d", category);
             db_exec(db_name, buffer, (gpointer)DB_GET_SYSTEM, db_callback_categories);
             tatami = j.belt;
        ***/
    }

    if (tatami == 0)
        return NULL;

    next_match = next_matches[tatami];

    for (i = 0; i < NEXT_MATCH_NUM; i++) {
        memset(&next_match[i], 0, sizeof(struct match));
        next_match[i].number = INVALID_MATCH;
    }

    next_match_num = 0;
    next_match_tatami = tatami;

    PROF;
    db_open();
    //db_cmd(NULL, NULL, "begin transaction");

    /* save current match data */
    num_saved_matches = 0;
    db_cmd((gpointer)SAVE_MATCH, db_callback_matches,
	   "SELECT matches.* "
	   "FROM matches, categories "
	   "WHERE categories.\"tatami\"=%d AND categories.\"index\"=matches.\"category\"&%d "
	   "AND categories.\"deleted\"&1=0 "
	   "AND (matches.\"comment\"=%d OR matches.\"comment\"=%d)"
	   "AND matches.\"forcedtatami\"=0",
	   tatami, MATCH_CATEGORY_MASK, COMMENT_MATCH_1, COMMENT_MATCH_2);

    db_cmd((gpointer)SAVE_MATCH, db_callback_matches,
	   "SELECT * FROM matches WHERE \"forcedtatami\"=%d "
	   "AND (\"comment\"=%d OR \"comment\"=%d)",
	   tatami, COMMENT_MATCH_1, COMMENT_MATCH_2);

    /* remove comments from the display */
    db_cmd((gpointer)DB_REMOVE_COMMENT, db_callback_matches,
	   "SELECT matches.* "
	   "FROM matches, categories "
	   "WHERE categories.\"tatami\"=%d AND categories.\"index\"=matches.\"category\"&%d "
	   "AND categories.\"deleted\"&1=0 AND matches.\"comment\">%d "
	   "AND matches.\"forcedtatami\"=0",
	   tatami, MATCH_CATEGORY_MASK, COMMENT_EMPTY);

    db_cmd((gpointer)DB_REMOVE_COMMENT, db_callback_matches,
	   "SELECT * FROM matches WHERE \"forcedtatami\"=%d "
	   "AND \"comment\">%d",
	   tatami, COMMENT_EMPTY);

    /* remove comments from old matches */
    db_cmd(NULL, NULL,
	   "UPDATE matches SET \"comment\"=%d "
	   "WHERE EXISTS (SELECT * FROM categories WHERE "
	   "categories.\"tatami\"=%d AND (matches.\"blue_points\">0 OR "
	   "matches.\"white_points\">0) AND categories.\"index\"=matches.\"category\"&%d "
	   "AND matches.\"comment\">%d AND categories.\"deleted\"&1=0 "
	   "AND matches.\"forcedtatami\"=0)",
	   COMMENT_EMPTY, tatami, MATCH_CATEGORY_MASK, COMMENT_EMPTY);

    db_cmd(NULL, NULL,
	   "UPDATE matches SET \"comment\"=%d "
	   "WHERE \"forcedtatami\"=%d AND \"comment\">%d "
	   "AND (\"blue_points\">0 OR \"white_points\">0)",
	   COMMENT_EMPTY, tatami, COMMENT_EMPTY);

    /* find all matches */
    /* first french matches */
    db_cmd((gpointer)DB_NEXT_MATCH, db_callback_matches,
	   "SELECT * FROM matches "
	   "WHERE matches.\"blue_points\"=0 "
	   "AND matches.\"deleted\"&1=0 "
	   "AND matches.\"white_points\"=0 "
	   "AND matches.\"blue\"<>%d AND matches.\"white\"<>%d "
	   "AND categoryprop(matches.\"category\", 0)=1 "
	   "AND (matches.\"blue\"=0 OR EXISTS (SELECT * FROM competitors WHERE "
	   "competitors.\"index\"=matches.\"blue\" AND competitors.\"deleted\"&3=0))"
	   "AND (matches.\"white\"=0 OR EXISTS (SELECT * FROM competitors WHERE "
	   "competitors.\"index\"=matches.\"white\" AND competitors.\"deleted\"&3=0))"
	   "ORDER BY matches.\"number\" ASC",
	   GHOST, GHOST /*COMPETITOR, COMPETITOR*/);
    /* then others */
    db_cmd((gpointer)DB_NEXT_MATCH, db_callback_matches,
	   "SELECT * FROM matches "
	   "WHERE matches.\"blue_points\"=0 "
	   "AND matches.\"deleted\"&1=0 "
	   "AND matches.\"white_points\"=0 "
	   "AND matches.\"blue\"<>%d AND matches.\"white\"<>%d "
	   "AND categoryprop(matches.\"category\", 1)=1 "
	   "AND (matches.\"blue\"=0 OR EXISTS (SELECT * FROM competitors WHERE "
	   "competitors.\"index\"=matches.\"blue\" AND competitors.\"deleted\"&3=0))"
	   "AND (matches.\"white\"=0 OR EXISTS (SELECT * FROM competitors WHERE "
	   "competitors.\"index\"=matches.\"white\" AND competitors.\"deleted\"&3=0)) "
	   "ORDER BY matches.\"number\" ASC",
	   GHOST, GHOST /*COMPETITOR, COMPETITOR*/);

    //db_cmd(NULL, NULL, "commit");
    db_close();
    PROF;

    // Distribute matches from the same category far away
#define CANNOT_MOVE(_i)  (next_match[_i].number == INVALID_MATCH ||     \
                          next_match[_i].comment == COMMENT_MATCH_1 ||  \
                          next_match[_i].comment == COMMENT_MATCH_2 ||  \
                          next_match[_i].forcednumber ||                \
                          next_match[_i].forcedtatami)

#define SAME_CAT_OR_PLAYERS(_m1, _m2)                             \
    (next_match[_m1].category == next_match[_m2].category ||      \
     next_match[_m1].blue == next_match[_m2].blue ||              \
     next_match[_m1].white == next_match[_m2].white ||            \
     next_match[_m1].blue == next_match[_m2].white ||             \
     next_match[_m1].white == next_match[_m2].blue)

#if 0  // can change order of bronze and gold fights
    for (i = 0; i < next_match_num-2; i++) {
        gint j;

        if (CANNOT_MOVE(i+1))
            continue;

        if (!SAME_CAT_OR_PLAYERS(i, i+1))
            continue;

        for (j = i+2; j < next_match_num; j++) {
            if (next_match[i+1].group == next_match[j].group &&
                !SAME_CAT_OR_PLAYERS(i, j)) {
                gint k;
                struct match m = next_match[j];
                for (k = j; k > i+1; k--) {
                    next_match[k] = next_match[k-1];
                }
                next_match[i+1] = m;
                break;
            }
        }
    }
#endif

    /* check for rest times */
    if (auto_arrange &&
        next_match[1].number != INVALID_MATCH &&
        next_match[1].comment != COMMENT_MATCH_1 &&
        next_match[1].comment != COMMENT_MATCH_2) {
        gint result = 0, rtb = 0, rtw = 0;

        if ((rtb = competitor_cannot_match(next_match[1].blue, tatami, 1)))
            result |= MATCH_FLAG_BLUE_DELAYED | ((rtb & MATCH_FLAG_REST_TIME) ? MATCH_FLAG_BLUE_REST : 0);
        if ((rtw = competitor_cannot_match(next_match[1].white, tatami, 1)))
            result |= MATCH_FLAG_WHITE_DELAYED | ((rtw & MATCH_FLAG_REST_TIME) ? MATCH_FLAG_WHITE_REST : 0);

        next_match[1].flags = result;

        if (result && next_match[1].forcednumber == 0) { // don't change fixed order
            for (i = 2; i < NEXT_MATCH_NUM; i++) {
                gint r = 0;
                rtb = rtw = 0;

                if (next_match[i].number == INVALID_MATCH)
                    break;

                if ((rtb = competitor_cannot_match(next_match[i].blue, tatami, i)))
                    r |= MATCH_FLAG_BLUE_DELAYED | ((rtb & MATCH_FLAG_REST_TIME) ? MATCH_FLAG_BLUE_REST : 0);
                if ((rtw = competitor_cannot_match(next_match[i].white, tatami, i)))
                    r |= MATCH_FLAG_WHITE_DELAYED | ((rtw & MATCH_FLAG_REST_TIME) ? MATCH_FLAG_WHITE_REST : 0);
                if (r == 0 &&
                    next_match[i].blue >= COMPETITOR &&
                    next_match[i].white >= COMPETITOR &&
                    (next_match[1].category != next_match[i].category ||
                     get_match_number_flag(next_match[i].category,
                                           next_match[i].number) == 0)) {
                    struct match tmp = next_match[1];
                    next_match[1] = next_match[i];
                    next_match[i] = tmp;
                    break;
                } else {
                    next_match[i].flags = r;
                }
            }
        }
    }
    PROF;

    db_open();
    //db_cmd(NULL, NULL, "begin transaction");

    /* remove all match comments */
    db_cmd(NULL, NULL,
	   "UPDATE matches SET \"comment\"=%d "
	   "WHERE EXISTS (SELECT * FROM categories WHERE categories.\"tatami\"=%d AND "
	   "categories.\"index\"=matches.\"category\"&%d "
	   "AND categories.\"deleted\"&1=0 "
	   "AND (matches.\"comment\"=%d OR matches.\"comment\"=%d) "
	   "AND matches.\"forcedtatami\"=0)",
	   COMMENT_EMPTY, tatami, MATCH_CATEGORY_MASK, COMMENT_MATCH_1, COMMENT_MATCH_2);

    db_cmd(NULL, NULL,
	   "UPDATE matches SET \"comment\"=%d "
	   "WHERE (\"comment\"=%d OR \"comment\"=%d) "
	   "AND \"forcedtatami\"=%d",
	   COMMENT_EMPTY, COMMENT_MATCH_1, COMMENT_MATCH_2, tatami);
    //db_cmd(NULL, NULL, "commit");

    /* set new next match */
    if (next_match[0].number != INVALID_MATCH) {
	db_cmd(NULL, NULL,
	       "UPDATE matches SET \"comment\"=%d WHERE "
	       "\"category\"=%d AND \"number\"=%d "
	       "AND \"deleted\"&1=0",
	       COMMENT_MATCH_1, next_match[0].category, next_match[0].number);
    }

    /* set new second match */
    if (next_match[1].number != INVALID_MATCH) {
	db_cmd(NULL, NULL,
	       "UPDATE matches SET \"comment\"=%d WHERE "
	       "\"category\"=%d AND \"number\"=%d "
	       "AND \"deleted\"&1=0",
	       COMMENT_MATCH_2, next_match[1].category, next_match[1].number);
    }

    //db_cmd(NULL, NULL, "begin transaction");
    /* update the display */
    db_cmd((gpointer)ADD_MATCH, db_callback_matches,
	   "SELECT matches.* "
	   "FROM matches, categories "
	   "WHERE categories.\"tatami\"=%d AND categories.\"index\"=matches.\"category\"&%d "
	   "AND categories.\"deleted\"&1=0 AND matches.\"comment\">%d "
	   "AND matches.\"forcedtatami\"=0",
	   tatami, MATCH_CATEGORY_MASK, COMMENT_EMPTY);

    db_cmd((gpointer)ADD_MATCH, db_callback_matches,
	   "SELECT * FROM matches "
	   "WHERE \"forcedtatami\"=%d "
	   "AND \"comment\">%d",
	   tatami, COMMENT_EMPTY);
    //db_cmd(NULL, NULL, "commit");

    for (i = 0; i < num_saved_matches; i++) {
	db_cmd((gpointer)ADD_MATCH, db_callback_matches,
	       "SELECT * FROM matches WHERE \"category\"&%u=%d AND \"number\"=%d",
	       ~MATCH_CATEGORY_CAT_MASK, saved_matches[i].category, saved_matches[i].number);
    }
    num_saved_matches = 0;

    db_close();
    PROF;

#ifdef TATAMI_DEBUG
    if (tatami == TATAMI_DEBUG) {
        mylog("\nNEXT MATCHES ON TATAMI %d\n", tatami);
        for (i = 0; i < next_match_num; i++) {
	    struct category_data *cd = avl_get_category(next_match[i].category);
            mylog("%d: %s:%d %s (%d vs. %d)\n", i,
                    cd->category, next_match[i].number,
		    round_name(cd, next_match[i].number),
		    next_match[i].blue, next_match[i].white);
	}
        mylog("\n");
    }
#endif
    // coach info and crc
    match_crc[tatami] = 0xffffffff;

    for (i = 0; i < NEXT_MATCH_NUM; i++) {
	/* calc crc */
	match_crc[tatami] ^=
	    (match_crc[tatami] >> 8) ^
	    (next_match[i].category << 16) ^
	    (next_match[i].number << 8) ^
	    (next_match[i].blue << 6) ^
	    next_match[i].white;

	/* coach info */
	if (next_match[i].number == INVALID_MATCH)
            continue;

        gint blue = next_match[i].blue;
        gint white = next_match[i].white;

        if (blue > 0 && blue < 10000 && coach_info[blue].waittime < 0)
            coach_info[blue].waittime = i;
        if (white > 0 && white < 10000 && coach_info[white].waittime < 0)
            coach_info[white].waittime = i;
    }

    update_next_matches_coach_info();
    PROF_END;

    return next_match;
}

struct match *db_matches_waiting(void)
{
    gchar buffer[1000];
    gint i;

    for (i = 0; i < WAITING_MATCH_NUM; i++)
        matches_waiting[i].number = INVALID_MATCH;

    num_matches_waiting = 0;

    sprintf(buffer,
            "SELECT * FROM matches "
            "WHERE matches.'blue_points'=0 "
            "AND matches.'deleted'&1=0 "
            "AND matches.'white_points'=0 "
            "AND matches.'blue'<>%d AND matches.'white'<>%d "
            "AND matches.'comment'=%d "
            "AND (matches.'blue'=0 OR EXISTS (SELECT * FROM competitors WHERE "
            "competitors.'index'=matches.'blue' AND competitors.'deleted'&3=0)) "
            "AND (matches.'white'=0 OR EXISTS (SELECT * FROM competitors WHERE "
            "competitors.'index'=matches.'white' AND competitors.'deleted'&3=0)) ",
            GHOST, GHOST, COMMENT_WAIT);
    db_exec(db_name, buffer, (gpointer)DB_MATCH_WAITING, db_callback_matches);

    return matches_waiting;
}

gint db_find_match_tatami(gint category, gint number)
{
    gchar buffer[1000];

    m_static.forcedtatami = 0;
    sprintf(buffer, "SELECT * FROM matches WHERE \"category\"&%u=%d "
            "AND \"number\"=%d", ~MATCH_CATEGORY_CAT_MASK, category, number);
    db_exec(db_name, buffer, NULL, db_callback_matches);

    if (m_static.forcedtatami)
        return m_static.forcedtatami;

    return db_get_tatami(category);
}

void db_set_comment(gint category, gint number, gint comment)
{
    gchar buffer[1000];

    num_saved_matches = 0;
#ifdef TATAMI_DEBUG
    mylog("set comment cat=%d num=%d comm=%d\n", category, number, comment);
#endif
    /* remove existing */
    if (comment == COMMENT_MATCH_1 || comment == COMMENT_MATCH_2) {
        gint tatami = db_find_match_tatami(category, number);
#ifdef TATAMI_DEBUG
        mylog("%s: tatami=%d\n", __FUNCTION__, tatami);
#endif
#if 0
        sprintf(buffer, "SELECT * FROM categories WHERE \"category\"=%d", category);
        db_exec(db_name, buffer, (gpointer)DB_GET_SYSTEM, db_callback_categories);
        tatami = j.belt;
#endif
        /* save current match data */
        num_saved_matches = 0;
        sprintf(buffer,
                "SELECT matches.* "
                "FROM matches, categories "
                "WHERE categories.\"tatami\"=%d AND categories.\"index\"=matches.\"category\"&%d "
                "AND categories.\"deleted\"&1=0 "
                "AND (matches.\"comment\"=%d OR matches.\"comment\"=%d) "
                "AND matches.\"forcedtatami\"=0",
                tatami, MATCH_CATEGORY_MASK, COMMENT_MATCH_1, COMMENT_MATCH_2);
        db_exec(db_name, buffer, (gpointer)SAVE_MATCH, db_callback_matches);
        sprintf(buffer,
                "SELECT * FROM matches WHERE \"forcedtatami\"=%d "
                "AND (\"comment\"=%d OR \"comment\"=%d)",
                tatami, COMMENT_MATCH_1, COMMENT_MATCH_2);
        db_exec(db_name, buffer, (gpointer)SAVE_MATCH, db_callback_matches);

        // remove existing old comment
        sprintf(buffer,
                "UPDATE matches SET \"comment\"=%d "
                "WHERE EXISTS (SELECT * FROM categories WHERE categories.\"tatami\"=%d AND "
                "categories.\"index\"=matches.\"category\"&%d "
                "AND categories.\"deleted\"&1=0 AND matches.\"comment\"=%d "
                "AND matches.\"forcedtatami\"=0)",
                COMMENT_EMPTY, tatami, MATCH_CATEGORY_MASK, comment);
        db_exec(db_name, buffer, 0, 0);

        sprintf(buffer,
                "UPDATE matches SET \"comment\"=%d "
                "WHERE \"comment\"=%d "
                "AND \"forcedtatami\"=%d",
                COMMENT_EMPTY, comment, tatami);
        db_exec(db_name, buffer, 0, 0);
    }

    // set new comment
    sprintf(buffer, "UPDATE matches SET \"comment\"=%d WHERE \"category\"&%u=%d AND \"number\"=%d",
            comment, ~MATCH_CATEGORY_CAT_MASK, category, number);
    db_exec(db_name, buffer, 0, 0);

    /* update the display */
    sprintf(buffer,
            "SELECT * FROM matches WHERE \"category\"&%u=%d AND \"number\"=%d AND \"deleted\"&1=0",
            ~MATCH_CATEGORY_CAT_MASK, category, number);
    db_exec(db_name, buffer, (gpointer)ADD_MATCH, db_callback_matches);

    /* update saved macthes display */
    gint i;
    for (i = 0; i < num_saved_matches; i++)
        db_read_match(saved_matches[i].category, saved_matches[i].number);

    num_saved_matches = 0;
}

void db_set_forced_tatami_number_delay(gint category, gint matchnumber, gint tatami, gint num, gboolean delay)
{
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"forcedtatami\"=%d, \"forcednumber\"=%d, "
                    "\"comment\"=%d WHERE \"category\"&%u=%d AND \"number\"=%d AND "
                    "\"forcedtatami\"=0 AND \"forcednumber\"=0",
                    tatami, num,
                    delay ? COMMENT_WAIT : COMMENT_EMPTY, ~MATCH_CATEGORY_CAT_MASK, category, matchnumber);
}


void set_judogi_status(gint index, gint flags)
{
    gint t, n;

    // only the three first matches are affected
    for (n = 0; n < 3; n++) {
        for (t = 1; t <= NUM_TATAMIS; t++) {
            if (next_matches[t][n].number == INVALID_MATCH)
                continue;
            if (next_matches[t][n].blue == index || next_matches[t][n].white == index) {
                if (next_matches[t][n].blue == index) {
                    next_matches[t][n].deleted &= ~(MATCH_FLAG_JUDOGI1_OK | MATCH_FLAG_JUDOGI1_NOK);
                    if (flags & JUDOGI_OK)
                        next_matches[t][n].deleted |= MATCH_FLAG_JUDOGI1_OK;
                    else if (flags & JUDOGI_NOK)
                        next_matches[t][n].deleted |= MATCH_FLAG_JUDOGI1_NOK;
                } else {
                    next_matches[t][n].deleted &= ~(MATCH_FLAG_JUDOGI2_OK | MATCH_FLAG_JUDOGI2_NOK);
                    if (flags & JUDOGI_OK)
                        next_matches[t][n].deleted |= MATCH_FLAG_JUDOGI2_OK;
                    else if (flags & JUDOGI_NOK)
                        next_matches[t][n].deleted |= MATCH_FLAG_JUDOGI2_NOK;
                }

                db_exec_str(NULL, NULL,
                            "UPDATE matches SET \"deleted\"=%d "
                            "WHERE \"category\"&%u=%d AND \"number\"=%d",
                            next_matches[t][n].deleted, ~MATCH_CATEGORY_CAT_MASK,
                            next_matches[t][n].category, next_matches[t][n].number);
                send_match(t, n+1, &(next_matches[t][n]));
                send_next_matches(0, t, &(next_matches[t][0]));
            }
        }
    }
}

gint get_judogi_status(gint index)
{
    gint t, n;

    for (n = 0; n < NEXT_MATCH_NUM; n++) {
        for (t = 1; t <= NUM_TATAMIS; t++) {
            if (next_matches[t][n].number == INVALID_MATCH)
                continue;

            if (next_matches[t][n].blue == index) {
                if (next_matches[t][n].deleted & MATCH_FLAG_JUDOGI1_OK)
                    return JUDOGI_OK;
                else if (next_matches[t][n].deleted & MATCH_FLAG_JUDOGI1_NOK)
                    return JUDOGI_NOK;
                return 0;
            } else if (next_matches[t][n].white == index) {
                if (next_matches[t][n].deleted & MATCH_FLAG_JUDOGI2_OK)
                    return JUDOGI_OK;
                else if (next_matches[t][n].deleted & MATCH_FLAG_JUDOGI2_NOK)
                    return JUDOGI_NOK;
                return 0;
            }
        }
    }

    return 0;
}

static gint maxmatch;
static gint nextforcednum = 1;
static gint medalmatchexists;

static int db_callback_match_num(void *data, int argc, char **argv, char **azColName)
{
    gint i;

    for (i = 0; i < argc; i++) {
        if (!strcmp(azColName[i], "MAX(number)"))
            maxmatch = argv[i] ? atoi(argv[i]) : 0;
        else if (!strcmp(azColName[i], "MAX(forcednumber)"))
            nextforcednum = argv[i] ? atoi(argv[i]) : 0;
    }

    return 0;
}

static int db_callback_medal_match_exists(void *data, int argc, char **argv, char **azColName)
{
    gint i;
    for (i = 0; i < argc; i++) {
        if (!strcmp(azColName[i], "forcedtatami"))
            medalmatchexists |= argv[i] ? 1 << atoi(argv[i]) : 0;
    }
    return 0;
}

gint db_force_match_number(gint category)
{
    gint i;

    maxmatch = 0;
    medalmatchexists = 0;

    db_exec_str(NULL, db_callback_match_num,
                "SELECT MAX(number) FROM  matches WHERE \"category\"&%u=%d AND"
                "(\"blue_points\">0 OR \"white_points\">0)",
                ~MATCH_CATEGORY_CAT_MASK, category);

    db_exec_str(NULL, db_callback_medal_match_exists,
                "SELECT forcedtatami FROM matches WHERE \"category\"&%u=%d AND "
                "\"number\"=%d AND \"forcedtatami\">0 AND \"forcednumber\"=0",
                ~MATCH_CATEGORY_CAT_MASK, category, maxmatch+1);

    if (!medalmatchexists)
        return 0;

    db_exec_str(NULL, db_callback_match_num,
                "SELECT MAX(forcednumber) FROM matches");

    db_exec_str(NULL, db_callback_medal_match_exists,
                "SELECT forcedtatami FROM matches WHERE \"category\"&%u=%d AND "
                "\"number\">%d AND \"forcedtatami\">0 AND \"forcednumber\"=0",
                ~MATCH_CATEGORY_CAT_MASK, category, maxmatch);

    db_exec_str(NULL, NULL,
                "UPDATE matches SET \"forcednumber\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                ++nextforcednum, ~MATCH_CATEGORY_CAT_MASK, category, maxmatch+1);
    db_exec_str(NULL, NULL,
                "UPDATE matches SET \"forcednumber\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                ++nextforcednum, ~MATCH_CATEGORY_CAT_MASK, category, maxmatch+2);
    db_exec_str(NULL, NULL,
                "UPDATE matches SET \"forcednumber\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                ++nextforcednum, ~MATCH_CATEGORY_CAT_MASK, category, maxmatch+3);

    for (i = 1; i <= number_of_tatamis; i++)
        if (medalmatchexists & (1 << i))
            update_matches(0, (struct compsys){0,0,0,0}, i);

    return medalmatchexists;
}

void update_next_matches_coach_info(void)
{
    gint i;

    if (automatic_web_page_update == FALSE)
        return;

    gchar *file = g_build_filename(current_directory, "c-matches.txt", NULL);
    FILE *f = g_fopen(file, "wb");
    g_free(file);

    if (f) {
        for (i = 0; i < 10000; i++)
            fprintf(f, "%d\t%d\n", coach_info[i].tatami, coach_info[i].waittime);

        fclose(f);
    }
}

gboolean db_event_matches_update(guint category, struct match *last, gint *weightclass)
{
    gint number = MATCH_CATEGORY_SUB_GET(category);
    gint category1 = category & MATCH_CATEGORY_MASK;
    gboolean winner_found = FALSE;

    team1_wins = team2_wins = no_team_wins = team1_pts = team2_pts = 0;
    team1_tot_time = team2_tot_time = team1_matches = team2_matches = 0;
    team_matches_total = team_num_weight_classes = 0;
    memset(&team_last_match, 0, sizeof(team_last_match));
    team_extra_exists = FALSE;

    db_exec_str(gint_to_ptr(DB_FIND_TEAM_WINNER), db_callback_matches,
                "SELECT * FROM matches WHERE \"category\"&%u=%d",
		~MATCH_CATEGORY_CAT_MASK, category);

    /*mylog("cat=%d/%d nowins=%d t1wins=%d/%d t2wins=%d/%d\n",
            category1, number,
            no_team_wins, team1_wins, team1_pts, team2_wins, team2_pts);*/

    if (prop_get_int_val(PROP_TEAM_EVENT_IGNORE_POINTS)) {
	// IJF does not care about points
	team1_pts = team2_pts = 0;
    }

    if (no_team_wins == 0 && team_extra_exists == FALSE &&
	(team1_wins == team2_wins) && (team1_pts == team2_pts)) {
	/* No matches left and equal result. One match more is required. */
	*last = team_last_match;

	if (team_num_weight_classes) {
	    gint sel_ix = rand()%team_num_weight_classes;
	    *weightclass = team_weight_classes[sel_ix];

	    gint j, i = find_cat_data_index_by_index(category);
	    if (i >= 0) {
		GtkTextBuffer *msgwin = message_window();
		show_msg(msgwin, "bold", "\nDraw was done from categories\n");

		for (j = 0; j < team_num_weight_classes; j++) {
		    gint ix = team_weight_classes[j] - 1;
		    if (ix >= 0 && ix < NUM_CAT_DEF_WEIGHTS)
			show_msg(msgwin, NULL, "%s ",
				 category_definitions[i].weights[ix].weighttext);
		}
		show_msg(msgwin, "red", "\n\nDrawn category: %s.\n",
			 category_definitions[i].weights[*weightclass - 1].weighttext);

		change_team_competitors_to_real_persons(category,
							*weightclass, &last->blue, &last->white);
	    }
	}

	return TRUE;
    }

    if (prop_get_int_val(PROP_TEAM_EVENT_SKIP_UNNECESSARY_MATCHES) &&
	(team1_wins > team_matches_total/2 ||
	 team2_wins > team_matches_total/2)) {
	// The first team reaching the majority of wins is declared the winner
	winner_found = TRUE;
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"blue_points\"=11, \"white_points\"=11, "
		    "\"time\"=0 "
                    "WHERE \"category\"&%u=%d AND \"blue_points\"=0 AND \"white_points\"=0",
                    ~MATCH_CATEGORY_CAT_MASK, category);
    }

    if (!winner_found &&
	(no_team_wins || ((team1_wins == team2_wins) && (team1_pts == team2_pts)))) {
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"blue_points\"=0, \"white_points\"=0, "
		    "\"time\"=0 "
                    "WHERE \"category\"=%d AND \"number\"=%d",
                    category1, number);

	if (team_extra_exists)
	    db_exec_str(NULL, NULL,
			"UPDATE matches SET \"blue_points\"=0, \"white_points\"=0, "
			"\"time\"=0 "
			"WHERE \"category\"&%u=%d AND \"number\"=999",
			~MATCH_CATEGORY_CAT_MASK, category);
    } else {
	gint ave_time = 0;

	if (team1_wins*10000+team1_pts > team2_wins*10000+team2_pts) {
	    if (team1_matches && team1_tot_time)
		ave_time = team1_tot_time/team1_matches;
	} else {
	    if (team2_matches && team2_tot_time)
		ave_time = team2_tot_time/team2_matches;
	}

        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"blue_points\"=%d, \"white_points\"=%d, "
		    "\"time\"=%d "
                    "WHERE \"category\"=%d AND \"number\"=%d",
                    team1_wins*10000+team1_pts, team2_wins*10000+team2_pts,
		    ave_time,
		    category1, number);

    }
    /*
    } else  if (team1_wins > team2_wins) {
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"blue_points\"=%d "
                    "WHERE \"category\"=%d AND \"number\"=%d",
                    team1_wins, category1, number);
    } else {
        db_exec_str(NULL, NULL,
                    "UPDATE matches SET \"white_points\"=%d "
                    "WHERE \"category\"=%d AND \"number\"=%d",
                    team2_wins, category1, number);
    }
    */
    return FALSE;
}

static FILE *matches_file = NULL;

static void db_print_one_match(struct match *m)
{
    const gchar *first1 = "Xxx", *last1 = "Xxx";
    const gchar *first2 = "Xxx", *last2 = "Xxx";

    if (m->blue_points == 0 && m->white_points == 0)
        return;

    gboolean c1_wins = m->blue_points > m->white_points;
    struct judoka *j1 = get_data(m->blue);
    struct judoka *j2 = get_data(m->white);
    if (j1 == NULL || j2 == NULL)
        goto out;

    // GDPR
    if (gdpr_ok(j1)) {
	first1 = j1->first;
	last1 = j1->last;
    }
    if (gdpr_ok(j2)) {
	first2 = j2->first;
	last2 = j2->last;
    }

    if (m->category & MATCH_CATEGORY_SUB_MASK)
        fprintf(matches_file,
                "<tr><td>%d/%d</td>", MATCH_CATEGORY_SUB_GET(m->category), m->number);
    else
        fprintf(matches_file,
                "<tr><td>%d</td>", m->number);

    fprintf(matches_file,
            "<td onclick=\"top.location.href='%d.html'\" "
            "style=\"cursor: pointer;%s\">%s %s</td><td class=\"%s\">",
            j1->index,
	    c1_wins ? "font-weight:bold" : "",
            utf8_to_html(firstname_lastname() ? first1 : last1),
            utf8_to_html(firstname_lastname() ? last1 : first1),
            prop_get_int_val(PROP_WHITE_FIRST) ? "wscore" : "bscore");

    if (prop_get_int_val_cat(PROP_RULES_2017, m->category) ||
	prop_get_int_val_cat(PROP_RULES_2018, m->category))
	fprintf(matches_file,
		"%d%d/%d%s</td>",
		(m->blue_score>>16)&15, (m->blue_score>>12)&15,
		m->blue_score&7, m->blue_score&8?"H":"");
    else
	fprintf(matches_file,
		"%d%d%d/%d%s</td>",
		(m->blue_score>>16)&15, (m->blue_score>>12)&15, (m->blue_score>>8)&15,
		m->blue_score&7, m->blue_score&8?"H":"");

    fprintf(matches_file,
	    "<td align=\"center\">%s - %s</td>"
            "<td class=\"%s\">",
            get_points_str(m->blue_points, m->category),
            get_points_str(m->white_points, m->category),
            prop_get_int_val(PROP_WHITE_FIRST) ? "bscore" : "wscore");

    if (prop_get_int_val_cat(PROP_RULES_2017, m->category) ||
	prop_get_int_val_cat(PROP_RULES_2018, m->category))
	fprintf(matches_file,
		"%d%d/%d%s</td>",
		(m->white_score>>16)&15, (m->white_score>>12)&15,
		m->white_score&7, m->white_score&8?"H":"");
    else
	fprintf(matches_file,
		"%d%d%d/%d%s</td>",
		(m->white_score>>16)&15, (m->white_score>>12)&15, (m->white_score>>8)&15,
		m->white_score&7, m->white_score&8?"H":"");

    fprintf(matches_file,
            "<td onclick=\"top.location.href='%d.html'\" "
            "style=\"cursor: pointer;%s\">%s %s</td>"
            "<td>%d:%02d</td></tr>\r\n",
            j2->index,
	    c1_wins ? "" : "font-weight:bold",
            utf8_to_html(firstname_lastname() ? first2 : last2),
            utf8_to_html(firstname_lastname() ? last2 : first2),
	    m->match_time/60, m->match_time%60);

 out:
    free_judoka(j1);
    free_judoka(j2);
}

void db_print_category_matches(struct category_data *catdata, FILE *f)
{
    matches_file = f;

    if (catdata->deleted & TEAM_EVENT)
        db_exec_str(gint_to_ptr(DB_PRINT_CAT_MATCHES), db_callback_matches,
                    "SELECT * FROM matches WHERE \"category\"&%u=%d AND \"category\"&%d>0",
                    MATCH_CATEGORY_MASK, catdata->index, MATCH_CATEGORY_SUB_MASK);
    else
        db_exec_str(gint_to_ptr(DB_PRINT_CAT_MATCHES), db_callback_matches,
                    "SELECT * FROM matches WHERE \"category\"=%d", catdata->index);
}

void db_change_competitor(gint category, gint number, gboolean is_blue, gint index)
{
    db_exec_str(NULL, NULL,
                "UPDATE matches SET \"%s\"=%d "
                "WHERE \"category\"&%u=%d AND \"number\"=%d",
                is_blue ? "blue" : "white", index,
                ~MATCH_CATEGORY_CAT_MASK, category, number);
}

static gboolean nocomment = FALSE;

#if 0
static int db_callback_matches_pdf(void *data, int argc, char **argv, char **azColName)
{
    gint i;
    FILE *out = data;
    fprintf(out, "<js>");
    for (i = 0; i < argc; i++) {
	if (nocomment && IS(comment)) // dont save comments (can be anything...)
	    fprintf(out, "|");
	else
	    fprintf(out, "%s|", argv[i]);
    }
    fprintf(out, "</js>\n");
    return 0;
}
#endif

#define LINE_LEN 512
#define LINE_SPACE (LINE_LEN+4)

#define GO_OUT(_s) do { mylog("%s[%d] Error: %s\n", __FUNCTION__, __LINE__, _s); goto out; } while (0)

#define GETNUM(_a) do {					\
	if (*p < '0' || *p > '9') p++;			\
	if (*p < '0' || *p > '9') p++;			\
	if (*p < '0' || *p > '9') GO_OUT("no num");	\
	_a = atoi(p); } while (0)

#define READ_OBJ(_n)							\
    do {								\
	if (_n >= num_xref) GO_OUT("obj too big");			\
	if (fseek(pdf, xref[_n], SEEK_SET) < 0) GO_OUT("fseek");	\
	gint r = fread(buf, 1, LINE_LEN, pdf);				\
	if (r <= 0) GO_OUT("fread");					\
	buf[r] = buf[r+1] = 0;						\
	p = strstr(buf, "endobj");					\
	if (!p) {mylog("obj=%d\n", _n);GO_OUT("no endobj");}		\
	p[6] = 0;								\
    } while (0)

#define READ_OBJ_ATTR(_ret, _obj, _name)			\
    do {							\
	READ_OBJ(_obj);						\
	p = strstr(buf, _name);					\
	if (!p) GO_OUT("no name");				\
	p += strlen(_name);					\
	while (*p && (*p < '0' || *p > '9')) p++;		\
	if (!p) GO_OUT("num not found");			\
	_ret = atoi(p);						\
    } while (0)

#if 0
#define goto_out do { mylog("%s[%d] Error\n", __FUNCTION__, __LINE__); goto out; } while (0)

#define NUM_XREF 1024
static gint xref[NUM_XREF];

void db_print_category_to_pdf_comments(gint catix, gchar *filename)
{
    gchar buf[LINE_SPACE], trailer[LINE_SPACE];
    FILE *pdf = NULL;
    gchar *p;
    gint xrefstart = 0, root = 0, i, i1, i2;
    gint num_xref = 0;
    gint obj1pos, obj2pos, obj3pos, xrefpos;
    gchar *trailer_p = NULL, *startxref_p = NULL;
    gint pages, kids;
    gchar saved;
    glong metastart;
    gint metalen;
    gchar *pdfname = NULL;
    metalen = 0;

    if (catix && !filename) {
	struct category_data *catdata =
	    avl_get_category(catix);
	if (!catdata) return;

	snprintf(buf, sizeof(buf), "%s.pdf", txt2hex(catdata->category));
	pdfname = g_build_filename(current_directory, buf, NULL);
    } else if (filename) {
	pdfname = g_strdup(filename);
    } else
	return;

    pdf = g_fopen(pdfname, "rb");
    if (!pdf) {
	mylog("%s[%d]: Cannot open %s\n", __FUNCTION__, __LINE__, pdfname);
	goto_out;
    }

    // read last 256 bytes
    fseek(pdf, -256, SEEK_END);
    fread(trailer, 256, 1, pdf);
    trailer[256] = trailer[257] = 0;

    // find trailer
    trailer_p = strstr(trailer, "trailer");
    if (!trailer_p) goto_out;
    // get root number
    p = strstr(trailer_p+8, "/Root");
    if (!p) goto_out;
    p += 6;
    GETNUM(root);

    // get startxref
    p = startxref_p = strstr(trailer_p, "startxref");
    if (!p) goto_out;
    p += 10;
    GETNUM(xrefstart);

    // read xref table
    if (fseek(pdf, xrefstart, SEEK_SET) < 0) goto_out;
    fgets(buf, sizeof(buf), pdf);
    if (strncmp(buf, "xref", 4)) goto_out;
    fgets(buf, sizeof(buf), pdf);
    sscanf(buf, "%d %d", &i1, &num_xref);
    if (i1) goto_out;
    if (num_xref > NUM_XREF) num_xref = NUM_XREF;
    for (i = 0; i < num_xref; i++) {
	fgets(buf, sizeof(buf), pdf);
	xref[i] = atoi(buf);
    }

    // read first page obj
    READ_OBJ_ATTR(pages, root, "/Pages");
    READ_OBJ_ATTR(kids, pages, "/Kids");
    READ_OBJ(kids);

    fclose(pdf);

    // append to pdf file
    pdf = g_fopen(pdfname, "ab");
    if (!pdf) goto_out;

    fseek(pdf, 0, SEEK_END);
    obj1pos = ftell(pdf);
    // first page obj is in buf
    // find << and insert metadata reference
    p = strstr(buf, "<<");
    if (!p) goto_out;
    saved = p[2];
    p[2] = 0;
    fprintf(pdf, "%s\n", buf);
    fprintf(pdf, " /Metadata %d 0 R\n", num_xref);
    p[2] = saved;
    fprintf(pdf, "%s\n", p+2);

    // add metadata obj
    obj2pos = ftell(pdf);
    fprintf(pdf, "%d 0 obj\n", num_xref);
    fprintf(pdf,
	    "<< /Type /Metadata /Subtype /XML /Length %d 0 R >>\n"
	    "stream\n", num_xref+1);
    metastart = ftell(pdf);
#if 0
    fprintf(pdf, "<?xpacket begin=\"\357\273\277\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n");
#else
    fprintf(pdf, "<judoshiai>\n");
#endif
    fprintf(pdf, "<js>VER %s</js>\n", SHIAI_VERSION);

#ifdef WIN32
    fprintf(pdf, "<js>OS Win32</js>\n");
#else
    fprintf(pdf, "<js>OS Linux</js>\n");
#endif

    fprintf(pdf, "<js>TBL info</js>\n");
    db_exec_str(pdf, db_callback_matches_pdf,
		"SELECT * FROM info");
    fprintf(pdf, "<js>END</js>\n");

    fprintf(pdf, "<js>TBL categories</js>\n");
    if (catix)
	db_exec_str(pdf, db_callback_matches_pdf,
		    "SELECT * FROM categories WHERE \"index\"=%d", catix);
    else
	db_exec_str(pdf, db_callback_matches_pdf,
		    "SELECT * FROM categories");
    fprintf(pdf, "<js>END</js>\n");

    nocomment = TRUE;
    fprintf(pdf, "<js>TBL competitors</js>\n");
    if (catix)
	db_exec_str(pdf, db_callback_matches_pdf,
		    "SELECT * FROM competitors WHERE \"index\" IN "
		    "(SELECT blue FROM matches WHERE \"category\"=%d UNION "
		    "SELECT white FROM matches WHERE \"category\"=%d)",
		    catix, catix);
    else
	db_exec_str(pdf, db_callback_matches_pdf,
		    "SELECT * FROM competitors");
    fprintf(pdf, "<js>END</js>\n");
    nocomment = FALSE;

    fprintf(pdf, "<js>TBL matches</js>\n");
    if (catix)
	db_exec_str(pdf, db_callback_matches_pdf,
		    "SELECT * FROM matches WHERE \"category\"=%d", catix);
    else
	db_exec_str(pdf, db_callback_matches_pdf,
		    "SELECT * FROM matches");
    fprintf(pdf, "<js>END</js>\n");

#if 0
    fprintf(pdf, "<?xpacket#end='w'?>\n");
#else
    fprintf(pdf, "</judoshiai>\n");
#endif
    metalen = ftell(pdf)-metastart;
    fprintf(pdf,
	    "endstream\n"
	    "endobj\n");

    // add metadata length obj
    obj3pos = ftell(pdf);
    fprintf(pdf, "%d 0 obj\n   %d\nendobj\n", num_xref+1, metalen-1);

    xrefpos = ftell(pdf);
    fprintf(pdf, "xref\n0 1\n0000000000 65535 f\n");
    fprintf(pdf, "%d 1\n%010d 00000 n\n", kids, obj1pos);
    fprintf(pdf, "%d 1\n%010d 00000 n\n", num_xref, obj2pos);
    fprintf(pdf, "%d 1\n%010d 00000 n\n", num_xref+1, obj3pos);

    *startxref_p = 0;
    p = strstr(trailer_p, ">>");
    if (!p) goto_out;
    *p = 0;
    p = strstr(trailer_p, "/Size");
    if (!p) goto_out;
    p += 5;
    GETNUM(i2);
    (void)i2;
    *p = 0;
    p++;
    while(*p >= '0' && *p <= '9') p++;
    fprintf(pdf, "%s%d%s\n   /Prev %d\n>>\n", trailer_p, num_xref+2, p, xrefstart);

    fprintf(pdf, "startxref\n%d\n%%EOF\n", xrefpos);

 out:
    if (pdfname) g_free(pdfname);
    if (pdf) fclose(pdf);
}
#endif
