/* -*- mode: C; c-basic-offset: 4;  -*- */

/*
 * Copyright (C) 2006-2010 by Hannu Jokinen
 * Full copyright text is included in the software package.
 */ 

#ifndef _JUDOTIMER_H_
#define _JUDOTIMER_H_

#include "comm.h"

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

#define FRAME_WIDTH  600
#define FRAME_HEIGHT 400

#define BLUE  1
#define WHITE 2
#define CMASK 3
#define GIVE_POINTS 4

#define OSAEKOMI_DSP_NO      0
#define OSAEKOMI_DSP_YES     1
#define OSAEKOMI_DSP_BLUE    2
#define OSAEKOMI_DSP_WHITE   3
#define OSAEKOMI_DSP_UNKNOWN 4

#define CLEAR_SELECTION   0
#define HANTEI_BLUE       1
#define HANTEI_WHITE      2
#define HANSOKUMAKE_BLUE  3
#define HANSOKUMAKE_WHITE 4

#define MODE_NORMAL 0
#define MODE_MASTER 1
#define MODE_SLAVE  2

#define START_BIG 128
#define STOP_BIG  129
#define START_ADVERTISEMENT 130


extern GTimer *timer;

extern gint tatami, mode;
extern gboolean blue_wins_voting;
extern gboolean white_wins_voting;
extern gboolean hansokumake_to_blue;
extern gboolean hansokumake_to_white;
extern gint osaekomi_winner;
extern gchar *installation_dir;
extern gulong my_ip_address, node_ip_addr;
extern gboolean automatic;
extern gboolean demo;
extern GKeyFile *keyfile;
extern gboolean rules_no_koka_dsp;
extern gboolean rules_leave_score;
extern gboolean clocks_only;
extern gboolean short_pin_times;
extern gint language;
extern gboolean rest_time;
extern gint current_category, current_match;

extern gboolean this_is_shiai(void);
extern void copy_packet(struct message *msg); // not used

//extern void clock_key(SDLKey key, SDLMod mod);
extern void update_clock(void);
extern int clock_running(void);
//extern void set_cursor(int cursor);
//extern int lz_uncompress(unsigned char *in, unsigned char *out, unsigned int insize);
extern void show_message(char *cat1, char *blue1, char *white1,
                         char *cat2, char *blue2, char *white2);
extern void send_result(int bluepts[4], int whitepts[4], char blue_vote, char white_vote, 
			char blue_hansokumake, char white_hansokumake);
extern void update_status(void);
extern void reset(guint key, struct msg_next_match *msg);
extern int get_match_time(void);

extern void set_hantei_winner(gint cmd);
extern void set_timer_run_color(gboolean running);
extern void set_timer_osaekomi_color(gint osaekomi_state, gint points);
extern void set_timer_value(guint min, guint tsec, guint sec);
extern void set_osaekomi_value(guint tsec, guint sec);
extern void set_points(gint blue[], gint white[]);
extern void set_score(guint score);
extern GtkWidget *get_menubar_menu(GtkWidget  *window);
extern GtkWidget *get_menubar_menu_en( GtkWidget  *window );
extern GtkWidget *get_menubar_menu_fi( GtkWidget  *window );
extern void clock_key(guint key, guint event_state);
extern void toggle_color(GtkWidget *menu_item, gpointer data);
extern void toggle_full_screen(GtkWidget *menu_item, gpointer data);
extern void toggle_rules_no_koka(GtkWidget *menu_item, gpointer data);
extern void toggle_rules_leave_points(GtkWidget *menu_item, gpointer data);
extern void select_display_layout(GtkWidget *menu_item, gpointer data);
extern gpointer client_thread(gpointer args);
extern void show_message(gchar *cat_1, gchar *blue_1, gchar *white_1, 
                         gchar *cat_2, gchar *blue_2, gchar *white_2);
extern void voting_result(GtkWidget *w, gpointer data);
extern void reset_display(gint key);
extern gboolean osaekomi_running(void);
extern void set_comment_text(gchar *txt);
extern gboolean set_osaekomi_winner(gint who);
extern void display_big(gchar *txt, gint tmo_sec);
extern void ask_node_ip_address( GtkWidget *w, gpointer data);
extern void show_my_ip_addresses( GtkWidget *w, gpointer data);
extern void open_comm_socket(void);
extern void change_language_1(void);
extern gboolean change_language(GtkWidget *eventbox, GdkEventButton *event, void *param);
extern void hajime_inc_func(void);
extern void hajime_dec_func(void);
extern void osaekomi_inc_func(void);
extern void osaekomi_dec_func(void);
extern void msg_to_queue(struct message *msg);
extern struct message *get_rec_msg(void);
extern void destroy( GtkWidget *widget, gpointer   data );
extern void set_preferences(void);
extern gulong host2net(gulong a);
//extern void undo_func(GtkWidget *menu_item, gpointer data);
extern gboolean undo_func(GtkWidget *menu_item, GdkEventButton *event, gpointer data);
extern void set_clocks(gint clock, gint osaekomi);
extern int array2int(int pts[4]);
extern gchar *get_name(gint who);
extern gchar *get_cat(void);
extern void judotimer_log(gchar *format, ...);
extern void cancel_rest_time(gboolean blue, gboolean white);
extern gboolean delete_big(gpointer data);
extern gint display_advertisement(gchar *FileName);
extern void toggle_advertise(GtkWidget *menu_item, gpointer data);
extern void display_ad_window(void);
extern void send_label_msg(struct message *msg);
extern gpointer master_thread(gpointer args);
extern void update_label(struct msg_update_label *msg);
extern gboolean send_label(gint bigdsp);


#endif