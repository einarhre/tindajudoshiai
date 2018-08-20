/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2017 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "sqlite3.h"
#include "judoshiai.h"

#define NUM_DEFAULT_CATS 6
#define NUM_TBLS 4

enum properties;
static GtkWidget *get_prop_widget(enum properties num, gboolean label);

struct default_cat {
    gint max_age;
    gint comp_min, comp_max;
    gint catsys, table;
};

struct default_cat default_cats[NUM_DEFAULT_CATS];

struct {
    GtkWidget *age, *min, *max, *sys;
} catwidgets[NUM_DEFAULT_CATS];

static GtkWidget *belt_widgets[NUM_BELTS];

#define CAT_OPT_YES 1
#define CAT_OPT_NO  2
#define CAT_OPT_SET(_i, _o, _yes) do {					\
	cat_opts[(_i)] = (_o) << 2 | ((_yes) ? CAT_OPT_YES : CAT_OPT_NO); \
	while (0)
#define CAT_OPT_GET(_i) (cat_opts[(_i)] >> 2)

gint cat_opts[NUM_CAT_OPTS];

enum {
    PROP_TYPE_TEXT,
    PROP_TYPE_INT,
    PROP_TYPE_CHECK
};

static GSList *points_group = NULL;

static GtkWidget *properties_button = NULL;
static void set_properties_button(GtkWidget *checkbox, gpointer arg);

static struct property {
    gchar *name; // name in database
    gchar *label;
    gint type;
    gint row, col;
    gchar *value; // value in database
    gint intval; // value of int and others
    GtkWidget *w;
    gint min, max;
    gint table;
    gint noshow;
    GSList **radio_group;
    void (*action_func)(GtkWidget *, gpointer);
    GtkWidget *label_w;
} props[NUM_PROPERTIES] = {
    {
        .name = "Competition",
        .label = N_("Tournament:"),
        .type = PROP_TYPE_TEXT,
        .row = 1, .col = 1,
    },
    {
        .name = "Date",
        .label = N_("Date:"),
        .type = PROP_TYPE_TEXT,
    },
    {
        .name = "Place",
        .label = N_("Place:"),
        .type = PROP_TYPE_TEXT,
    },
    {
        .name = "Time",
        .label = N_("Start time:"),
        .type = PROP_TYPE_TEXT,
    },
    {
        .name = "NumTatamis",
        .label = N_("Number of tatamis:"),
        .type = PROP_TYPE_INT,
        .min = 1, .max = NUM_TATAMIS,
    },
    {
        .name = "WhiteFirst",
        .label = N_("White first:"),
        .type = PROP_TYPE_CHECK,
	.noshow = 1,
    },
    {
        .name = "three_matches_for_two",
        .label = N_("Best two out of three matches:"),
        .type = PROP_TYPE_CHECK,
        .table = 1,
        .row = -1, .col = -1,
    },
    {
        .name = "WinNeededForMedal",
        .label = N_("Medal will be awarded \nonly if a contest was won:"),
        .type = PROP_TYPE_CHECK,
        .table = 3,
    },
    {
        .name = "SeededToFixedPlaces",
        .label = N_("Seeded to fixed places:"),
        .type = PROP_TYPE_CHECK,
        .table = 3,
    },
    {
        .name = "UseFirstPlacesOnly",
        .label = N_("Use the first places only:"),
        .type = PROP_TYPE_CHECK,
        .table = 3,
    },
/*
    {
        .name = "EqualScoreLessShidoWins",
        .label = N_("If equal score less shido wins"),
        .type = PROP_TYPE_CHECK,
    },
*/
    {
        .name = "GoldenScoreWinGives1Point",
        .label = N_("Win in Golden Score gives 1 point"),
        .type = PROP_TYPE_CHECK,
        .table = 3,
    },
    {
        .name = "Rules2017",
        .label = N_("2017 Rules:"),
        .type = PROP_TYPE_CHECK,
	.action_func = set_properties_button,
        .table = 3,
    },
    {
        .name = "Rules2018",
        .label = N_("2018 Rules:"),
        .type = PROP_TYPE_CHECK,
	.action_func = set_properties_button,
        .table = 3,
    },
    {
        .name = "DefaultCat1",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "DefaultCat2",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "DefaultCat3",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "DefaultCat4",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "DefaultCat5",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "DefaultCat6",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "DPool2CarriedForwardPoints",
        .label = N_("Double Pool 2 with carried forward points:"),
        .type = PROP_TYPE_CHECK,
        .table = 1,
    },
    {
        .name = "TwoPoolBronzes",
        .label = N_("Two pool bronzes:"),
        .type = PROP_TYPE_CHECK,
        .table = 1,
    },
    {
        .name = "Resolve3WayTiesByTime",
        .label = N_("Resolve 3-way ties by time:"),
        .type = PROP_TYPE_CHECK,
        .table = 1,
    },
    {
        .name = "Resolve3WayTiesByWeights",
        .label = N_("Resolve 3-way ties by weights:"),
        .type = PROP_TYPE_CHECK,
        .table = 1,
    },
    {
        .name = "MixPoolMatchesIntoRounds",
        .label = N_("Mix pool matches into rounds:"),
        .type = PROP_TYPE_CHECK,
        .table = 1,
    },
    {
        .name = "GradeNames",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "CatOpts",
        .label = "",
        .type = PROP_TYPE_TEXT,
        .row = -1, .col = -1,
    },
    {
        .name = "UseIjfPoints",
        .label = N_("Use points 10/1/0:"),
        .type = PROP_TYPE_CHECK,
        .table = 3,
	.radio_group = &points_group,
    },
    {
        .name = "UsePts_100_10_1_h",
        .label = N_("Use points 100/10/1/½:"),
        .type = PROP_TYPE_CHECK,
        //.table = 1,
	.radio_group = &points_group,
        .table = 3,
    },
    {
        .name = "UsePts_10_7_5_1",
        .label = N_("Use points 10/7/1:"),
        .type = PROP_TYPE_CHECK,
        //.table = 1,
	.radio_group = &points_group,
        .table = 3,
    },
    {
        .name = "UsePts_10_1_h",
        .label = N_("Use points 10/1/½:"),
        .type = PROP_TYPE_CHECK,
        //.table = 1,
	.radio_group = &points_group,
        .table = 3,
    },
    {
        .name = "UsePts_10_3_1",
        .label = N_("Use points 10/3/1:"),
        .type = PROP_TYPE_CHECK,
        //.table = 1,
	.radio_group = &points_group,
        .table = 3,
    },
    {
        .name = "UsePts_10_5_1",
        .label = N_("Use points 10/5/1:"),
        .type = PROP_TYPE_CHECK,
        //.table = 1,
	.radio_group = &points_group,
        .table = 3,
    },
    {
        .name = "ExtraMatchInTeamsPointsTie",
        .label = N_("Add extra match in teams points tie:"),
        .type = PROP_TYPE_CHECK,
        .table = 2,
    },
    {
        .name = "IgnorePointsInTeamEvent",
        .label = N_("Only wins count, ignore match points:"),
        .type = PROP_TYPE_CHECK,
        .table = 2,
    },
    {
        .name = "SkipUnnecessaryMatches",
        .label = N_("The first team reaching the majority\n"
		    "of wins is declared the winner.\n"
		    "The remaining contests will not be fought:"),
        .type = PROP_TYPE_CHECK,
        .table = 2,
    },
    {
        .name = "ExtraMatchGoldenScore",
        .label = N_("The athletes in the drawn category will\n"
		    "refight a golden score contest:"),
        .type = PROP_TYPE_CHECK,
        .table = 2,
    },
    {
        .name = "ExtraMatchIfOneCompMissing",
        .label = N_("Draw for extra match is done from\n"
		    "all categories regardless\n"
		    "if the team has a player or not:"),
        .type = PROP_TYPE_CHECK,
        .table = 2,
    },
};

static void set_properties_button(GtkWidget *checkbox, gpointer arg)
{
    gint i;
    GtkWidget *w;
    gint prop = ptr_to_gint(arg);

    if (prop == PROP_RULES_2017 &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(props[prop].w)))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(props[PROP_RULES_2018].w), FALSE);
    else if (prop == PROP_RULES_2018 &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(props[prop].w)))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(props[PROP_RULES_2017].w), FALSE);

    if (properties_button) {
	gtk_widget_set_name(properties_button, "button_red");
	for (i = PROP_USE_IJF_POINTS; i <= PROP_USE_PTS_10_5_1; i++) { 
	    w = get_prop_widget(i, TRUE);
	    if (w) gtk_widget_set_name(w, "button_red");
	}
    }
}

static GtkWidget *get_prop_widget(enum properties num, gboolean label) {
    if (label) return props[num].label_w;
    return props[num].w;
}

const gchar *get_prop_name(enum properties num) {
    return props[num].label;
}

gint prop_get_int_val(gint name)
{
    if (name == PROP_WHITE_FIRST)
	return 1;

    return props[name].intval;
}

gint prop_get_int_val_cat(gint name, gint ix)
{
    gint i;

    if (name == PROP_WHITE_FIRST)
	return 1;

    for (i = 0; i < NUM_CAT_OPTS; i++)
	if (CAT_OPT_GET(i) == name)
	    break;

    if (i >= NUM_CAT_OPTS)
	return prop_get_int_val(name);

    struct category_data *catdata = avl_get_category(ix & MATCH_CATEGORY_MASK);
    if (!catdata)
	return prop_get_int_val(name);

    gint age_ix = find_age_index(catdata->category);
    if (age_ix < 0)
	return prop_get_int_val(name);

    if ((1 << (2+i)) & category_definitions[age_ix].flags) {
	if (cat_opts[i] & CAT_OPT_YES) return 1;
	if (cat_opts[i] & CAT_OPT_NO) return 0;
    }

    return prop_get_int_val(name);
}

const gchar *prop_get_str_val(gint name)
{
    if (props[name].value == NULL) return "";
    return props[name].value;
}

void prop_set_val(gint n, const gchar *dbval, gint intval)
{
    g_free(props[n].value);
    if (dbval == NULL) {
        gchar buf[16];
        snprintf(buf, sizeof(buf), "%d", intval);
        props[n].value = g_strdup(buf);
    } else
        props[n].value = g_strdup(dbval);
    props[n].intval = intval;
}

static void value_to_widget(gint n)
{
    gchar buf[16];

    if (props[n].w == NULL)
        return;

    switch (props[n].type) {
    case PROP_TYPE_TEXT:
        gtk_entry_set_text(GTK_ENTRY(props[n].w), props[n].value ? props[n].value : "");
        break;
    case PROP_TYPE_INT:
        snprintf(buf, sizeof(buf), "%d", props[n].intval);
        gtk_entry_set_text(GTK_ENTRY(props[n].w), buf);
        break;
    case PROP_TYPE_CHECK:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(props[n].w), (gboolean)props[n].intval);
        break;
    }
}

static void widget_to_value(gint i)
{
    gint n;
    gchar buf[16];

    if (props[i].w == NULL)
        return;

    switch (props[i].type) {
    case PROP_TYPE_TEXT:
        prop_set_val(i, gtk_entry_get_text(GTK_ENTRY(props[i].w)), 0);
        break;
    case PROP_TYPE_INT:
        n = atoi(gtk_entry_get_text(GTK_ENTRY(props[i].w)));
        if (n < props[i].min) n = props[i].min;
        else if (n > props[i].max) n = props[i].max;
        snprintf(buf, sizeof(buf), "%d", n);
        prop_set_val(i, buf, n);
        break;
    case PROP_TYPE_CHECK:
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(props[i].w)))
            prop_set_val(i, "1", 1);
        else
            prop_set_val(i, "0", 0);
        break;
    default:
        g_print("error\n");
    }
}

void init_property(gchar *prop, gchar *val)
{
    gint i;

    for (i = 0; i < NUM_PROPERTIES; i++) {
        if (strcmp(prop, props[i].name) == 0) {
            g_free(props[i].value);
            props[i].value = g_strdup(val ? val : "");

            switch (props[i].type) {
            case PROP_TYPE_TEXT:
                break;
            case PROP_TYPE_INT:
            case PROP_TYPE_CHECK:
                props[i].intval = atoi(props[i].value);
                break;
            }

            if (i >= PROP_DEFAULT_CAT_1 && i < PROP_DEFAULT_CAT_1 + NUM_DEFAULT_CATS) {
                sscanf(props[i].value, "%d %d %d %d %d",
                       &default_cats[i-PROP_DEFAULT_CAT_1].max_age,
                       &default_cats[i-PROP_DEFAULT_CAT_1].comp_min,
                       &default_cats[i-PROP_DEFAULT_CAT_1].comp_max,
                       &default_cats[i-PROP_DEFAULT_CAT_1].catsys,
                       &default_cats[i-PROP_DEFAULT_CAT_1].table);
            }

            if (i == PROP_GRADE_NAMES) {
                gint k;
                gchar **b = g_strsplit(val, ";", NUM_BELTS);
                for (k = 0; k < NUM_BELTS; k++) {
                    g_free(belts[k]);
                    belts[k] = NULL;
                }
                for (k = 0; k < NUM_BELTS && b[k]; k++)
                    belts[k] = g_strdup(b[k]);
                g_strfreev(b);
                //g_print("init prop: %s\n", props[i].value);
            }

            if (i == PROP_NUM_TATAMIS)
                number_of_tatamis = props[i].intval;

            if (i == PROP_CAT_OPTS) {
                gint k;
                gchar **b = g_strsplit(props[i].value, ";", -1);
                for (k = 0; k < NUM_CAT_OPTS; k++) {
		    if (!b[k]) break;
		    cat_opts[k] = atoi(b[k]);
                }
                g_strfreev(b);
            }

            return;
        } // if strcmp

    }
    g_print("Unknown property '%s'='%s'\n", prop, val);
}

static void values_to_widgets(void)
{
    gint i;

    for (i = 0; i < NUM_PROPERTIES; i++)
        value_to_widget(i);

    for (i = 0; i < NUM_DEFAULT_CATS; i++) {
        gchar buf[16];

        snprintf(buf, sizeof(buf), "%d", default_cats[i].max_age);
        gtk_entry_set_text(GTK_ENTRY(catwidgets[i].age), buf);

        snprintf(buf, sizeof(buf), "%d", default_cats[i].comp_min);
        gtk_entry_set_text(GTK_ENTRY(catwidgets[i].min), buf);

        snprintf(buf, sizeof(buf), "%d", default_cats[i].comp_max);
        gtk_entry_set_text(GTK_ENTRY(catwidgets[i].max), buf);

        gtk_combo_box_set_active(GTK_COMBO_BOX(catwidgets[i].sys),
                                 get_system_menu_selection(default_cats[i].catsys,
							   default_cats[i].table));
    }

    for (i = 0; i < NUM_BELTS; i++) {
        gtk_entry_set_text(GTK_ENTRY(belt_widgets[i]), belts[i] ? belts[i] : "");
    }
}

static void widgets_to_values(void)
{
    gint i;
    gchar buf[512];

    for (i = 0; i < NUM_PROPERTIES; i++)
        widget_to_value(i);

    for (i = 0; i < NUM_DEFAULT_CATS; i++) {
	guint table;
        gint real = get_system_number_by_menu_pos
	    (gtk_combo_box_get_active(GTK_COMBO_BOX(catwidgets[i].sys)),
	     &table);
        snprintf(buf, sizeof(buf), "%s %s %s %d %d",
                 gtk_entry_get_text(GTK_ENTRY(catwidgets[i].age)),
                 gtk_entry_get_text(GTK_ENTRY(catwidgets[i].min)),
                 gtk_entry_get_text(GTK_ENTRY(catwidgets[i].max)),
                 real, table);
        //gtk_entry_set_text(GTK_ENTRY(props[i + PROP_DEFAULT_CAT_1].w), buf);
        prop_set_val(i + PROP_DEFAULT_CAT_1, buf, 0);

        sscanf(buf, "%d %d %d %d %d",
               &default_cats[i].max_age,
               &default_cats[i].comp_min,
               &default_cats[i].comp_max,
               &default_cats[i].catsys,
	       &default_cats[i].table);
    }

    buf[0] = 0;
    for (i = 0; i < NUM_BELTS; i++) {
        if (i) g_strlcat(buf, ";", sizeof(buf));
        g_strlcat(buf, gtk_entry_get_text(GTK_ENTRY(belt_widgets[i])), sizeof(buf));
        g_free(belts[i]);
        belts[i] = g_strdup(gtk_entry_get_text(GTK_ENTRY(belt_widgets[i])));
    }
    prop_set_val(PROP_GRADE_NAMES, buf, 0);
    //gtk_entry_set_text(GTK_ENTRY(props[PROP_GRADE_NAMES].w), buf);
}

void props_save_one(gint prop)
{
    db_set_info(props[prop].name, props[prop].value);
}

void props_save_to_db(void)
{
    gint i;

    for (i = 0; i < NUM_PROPERTIES; i++)
        db_set_info(props[i].name, props[i].value);

    number_of_tatamis = prop_get_int_val(PROP_NUM_TATAMIS);
}

#define SET_VAL(_n, _s, _i) do { if (if_unset==FALSE || props[_n].value[0]==0) prop_set_val(_n, _s, _i); } while (0)

void reset_props(GtkWidget *button, void *data)
{
    reset_props_1(button, data, FALSE);
}

void reset_props_1(GtkWidget *button, void *data, gboolean if_unset)
{
    gint i;
    GtkComboBox *combo = data;
    gchar buf[512];

    if (combo)
        draw_system = gtk_combo_box_get_active(combo);

    for (i = 0; i < NUM_PROPERTIES; i++)
        if (props[i].value == NULL) prop_set_val(i, "", 0);

    SET_VAL(PROP_WHITE_FIRST, "1", 1);
    SET_VAL(PROP_THREE_MATCHES_FOR_TWO, "0", 0);
    /*SET_VAL(PROP_EQ_SCORE_LESS_SHIDO_WINS, "1", 1);*/
    SET_VAL(PROP_GS_WIN_GIVES_1_POINT, "0", 0);

    if (number_of_tatamis == 0)
        number_of_tatamis = 3;

    SET_VAL(PROP_NUM_TATAMIS, NULL, number_of_tatamis);

    if (draw_system == DRAW_INTERNATIONAL || draw_system == DRAW_NORWEGIAN)
        SET_VAL(PROP_WIN_NEEDED_FOR_MEDAL, "1", 1);
    else
        SET_VAL(PROP_WIN_NEEDED_FOR_MEDAL, "0", 0);

    if (draw_system == DRAW_SPANISH) {
        SET_VAL(PROP_SEEDED_TO_FIXED_PLACES, "1", 1);
        SET_VAL(PROP_TWO_POOL_BRONZES, "1", 1);
    } else {
        SET_VAL(PROP_SEEDED_TO_FIXED_PLACES, "0", 0);
        SET_VAL(PROP_TWO_POOL_BRONZES, "0", 0);
    }

    if (draw_system == DRAW_AUSTRALIAN)
        SET_VAL(PROP_DPOOL2_WITH_CARRIED_FORWARD_POINTS, "1", 1);
    else
        SET_VAL(PROP_DPOOL2_WITH_CARRIED_FORWARD_POINTS, "0", 0);

    if (draw_system == DRAW_SWEDISH || draw_system == DRAW_ESTONIAN ||
	draw_system == DRAW_FINNISH)
        SET_VAL(PROP_RESOLVE_3_WAY_TIES_BY_TIME, "1", 1);
    else
        SET_VAL(PROP_RESOLVE_3_WAY_TIES_BY_TIME, "0", 0);

    if (draw_system == DRAW_FINNISH || draw_system == DRAW_SWEDISH ||
	draw_system == DRAW_ESTONIAN || draw_system == DRAW_NORWEGIAN)
        SET_VAL(PROP_RESOLVE_3_WAY_TIES_BY_WEIGHTS, "1", 1);
    else
        SET_VAL(PROP_RESOLVE_3_WAY_TIES_BY_WEIGHTS, "0", 0);

    SET_VAL(PROP_RULES_2017, "0", 0);
    SET_VAL(PROP_RULES_2018, "1", 1);
    SET_VAL(PROP_EXTRA_MATCH_IN_TEAMS_TIE, "1", 1);

    if (draw_system == DRAW_FINNISH)
	SET_VAL(PROP_USE_IJF_POINTS, "1", 1);

    // default catsystems
    if (if_unset==FALSE || props[PROP_DEFAULT_CAT_1].value[0]==0) {
        memset(default_cats, 0, sizeof(default_cats));
        default_cats[0] = (struct default_cat){0, 2, 2, CAT_SYSTEM_BEST_OF_3};
        default_cats[1] = (struct default_cat){0, 3, 5, CAT_SYSTEM_POOL};
        default_cats[2] = (struct default_cat){0, 6, 7, CAT_SYSTEM_DPOOL};
        default_cats[3] = (struct default_cat){0, 8, 0, CAT_SYSTEM_REPECHAGE};

        switch (draw_system) {
        case DRAW_INTERNATIONAL:
            default_cats[2] = (struct default_cat){0, 6, 0, CAT_IJF_DOUBLE_REPECHAGE};
            default_cats[3] = (struct default_cat){0, 0, 0, 0};
            break;
        case DRAW_FINNISH:
            break;
        case DRAW_SWEDISH:
            default_cats[3] = (struct default_cat){0, 8, 0, CAT_SYSTEM_DIREKT_AATERKVAL};
            break;
        case DRAW_ESTONIAN:
            default_cats[3] = (struct default_cat){10, 6, 0, CAT_SYSTEM_EST_D_KLASS};
            default_cats[4] = (struct default_cat){0, 6, 0, CAT_SYSTEM_REPECHAGE};
            break;
        case DRAW_SPANISH:
            default_cats[2] = (struct default_cat){0, 6, 10, CAT_SYSTEM_DPOOL};
            default_cats[3] = (struct default_cat){10, 8, 16, CAT_ESP_DOBLE_PERDIDA};
            default_cats[4] = (struct default_cat){0, 8, 0, CAT_ESP_REPESCA_DOBLE};
            break;
        case DRAW_NORWEGIAN:
            default_cats[2] = (struct default_cat){0, 6, 8, CAT_SYSTEM_DPOOL};
            default_cats[3] = (struct default_cat){0, 9, 0, CAT_SYSTEM_DEN_DOUBLE_ELIMINATION};
            break;
        case DRAW_BRITISH:
            default_cats[3] = (struct default_cat){0, 8, 0, CAT_SYSTEM_GBR_KNOCK_OUT};
            break;
        case DRAW_AUSTRALIAN:
            break;
        case DRAW_DANISH:
            default_cats[3] = (struct default_cat){0, 8, 0, CAT_SYSTEM_DEN_DOUBLE_ELIMINATION};
            break;
        case DRAW_POLISH:
            default_cats[2] = (struct default_cat){0, 6, 8, CAT_SYSTEM_DPOOL};
            default_cats[3] = (struct default_cat){0, 9, 0, CAT_IJF_DOUBLE_REPECHAGE};
            break;
        case DRAW_SLOVAKIAN:
            default_cats[2] = (struct default_cat){0, 6, 8, CAT_SYSTEM_DPOOL};
            default_cats[3] = (struct default_cat){0, 9, 0, CAT_SYSTEM_REPECHAGE};
            break;
        case DRAW_UKRAINIAN:
            default_cats[2] = (struct default_cat){0, 6, 0, CAT_SYSTEM_REPECHAGE};
            default_cats[3] = (struct default_cat){0, 0, 0, 0};
            break;
        case DRAW_GERMAN:
            default_cats[2] = (struct default_cat){0, 6, 0, CAT_SYSTEM_GBR_KNOCK_OUT};
            default_cats[3] = (struct default_cat){0, 0, 0, 0};
            break;
        }

        for (i = 0; i < NUM_DEFAULT_CATS; i++) {
            snprintf(buf, sizeof(buf), "%d %d %d %d %d",
                     default_cats[i].max_age,
                     default_cats[i].comp_min,
                     default_cats[i].comp_max,
                     default_cats[i].catsys,
		     default_cats[i].table);
            prop_set_val(PROP_DEFAULT_CAT_1 + i, buf, 0);
        }
    }

    if (if_unset==FALSE || props[PROP_GRADE_NAMES].value[0]==0) {
        for (i = 0; i < NUM_BELTS; i++) {
            g_free(belts[i]);
            belts[i] = NULL;
        }
        for (i = 0; belts_defaults[i]; i++) {
            belts[i] = g_strdup(belts_defaults[i]);
        }
        buf[0] = 0;
        for (i = 0; i < NUM_BELTS; i++) {
            if (i) g_strlcat(buf, ";", sizeof(buf));
            if (belts[i]) g_strlcat(buf, belts[i], sizeof(buf));
        }
        prop_set_val(PROP_GRADE_NAMES, buf, 0);
    }

    g_key_file_set_integer(keyfile, "preferences", "drawsystem", draw_system);
    refresh_window();
}

static void reset_props1(GtkWidget *button, void *data)
{
    reset_props(button, data);
    values_to_widgets();
}

static const gchar *draw_system_names[NUM_DRAWS] =
    {N_("International System"), N_("Finnish System"), N_("Swedish System"), N_("Estonian System"), N_("Spanish System"),
     N_("Norwegian System"), N_("British System"), N_("Australian System"), N_("Danish System"),
     N_("Polish System"), N_("Slovakian System"), N_("Ukrainian System"), N_("German System")};

static void update_match_order(void)
{
    gint i;
    db_read_matches();
    for (i = 1; i <= number_of_tatamis; i++)
	update_matches(0, (struct compsys){0,0,0,0}, i);
    update_category_status_info_all();
}

void open_properties(GtkWidget *w, gpointer data)
{
    GtkWidget *dialog, *tmp, *reset;
    GtkWidget *table2 = gtk_grid_new();
    GtkWidget *table3 = gtk_grid_new();
    GtkWidget *table5 = gtk_grid_new();
    GtkWidget *table6 = gtk_grid_new();

    GtkWidget *tables[NUM_TBLS];
    gint       num_comp, num_weighted;
    gint       row[NUM_TBLS] = {0}, col[NUM_TBLS] = {0}, i;
    gint       tbl = 0;
    gchar      buf[16];

    points_group = NULL;

    for (i = 0; i < NUM_TBLS; i++)
        tables[i] = gtk_grid_new();
    db_read_competitor_statistics(&num_comp, &num_weighted);

    dialog = gtk_dialog_new_with_buttons (_("Tournament properties"),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                          NULL);
    GtkWidget *prop_notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(prop_notebook), GTK_POS_TOP);

    for (i = 0; i < NUM_TBLS; i++) {
        gtk_grid_set_column_spacing(GTK_GRID(tables[i]), 5);
        gtk_grid_set_row_spacing(GTK_GRID(tables[i]), 5);
    }

    for (i = 0; i < NUM_PROPERTIES; i++) {
	if (props[i].noshow)
	    continue;

        tbl = props[i].table;

        if (props[i].row >= 0) {
            if (props[i].row) row[tbl] = props[i].row - 1;
            if (props[i].col) col[tbl] = props[i].col - 1;

            tmp = props[i].label_w = gtk_label_new(_(props[i].label));
            gtk_misc_set_alignment(GTK_MISC(tmp), 1, 0.5);
            gtk_grid_attach(GTK_GRID(tables[tbl]), tmp, col[tbl], row[tbl], 1, 1);

            switch (props[i].type) {
            case PROP_TYPE_TEXT:
            case PROP_TYPE_INT:
                //gtk_entry_set_max_length(GTK_ENTRY(tmp), 20);
                //gtk_entry_set_text(GTK_ENTRY(tmp), props[i].value ? props[i].value : "");
                tmp = gtk_entry_new();
                gtk_entry_set_width_chars(GTK_ENTRY(tmp), 20);
                gtk_grid_attach(GTK_GRID(tables[tbl]), tmp, col[tbl]+1, row[tbl], 1, 1);
                break;

            case PROP_TYPE_CHECK:
                //gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), (gboolean)props[i].intval);
		if (props[i].radio_group) {
		    tmp = gtk_radio_button_new(*props[i].radio_group);
		    *props[i].radio_group =
			gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmp));
		} else
		    tmp = gtk_check_button_new();

                gtk_grid_attach(GTK_GRID(tables[tbl]), tmp, col[tbl]+1, row[tbl], 1, 1);

		if (props[i].action_func)
		    g_signal_connect(G_OBJECT(tmp), "clicked",
				     G_CALLBACK(props[i].action_func), gint_to_ptr(i));
                break;
            }

            row[tbl]++;
            props[i].w = tmp;
        } // if (props[i].row >= 0)
    } // for (i = 0; i < NUM_PROPERTIES; i++)

    for (i = 0; i < NUM_DEFAULT_CATS; i++) {
        tmp = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(tmp), 3);
        gtk_grid_attach(GTK_GRID(table3), tmp, 0, i+3, 1, 1);
        catwidgets[i].age = tmp;

        tmp = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(tmp), 3);
        gtk_grid_attach(GTK_GRID(table3), tmp, 1, i+3, 1, 1);
        catwidgets[i].min = tmp;

        tmp = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(tmp), 3);
        gtk_grid_attach(GTK_GRID(table3), tmp, 2, i+3, 1, 1);
        catwidgets[i].max = tmp;

        tmp = gtk_combo_box_text_new();
	set_cat_system_menu(tmp, 0, 0);
        gtk_grid_attach(GTK_GRID(table3), tmp, 3, i+3, 1, 1);
        catwidgets[i].sys = tmp;
    }

    for (i = 0; i < NUM_BELTS; i++) {
        tmp = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(tmp), 5);
        gtk_grid_attach(GTK_GRID(table5), tmp, i%8, i/8, 1, 1);
        belt_widgets[i] = tmp;
    }

    values_to_widgets();

    // table6 -- statistics
    gtk_grid_attach(GTK_GRID(table6), gtk_label_new(_("Competitors:")), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table6), gtk_label_new(_("Weighted:")), 0, 1, 1, 1);
    sprintf(buf, "%d", num_comp);
    gtk_grid_attach(GTK_GRID(table6), gtk_label_new(buf), 1, 0, 1, 1);
    sprintf(buf, "%d", num_weighted);
    gtk_grid_attach(GTK_GRID(table6), gtk_label_new(buf), 1, 1, 1, 1);


    // table4: category properties button
    gtk_grid_attach(GTK_GRID(table2), gtk_label_new("Categories"), 2, 0, 1, 1);
    tmp = properties_button = gtk_button_new_with_label(_("Properties"));
    gtk_grid_attach(GTK_GRID(table2), tmp, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(table2), gtk_label_new(" "), 1, 0, 1, 1);
    g_signal_connect(G_OBJECT(tmp), "clicked", G_CALLBACK(set_categories_dialog), NULL);
    //gtk_box_pack_start(GTK_BOX(vbox1), table4, FALSE, FALSE, 0);

    // table2
    gtk_grid_attach(GTK_GRID(table2), gtk_label_new(_("Reset to")), 0, 0, 1, 1);

    tmp = gtk_combo_box_text_new();
    for (i = 0; i < NUM_DRAWS; i++)
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(tmp), NULL, _(draw_system_names[i]));
    gtk_grid_attach(GTK_GRID(table2), tmp, 0, 1, 1, 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp), draw_system);
    //g_signal_connect(G_OBJECT(tmp), "changed", G_CALLBACK(reset_props), NULL);

    reset = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(G_OBJECT(reset), "clicked", G_CALLBACK(reset_props1), tmp);
    gtk_grid_attach(GTK_GRID(table2), reset, 0, 2, 1, 1);

    // table3
    //gtk_table_attach_defaults(GTK_TABLE(table3), gtk_label_new(_("Default systems")), 0, 4, 0, 1);
    gtk_grid_attach(GTK_GRID(table3), gtk_label_new(_("Players")), 1, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(table3), gtk_label_new(_("Max age")), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(table3), gtk_label_new(_("Min")),     1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(table3), gtk_label_new(_("Max")),     2, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(table3), gtk_label_new(_("System")),  3, 2, 1, 1);

    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), tables[0], gtk_label_new(_("General")));
    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), table2,    gtk_label_new(_("Initialize")));
    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), tables[3], gtk_label_new(_("Rules")));
    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), tables[1], gtk_label_new(_("Pool settings")));
    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), tables[2], gtk_label_new(_("Team event")));
    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), table3,    gtk_label_new(_("Default systems")));
    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), table5,    gtk_label_new(_("Grade names")));
    gtk_notebook_append_page(GTK_NOTEBOOK(prop_notebook), table6,    gtk_label_new(_("Statistics")));

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, FRAME_WIDTH, FRAME_HEIGHT);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);

    gtk_container_add(GTK_CONTAINER(scrolled_window), prop_notebook);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       scrolled_window, TRUE, TRUE, 0);

    gtk_widget_show_all(scrolled_window);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        widgets_to_values();
        props_save_to_db();
	update_match_order();

    }

    properties_button = NULL;
    gtk_widget_destroy (dialog);

    update_match_pages_visibility();
    set_match_col_titles();
}

gint props_get_default_wishsys(gint age, gint competitors, guint *table)
{
    gint i;

    for (i = 0; i < NUM_DEFAULT_CATS; i++) {
        if ((age == 0 || default_cats[i].max_age == 0 || age <= default_cats[i].max_age) &&
            (competitors >= default_cats[i].comp_min || default_cats[i].comp_min == 0)   &&
            (competitors <= default_cats[i].comp_max || default_cats[i].comp_max == 0)) {
	    if (table) *table = default_cats[i].table;
            return default_cats[i].catsys;
	}
    }

    return 0;
}

gint props_get_grade(gchar *b)
{
    gint i;
    gchar first_letter = 0;
    gint grade = 0;
    gchar *b1 = g_utf8_casefold(b, -1);

    // look for exact match
    for (i = 0; i < NUM_BELTS; i++) {
        gchar *s = belts[i];
        if (s == NULL || s[0] == 0)
            continue;

        gchar *t = g_utf8_casefold(s, -1);
        if (strcmp(b1, t) == 0) { // hit
            g_free(b1);
            g_free(t);
            return i;
        }
        g_free(t);
    }
    g_free(b1);

    while (*b) {
        if (*b >= '0' && *b <= '9') grade = 10*grade + *b - '0';
        else if (first_letter == 0) {
            if (*b >= 'a' && *b <= 'z') first_letter = *b;
            else if (*b >= 'A' && *b <= 'Z') first_letter = *b + 'a' - 'A';
        }
        b++;
    }

    for (i = 0; i < NUM_BELTS; i++) {
        gchar letter = 0;
        gint  number = 0;
        gchar *s = belts[i];
        if (s == NULL || s[0] == 0)
            continue;

        while (*s) {
            if (*s >= '0' && *s <= '9') number = 10*number + *s - '0';
            else if (letter == 0) {
                if (*s >= 'a' && *s <= 'z') letter = *s;
                else if (*s >= 'A' && *s <= 'Z') letter = *s + 'a' - 'A';
            }
            s++;
        }

        if (first_letter == letter && grade == number)
            return i;
    }

    return 0;
}
