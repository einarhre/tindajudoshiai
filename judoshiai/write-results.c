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
#include <glib/gstdio.h>
#include <ftw.h>

#include "sqlite3.h"
#include "judoshiai.h"
#include "language.h"
#include "cJSON.h"

void write_results(FILE *f);
static void write_extra_link_files(void);

gboolean create_statistics = FALSE;

static gint saved_competitors[TOTAL_NUM_COMPETITORS];
static gint saved_competitor_cnt = 0;

#define NUM_EXTRA_LINKS 2
#define EXTRA_EMBED     1
#define EXTRA_NEW_WIN   2
static struct {
    gchar *name, *href;
    gint flags;
} extra_links[NUM_EXTRA_LINKS];

static void write_result(FILE *f, gint num, struct judoka *j, cJSON *json)
{
    const gchar *first = j->first, *last = j->last;
    const gchar *club = j->club, *country = j->country;

    // GDPR
    if (!gdpr_ok(j))
	return;

    if (club_text == (CLUB_TEXT_CLUB|CLUB_TEXT_COUNTRY)) {
        if (firstname_lastname())
            fprintf(f, "<tr><td>%d.</td><td>%s %s</td><td>%s/%s</td></tr>\r\n",
                    num, utf8_to_html(first), utf8_to_html(last), utf8_to_html(club), utf8_to_html(country));
        else
            fprintf(f, "<tr><td>%d.</td><td>%s %s</td><td>%s/%s</td></tr>\r\n",
                    num, utf8_to_html(last), utf8_to_html(first), utf8_to_html(club), utf8_to_html(country));
    } else {
	struct club_name_data *data = club_name_get(club);
	if (club_text == CLUB_TEXT_CLUB &&
	    data && data->address && data->address[0])
	    fprintf(f, "<tr><td>%d.</td><td>%s %s</td><td>%s, %s</td></tr>\r\n",
		    num, utf8_to_html(firstname_lastname() ? first : last),
                    utf8_to_html(firstname_lastname() ? last : first),
		    utf8_to_html(club), utf8_to_html(data->address));
	else
	    fprintf(f, "<tr><td>%d.</td><td>%s %s</td><td>%s</td></tr>\r\n",
		    num, utf8_to_html(firstname_lastname() ? first : last),
                    utf8_to_html(firstname_lastname() ? last : first),
                    utf8_to_html(club_text==CLUB_TEXT_CLUB ? club : country));
    }

    if (json) {
        cJSON *c = cJSON_CreateObject();
        cJSON_AddNumberToObject(c, "ix", j->index);
        cJSON_AddNumberToObject(c, "pos", num);
        cJSON_AddStringToObject(c, "first", first);
        cJSON_AddStringToObject(c, "last", last);
        cJSON_AddStringToObject(c, "club", club);
        cJSON_AddStringToObject(c, "country", country);
        cJSON_AddItemToArray(json, c);
    }
    
    if (create_statistics)
        club_stat_add(club, country, num);
}

void write_competitor(FILE *f, struct judoka *j, gint club_flags, gboolean by_club, cJSON *json)
{
    // save last club as a hash
    static gint last_crc = 0;
    static gint member_count = 0;
    const gchar *first = j->first, *last = j->last, *belt = "?";
    const gchar *club, *category = j->category;
    gint index = j->index;

    if (category == NULL || category[0] == '?' || category[0] == '_')
        return;

    // GDPR
    if (!gdpr_ok(j))
	return;

    if (j->belt >= 0 && j->belt < NUM_BELTS)
            belt = belts[j->belt];

    club = get_club_text(j, club_flags);

    // JSON
    if (json) {
        gint pos = avl_get_competitor_position(index);
        cJSON *c = cJSON_CreateObject();
        cJSON_AddNumberToObject(c, "ix", index);
        cJSON_AddStringToObject(c, "id", j->id ? j->id : "");
        cJSON_AddStringToObject(c, "coachid", j->coachid ? j->coachid : "");
        cJSON_AddNumberToObject(c, "pos", pos);
        cJSON_AddStringToObject(c, "last", last);
        cJSON_AddStringToObject(c, "first", first);
        cJSON_AddStringToObject(c, "club", club);
        cJSON_AddStringToObject(c, "category", category);
        cJSON_AddStringToObject(c, "belt", grade_visible ? belt : "");
        json_add_coach_info(c, j);
        cJSON_AddItemToArray(json, c);
    }
    
    if (by_club) {
        gchar buf[16], buf2[16];
        gint crc = pwcrc32((guchar *)club, strlen(club));

        if (crc != last_crc)
            member_count = 0;
        member_count++;
        snprintf(buf2, sizeof(buf2), "&nbsp;#%d", member_count);

        if (create_statistics) {
            gint pos = avl_get_competitor_position(index);

            if (pos > 0)
                snprintf(buf, sizeof(buf), "%d.", pos);
            else
                snprintf(buf, sizeof(buf), "-");

            fprintf(f,
                    "<tr><td>%d</td><td>%s</td><td><a href=\"%d.html\">%s %s</a></td><td>%s</td>"
                    "<td onclick=\"openCatWindow('%s','%d')\">"
                    "<a href=\"competitors2.html\">%s</a></td><td align=\"center\">%s</td></tr>\r\n",
                    member_count, member_count == 1 ? utf8_to_html(club) : "",
                    index,
                    firstname_lastname() ? utf8_to_html(first) : utf8_to_html(last),
                    firstname_lastname() ? utf8_to_html(last) : utf8_to_html(first),
                    grade_visible ? belt : "",
                    category, index, category, buf);
        } else
            fprintf(f,
                    "<tr><td>%d</td><td>%s</td><td>%s %s</td><td>%s</td>"
                    "<td onclick=\"openCatWindow('%s','%d')\">"
                    "<a href=\"competitors2.html\">%s</a></td><td>&nbsp;</td></tr>\r\n",
                    member_count, member_count == 1 ? utf8_to_html(club) : "",
                    firstname_lastname() ? utf8_to_html(first) : utf8_to_html(last),
                    firstname_lastname() ? utf8_to_html(last) : utf8_to_html(first),
                    grade_visible ? belt : "",
                    category, index, category);

        last_crc = crc;
    } else {
        if (create_statistics) {
            gchar buf[16];
            gint pos = avl_get_competitor_position(index);

            if (pos > 0)
                snprintf(buf, sizeof(buf), "%d.", pos);
            else
                snprintf(buf, sizeof(buf), "-");

            fprintf(f,
                    "<tr><td><a href=\"%d.html\">%s %s</a></td><td>%s</td><td>%s</td>"
                    "<td onclick=\"openCatWindow('%s','%d')\">"
                    "<a href=\"competitors.html\">%s</a></td><td align=\"center\">%s</td></tr>\r\n",
                    index,
                    firstname_lastname() ? utf8_to_html(first) : utf8_to_html(last),
                    firstname_lastname() ? utf8_to_html(last) : utf8_to_html(first),
                    grade_visible ? belt : "",
                    utf8_to_html(club), category, index, category, buf);
        } else
            fprintf(f,
                    "<tr><td>%s %s</td><td>%s</td><td>%s</td>"
                    "<td onclick=\"openCatWindow('%s','%d')\">"
                    "<a href=\"competitors.html\">%s</a></td><td>&nbsp;</td></tr>\r\n",
                    firstname_lastname() ? utf8_to_html(first) : utf8_to_html(last),
                    firstname_lastname() ? utf8_to_html(last) : utf8_to_html(first),
                    grade_visible ? belt : "", utf8_to_html(club),
                    category, index, category);

        saved_competitors[saved_competitor_cnt] = index;
        if (saved_competitor_cnt < TOTAL_NUM_COMPETITORS)
            saved_competitor_cnt++;

        last_crc = 0; // this branch is called first and thus last_crc is initialised (many times)
    }
}

static void make_top_frame_1(FILE *f, gchar *meta, gboolean frame)
{
    if (frame)
	fprintf(f, "<html><head>"
	    "<meta name=\"viewport\" content=\"width=device-width, "
	    "target-densitydpi=device-dpi\">\r\n"
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n"
            "%s"
            "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\r\n"
            "<meta name=\"keywords\" content=\"JudoShiai-%s\" />\r\n"
            "<title>%s  %s  %s</title></head>\r\n"
            "<body class=\"titleframe\"><table><tr>\r\n"
            "<td colspan=\"2\" align=\"center\" class=\"tournamentheader\"><h1>%s  %s  %s</h1></td></tr><tr>\r\n",
            meta,
            SHIAI_VERSION,
            prop_get_str_val(PROP_NAME), prop_get_str_val(PROP_DATE), prop_get_str_val(PROP_PLACE),
            prop_get_str_val(PROP_NAME), prop_get_str_val(PROP_DATE), prop_get_str_val(PROP_PLACE));
    else
	fprintf(f, "<html><head>"
	    "<meta name=\"viewport\" content=\"width=device-width, "
	    "target-densitydpi=device-dpi\">\r\n"
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n"
            "%s"
            "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\r\n"
            "<meta name=\"keywords\" content=\"JudoShiai-%s\" />\r\n"
            "<title>%s  %s  %s</title></head>\r\n"
		"<body class=\"titleframe\"><table><tr>\r\n",
            meta,
            SHIAI_VERSION,
            prop_get_str_val(PROP_NAME), prop_get_str_val(PROP_DATE), prop_get_str_val(PROP_PLACE));
}

static void make_top_frame(FILE *f)
{
    make_top_frame_1(f, "", TRUE);
}

static void make_top_frame_refresh(FILE *f, gboolean frame)
{
    make_top_frame_1(f, "<meta http-equiv=\"refresh\" content=\"30\"/>\r\n", frame);
}

static void make_bottom_frame(FILE *f)
{
    fprintf(f, "</tr></table></body></html>\r\n");
}

static gint make_left_frame(FILE *f)
{
    GtkTreeIter iter;
    gboolean ok;
    gint num_cats = 0, i;

    fprintf(f, "<td valign=\"top\">\r\n");

    if (extra_links[0].name && extra_links[0].name[0] &&
	extra_links[0].href && extra_links[0].href[0]) {
	fprintf(f, "<table class=\"extralink\">");

	for (i = 0; i < NUM_EXTRA_LINKS; i++) {
	    if (extra_links[i].name && extra_links[i].name[0] &&
		extra_links[i].href && extra_links[i].href[0]) {
		gchar buf[32];
		snprintf(buf, sizeof(buf), "extralink%d.html", i);
		fprintf(f,
			"<tr><td class=\"extralink\"><a href=\"%s\" %s>%s</a></td></tr>",
			(extra_links[i].flags & EXTRA_EMBED) ? buf : extra_links[i].href,
			(extra_links[i].flags & EXTRA_NEW_WIN) ? "target=\"_blank\"" : "",
			extra_links[i].name);
		
	    }
	}
	fprintf(f, "</table>\r\n");
    }
    
    fprintf(f,
	    "<table class=\"resultslink\"><tr><td class=\"resultslink\">"
            "<a href=\"index1.html\" target=\"_blank\">%s</a></td></tr></table>\r\n", _("Mobile"));
    fprintf(f,
	    "<table class=\"resultslink\"><tr><td class=\"resultslink\">"
            "<a href=\"index.html\">%s</a></td></tr></table>\r\n", _T(results));
    fprintf(f,
            "<table class=\"competitorslink\"><tr><td class=\"competitorslink\">"
            "<a href=\"competitors.html\">%s</a></td></tr></table>\r\n", _T(competitor));
    if (create_statistics) {
        fprintf(f,
                "<table class=\"medalslink\"><tr><td class=\"medalslink\">"
                "<a href=\"medals.html\">%s</a></td></tr></table>\r\n", _T(medals));
        fprintf(f,
                "<table class=\"statisticslink\"><tr><td class=\"statisticslink\">"
                "<a href=\"statistics.html\">%s</a></td></tr></table>\r\n", _T(statistics));
    }

    if (automatic_web_page_update) {
        fprintf(f,
                "<table class=\"nextmatcheslink\"><tr><td class=\"nextmatcheslink\">"
                "<a href=\"nextmatches.html\">%s</a></td></tr></table>\r\n", _T(nextmatch2));

        fprintf(f,
                "<table class=\"nextmatcheslink\"><tr><td class=\"nextmatcheslink\">"
                "<a href=\"coach.html\">%s</a></td></tr></table>\r\n", _T(coach));
    }

    fprintf(f,
            "<table class=\"categorieshdr\"><tr><td class=\"categorieshdr\">"
            "%s</td></tr></table>\r\n", _T(categories));

    fprintf(f, "<table class=\"categorylinks\">");

    gdpr_enable++;
    ok = gtk_tree_model_get_iter_first(current_model, &iter);

    while (ok) {
        gint n = gtk_tree_model_iter_n_children(current_model, &iter);
        gint index;

        num_cats++;

        if (n >= 1 && n <= NUM_COMPETITORS) {
            struct judoka *j;

            gtk_tree_model_get(current_model, &iter,
                               COL_INDEX, &index,
                               -1);

            struct category_data *catdata = avl_get_category(index);
            gboolean team = catdata && (catdata->deleted & TEAM);

            j = get_data(index);
            if (j) {
                if (j->last && j->last[0] != '?' && j->last[0] != '_' &&
		    team == FALSE && gdpr_ok(j)) {
                    if (!automatic_web_page_update) {
                        fprintf(f,
                                "<tr><td class=\"categorylinksleft\"><a href=\"%s.html\">%s</a></td>"
                                "<td class=\"categorylinksright\">"
                                "<a href=\"%s.pdf\" target=\"_blank\"> "
                                "(PDF)</a></td></tr>\r\n",
                                txt2hex(j->last), utf8_to_html(j->last),
                                txt2hex(j->last));
                    } else {
                        fprintf(f, "<tr><td class=\"categorylinksonly\"><a href=\"%s.html\">%s</a></td></tr>\r\n",
                                txt2hex(j->last), utf8_to_html(j->last));
                    }
                }
                free_judoka(j);
            } // j
        }

        ok = gtk_tree_model_iter_next(current_model, &iter);
    }

    gdpr_enable--;
    fprintf(f, "</table></td>\r\n");
    return num_cats;
}


static void pool_results(FILE *f, gint category, struct judoka *ctg, gint num_judokas, gboolean dpool2, cJSON *json)
{
    struct pool_matches pm;
    gint i;

    fill_pool_struct(category, num_judokas, &pm, dpool2);

    if (dpool2)
        num_judokas = 4;

    if (pm.finished)
        get_pool_winner(num_judokas, pm.c, pm.yes, pm.wins, pm.pts, pm.tim, pm.mw, pm.j, pm.all_matched, pm.tie);

    for (i = 1; i <= num_judokas; i++) {
        if (pm.finished == FALSE || pm.j[pm.c[i]] == NULL)
            continue;

        gint nrprint = 8; // database table limit
        //if (IS_LANG_NB) nrprint = 5;
        //else if (IS_LANG_SK) nrprint = 6;

        // Spanish have two bronzes in pool system
        if (i <= 4 && prop_get_int_val_cat(PROP_TWO_POOL_BRONZES, category) &&
            (pm.j[pm.c[i]]->deleted & HANSOKUMAKE) == 0) {
            write_result(f, i == 4 ? 3 : i, pm.j[pm.c[i]], json);
            avl_set_competitor_position(pm.j[pm.c[i]]->index, i == 4 ? 3 : i);
            db_set_category_positions(category, pm.j[pm.c[i]]->index, i);
        } else if (i <= nrprint &&
            (pm.j[pm.c[i]]->deleted & HANSOKUMAKE) == 0) {
            // Need a victory in Norwegian system to get a result.
            if (prop_get_int_val_cat(PROP_WIN_NEEDED_FOR_MEDAL, category) &&
		pm.wins[pm.c[i]] == 0)
                continue;

            write_result(f, i, pm.j[pm.c[i]], json);
            avl_set_competitor_position(pm.j[pm.c[i]]->index, i);
            db_set_category_positions(category, pm.j[pm.c[i]]->index, i);
        }
    }

    /* clean up */
    empty_pool_struct(&pm);
}

static void dqpool_results(FILE *f, gint category, struct judoka *ctg, gint num_judokas, struct compsys sys, cJSON *json)
{
    struct pool_matches pm;
    struct judoka *j1 = NULL;
    gint i;
    int gold = 0, silver = 0, bronze1 = 0, bronze2 = 0;

    fill_pool_struct(category, num_judokas, &pm, FALSE);

    i = num_matches(sys.system, num_judokas) +
        ((sys.system == SYSTEM_DPOOL || sys.system == SYSTEM_DPOOL3) ? 1 : 5);

    if (sys.system == SYSTEM_DPOOL3) {
        if (COMP_1_PTS_WIN(pm.m[i]))
            bronze1 = pm.m[i].blue;
        else
            bronze1 = pm.m[i].white;

        i++;
    } else {
        /* first semifinal */
        if (COMP_1_PTS_WIN(pm.m[i]))
            bronze1 = pm.m[i].white;
        else
            bronze1 = pm.m[i].blue;

        i++;

        /* second semifinal */
        if (COMP_1_PTS_WIN(pm.m[i]))
            bronze2 = pm.m[i].white;
        else
            bronze2 = pm.m[i].blue;

        i++;
    }

    /* final */
    if (COMP_1_PTS_WIN(pm.m[i]) || pm.m[i].white == GHOST) {
        gold = pm.m[i].blue;
        silver = pm.m[i].white;
    } else if (COMP_2_PTS_WIN(pm.m[i]) || pm.m[i].blue == GHOST) {
        gold = pm.m[i].white;
        silver = pm.m[i].blue;
    }

    if (gold && (j1 = get_data(gold))) {
        write_result(f, 1, j1, json);
        avl_set_competitor_position(gold, 1);
        db_set_category_positions(category, gold, 1);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(silver))) {
        write_result(f, 2, j1, json);
        avl_set_competitor_position(silver, 2);
        db_set_category_positions(category, silver, 2);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(bronze1))) {
        write_result(f, 3, j1, json);
        avl_set_competitor_position(bronze1, 3);
        db_set_category_positions(category, bronze1, 3);
        free_judoka(j1);
    }
    if (sys.system != SYSTEM_DPOOL3 && gold && (j1 = get_data(bronze2))) {
        write_result(f, 3, j1, json);
        avl_set_competitor_position(bronze2, 3);
        db_set_category_positions(category, bronze2, 4);
        free_judoka(j1);
    }

    /* clean up */
    empty_pool_struct(&pm);
}

#define GET_WINNER_AND_LOSER(_w)                                        \
    winner = (COMP_1_PTS_WIN(m[_w]) || m[_w].white==GHOST) ? m[_w].blue : \
        ((COMP_2_PTS_WIN(m[_w]) || m[_w].blue==GHOST) ? m[_w].white : 0); \
    loser = (COMP_1_PTS_WIN(m[_w]) || m[_w].white==GHOST) ? m[_w].white : \
        ((COMP_2_PTS_WIN(m[_w]) || m[_w].blue==GHOST) ? m[_w].blue : 0)

#define GET_GOLD(_w)                                                    \
    gold = (COMP_1_PTS_WIN(m[_w]) || m[_w].white==GHOST) ? m[_w].blue : \
        ((COMP_2_PTS_WIN(m[_w]) || m[_w].blue==GHOST) ? m[_w].white : 0); \
    silver = (COMP_1_PTS_WIN(m[_w]) || m[_w].white==GHOST) ? m[_w].white : \
        ((COMP_2_PTS_WIN(m[_w]) || m[_w].blue==GHOST) ? m[_w].blue : 0)

#define GET_BRONZE1(_w)                                                 \
    bronze1 = (COMP_1_PTS_WIN(m[_w]) || m[_w].white==GHOST) ? m[_w].blue : \
        ((COMP_2_PTS_WIN(m[_w]) || m[_w].blue==GHOST) ? m[_w].white : 0); \
    fifth1 = (COMP_1_PTS_WIN(m[_w]) && m[_w].white > GHOST) ? m[_w].white : \
        ((COMP_2_PTS_WIN(m[_w]) && m[_w].blue > GHOST) ? m[_w].blue : 0)

#define GET_BRONZE2(_w)                                                 \
    bronze2 = (COMP_1_PTS_WIN(m[_w]) || m[_w].white==GHOST) ? m[_w].blue : \
        ((COMP_2_PTS_WIN(m[_w]) || m[_w].blue==GHOST) ? m[_w].white : 0); \
    fifth2 = (COMP_1_PTS_WIN(m[_w]) && m[_w].white > GHOST) ? m[_w].white : \
        ((COMP_2_PTS_WIN(m[_w]) && m[_w].blue > GHOST) ? m[_w].blue : 0)

static void french_results(FILE *f, gint category, struct judoka *ctg,
                           gint num_judokas, struct compsys systm, gint pagenum, cJSON *json)
{
    struct match m[NUM_MATCHES];
    struct judoka *j1;
    gint gold = 0, silver = 0, bronze1 = 0, bronze2 = 0, fourth = 0,
        fifth1 = 0, fifth2 = 0, seventh1 = 0, seventh2 = 0;
    gint winner= 0, loser = 0;
    gint sys = systm.system - SYSTEM_FRENCH_8;
    gint table = systm.table;

    memset(m, 0, sizeof(m));
    db_read_category_matches(category, m);

    GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 1, 1));
    gold = winner;
    silver = loser;
    if (table == TABLE_MODIFIED_DOUBLE_ELIMINATION) {
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 2, 1));
        silver = winner;
        bronze1 = loser;
    } else if (one_bronze(table, sys)) {
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 3, 1));
        bronze1 = winner;
        fourth = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 5, 1));
        fifth1 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 5, 2));
        fifth2 = loser;
    } else if (table == TABLE_DOUBLE_LOST) {
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 3, 1));
        bronze1 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 3, 2));
        bronze2 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 5, 1));
        fifth1 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 5, 2));
        fifth2 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 7, 1));
        seventh1 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 7, 2));
        seventh2 = loser;
    } else {
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 3, 1));
        bronze1 = winner;
        fifth1 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 3, 2));
        bronze2 = winner;
        fifth2 = loser;

        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 7, 1));
        seventh1 = loser;
        GET_WINNER_AND_LOSER(get_abs_matchnum_by_pos(systm, 7, 2));
        seventh2 = loser;
    }

    if (table == TABLE_NO_REPECHAGE ||
        table == TABLE_ESP_DOBLE_PERDIDA) {
	bronze1 = fifth1;
	bronze2 = fifth2;
	fifth1 = fifth2 = 0;
        seventh1 = seventh2 = 0;
    }

    if (gold && (j1 = get_data(gold))) {
        write_result(f, 1, j1, json);
        avl_set_competitor_position(gold, 1);
        db_set_category_positions(category, gold, 1);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(silver))) {
        write_result(f, 2, j1, json);
        avl_set_competitor_position(silver, 2);
        db_set_category_positions(category, silver, 2);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(bronze1))) {
        write_result(f, 3, j1, json);
        avl_set_competitor_position(bronze1, 3);
        db_set_category_positions(category, bronze1, 3);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(bronze2))) {
        write_result(f, 3, j1, json);
        avl_set_competitor_position(bronze2, 3);
        db_set_category_positions(category, bronze2, 4);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(fourth))) {
        write_result(f, 4, j1, json);
        avl_set_competitor_position(fourth, 4);
        db_set_category_positions(category, fourth, 4);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(fifth1))) {
        write_result(f, 5, j1, json);
        avl_set_competitor_position(fifth1, 5);
        db_set_category_positions(category, fifth1, 5);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(fifth2))) {
        write_result(f, 5, j1, json);
        avl_set_competitor_position(fifth2, 5);
        db_set_category_positions(category, fifth2, 6);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(seventh1))) {
        write_result(f, 7, j1, json);
        avl_set_competitor_position(seventh1, 7);
        db_set_category_positions(category, seventh1, 7);
        free_judoka(j1);
    }
    if (gold && (j1 = get_data(seventh2))) {
        write_result(f, 7, j1, json);
        avl_set_competitor_position(seventh2, 7);
        db_set_category_positions(category, seventh2, 8);
        free_judoka(j1);
    }
}

static void write_cat_result(FILE *f, gint category, cJSON *catlist)
{
    gint num_judokas;
    struct judoka *ctg = NULL;
    struct compsys sys;
    cJSON *jsoncat = NULL, *jsoncomp = NULL;

    if (catlist)
        jsoncat = cJSON_CreateObject();

    ctg = get_data(category);

    if (ctg == NULL || ctg->last == NULL || ctg->last[0] == '?' || ctg->last[0] == '_')
        return;

    if (!gdpr_ok(ctg))
	return;

    /* find system */
    sys = db_get_system(category);
    num_judokas = sys.numcomp;

    fprintf(f, "<tr><td colspan=\"3\"><b><a href=\"%s.html\">%s</a></b> (%d) </td></tr>\r\n",
            txt2hex(ctg->last), utf8_to_html(ctg->last), num_judokas);

    if (jsoncat) {
        cJSON_AddStringToObject(jsoncat, "category", ctg->last);
        cJSON_AddNumberToObject(jsoncat, "numcomp", num_judokas);
        cJSON_AddNumberToObject(jsoncat, "clubtext", club_text);
        cJSON_AddNumberToObject(jsoncat, "nameord", firstname_lastname());
        jsoncomp = cJSON_CreateArray();
        cJSON_AddItemToObject(jsoncat, "competitors", jsoncomp);
    }
    
    switch (sys.system) {
    case SYSTEM_POOL:
    case SYSTEM_DPOOL2:
    case SYSTEM_BEST_OF_3:
        pool_results(f, category, ctg, num_judokas, sys.system == SYSTEM_DPOOL2, jsoncomp);
        break;

    case SYSTEM_DPOOL:
    case SYSTEM_DPOOL3:
    case SYSTEM_QPOOL:
        dqpool_results(f, category, ctg, num_judokas, sys, jsoncomp);
        break;

    case SYSTEM_FRENCH_8:
    case SYSTEM_FRENCH_16:
    case SYSTEM_FRENCH_32:
    case SYSTEM_FRENCH_64:
    case SYSTEM_FRENCH_128:
        french_results(f, category, ctg, num_judokas,
                       sys, 0, jsoncomp);
        break;
    }

    if (jsoncat && catlist)
        cJSON_AddItemToArray(catlist, jsoncat);
    
    /* clean up */
    free_judoka(ctg);
}

void write_results(FILE *f)
{
    GtkTreeIter iter;
    gboolean ok;

    if (create_statistics)
        init_club_tree();

    fprintf(f, "<td valign=\"top\"><table class=\"resultlist\">\n"
            "<tr><td colspan=\"3\" align=\"center\"><h2>%s</h2></td></tr>\r\n",
            _T(results));

    ok = gtk_tree_model_get_iter_first(current_model, &iter);

    while (ok) {
        gint n = gtk_tree_model_iter_n_children(current_model, &iter);
        gint index;

        if (n >= 1 && n <= NUM_COMPETITORS) {
            gtk_tree_model_get(current_model, &iter,
                               COL_INDEX, &index,
                               -1);
            write_cat_result(f, index, NULL);
        }

        ok = gtk_tree_model_iter_next(current_model, &iter);
    }
    fprintf(f, "</table></td>\n");
}

static FILE *open_write(gchar *filename)
{
    gchar *file = g_build_filename(current_directory, filename, NULL);
    FILE *f = g_fopen(file, "wb");
    g_free(file);
    return f;
}

static FILE *open_read(gchar *filename)
{
    gchar *file = g_build_filename(current_directory, filename, NULL);
    FILE *f = g_fopen(file, "rb");
    g_free(file);
    return f;
}

static void print_embedded_text(FILE *f, gchar *name, struct compsys systm, gint pagenum)
{
    gint w, h;
    if (get_svg_size(systm, pagenum, &w, &h) < 0) {
        return;
    }

    if (pagenum)
        fprintf(f,
                "<object data=\"%s-%d.svg\" type=\"image/svg+xml\" width=\"%d\" height=\"%d\"></object>\r\n",
                name, pagenum, w, h);
    else
        fprintf(f,
                "<object data=\"%s.svg\" type=\"image/svg+xml\" width=\"%d\" height=\"%d\"></object>\r\n",
                name, w, h);

#if 0
    if (pagenum)
        fprintf(f,
                "\r\n<!--[if !IE]>-->\r\n"
                "<object data=\"%s-%d.svg\" type=\"image/svg+xml\" class=\"catimg\"\r\n"
                "width=\"%d\" height=\"%d\" id=\"svgobj%d\"> <!--<![endif]-->\r\n"
                "<!--[if lt IE 9]>\r\n"
                "<object src=\"%s-%d.svg\" classid=\"image/svg+xml\"\r\n"
                "width=\"%d\" height=\"%d\" id=\"svgobj%d\"> <![endif]-->\r\n"
                "<!--[if gte IE 9]>\r\n"
                "<object data=\"%s-%d.svg\" type=\"image/svg+xml\"\r\n"
                "width=\"%d\" height=\"%d\" id=\"svgobj%d\"> <![endif]-->\r\n"
                "</object>\r\n", name, pagenum, w, h, pagenum,
                name, pagenum, w, h, pagenum, name, pagenum, w, h, pagenum);
    else
        fprintf(f,
                "\r\n<!--[if !IE]>-->\r\n"
                "<object data=\"%s.svg\" type=\"image/svg+xml\" class=\"catimg\"\r\n"
                "width=\"%d\" height=\"%d\" id=\"svgobj%d\"> <!--<![endif]-->\r\n"
                "<!--[if lt IE 9]>\r\n"
                "<object src=\"%s.svg\" classid=\"image/svg+xml\"\r\n"
                "width=\"%d\" height=\"%d\" id=\"svgobj%d\"> <![endif]-->\r\n"
                "<!--[if gte IE 9]>\r\n"
                "<object data=\"%s.svg\" type=\"image/svg+xml\"\r\n"
                "width=\"%d\" height=\"%d\" id=\"svgobj%d\"> <![endif]-->\r\n"
                "</object>\r\n", name, w, h, pagenum,
                name, w, h, pagenum, name, w, h, pagenum);
    // <embed src=\"%s.svg\" type=\"image/svg+xml\" class=\"catimg\"/>
#endif
}

static void copy_from_file(gchar *fname, FILE *f)
{
    FILE *fmap = open_read(fname);
    if (fmap) {
        gint n, x1, y1, x2, y2, judoka;

        fprintf(f, "<map name=\"judokas\">\r\n");

        if (create_statistics) {
            while ((n = fscanf(fmap, "%d,%d,%d,%d,%d\n", &x1, &y1, &x2, &y2, &judoka)) == 5)
                fprintf(f, "<area shape=\"rect\" coords=\"%d,%d,%d,%d\" href=\"%d.html\">\r\n",
                        x1, y1, x2, y2, judoka);
        }

        fprintf(f, "</map>\r\n");
        fclose(fmap);
    }
}

void write_html(gint cat)
{
    gchar buf[64];
    gchar *hextext;
    gint i;

    struct judoka *j = get_data(cat);
    if (!j)
        return;

    progress_show(-1.0, j->last);

    struct compsys sys = db_get_system(cat);

    hextext = txt2hex(j->last);
    snprintf(buf, sizeof(buf), "%s.html", hextext);

    FILE *f = open_write(buf);
    if (!f)
        goto out;

    gdpr_enable++;
    make_top_frame_1(f, "<script defer=\"defer\" type=\"text/javascript\" src=\"sie.js\"></script>\r\n"
                     "<script type=\"text/javascript\" src=\"jquery-1.10.2.min.js\" charset=\"utf-8\"></script>\r\n"
                     "<script type=\"text/javascript\" src=\"coach.js\" charset=\"utf-8\"></script>\r\n",
		     TRUE);
    make_left_frame(f);

    hextext = txt2hex(j->last);

    if (print_svg) {
        fprintf(f, "<td valign=\"top\"><table><tr class=\"cattr1\"><td>"
                "<div class=\"catdiv\">");
        print_embedded_text(f, hextext, sys, 0);
        fprintf(f, "</div></td></tr>\r\n");

        for (i = 1; i < num_pages(sys); i++) {
            fprintf(f, "  <tr class=\"cattr2\"><td><div class=\"catdiv\">");
            print_embedded_text(f, hextext, sys, i);
            fprintf(f, "</div></td></tr>\r\n");
        }
    } else {
        fprintf(f, "<td valign=\"top\"><table><tr class=\"cattr1\"><td>"
                "<div class=\"catdiv\"><img src=\"%s.png\" class=\"catimg\" usemap=\"#judokas\"/>", hextext);

        snprintf(buf, sizeof(buf), "%s.map", hextext);
        copy_from_file(buf, f);

        fprintf(f, "</div></td></tr>\r\n");

        for (i = 1; i < num_pages(sys); i++) {
            fprintf(f, "  <tr class=\"cattr2\"><td><div class=\"catdiv\">"
                    "<img src=\"%s-%d.png\" class=\"catimg\" usemap=\"#judokas%d\"/>\r\n", hextext, i, i);
            snprintf(buf, sizeof(buf), "%s-%d.map", hextext, i);
            copy_from_file(buf, f);
            fprintf(f, "</div></td></tr>\r\n");
        }
    }

    fprintf(f, "  </table><br><a href=\"%d.html\" class=\"dflt\">%s</a>\r\n</td>\r\n", cat, _("Matches"));

    make_bottom_frame(f);
    fclose(f);

    write_png(NULL, gint_to_ptr(cat));
out:
    free_judoka(j);
    gdpr_enable--;
}

void match_statistics(FILE *f, cJSON *json)
{
    extern gint num_categories;
    extern struct cat_def category_definitions[];

    gint i, k;
    gint toti = 0, totw = 0, toty = 0, toth = 0, tott = 0, tots = 0, totc = 0;
    gint totsw = 0, totg = 0;
    gchar *cmd = g_strdup_printf("select * from matches");
    gint numrows = db_get_table(cmd);
    g_free(cmd);

    if (numrows < 0)
        goto out;

    for (i = 0; i < num_categories; i++) {
        memset(&category_definitions[i].stat, 0, sizeof(category_definitions[0].stat));
    }

    for (k = 0; k < numrows; k++) {
        gint cat = atoi(db_get_data(k, "category"));
        struct judoka *j = get_data(cat);
        if (!j)
            continue;
        i = find_age_index(j->last);
        free_judoka(j);

        if (i < 0)
            continue;

        gint blue_points = atoi(db_get_data(k, "blue_points"));
        gint white_points = atoi(db_get_data(k, "white_points"));
        gint blue_score = atoi(db_get_data(k, "blue_score"));
        gint white_score = atoi(db_get_data(k, "white_score"));
        gint time = atoi(db_get_data(k, "time"));
        gint legend = atoi(db_get_data(k, "legend"));

        if (blue_points == 0 && white_points == 0)
            continue;

        category_definitions[i].stat.total++;
        category_definitions[i].stat.time += time;

        if (blue_points == 10 || white_points == 10)
            category_definitions[i].stat.ippons++;
        else if (blue_points == 7 || white_points == 7)
            category_definitions[i].stat.wazaaris++;
        else if (blue_points == 5 || white_points == 5)
            category_definitions[i].stat.yukos++;
        else if (blue_points == 3 || white_points == 3)
            category_definitions[i].stat.kokas++;
        else if (/*prop_get_int_val(PROP_EQ_SCORE_LESS_SHIDO_WINS) &&*/
                 (blue_score & 0xffff0) == (white_score & 0xffff0))
            category_definitions[i].stat.shidowins++;
        else
            category_definitions[i].stat.hanteis++;

        if ((legend & 0x300) == 0x100)
            category_definitions[i].stat.goldenscores++;
    }

    db_close_table();

    /* competitors */
    cmd = g_strdup_printf("select * from competitors where \"deleted\"&1=0");
    numrows = db_get_table(cmd);
    g_free(cmd);

    if (numrows < 0)
        goto out;

    for (k = 0; k < numrows; k++) {
        gchar *cat = db_get_data(k, "category");
        if (!cat)
            continue;
        i = find_age_index(cat);
        if (i < 0)
            continue;
        category_definitions[i].stat.comp++;
    }

    db_close_table();
out:
    for (i = 0; i < num_categories; i++) {
        totc += category_definitions[i].stat.comp;
        toti += category_definitions[i].stat.ippons;
        totw += category_definitions[i].stat.wazaaris;
        toty += category_definitions[i].stat.yukos;
        toth += category_definitions[i].stat.hanteis;
        totsw+= category_definitions[i].stat.shidowins;
        totg += category_definitions[i].stat.goldenscores;
        tott += category_definitions[i].stat.total;
        tots += category_definitions[i].stat.time;
    }

    fprintf(f, "<td valign=\"top\"><table class=\"statistics\">\n"
            "<tr><td class=\"stat2\" colspan=\"%d\"><h2>%s</h2></td></tr>\r\n"
            "<tr><th class=\"stat1\">%s:<th>%s", num_categories + 2,
            _T(statistics), _T(category), _T(total));
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total)
            fprintf(f, "<th>%s", category_definitions[i].agetext);

    fprintf(f, "\n<tr><td class=\"stat1\">%s:<td>%d", _T(competitor), totc);
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total)
            fprintf(f, "<td>%d", category_definitions[i].stat.comp);

    fprintf(f, "\n<tr><td class=\"stat1\">%s:<td>%d", _T(matches), tott);
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total)
            fprintf(f, "<td>%d", category_definitions[i].stat.total);

    fprintf(f, "\n<tr><td class=\"stat1\">Ippon:<td>%d", toti);
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total)
            fprintf(f, "<td>%d", category_definitions[i].stat.ippons);

    fprintf(f, "\n<tr><td class=\"stat1\">Waza-ari:<td>%d", totw);
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total)
            fprintf(f, "<td>%d", category_definitions[i].stat.wazaaris);

    fprintf(f, "\n<tr><td class=\"stat1\">Yuko:<td>%d", toty);
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total)
            fprintf(f, "<td>%d", category_definitions[i].stat.yukos);

    if (TRUE /*prop_get_int_val(PROP_EQ_SCORE_LESS_SHIDO_WINS)*/) {
        fprintf(f, "\n<tr><td class=\"stat1\">Shido:<td>%d", totsw);
        for (i = 0; i < num_categories; i++)
            if (category_definitions[i].stat.total)
                fprintf(f, "<td>%d", category_definitions[i].stat.shidowins);
    } else {
        fprintf(f, "\n<tr><td class=\"stat1\">Hantei:<td>%d", toth);
        for (i = 0; i < num_categories; i++)
            if (category_definitions[i].stat.total)
                fprintf(f, "<td>%d", category_definitions[i].stat.hanteis);
    }

    fprintf(f, "\n<tr><td class=\"stat1\">Golden score:<td>%d", totg);
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total)
            fprintf(f, "<td>%d", category_definitions[i].stat.goldenscores);

    fprintf(f, "\n<tr><td class=\"stat1\">%s:<td>%d:%02d", _T(time),
            tott ? (tots/tott)/60 : 0, tott ? (tots/tott)%60 : 0);
    for (i = 0; i < num_categories; i++)
        if (category_definitions[i].stat.total) {
            gint tmo = category_definitions[i].stat.time /
                category_definitions[i].stat.total;
            fprintf(f, "<td>%d:%02d", tmo/60, tmo%60);
        }

    fprintf(f, "</table></td>\n");

    if (json) {
        cJSON *e;
        for (i = 0; i < num_categories; i++) {
            e = cJSON_CreateObject();
            cJSON_AddStringToObject(e, "cat", category_definitions[i].agetext);
            cJSON_AddNumberToObject(e, "competitors", category_definitions[i].stat.comp);
            cJSON_AddNumberToObject(e, "matches", category_definitions[i].stat.total);
            cJSON_AddNumberToObject(e, "ippons", category_definitions[i].stat.ippons);
            cJSON_AddNumberToObject(e, "wazaaris", category_definitions[i].stat.wazaaris);
            cJSON_AddNumberToObject(e, "shidowins", category_definitions[i].stat.shidowins);
            cJSON_AddNumberToObject(e, "goldenscores", category_definitions[i].stat.goldenscores);
            gint tmo = category_definitions[i].stat.total > 0 ?
                category_definitions[i].stat.time / category_definitions[i].stat.total : 0;
            cJSON_AddNumberToObject(e, "time", tmo);
            cJSON_AddItemToArray(json, e);
        }
        e = cJSON_CreateObject();
        cJSON_AddStringToObject(e, "cat", "total");
        cJSON_AddNumberToObject(e, "competitors", totc);
        cJSON_AddNumberToObject(e, "matches", tott);
        cJSON_AddNumberToObject(e, "ippons", toti);
        cJSON_AddNumberToObject(e, "wazaaris", totw);
        cJSON_AddNumberToObject(e, "shidowins", totsw);
        cJSON_AddNumberToObject(e, "goldenscores", totg);
        cJSON_AddNumberToObject(e, "time", tott > 0 ? (tots/tott) : 0);
        cJSON_AddItemToArray(json, e);
    }
}

void write_comp_stat(gint index)
{
    gchar buf[32];
    gint i;
    gchar *cmd = NULL;
    gint numrows = 0;
    struct category_data *catdata = NULL;
    gint catix = 0;
    struct judoka *j = get_data(index);
    if (!j)
        return;

    snprintf(buf, sizeof(buf), "%d.html", index);

    FILE *f = open_write(buf);
    if (!f)
        goto out;

    make_top_frame(f);
    make_left_frame(f);

    if (index >= 10000) catdata = avl_get_category(index);
    if (catdata) {
	catix = catdata->index;
        fprintf(f, "<td valign=\"top\"><table class=\"compstat\">"
                "<tr><th colspan=\"7\">%s</th></tr>\r\n"
                "<tr><td class=\"cshdr\">#<td class=\"cshdr\">%s<td class=\"cshdr\">%s"
                "<td align=\"center\" class=\"cshdr\">%s<td class=\"cshdr\">%s"
                "<td class=\"cshdr\">%s<td class=\"cshdr\">%s</tr>\r\n",
                j->last, _T(name),
		(prop_get_int_val_cat(PROP_RULES_2017, catix) ||
		 prop_get_int_val_cat(PROP_RULES_2018, catix)) ? "IW/S" : "IWY/S",
		_T(points),
		(prop_get_int_val_cat(PROP_RULES_2017, catix) ||
		 prop_get_int_val_cat(PROP_RULES_2018, catix)) ? "IW/S" : "IWY/S",
		_T(name), _T(time));

        db_print_category_matches(catdata, f);

        goto end_cat;
    }

    fprintf(f, "<td valign=\"top\"><table class=\"compstat\">"
            "<tr><th colspan=\"7\">%s %s, %s</th></tr>\r\n"
            "<tr><td class=\"cshdr\">%s<td class=\"cshdr\">%s<td class=\"cshdr\">%s"
            "<td align=\"center\" class=\"cshdr\">%s<td class=\"cshdr\">%s"
            "<td class=\"cshdr\">%s<td class=\"cshdr\">%s</tr>\r\n",
            utf8_to_html(firstname_lastname() ? j->first : j->last),
            utf8_to_html(firstname_lastname() ? j->last : j->first),
            utf8_to_html(j->club),
            _T(category), _T(name),
	    (prop_get_int_val_cat(PROP_RULES_2017, catix) ||
	     prop_get_int_val_cat(PROP_RULES_2018, catix)) ? "IW/S" : "IWY/S",
	    _T(points),
	    (prop_get_int_val_cat(PROP_RULES_2017, catix) ||
	     prop_get_int_val_cat(PROP_RULES_2018, catix)) ? "IW/S" : "IWY/S",
	    _T(name), _T(time));


        cmd = g_strdup_printf("select * from matches where \"blue\"=%d or \"white\"=%d "
                              "order by \"category\"",
                              index, index);
        numrows = db_get_table(cmd);
        g_free(cmd);

    if (numrows < 0)
        goto out;

    for (i = 0; i < numrows; i++) {
        gint blue = atoi(db_get_data(i, "blue"));
        gint white = atoi(db_get_data(i, "white"));
        gint cat = atoi(db_get_data(i, "category"));
        catdata = avl_get_category(cat);
        struct judoka *j1 = get_data(blue);
        struct judoka *j2 = get_data(white);
        struct judoka *c = get_data(cat);
        if (j1 == NULL || j2 == NULL || c == NULL)
            goto done;

        if (catdata && (catdata->deleted & TEAM_EVENT) && (cat & MATCH_CATEGORY_SUB_MASK) == 0)
            goto done;

        {
            gint blue_score = atoi(db_get_data(i, "blue_score"));
            gint white_score = atoi(db_get_data(i, "white_score"));
            gint blue_points = atoi(db_get_data(i, "blue_points"));
            gint white_points = atoi(db_get_data(i, "white_points"));
            gint mtime = atoi(db_get_data(i, "time"));
            if (blue_points || white_points) {
		gboolean c1_wins = blue_points > white_points;
                fprintf(f,
                        "<tr><td "
                        "onclick=\"top.location.href='%s.html'\" "
                        "style=\"cursor: pointer\""
                        ">%s</td><td "
                        "onclick=\"top.location.href='%d.html'\" "
                        "style=\"cursor: pointer;%s\""
                        ">%s %s</td><td class=\"%s\">",
                        txt2hex(c->last),
                        utf8_to_html(c->last),
                        j1->index,
			c1_wins ? "font-weight:bold" : "",
                        utf8_to_html(firstname_lastname() ? j1->first : j1->last),
                        utf8_to_html(firstname_lastname() ? j1->last : j1->first),
                        prop_get_int_val(PROP_WHITE_FIRST) ? "wscore" : "bscore");

		if (prop_get_int_val_cat(PROP_RULES_2017, cat) ||
		    prop_get_int_val_cat(PROP_RULES_2018, cat))
		    fprintf(f,
			    "%d%d/%d%s",
			    (blue_score>>16)&15, (blue_score>>12)&15,
			    blue_score&7, blue_score&8?"H":"");
		else
		    fprintf(f,
			    "%d%d%d/%d%s",
			    (blue_score>>16)&15, (blue_score>>12)&15, (blue_score>>8)&15,
			    blue_score&7, blue_score&8?"H":"");

		fprintf(f,
                        "</td><td align=\"center\">%s - %s</td>"
                        "<td class=\"%s\">",
                        get_points_str(blue_points, catix),
                        get_points_str(white_points, catix),
                        prop_get_int_val_cat(PROP_WHITE_FIRST, cat) ? "bscore" : "wscore");

		if (prop_get_int_val_cat(PROP_RULES_2017, cat) ||
		    prop_get_int_val_cat(PROP_RULES_2018, cat))
		    fprintf(f,
			    "%d%d/%d%s",
			    (white_score>>16)&15, (white_score>>12)&15,
			    white_score&7, white_score&8?"H":"");
		else
		    fprintf(f,
			    "%d%d%d/%d%s",
			    (white_score>>16)&15, (white_score>>12)&15, (white_score>>8)&15,
			    white_score&7, white_score&8?"H":"");

		fprintf(f,
                        "</td><td "
                        "onclick=\"top.location.href='%d.html'\" "
                        "style=\"cursor: pointer;%s\""
                        ">%s %s</td><td>%d:%02d</td></tr>\r\n",
                        j2->index,
			c1_wins ? "" : "font-weight:bold",
                        utf8_to_html(firstname_lastname() ? j2->first : j2->last),
                        utf8_to_html(firstname_lastname() ? j2->last : j2->first), mtime/60, mtime%60);
	    }
        }
    done:
        free_judoka(j1);
        free_judoka(j2);
        free_judoka(c);
    }

    db_close_table();

 end_cat:
    fprintf(f, "</table></td>");

    make_bottom_frame(f);
 out:
    if (f)
        fclose(f);
    free_judoka(j);
}

void write_htmls(gint num_cats)
{
    GtkTreeIter iter;
    gboolean ok;
    gint cnt = 0;

    ok = gtk_tree_model_get_iter_first(current_model, &iter);

    while (ok) {
        gint n = gtk_tree_model_iter_n_children(current_model, &iter);
        gint index;

        cnt++;

        progress_show(cnt < num_cats ? 1.0*cnt/num_cats : 1.0, NULL);

        if (n >= 1 && n <= NUM_COMPETITORS) {
            gtk_tree_model_get(current_model, &iter,
                               COL_INDEX, &index,
                               -1);
            write_html(index);
        }

        ok = gtk_tree_model_iter_next(current_model, &iter);
    }

    progress_show(0.0, "");
}

static void init_club_data(void)
{
    /* competitors */
    gchar *cmd = g_strdup_printf("select * from competitors where \"deleted\"&1=0");
    gint numrows = db_get_table(cmd), k;
    g_free(cmd);

    if (numrows < 0)
        return;

    for (k = 0; k < numrows; k++) {
        gchar *club = db_get_data(k, "club");
        gchar *country = db_get_data(k, "country");
        if (!club)
            continue;
        club_stat_add(club, country, 0);
    }

    db_close_table();
}

#define T mylog("LINE=%d t=%ld\n", __LINE__, time(NULL))

static gint make_png_all_id = 0;

static enum {
    MAKE_PNG_STATE_IDLE = 0,
    MAKE_PNG_STATE_INDEX_HTML_1,
    MAKE_PNG_STATE_INDEX_HTML_2, // 15 sec
    MAKE_PNG_STATE_INDEX_HTML_3,
    MAKE_PNG_STATE_NEXT_MATCHES,
    MAKE_PNG_STATE_CAT_HTMLS_1,
    MAKE_PNG_STATE_CAT_HTMLS_2, // 3 sec
    MAKE_PNG_STATE_COMPETITORS_HTML_1,
    MAKE_PNG_STATE_COMPETITORS_HTML_2,
    MAKE_PNG_STATE_COMPETITORS_HTML_3,
    MAKE_PNG_STATE_COMPETITORS2_HTML_1,
    MAKE_PNG_STATE_COMPETITORS2_HTML_2,
    MAKE_PNG_STATE_COMPETITORS2_HTML_3,
    MAKE_PNG_STATE_MEDALS,
    MAKE_PNG_STATE_COMP_STAT, // 23 s
    MAKE_PNG_STATE_CAT_MATCHES,
    MAKE_PNG_STATE_STATISTICS_HTML
}
    make_png_all_state = MAKE_PNG_STATE_IDLE;

static gint competitor_table[TOTAL_NUM_COMPETITORS];
static gint competitor_count;

static gint list_categories(gint *num_comp)
{
    GtkTreeIter iter;
    gboolean ok;
    gint comps = 0;

    competitor_count = 0;
    ok = gtk_tree_model_get_iter_first(current_model, &iter);

    while (ok) {
        gint n = gtk_tree_model_iter_n_children(current_model, &iter);
        gint index;

        if (n >= 1 && n <= NUM_COMPETITORS) {
            gtk_tree_model_get(current_model, &iter,
                               COL_INDEX, &index,
                               -1);
            if (competitor_count < TOTAL_NUM_COMPETITORS)
                competitor_table[competitor_count++] = index;
            comps += n;
        }

        ok = gtk_tree_model_iter_next(current_model, &iter);
    }

    if (num_comp) *num_comp = comps;

    return competitor_count;
}

static int copy_file_cb(const char *path,
       const struct stat *st,
       int flag)
{
    gchar *p = strstr(path, "results");
    if (p) {
        gchar *dst = g_build_filename(current_directory, p+8, NULL);
        if (flag == FTW_D) {
            g_mkdir_with_parents(dst, 0777);
        } else if (flag == FTW_F) {
            gchar *contents = NULL;
            gsize length;
            if (g_file_get_contents(path, &contents, &length, NULL)) {
                g_file_set_contents(dst, contents, length, NULL);
                g_free(contents);
            }
        }
        g_free(dst);
    }
    return 0;
}

static void write_json(gchar *filename, cJSON *json)
{
    char *txt = cJSON_Print(json);
    if (txt) {
        gchar *file = g_build_filename(current_directory, filename, NULL);
        g_file_set_contents(file, txt, -1, NULL);
        g_free(file);
        free(txt);
    }
}

static gboolean make_png_all_bg(gpointer user_data)
{
    static FILE *f = NULL;
    static cJSON *jsonroot = NULL;
    //static gchar fname[1024];
    static gint i, num_cats, num_comps;

    num_cats = num_cats; // make compiler happy

    switch (make_png_all_state) {

        /*
         * clean up
         */
    case MAKE_PNG_STATE_IDLE:
        {
            gchar *src = g_build_filename(installation_dir, "etc", "web", "results", NULL);
            gchar *dst = g_build_filename(current_directory, NULL);
            g_mkdir_with_parents(dst, 0777);
            g_free(dst);
            mylog("COPYING %s\n", src);
            ftw(src, copy_file_cb, 1);
            g_free(src);
            src = g_build_filename(current_directory, "index.html", NULL);
            dst = g_build_filename(current_directory, "index1.html", NULL);
            g_rename(src, dst);
            g_free(src);
            g_free(dst);
        }
    
        if (f) fclose(f);
        f = NULL;
        num_cats = gtk_tree_model_iter_n_children(current_model, NULL);
        make_png_all_state = MAKE_PNG_STATE_INDEX_HTML_1;
	write_extra_link_files();

        if (jsonroot) cJSON_Delete(jsonroot);
        jsonroot = cJSON_CreateObject();
        if (jsonroot) {
            cJSON_AddStringToObject(jsonroot, "name", prop_get_str_val(PROP_NAME));
            cJSON_AddStringToObject(jsonroot, "date", prop_get_str_val(PROP_DATE));
            cJSON_AddStringToObject(jsonroot, "place", prop_get_str_val(PROP_PLACE));
            write_json("info.json", jsonroot);
            cJSON_Delete(jsonroot);
            jsonroot = NULL;
        }
        
        return TRUE;

        /*
         * index.html
         */
    case MAKE_PNG_STATE_INDEX_HTML_1:
        f = open_write("index.html");
        if (!f) return FALSE;

        jsonroot = cJSON_CreateArray();

        make_top_frame(f);
        make_left_frame(f);
        if (create_statistics)
            init_club_tree();

        fprintf(f, "<td valign=\"top\"><table class=\"resultlist\">\n"
                "<tr><td colspan=\"3\" align=\"center\"><h2>%s</h2></td></tr>\r\n",
                _T(results));

        num_cats = list_categories(&num_comps);
        i = 0;
        make_png_all_state = MAKE_PNG_STATE_INDEX_HTML_2;
        return TRUE;

    case MAKE_PNG_STATE_INDEX_HTML_2:
        // print one category result to index.html
        if (i < competitor_count) {
	    gdpr_enable++;
            write_cat_result(f, competitor_table[i++], jsonroot);
	    gdpr_enable--;

            if (create_statistics)
                progress_show(0.3*(gdouble)i/(gdouble)competitor_count, "");
            else
                progress_show(0.8*(gdouble)i/(gdouble)competitor_count, "");

            return TRUE;
        }
        make_png_all_state = MAKE_PNG_STATE_INDEX_HTML_3;
        return TRUE;

    case MAKE_PNG_STATE_INDEX_HTML_3:
        // finish page
        fprintf(f, "</table></td>\n");
        make_bottom_frame(f);
        fclose(f);
        f = NULL;

        if (jsonroot) {
            write_json("results.json", jsonroot);
            cJSON_Delete(jsonroot);
            jsonroot = NULL;
        }
        
        make_png_all_state = MAKE_PNG_STATE_NEXT_MATCHES;
        return TRUE;

        /*
         * netxmatches.html
         */
    case MAKE_PNG_STATE_NEXT_MATCHES:
        init_club_data();

        if (automatic_web_page_update) {
            make_next_matches_html(TRUE);
            make_next_matches_html(FALSE);
        }
        make_png_all_state = MAKE_PNG_STATE_CAT_HTMLS_1;
        return TRUE;

        /*
         * sheets
         */
    case MAKE_PNG_STATE_CAT_HTMLS_1:
        list_categories(NULL);
        i = 0;
        make_png_all_state = MAKE_PNG_STATE_CAT_HTMLS_2;
        return TRUE;

    case MAKE_PNG_STATE_CAT_HTMLS_2:
        if (i < competitor_count) {
            write_html(competitor_table[i++]);

            if (create_statistics)
                progress_show(0.3 + 0.06*(gdouble)i/(gdouble)competitor_count, "");
            else
                progress_show(0.8 + 0.2*(gdouble)i/(gdouble)competitor_count, "");

            return TRUE;
        }
        make_png_all_state = MAKE_PNG_STATE_COMPETITORS_HTML_1;
        return TRUE;

        /*
         * competitors
         */
    case MAKE_PNG_STATE_COMPETITORS_HTML_1:
        f = open_write("competitors.html");
        if (!f) return FALSE;

        if (jsonroot) cJSON_Delete(jsonroot);
        jsonroot = cJSON_CreateArray();
        
        make_top_frame_1(f, "<script type=\"text/javascript\" src=\"coach.js\" charset=\"utf-8\"></script>",
			 TRUE);
	gdpr_enable++;
        make_left_frame(f);
        saved_competitor_cnt = 0;
        db_list_competitors(FALSE);
	gdpr_enable--;

        fprintf(f,
                "<td><table class=\"competitors\">"
                "<tr><td colspan=\"5\" align=\"center\"><h2>%s</h2></td></tr>\n", _T(competitor));
        fprintf(f, "<tr><th>%s</th><th>%s</th><th><a href=\"competitors2.html\">%s</a></th><th>%s</th><th>&nbsp;%s&nbsp;</th></tr>\n",
                _T(name), grade_visible ? _T(grade) : "",
                (club_text & CLUB_TEXT_COUNTRY) ? _T(country) :_T(club),
                _T(category), create_statistics ? _T(position) : "");
        write_competitor_for_coach_display(NULL); // init file

        make_png_all_state = MAKE_PNG_STATE_COMPETITORS_HTML_2;
        return TRUE;

    case MAKE_PNG_STATE_COMPETITORS_HTML_2:
        i = db_get_next_listed_competitor();
        if (i) {
            struct judoka *j = get_data(i);
            if (j) {
		gdpr_enable++;
                write_competitor(f, j, CLUB_TEXT_ADDRESS, 0, jsonroot);
                write_competitor_for_coach_display(j);
		gdpr_enable--;
            }
            return TRUE;
        }
        make_png_all_state = MAKE_PNG_STATE_COMPETITORS_HTML_3;
        return TRUE;

    case MAKE_PNG_STATE_COMPETITORS_HTML_3:
        fprintf(f, "</table></td>\n");
        make_bottom_frame(f);
        fclose(f);
        f = NULL;

        if (jsonroot) {
            write_json("competitors.json", jsonroot);
            cJSON_Delete(jsonroot);
            jsonroot = NULL;
        }
        
        make_png_all_state = MAKE_PNG_STATE_COMPETITORS2_HTML_1;
        return TRUE;

        /*
         * competitors 2
         */
    case MAKE_PNG_STATE_COMPETITORS2_HTML_1:
        f = open_write("competitors2.html");
        if (!f) return FALSE;
        make_top_frame_1(f, "<script type=\"text/javascript\" src=\"coach.js\" charset=\"utf-8\"></script>",
			 TRUE);
	gdpr_enable++;
        make_left_frame(f);
        db_list_competitors(TRUE);
	gdpr_enable--;

        fprintf(f,
                "<td><table class=\"competitors\">"
                "<tr><td colspan=\"6\" align=\"center\"><h2>%s</h2></td></tr>\n", _T(competitor));
        fprintf(f, "<tr><th></th><th>%s</th><th><a href=\"competitors.html\">%s</a></th>"
                "<th>%s</th><th>%s</th><th>&nbsp;%s&nbsp;</th></tr>\n",
                _T(club), _T(name), grade_visible ? _T(grade) : "", _T(category), create_statistics ? _T(position) : "");

        make_png_all_state = MAKE_PNG_STATE_COMPETITORS2_HTML_2;
        return TRUE;

    case MAKE_PNG_STATE_COMPETITORS2_HTML_2:
        i = db_get_next_listed_competitor();
        if (i) {
            struct judoka *j = get_data(i);
            if (j) {
		gdpr_enable++;
                write_competitor(f, j, CLUB_TEXT_ADDRESS, TRUE, NULL);
		gdpr_enable--;
                //write_competitor_for_coach_display(j);
            }
            return TRUE;
        }
        make_png_all_state = MAKE_PNG_STATE_COMPETITORS2_HTML_3;
        return TRUE;

    case MAKE_PNG_STATE_COMPETITORS2_HTML_3:
        fprintf(f, "</table></td>\n");
        make_bottom_frame(f);
        fclose(f);
        f = NULL;
        make_png_all_state = MAKE_PNG_STATE_MEDALS;
        return TRUE;

    case MAKE_PNG_STATE_MEDALS:
        /* statistics */
        if (create_statistics) {
            /* medal list */
            f = open_write("medals.html");
            if (f) {
                if (jsonroot) cJSON_Delete(jsonroot);
                jsonroot = cJSON_CreateArray();
		gdpr_enable++;
                make_top_frame(f);
                make_left_frame(f);
                club_stat_print(f, jsonroot);
                make_bottom_frame(f);
		gdpr_enable--;
                fclose(f);
                f = NULL;
                if (jsonroot) {
                    write_json("medals.json", jsonroot);
                    cJSON_Delete(jsonroot);
                    jsonroot = NULL;
                }
            }
        }
        i = 0;
        make_png_all_state = MAKE_PNG_STATE_COMP_STAT;
        return TRUE;

    case MAKE_PNG_STATE_COMP_STAT:
        /* competitor stat */
        if (create_statistics) {
            if (i < saved_competitor_cnt) {
                //gchar buf[32];
                //snprintf(buf, sizeof(buf), "%s %d%%", _("Competitors:"), 100*i/saved_competitor_cnt);
		gdpr_enable++;
                write_comp_stat(saved_competitors[i]);
		gdpr_enable--;
                progress_show(0.36 + 0.64*(gdouble)i/(gdouble)saved_competitor_cnt, "");
            }
            if (++i < saved_competitor_cnt)
                return TRUE;
        }
        make_png_all_state = MAKE_PNG_STATE_CAT_MATCHES;
        return TRUE;

    case MAKE_PNG_STATE_CAT_MATCHES:
        /* category matches */
        if (create_statistics) {
            for (i = 1; i <= NUM_TATAMIS; i++) {
                struct category_data *queue = &category_queue[i];
                while (queue) {
		    gdpr_enable++;
                    write_comp_stat(queue->index);
		    gdpr_enable--;
                    queue = queue->next;
                }
            }
        }
        make_png_all_state = MAKE_PNG_STATE_STATISTICS_HTML;
        return TRUE;

    case MAKE_PNG_STATE_STATISTICS_HTML:
        /* statistics */
	gdpr_enable++;
        if (create_statistics) {
            f = open_write("statistics.html");
            if (f) {
                if (jsonroot) cJSON_Delete(jsonroot);
                jsonroot = cJSON_CreateArray();

                make_top_frame(f);
                make_left_frame(f);
                match_statistics(f, jsonroot);
                make_bottom_frame(f);
                fclose(f);
                f = NULL;

                if (jsonroot) {
                    write_json("statistics.json", jsonroot);
                    cJSON_Delete(jsonroot);
                    jsonroot = NULL;
                }
            }
        }
	gdpr_enable--;

        make_png_all_state = MAKE_PNG_STATE_IDLE;
	progress_show(0, "");
	return FALSE;
    }
    return FALSE;
}

static void make_png_all_end(gpointer user_data)
{
    make_png_all_id = 0;
    make_png_all_state = MAKE_PNG_STATE_IDLE;
    progress_show(0.0, "");
}

void make_png_all(GtkWidget *w, gpointer data)
{
    FILE *f;
    gchar fname[1024];
    gint /*num_cats = 0,*/ i;

    if (make_png_all_id)
        return;

    if (get_output_directory() < 0)
        return;

    // init coach info stuff
    if (automatic_web_page_update) {
        update_category_status_info_all();
        update_next_matches_coach_info();
    }

    /* make competitor positions = 0. write_results() will recalculate them. */
    avl_init_competitor_position();

    /* copy files */
    struct {
	gchar *dir;
	gchar *file;
    } files_to_copy[] =
	  {{"css", "style.css"}, {"html", "coach.html"},
	   {"js", "coach.js"},  {"png", "asc.png"}, {"png", "desc.png"},
	   {"png", "bg.png"}, {"png", "refresh.png"}, {"png", "clear.png"},
	   {"js", "jquery-1.10.2.min.js"}, {"js", "jquery-1.10.2.min.map"},
	   {"js", "jquery.tablesorter.js"}, {"js", "sie.js"}, {NULL, NULL}};
    for (i = 0; files_to_copy[i].dir; i++) {
        f = open_write(files_to_copy[i].file);
        if (f) {
            gint n;
            gchar *src = g_build_filename(installation_dir, "etc",
					  files_to_copy[i].dir,
					  files_to_copy[i].file,
					  NULL);
            FILE *sf = g_fopen(src, "rb");
            g_free(src);

            if (sf) {
                while ((n = fread(fname, 1, sizeof(fname), sf)) > 0) {
                    fwrite(fname, 1, n, f);
                }
                fclose(sf);
            } else
		mylog("Cannot read %s/%s\n", files_to_copy[i].dir, files_to_copy[i].file);
            fclose(f);
        }
    }

    /* dictionary file for javascripts */
    f = open_write("words.js");
    if (f) {
        gint txts[] = {notdrawntext, finishedtext, matchongoingtext, startedtext,
                       drawingreadytext, coachtext, nametext, surnametext, weighttext,
                       categorytext, statustext, positiontext, displaytext,
                       matchafter1text, matchafter2text, -1};
        gchar *vars[] = {"not_drawn", "finished", "match_ongoing", "started",
                         "drawing_ready", "coach", "firstname", "lastname", "weight",
                         "category", "status", "place", "display",
                         "match_after_1", "match_after_2", NULL};

        for (i = 0; txts[i] >= 0; i++)
            fprintf(f, "var txt_%s = \"%s\";\n", vars[i], print_texts[txts[i]][print_lang]);
        fclose(f);
    }

    make_png_all_state = MAKE_PNG_STATE_IDLE;
    make_png_all_id = g_idle_add_full(G_PRIORITY_LOW,
                                      make_png_all_bg,
                                      NULL,
                                      make_png_all_end);
    return;

    progress_show(0.0, "");
}

void make_next_matches_html(gboolean frame)
{
    FILE *f;
    gint i, k;

    if (strlen(current_directory) <= 1) {
        if (get_output_directory() < 0)
            return;
    }

    if (frame)
	f = open_write("nextmatches.html");
    else
	f = open_write("nextm.html");
    if (!f)
        return;

    make_top_frame_refresh(f, frame);
    if (frame)
	make_left_frame(f);
    else
	fprintf(f, "<td></td>\r\n");

    // header row
    fprintf(f, "<td><table class=\"nextmatches\"><tr>");
    for (i = 0; i < number_of_tatamis; i++) {
        fprintf(f, "<th align=\"center\" colspan=\"2\">Tatami %d</th>", i+1);
    }
    fprintf(f, "</tr><tr>\r\n");

    // last winners
    for (i = 0; i < number_of_tatamis; i++) {
        gchar *xf, *xl, *xc;

        //fprintf(f, "<td><table class=\"tatamimatches\">");

        if (next_matches_info[i][0].won_catnum) {
            xf = next_matches_info[i][0].won_first;
            xl = next_matches_info[i][0].won_last;
            xc = next_matches_info[i][0].won_cat;
        } else {
            xf = "";
            xl = "";
            xc = "";
        }

        fprintf(f, "<td class=\"cpl\">%s:<br>", _T(prevwinner));
        fprintf(f, "</td>\r\n<td class=\"cpr\">%s<br>%s %s<br>&nbsp;</td>",
                utf8_to_html(xc), utf8_to_html(xf), utf8_to_html(xl));
    }
    fprintf(f, "</tr>\r\n");

    // next matches
#if (GTKVER == 3)
    G_LOCK(next_match_mutex);
#else
    g_static_mutex_lock(&next_match_mutex);
#endif
    // JSON
    cJSON *jsonroot = cJSON_CreateArray();
    for (i = 0; i < number_of_tatamis; i++) {
        struct match *m = get_cached_next_matches(i+1);
        cJSON *t = cJSON_CreateObject();
        cJSON_AddNumberToObject(t, "tatami", i+1);
        cJSON *matches = cJSON_CreateArray();

        for (k = 0; k < INFO_MATCH_NUM; k++) {
            if (m[k].number >= 1000)
                continue;
            struct judoka *cat = get_data(m[k].category);
            struct judoka *comp1 = get_data(m[k].blue);
            struct judoka *comp2 = get_data(m[k].white);

            if (cat) {
                cJSON *e = cJSON_CreateObject();
                cJSON_AddNumberToObject(e, "num", k+1);
                cJSON_AddStringToObject(e, "cat", cat->last);
                cJSON_AddStringToObject(e, "first1", comp1 ? comp1->first : "");
                cJSON_AddStringToObject(e, "last1", comp1 ? comp1->last : "");
                cJSON_AddStringToObject(e, "club1", comp1 ? get_club_text(comp1, 0) : "");
                cJSON_AddStringToObject(e, "first2", comp2 ? comp2->first : "");
                cJSON_AddStringToObject(e, "last2", comp2 ? comp2->last : "");
                cJSON_AddStringToObject(e, "club2", comp2 ? get_club_text(comp2, 0) : "");
                cJSON_AddItemToArray(matches, e);
            }

            free_judoka(cat);
            free_judoka(comp1);
            free_judoka(comp2);
        }
        
        cJSON_AddItemToObject(t, "matches", matches);
        cJSON_AddItemToArray(jsonroot, t);
    }
    write_json("nextmatches.json", jsonroot);
    cJSON_Delete(jsonroot);
    // JSON

    for (k = 0; k < INFO_MATCH_NUM; k++) {
        //gchar *bgcolor = (k & 1) ? "bgcolor=\"#a0a0a0\"" : "bgcolor=\"#f0f0f0\"";
        gchar *class_ul = (k & 1) ? "class=\"cul1\"" : "class=\"cul2\"";
        gchar *class_ur = (k & 1) ? "class=\"cur1\"" : "class=\"cur2\"";
        gchar *class_dl = (k & 1) ? "class=\"cdl1\"" : "class=\"cdl2\"";
        gchar *class_dr = (k & 1) ? "class=\"cdr1\"" : "class=\"cdr2\"";
        struct judoka *blue, *white, *cat;

        fprintf(f, "<tr>");

        // match number and category
        for (i = 0; i < number_of_tatamis; i++) {
            struct match *m = get_cached_next_matches(i+1);

            if (m[k].number >= 1000) {
                fprintf(f, "<td %s>&nbsp;</td><td %s>&nbsp;</td>", class_ul, class_ur);
                continue;
            }

            cat = get_data(m[k].category);

            fprintf(f, "<td %s><b>%s %d:</b></td><td %s><b>%s</b></td>",
                    class_ul, _T(match), k+1, class_ur, cat ? cat->last : "?");

            free_judoka(cat);
        }

        fprintf(f, "</tr>\r\n<tr>");

        for (i = 0; i < number_of_tatamis; i++) {
            struct match *m = get_cached_next_matches(i+1);

            if (m[k].number >= 1000) {
                fprintf(f, "<td %s>&nbsp;</td><td %s>&nbsp;</td>", class_dl, class_dr);
                continue;
            }

            blue = get_data(m[k].blue);
            white = get_data(m[k].white);

            fprintf(f, "<td %s>%s %s<br>%s<br>&nbsp;</td><td %s>%s %s<br>%s<br>&nbsp;</td>",
                    class_dl,
                    blue ? utf8_to_html(blue->first) : "?",
                    blue ? utf8_to_html(blue->last) : "?",
                    blue ? utf8_to_html(get_club_text(blue, 0)) : "?",
                    class_dr,
                    white ? utf8_to_html(white->first) : "?",
                    white ? utf8_to_html(white->last) : "?",
                    white ? utf8_to_html(get_club_text(white, 0)) : "?");

            free_judoka(blue);
            free_judoka(white);
        }

        fprintf(f, "</tr>\r\n");
    }
#if (GTKVER == 3)
    G_UNLOCK(next_match_mutex);
#else
    g_static_mutex_unlock(&next_match_mutex);
#endif

    fprintf(f, "</table></td>\n");
    make_bottom_frame(f);
    fclose(f);
}

int get_output_directory(void)
{
    GtkWidget *dialog, *auto_update, *statistics, *vbox, *svg = NULL, *tmp;
    gchar *name;
    GError *error = NULL;
    gint row = 0, i;
    gchar buf[64];
    struct {
	GtkWidget *name, *href, *embed, *newwin;
    } extras[NUM_EXTRA_LINKS];
    
    dialog = gtk_file_chooser_dialog_new(_("Choose a directory"),
                                         GTK_WINDOW(main_window),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);

    vbox = gtk_grid_new();
    auto_update = gtk_check_button_new_with_label(_("Automatic Web Page Update"));
    statistics = gtk_check_button_new_with_label(_("Create Statistics"));
    if (svg_in_use()) {
        svg = gtk_check_button_new_with_label(_("Print SVG"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(svg),
                                     g_key_file_get_boolean(keyfile, "preferences", "printsvg", &error));
    }
    gtk_grid_attach(GTK_GRID(vbox), statistics, 0, row++, 2, 1);
    gtk_grid_attach(GTK_GRID(vbox), auto_update, 0, row++, 2, 1);
    if (svg)
        gtk_grid_attach(GTK_GRID(vbox), svg, 0, row++, 2, 1);

    snprintf(buf, sizeof(buf), "<b>%s:</b>", _("Extra Links"));
    tmp = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tmp), buf);
    gtk_grid_attach(GTK_GRID(vbox), tmp, 0, row, 1, 1);

    snprintf(buf, sizeof(buf), "<b>%s</b>", _("Name"));
    tmp = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tmp), buf);
    gtk_grid_attach(GTK_GRID(vbox), tmp, 1, row, 1, 1);

    snprintf(buf, sizeof(buf), "<b>%s</b>", _("Link"));
    tmp = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tmp), buf);
    gtk_grid_attach(GTK_GRID(vbox), tmp, 2, row++, 1, 1);

    gtk_grid_attach(GTK_GRID(vbox), gtk_label_new(_("(Example)")), 3, row, 1, 1);
    tmp = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(tmp), "Current time");
    gtk_editable_set_editable(GTK_EDITABLE(tmp), FALSE);
    gtk_grid_attach(GTK_GRID(vbox), tmp, 1, row, 1, 1);

    tmp = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(tmp), "https://time.is/");
    gtk_editable_set_editable(GTK_EDITABLE(tmp), FALSE);
    gtk_grid_attach(GTK_GRID(vbox), tmp, 2, row++, 1, 1);
    
    for (i = 0; i < NUM_EXTRA_LINKS; i++) {
	if (!extra_links[i].name) {
	    snprintf(buf, sizeof(buf), "extralinkname%d", i);
	    extra_links[i].name = g_key_file_get_string(keyfile, "preferences", buf, NULL);
	    snprintf(buf, sizeof(buf), "extralinkflags%d", i);
	    extra_links[i].flags = g_key_file_get_integer(keyfile, "preferences", buf, &error);
	}
	if (!extra_links[i].href) {
	    snprintf(buf, sizeof(buf), "extralinkhref%d", i);
	    extra_links[i].href = g_key_file_get_string(keyfile, "preferences", buf, NULL);
	}

	extras[i].name = gtk_entry_new();
	extras[i].href = gtk_entry_new();
	extras[i].embed = gtk_check_button_new_with_label(_("Embed"));
	extras[i].newwin = gtk_check_button_new_with_label(_("New Window"));

	if (extra_links[i].name)
	    gtk_entry_set_text(GTK_ENTRY(extras[i].name), extra_links[i].name);
	if (extra_links[i].href)
	    gtk_entry_set_text(GTK_ENTRY(extras[i].href), extra_links[i].href);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extras[i].embed), extra_links[i].flags & EXTRA_EMBED);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(extras[i].newwin), extra_links[i].flags & EXTRA_NEW_WIN);
	    
	gtk_grid_attach(GTK_GRID(vbox), extras[i].name, 1, row, 1, 1);
	gtk_grid_attach(GTK_GRID(vbox), extras[i].href, 2, row, 1, 1);
	gtk_grid_attach(GTK_GRID(vbox), extras[i].embed, 3, row, 1, 1);
	gtk_grid_attach(GTK_GRID(vbox), extras[i].newwin, 4, row, 1, 1);
	row++;
    }    

    gtk_widget_show_all(vbox);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), vbox);

    if (strlen(current_directory) > 1) {
        gchar *dirname = g_path_get_dirname(current_directory);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    } else if (database_name[0] == 0) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    } else {
        gchar *dirname = g_path_get_dirname(database_name);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dirname);
        g_free(dirname);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return -1;
    }

    name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    strcpy(current_directory, name);
    g_free (name);

    //valid_ascii_string(current_directory);

    automatic_web_page_update = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_update));
    create_statistics = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(statistics));
    if (svg)
        print_svg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(svg));
    else
        print_svg = FALSE;

    g_key_file_set_boolean(keyfile, "preferences", "printsvg", print_svg);

    for (i = 0; i < NUM_EXTRA_LINKS; i++) {
	const gchar *t; 
	g_free(extra_links[i].name);
	g_free(extra_links[i].href);
	extra_links[i].name = NULL;
	extra_links[i].href = NULL;
	t = gtk_entry_get_text(GTK_ENTRY(extras[i].name));
	if (t) extra_links[i].name = strdup(t);
	t = gtk_entry_get_text(GTK_ENTRY(extras[i].href));
	if (t) extra_links[i].href = strdup(t);

	extra_links[i].flags = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extras[i].embed)))
	    extra_links[i].flags |= EXTRA_EMBED;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(extras[i].newwin)))
	    extra_links[i].flags |= EXTRA_NEW_WIN;

	snprintf(buf, sizeof(buf), "extralinkname%d", i);
	g_key_file_set_string(keyfile, "preferences", buf, extra_links[i].name ? extra_links[i].name : "");
	snprintf(buf, sizeof(buf), "extralinkhref%d", i);
	g_key_file_set_string(keyfile, "preferences", buf, extra_links[i].href ? extra_links[i].href : "");
	snprintf(buf, sizeof(buf), "extralinkflags%d", i);
	g_key_file_set_integer(keyfile, "preferences", buf, extra_links[i].flags);
    }
    
    gtk_widget_destroy(dialog);
    return 0;
}

gchar *txt2hex(const gchar *txt)
{
    static gchar buffers[4][64];
    static gint sel = 0;
    guchar *p = (guchar *)txt;
    buffers[sel][0] = 0;
    gchar *ret;
    gint n = 0;

    while (*p && n < 62)
        n += snprintf(&buffers[sel][n], 64 - n, "%02x", *p++);

    ret = buffers[sel];
    sel++;
    if (sel >= 4)
        sel = 0;
    return ret;
}

static void write_extra_link_files(void)
{
    gint i;
    gchar buf[32];
    
    for (i = 0; i < NUM_EXTRA_LINKS; i++) {
	snprintf(buf, sizeof(buf), "extralink%d.html", i);
	FILE *f = open_write(buf);
	if (!f) return;

	make_top_frame(f);
        make_left_frame(f);
        fprintf(f, "<td valign=\"top\"><embed src=\"%s\" style=\"width:640; height:900\"></td>\r\n", extra_links[i].href);
	make_bottom_frame(f);

	fclose(f);
    }
}
