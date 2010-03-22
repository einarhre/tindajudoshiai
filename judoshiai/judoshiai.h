/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2010 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

#ifndef _SHIAI_H_
#define _SHIAI_H_

#include "comm.h"
#include "avl.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) gettext(String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain) 
#define bind_textdomain_codeset(Domain,Codeset) (Codeset) 
#endif /* ENABLE_NLS */

#define LANG_FI 0
#define LANG_SW 1
#define LANG_EN 2

#if GTK_CHECK_VERSION(2,10,0)
#define PRINT_SUPPORTED
#endif

#define PRINT_TIME print_time(__FUNCTION__, __LINE__)

#define FRAME_WIDTH  600
#define FRAME_HEIGHT 400

#define PAGE_JUDOKAS 0
#define PAGE_SHEETS  1

#define BELT_6_KYU 1
#define BELT_5_KYU 2
#define BELT_4_KYU 3
#define BELT_3_KYU 4
#define BELT_2_KYU 5
#define BELT_1_KYU 6
#define BELT_2_DAN 7
#define BELT_3_DAN 8
#define BELT_4_DAN 9
#define BELT_5_DAN 10

#define NUM_TATAMIS 8

#define COMPETITORS_MASK   0x0000ffff
#define SYSTEM_MASK        0x000f0000
#define SYSTEM_MASK_SHIFT  16
#define SYSTEM_POOL        0x00010000
#define SYSTEM_DPOOL       0x00020000
#define SYSTEM_FRENCH_8    0x00030000
#define SYSTEM_FRENCH_16   0x00040000
#define SYSTEM_FRENCH_32   0x00050000
#define SYSTEM_FRENCH_64   0x00060000
#define SYSTEM_TABLE_MASK  0x00f00000
#define SYSTEM_TABLE_SHIFT 20
#define SYSTEM_WISH_MASK   0x0f000000
#define SYSTEM_WISH_SHIFT  24

enum tables {
    TABLE_DOUBLE_REPECHAGE = 0,
    TABLE_SWE_DUBBELT_AATERKVAL,
    TABLE_SWE_DIREKT_AATERKVAL,
    TABLE_EST_D_KLASS,
    TABLE_NO_REPECHAGE,
    TABLE_SWE_ENKELT_AATERKVAL,
    NUM_TABLES
};

enum cat_systems {
    CAT_SYSTEM_DEFAULT = 0,
    CAT_SYSTEM_POOL,
    CAT_SYSTEM_DPOOL,
    CAT_SYSTEM_REPECHAGE,
    CAT_SYSTEM_DUBBELT_AATERKVAL,
    CAT_SYSTEM_DIREKT_AATERKVAL,
    CAT_SYSTEM_EST_D_KLASS,
    CAT_SYSTEM_NO_REPECHAGE,
    CAT_SYSTEM_ENKELT_AATERKVAL,
    NUM_SYSTEMS
};

enum french_systems {
    FRENCH_8 = 0,
    FRENCH_16,
    FRENCH_32,
    FRENCH_64,
    NUM_FRENCH
};

#define NUM_MATCHES 128

#define NO_COMPETITOR  0
#define GHOST          1
#define COMPETITOR     9

#define REPECHAGE      1

#define DELETED      1
#define HANSOKUMAKE  2

#define NEXT_MATCH_NUM 10

#define SHOW_MESSAGE(_a...) do {gchar b[100]; snprintf(b, sizeof(b), _a); show_message(b); } while (0)

#define MATCHED_POOL(_a) (pm.m[_a].blue_points || pm.m[_a].white_points || \
                          pm.m[_a].blue == GHOST || pm.m[_a].white == GHOST)

#define MATCHED_POOL_P(_a) (pm.m[_a].blue_points || pm.m[_a].white_points)

#define NUM_OTHERS 4

#define PRINT_ITEM_MASK      0xf0000000
#define PRINT_DEST_MASK      0x0f000000
#define PRINT_DATA_MASK      0x00ffffff
#define PRINT_TO_PRINTER     0x00000000
#define PRINT_TO_PDF         0x01000000
#define PRINT_SHEET          0x00000000
#define PRINT_WEIGHING_NOTES 0x10000000
#define PRINT_SCHEDULE       0x20000000
#define PRINT_ALL_CATEGORIES 0x30000000

#define IS(x) (!strcmp(azColName[i], #x))
#define VARVAL(_var, _val) "\"" #_var "\"=\"" #_val "\""
#define VALINT "\"%d\""
#define VALSTR "\"%s\""

#define ADD_MATCH                    0x00000004
#define DB_READ_CATEGORY_MATCHES     0x00000008
#define DB_NEXT_MATCH                0x00000010
#define DB_REMOVE_COMMENT            0x00000020
#define SAVE_MATCH                   0x00000040
#define DB_MATCH_WAITING             0x00000080
#define DB_UPDATE_LAST_MATCH_TIME    0x00000100
#define DB_RESET_LAST_MATCH_TIME_B   0x00000200
#define DB_RESET_LAST_MATCH_TIME_W   0x00000400

#define DB_GET_SYSTEM                32
#define DB_MAKE_PNG                  64

#define INVALID_MATCH              1000

enum {
    COL_INDEX = 0,
    COL_LAST_NAME,
    COL_FIRST_NAME,
    COL_BIRTHYEAR,
    COL_BELT,
    COL_CLUB,
    COL_COUNTRY,
    COL_WCLASS,
    COL_WEIGHT,
    COL_VISIBLE,
    COL_CATEGORY,
    COL_DELETED,
    COL_ID,
    NUM_COLS
};

enum {
    SORTID_NAME = 0,
    SORTID_WEIGHT,
    SORTID_CLUB,
    SORTID_WCLASS,
    SORTID_BELT,
    SORTID_COUNTRY
};

enum comments {
    COMMENT_EMPTY = 0,
    COMMENT_MATCH_1,
    COMMENT_MATCH_2,
    COMMENT_WAIT,
    NUM_COMMENTS
};

enum {
    COL_MATCH_GROUP,
    COL_MATCH_CAT,
    COL_MATCH_NUMBER,
    COL_MATCH_BLUE,
    COL_MATCH_BLUE_SCORE,
    COL_MATCH_BLUE_POINTS,
    COL_MATCH_WHITE_POINTS,
    COL_MATCH_WHITE_SCORE,
    COL_MATCH_WHITE,
    COL_MATCH_TIME,
    COL_MATCH_COMMENT,
    COL_MATCH_VIS,
    COL_MATCH_STATUS,
    NUM_MATCH_COLUMNS
};

enum {
    competitortext,
    competitoratext,
    competitorbtext,
    numbertext,
    nametext,
    gradetext,
    clubtext,
    matchestext,
    matchesatext,
    matchesbtext,

    matchtext,
    bluetext,
    whitetext,
    resulttext,
    resultstext,
    timetext,
    wintext,
    pointstext,
    positiontext,
    prevwinnertext,

    nextmatchtext,
    weighttext,
    scheduletext,
    weighinfiletext,
    schedulefiletext,
    categoriesfiletext,
    categorytext,
    nextmatch2text,
    categoriestext,
    statisticstext,

    medalstext,
    totaltext,

    numprinttexts
};

#define _T(_txt) print_texts[_txt##text][print_lang]

#define MATCH_STATUS_TATAMI_MASK  0xf
#define MATCH_STATUS_TATAMI_SHIFT   0

#define FREEZE_MATCHES    0
#define UNFREEZE_EXPORTED 1
#define UNFREEZE_IMPORTED 2
#define UNFREEZE_THIS     3

#define MATCH_EXISTS    1
#define MATCH_MATCHED   2
#define MATCH_UNMATCHED 4
#define CAT_PRINTED     8

enum official_categories {
    CAT_UNKNOWN = 0,
    CAT_M,
    CAT_BA,
    CAT_BB,
    CAT_BC,
    CAT_BD,
    CAT_BE,
    CAT_W,
    CAT_GA,
    CAT_GB,
    CAT_GC,
    CAT_GD,
    CAT_GE,
    NUM_CAT
};

enum button_responses {
    Q_LOCAL = 1000,
    Q_REMOTE,
    Q_CONTINUE,
    Q_QUIT
};

// Club text is composed of club name and/or country name.
// Also abbreviation and club address may be used.
#define CLUB_TEXT_CLUB         1
#define CLUB_TEXT_COUNTRY      2
#define CLUB_TEXT_ABBREVIATION 4
#define CLUB_TEXT_ADDRESS      8

// Default drawing system
enum default_drawing_system {
    DRAW_INTERNATIONAL,
    DRAW_FINNISH,
    DRAW_SWEDISH,
    DRAW_ESTONIAN
};

struct judoka {
    gint          index;
    const  gchar *last;
    const  gchar *first;
    gint          birthyear;
    gint          belt;
    const  gchar *club;
    const  gchar *regcategory;
    gint          weight;
    GtkWidget    *label;
    GtkWidget    *edit;
    gboolean      visible;
    const gchar  *category;
    guint         deleted;
    const gchar  *country;
    const gchar  *id;
};

struct match {
    guint category;
    guint number;
    guint blue;
    guint white;
    guint blue_score; 
    guint white_score;
    guint blue_points; 
    guint white_points;
    guint match_time;
    guint comment;
    gboolean visible;
    guint deleted;
    gboolean ignore;
    gint  tatami;
    gint  group;
    gint  flags; // defined in comm.h
    gint  forcedtatami;
    gint  forcednumber;
};

struct pool_matches {
    gint c[8], wins[8], pts[8], mw[8][8];
    struct match m[16];
    struct judoka *j[8];
    gboolean yes[8];
    gboolean finished;
    gboolean all_matched[8];
    gint num_matches;
};

struct next_match_info {
    gchar category[10];
    gchar blue_first[20];
    gchar blue_last[30];
    gchar blue_club[30];
    gchar white_first[20];
    gchar white_last[30];
    gchar white_club[30];
    gchar won_last[30];
    gchar won_first[20];
    gchar won_cat[10];
    gint  catnum;
    gint  matchnum;
    gint  blue;
    gint  white;
    gint  won_catnum;
    gint  won_matchnum;
    gint  won_ix;
};

#define NUM_CAT_DEF_WEIGHTS 16

struct cat_def {
    gint     age;
    gchar    agetext[16];
    gint     gender;
    gint     match_time;
    gint     pin_time_koka;
    gint     pin_time_yuko;
    gint     pin_time_wazaari;
    gint     pin_time_ippon;
    gint     rest_time;
    gint     gs_time;
    struct {
        gint comp;
        gint ippons;
        gint wazaaris;
        gint yukos;
        gint kokas;
        gint hanteis;
        gint total;
        gint time;
    } stat;
    struct {
        gint  weight;
        gchar weighttext[16];
    } weights[NUM_CAT_DEF_WEIGHTS];
};

struct category_data {
    gint index;
    gchar category[32];
    gint tatami;
    gint group;
    gint system;
    gint match_status;
    gint match_count;
    gint matched_matches_count;
    gint rest_time;
    struct category_data *prev, *next;
};

struct competitor_data {
    gint index;
    gint hash;
    gint status;
};

struct club_data {
    gchar *name;
    gchar *country;
    gint gold;
    gint silver;
    gint bronze;
    gint fourth;
    gint fifth;
    gint competitors;
    struct club_data *next;
};

struct club_name_data {
    gchar *name;
    gchar *abbreviation;
    gchar *address;
};

struct paint_data {
    cairo_t *c;
    gint     category;
    gint     page;
    gint     systm;
    gdouble  paper_width;
    gdouble  paper_height;
    gdouble  total_width;
    gboolean write_results;
};

extern avl_tree *categories_tree;
extern struct category_data category_queue[NUM_TATAMIS+1];

extern guint estim_num_matches[65];

extern GtkWidget *match_view[NUM_TATAMIS];

extern gchar *program_path;
extern guint current_year;
extern gchar *installation_dir;
extern gulong my_ip_address, node_ip_addr;
extern gint print_lang, club_text, draw_system;
extern gint number_of_tatamis;
extern gint language;

extern gchar          current_directory[1024];
extern gchar          database_name[200];
extern gchar          logfile_name[200];
extern gchar          info_competition[];
extern gchar          info_date[];
extern gchar          info_time[];
extern gchar          info_place[];
extern gchar          info_num_tatamis[];
extern gboolean       three_matches_for_two;

extern guint          current_index;
extern guint          current_category_index;
extern GtkWidget     *current_view;
extern GtkTreeModel  *current_model;
extern gint           current_category;
extern GtkWidget     *main_window;
extern gint           my_address;
extern GdkCursor     *wait_cursor;

extern char *belts[];
extern gchar *cat_names[];
extern gint official[][10];

extern const guint pools[8][11][2];
//extern guint competitors_1st_match[8][8][2];
//extern const guint french_size[NUM_FRENCH];
extern const guint french_num_matches[NUM_TABLES][NUM_FRENCH];
extern const gint french_matches[NUM_TABLES][NUM_FRENCH][NUM_MATCHES][2];
extern const gint medal_matches[NUM_TABLES][NUM_FRENCH][3];
extern const gchar french_64_matches_to_page[NUM_TABLES][NUM_MATCHES];
extern const gint result_y_position[NUM_TABLES][NUM_FRENCH];
extern const gint repechage_start[NUM_TABLES][NUM_FRENCH][2];
extern struct next_match_info next_matches_info[NUM_TATAMIS][2];

extern FILE *result_file;
extern GKeyFile *keyfile;

extern GCompletion *club_completer;

extern struct message hello_message;

extern gboolean automatic_sheet_update;
extern gboolean automatic_web_page_update;
extern gboolean weights_in_sheets;
extern gboolean cleanup_import;

extern gint webpwcrc32;

const char *db_name;

extern void progress_show(gdouble progress, const gchar *text);
extern gboolean this_is_shiai(void);
extern void refresh_window(void);
extern void set_competitors_col_titles(void);
extern void set_match_col_titles(void);
extern void set_log_col_titles(void);
extern void set_cat_graph_titles(void);
extern void set_match_graph_titles(void);
extern void set_sheet_titles(void);

extern void destroy(GtkWidget *widget, gpointer data);
extern gint num_matches(gint num_judokas);
extern void set_judokas_page(GtkWidget *notebook);
extern void set_sheet_page(GtkWidget *notebook);
extern GtkWidget *get_menubar_menu( GtkWidget  *window );
extern void open_shiai_display(void);
extern void new_judoka(GtkWidget *w, gpointer   data );
extern void new_regcategory(GtkWidget *w, gpointer   data );
extern void view_popup_menu(GtkWidget *treeview, 
                            GdkEventButton *event, 
                            gpointer userdata,
                            gchar *regcategory,
                            gboolean visible);
extern void remove_empty_regcategories(GtkWidget *w, gpointer data);
extern void remove_unweighed_competitors(GtkWidget *w, gpointer data);
extern gint sort_by_name(const gchar *name1, const gchar *name2);
extern void remove_competitors(GtkWidget *w, gpointer data);
extern void barcode_search(GtkWidget *w, gpointer data);
extern gint sort_iter_compare_func(GtkTreeModel *model, GtkTreeIter *a,
				   GtkTreeIter *b, gpointer userdata);
extern void last_name_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer,
				      GtkTreeModel *model, GtkTreeIter *iter,
				      gpointer user_data);
extern void weight_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer,
				   GtkTreeModel *model, GtkTreeIter *iter,
				   gpointer user_data);
extern void belt_cell_data_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer,
				 GtkTreeModel *model, GtkTreeIter *iter,
				 gpointer user_data);
extern gboolean change_language(GtkWidget *eventbox, GdkEventButton *event, void *param);


/* utils */
extern gboolean find_iter(GtkTreeIter *iter, guint index);
extern gboolean find_iter_model(GtkTreeIter *iter, guint index, GtkTreeModel *model);
extern gboolean find_iter_name(GtkTreeIter *iter, const gchar *last, const gchar *first, const gchar *club);
extern gboolean find_iter_name_2(GtkTreeIter *iter, const gchar *last, const gchar *first, 
				 const gchar *club, const gchar *category);
extern gboolean find_iter_category(GtkTreeIter *iter, const gchar *category);
extern gboolean find_iter_category_model(GtkTreeIter *iter, const gchar *category, GtkTreeModel *model);
extern void put_data(struct judoka *j, guint index);
extern void put_data_by_iter(struct judoka *j, GtkTreeIter *iter);
extern struct judoka *get_data(guint index);
extern struct judoka *get_data_by_iter(GtkTreeIter *iter);
extern struct judoka *get_data_by_iter_model(GtkTreeIter *iter, GtkTreeModel *model);
extern void put_data_by_iter_model(struct judoka *j, GtkTreeIter *iter, GtkTreeModel *model);
extern void free_judoka(struct judoka *j);
extern void show_message(gchar *msg);
extern gint ask_question(gchar *message);
extern gint weight_grams(const gchar *s);
extern void show_note(gchar *format, ...);
extern void print_time(gchar *fname, gint lineno);
extern gint timeval_subtract(GTimeVal *result, GTimeVal *x, GTimeVal *y);
extern gboolean valid_ascii_string(const gchar *s);
extern const gchar *get_club_text(struct judoka *j, gint flags);


/* db */

#define DB_MATCH_ROW 1
extern GStaticMutex next_match_mutex;

extern void db_new(const char *dbname, 
                   const gchar *competition, 
                   const gchar *date,
                   const gchar *place,
		   const gchar *start_time,
		   const gchar *num_tatamis);
extern void db_init(const char *db);
extern void db_matches_init(void);
extern void db_save_config(void);
extern void set_configuration(void);

extern gint db_exec(const char *dbn, char *cmd, void *data, void *dbcb);
extern void db_exec_str(void *data, void *dbcb, gchar *format, ...);
extern gint db_open(void);
extern void db_close(void);
extern gint db_cmd(void *data, void *dbcb, gchar *format, ...);

extern void db_read_judokas(void);
extern void db_add_judoka(int num, struct judoka *j);
extern void db_update_judoka(int num, struct judoka *j);
extern void db_restore_removed_competitors(void);

extern void db_add_category(int num, struct judoka *j);
extern void db_update_category(int num, struct judoka *j);
extern void db_read_categories(void);
extern void db_set_system(int num, int sys);
extern gint db_get_system(gint num);
extern gint db_get_tatami(gint num);

extern void db_read_competitor_statistics(gint *numcomp, gint *numweighted);
extern void db_add_competitors(const gchar *competition, gboolean with_weight, 
			       gint *added, gint *not_added);
extern void db_set_match(struct match *m);
extern void db_reset_last_match_times(gint category, gint number, gboolean blue, gboolean white);
extern void db_set_match_hansokumake(gint category, gint number, gint blue, gint white);
extern void db_read_category_matches(gint category, struct match *m);
extern void db_read_matches(void);
extern gboolean db_match_exists(gint category, gint number, gint flags);
extern gboolean db_matches_exists(void);
extern gboolean db_matched_matches_exist(gint category);
extern gint db_category_match_status(gint category);
extern gint db_competitor_match_status(gint competitor);
extern void db_remove_matches(guint category);
extern void db_set_points(gint category, gint number, gint minutes, 
                          gint blue, gint white, gint blue_score, gint white_score);
extern void db_read_match(gint category, gint number);
extern void db_read_matches_of_category(gint category);
extern struct match *db_next_match(gint category, gint tatami);
extern struct match *db_matches_waiting(void);
extern struct match *get_cached_next_matches(gint tatami);
extern void db_freeze_matches(gint tatami, gint category, gint number, gint arg);
extern void db_change_freezed(gint category, gint number, 
			      gint tatami, gint position, gboolean after);
extern gboolean db_has_hansokumake(gint competitor);
extern gint db_find_match_tatami(gint category, gint number);
extern void db_set_comment(gint category, gint number, gint comment);
extern void db_synchronize(char *name_2);
extern void db_print_competitors(FILE *f);
extern int db_get_table(char *command);
extern void db_close_table(void);
extern char *db_get_data(int row, char *name);
extern gchar *db_sql_command(const gchar *command);
extern void db_set_info(const gchar *competition, 
			const gchar *date, 
			const gchar *place,
			const gchar *start_time,
			const gchar *num_tatamis);
extern void db_create_cat_def_table(void);
extern void db_delete_cat_def_table_data(void);
extern void db_insert_cat_def_table_data(struct cat_def *line);
extern void db_cat_def_table_done(void);
extern void db_add_colums_to_cat_def_table(void);
extern gboolean catdef_needs_init(void);
extern gchar *db_get_col_data(gchar **data, gint numcols, gint row, gchar *name);

/* categories */
extern void create_categories(GtkWidget *w, gpointer   data);
extern void create_best_categories(GtkWidget *w, gpointer   data);
extern void toggle_w_belt(gpointer callback_data, 
                          guint callback_action, GtkWidget *menu_item);
extern void toggle_gender(gpointer callback_data, 
                          guint callback_action, GtkWidget *menu_item);
extern void toggle_age(gpointer callback_data, 
                       guint callback_action, GtkWidget *menu_item);
extern void toggle_three_matches(GtkWidget *menu_item, gpointer data);
extern void draw_all(GtkWidget *w, gpointer data);
extern void view_popup_menu_draw_category(GtkWidget *menuitem, gpointer userdata);
extern void view_popup_menu_draw_category_manually(GtkWidget *menuitem, gpointer userdata);
extern void locate_1(GtkWidget *w, gpointer data);
extern void locate_2(GtkWidget *w, gpointer data);
extern void locate_3(GtkWidget *w, gpointer data);
extern void locate_4(GtkWidget *w, gpointer data);
extern void make_png_all(GtkWidget *w, gpointer data);
extern int get_output_directory(void);
extern void make_next_matches_html(void);
extern void update_category_status_info(gint category);
extern void update_category_status_info_all(void);
extern gint get_cat_system(gint index);

/* set_one_judoka */
extern gint set_category(GtkTreeIter *iter, guint index, 
                         const gchar *category, guint tatami, guint group);
extern gint display_one_judoka(struct judoka *j);

/* log */
extern void set_log_page(GtkWidget *notebook);
extern void shiai_log(guint severity, guint tatami, gchar *format, ...);

#define SYS_LOG(_severity, _tatami, _txt...)    \
    do { gchar buf[100];                        \
        snprintf(buf, sizeof(buf), _txt);       \
        shiai_log(_severity, _tatami, buf);     \
    } while (0)

#define SYS_LOG_INFO(_txt...) shiai_log(1, 0, _txt)
#define SYS_LOG_WARNING(_txt...) shiai_log(2, 0, _txt)
#define COMP_LOG_INFO(_tatami, _txt...) shiai_log(1, _tatami, _txt)
#define COMP_LOG_WARNING(_tatami, _txt...) shiai_log(2, _tatami, _txt)

/* matches */
extern void set_match_pages(GtkWidget *notebook);
extern void matches_refresh(void);
extern void matches_clear(void);
extern void set_match(struct match *m);
extern void fill_pool_struct(gint category, gint num, struct pool_matches *pm);
extern void empty_pool_struct(struct pool_matches *pm);
extern void get_pool_winner(gint num, gint c[], gboolean yes[], 
                            gint wins[], gint pts[], 
                            gboolean mw[8][8], struct judoka *ju[], gboolean all_matched[]);
extern void update_competitors_categories(gint competitor);
extern void set_points_and_score(struct message *msg);
extern void set_comment_from_net(struct message *msg);
extern void set_points_from_net(struct message *msg);
extern gint find_match_time(const gchar *cat);
extern void set_points(GtkWidget *menuitem, gpointer userdata);
extern void update_matches_small(guint category, gint sys_or_tatami);
extern void update_matches(guint category, gint sys, gint tatami);
extern void category_refresh(gint category);
extern void gategories_refresh(void);
extern void update_match_pages_visibility(void);
extern gint get_match_number_flag(gint category, gint number);
extern gchar *get_match_number_text(gint category, gint number);
extern void send_matches(gint tatami);

/* sheets */
extern void paint_category(struct paint_data *pd);
extern void refresh_sheet_display(gboolean forced);
extern void set_font(gchar *font);
extern gchar *get_font_face(void);
extern void category_window(gint cat);
extern void write_sheet_to_stream(gint cat, cairo_write_func_t write_func, void *closure);

/* names */
#define IS_MALE   1
#define IS_FEMALE 2
gint find_gender(const gchar *name);

/* comm */
extern void toggle_tatami_1(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern void toggle_tatami_2(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern void toggle_tatami_3(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern void toggle_tatami_4(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern void toggle_tatami_5(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern void toggle_tatami_6(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern void toggle_tatami_7(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern void toggle_tatami_8(gpointer callback_data, 
                            guint callback_action, GtkWidget *menu_item);
extern gpointer server_thread(gpointer args);
extern gpointer node_thread(gpointer args);
extern gpointer client_thread(gpointer args);
extern gpointer httpd_thread(gpointer args);
extern gchar *other_info(gint num);
extern gint read_file_from_net(gchar *filename, gint num);
extern gulong get_my_address();
extern gint nodescan(guint network);
extern void show_node_connections(GtkWidget *w, gpointer data);
extern void copy_packet(struct message *msg);

/* ip */
extern void ask_node_ip_address( GtkWidget *w, gpointer data);
extern void show_my_ip_addresses( GtkWidget *w, gpointer data);
extern void msg_to_queue(struct message *msg);
extern struct message *get_rec_msg(void);
extern void set_preferences(void);
extern gulong host2net(gulong a);
extern struct message *put_to_rec_queue(volatile struct message *m);

extern GCompletion *club_completer_new(void);
extern int complete_cb(GtkWidget *widget, GdkEventKey *event, gpointer data);
extern void complete_add_if_not_exist(GCompletion *completer, const gchar *txt);

/* old_shiai */
extern void set_old_shiai_display(GtkWidget *w, gpointer data);

/* import */
extern void import_txt_dialog(GtkWidget *w, gpointer data);

/* set_webpassword */
extern void set_webpassword_dialog(GtkWidget *w, gpointer data);
extern gint pwcrc32(const guchar *str, gint len);

/* set_categories */
extern gint find_age_index(const gchar *category);
extern void read_cat_definitions(void);
extern void set_categories_dialog(GtkWidget *w, gpointer arg);
extern gint compare_categories(gchar *cat1, gchar *cat2);
extern gchar *find_correct_category(gint age, gint weight, gint gender, const gchar *category_now, gboolean best_match);
extern gboolean fill_in_next_match_message_data(const gchar *cat, struct msg_next_match *msg);
extern gint get_category_rest_time(const gchar *cat);

/* category_graph */
extern void set_category_graph_page(GtkWidget *notebook);

/* trees */
extern void init_trees(void);
extern struct category_data *avl_get_category(gint index);
extern void avl_set_category(gint index, const gchar *category, gint tatami, 
			     gint group, gint system);
extern void avl_set_category_rest_times(void);
extern gint avl_get_category_status_by_name(const gchar *name);
extern void set_category_to_queue(struct category_data *data);
extern void avl_set_competitor(gint index, const gchar *first, const gchar *last);
extern gint avl_get_competitor_hash(gint index);
extern void avl_set_competitor_last_match_time(gint index);
extern void avl_reset_competitor_last_match_time(gint index);
extern time_t avl_get_competitor_last_match_time(gint index);
extern void avl_set_competitor_status(gint index, gint status);
extern gint avl_get_competitor_status(gint index);
extern void init_club_tree(void);
extern void club_stat_add(const gchar *club, const gchar *country, gint num);
extern void club_stat_print(FILE *f);
extern const gchar *utf8_to_html(const gchar *txt);
extern void init_club_name_tree(void);
extern void club_name_set(const gchar *club, 
			  const gchar *abbr,
			  const gchar *address);
extern struct club_name_data *club_name_get(const gchar *club);

/* match_graph */
extern void set_match_graph_page(GtkWidget *notebook);
extern void set_graph_rest_time(gint tatami, time_t rest_end, gint flags);

/* search */
extern void search_competitor(GtkWidget *w, gpointer arg);

/* sql_dialog */
extern void sql_window(GtkWidget *w, gpointer data);

/* drawing */
extern void draw_one_category(GtkTreeIter *parent, gint competitors);
extern void draw_one_category_manually(GtkTreeIter *parent, gint competitors);
extern void draw_all(GtkWidget *w, gpointer data);
extern gint get_drawed_number(gint pos, gint sys);

/* menuacts */
extern void make_backup(void);

/* print */
extern void write_png(GtkWidget *menuitem, gpointer userdata);
extern void do_print(GtkWidget *menuitem, gpointer userdata);
extern void print_doc(GtkWidget *menuitem, gpointer userdata);
extern void print_matches(GtkWidget *menuitem, gpointer userdata);

/* print_texts */
extern gchar *print_texts[][3];

/* match-data */
extern gchar *get_system_name_for_menu(gint num);
extern gint get_system_number_by_menu_pos(gint num);
extern gint get_system_menu_selection(gint active);

#endif