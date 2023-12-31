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

#define FRAME_WIDTH  600
#define FRAME_HEIGHT 400

#define BLUE  1
#define WHITE 2
#define CMASK 3
#define GIVE_POINTS 4

#define OSAEKOMI_DSP_NO      0
#define OSAEKOMI_DSP_YES     1
#define OSAEKOMI_DSP_YES2    2
#define OSAEKOMI_DSP_BLUE    3
#define OSAEKOMI_DSP_WHITE   4
#define OSAEKOMI_DSP_UNKNOWN 5

#define CLEAR_SELECTION   0
#define HANTEI_BLUE       1
#define HANTEI_WHITE      2
#define HANSOKUMAKE_BLUE  3
#define HANSOKUMAKE_WHITE 4
#define HIKIWAKE          5
#define NO_IPPON_CHECK   16
#define SELECTION_MASK   15

#define MODE_NORMAL 0
#define MODE_MASTER 1
#define MODE_SLAVE  2

struct paint_data {
    cairo_t *c;
    gint page;
    gdouble  paper_width;
    gdouble  paper_height;
};

extern GTimer *timer;

extern GtkWidget *main_window;
extern gint tatami, mode;
extern gboolean blue_wins_voting;
extern gboolean white_wins_voting;
extern gboolean hansokumake_to_blue;
extern gboolean hansokumake_to_white;
extern gboolean result_hikiwake;
extern gchar *installation_dir;
extern gulong my_ip_address, node_ip_addr;
extern gboolean automatic;
extern gint demo;
extern gchar *matchlist;
extern GKeyFile *keyfile;
//extern gboolean rule_no_free_shido;
extern gboolean use_2017_rules;
extern gboolean use_2018_rules;
extern gint use_ger_u12_rules;
extern gboolean rules_stop_ippon_2;
extern gboolean rules_confirm_match;
extern gboolean golden_score;
extern gboolean clocks_only;
extern gboolean short_pin_times;
extern gint language;
extern gboolean rest_time;
extern gint current_category, current_match;
extern gboolean sides_switched;
extern gboolean white_first;
extern gboolean no_big_text;
extern gboolean show_competitor_names;
extern gchar saved_first1[32], saved_first2[32], saved_last1[32], saved_last2[32], saved_cat[16];
extern gchar saved_country1[8], saved_country2[8];
extern gchar saved_club1[32], saved_club2[32];
extern gint saved_round;
extern gboolean fullscreen;
extern gboolean require_judogi_ok;
extern gboolean showflags, showletter;
extern gdouble  flagsize, namesize;

extern gboolean video_update;
extern gchar  video_http_host[128];
extern guint  video_http_port;
extern gchar  video_http_path[128];
extern gchar  video_http_user[32];
extern gchar  video_http_password[32];
extern gchar  video_proxy_host[128];
extern guint  video_proxy_port;

#define NUM_LOGO_FILES 4
extern gchar   *tvlogofile[NUM_LOGO_FILES];
extern gint     tvlogo_update;
extern gdouble  tvlogo_scale;
extern guint    vlc_port;
extern gint     tvlogo_x;
extern gint     tvlogo_y;
extern guint    tvlogo_port;
extern gboolean update_tvlogo; 
extern cairo_surface_t *tvcs;
extern cairo_t *tvc;
extern gboolean vlc_connection_ok;
extern gchar *custom_layout_file;
extern gint   custom_layout_file_num;

extern gchar font_face[32];
extern gint  font_slant, font_weight;

extern gint adwin_x, adwin_y, adwin_w, adwin_h;
extern gint last_reported_category;
extern gint last_reported_number;

extern gboolean this_is_shiai(void);
extern void copy_packet(struct message *msg); // not used

//extern void clock_key(SDLKey key, SDLMod mod);
extern void update_clock(void);
extern int clock_running(void);
//extern void set_cursor(int cursor);
//extern int lz_uncompress(unsigned char *in, unsigned char *out, unsigned int insize);
extern void show_message(char *cat1, char *blue1, char *white1,
                         char *cat2, char *blue2, char *white2, gint flags, gint round);
extern void send_result(int bluepts[4], int whitepts[4], char blue_vote, char white_vote,
			char blue_hansokumake, char white_hansokumake, gint legend, gint hikiwake);
extern void update_status(void);
extern void reset(guint key, struct msg_next_match *msg);
extern int get_match_time(void);

extern void set_hantei_winner(gint cmd);
extern void set_timer_run_color(gboolean running, gboolean resttime);
extern void set_timer_osaekomi_color(gint osaekomi_state, gint points, gboolean orun);
extern void set_timer_value(guint min, guint tsec, guint sec, gboolean isrest, guint flags);
extern void set_osaekomi_value(guint tsec, guint sec);
extern void set_points(gint blue[5], gint white[5]);
extern void set_score(guint score);
extern GtkWidget *get_menubar_menu(GtkWidget  *window);
extern GtkWidget *get_menubar_menu_en( GtkWidget  *window );
extern GtkWidget *get_menubar_menu_fi( GtkWidget  *window );
extern void clock_key(guint key, guint event_state);
extern void toggle_color(GtkWidget *menu_item, gpointer data);
extern void toggle_full_screen(GtkWidget *menu_item, gpointer data);
extern void toggle_rules_stop_ippon(GtkWidget *menu_item, gpointer data);
extern void toggle_rules_no_free_shido(GtkWidget *menu_item, gpointer data);
extern void toggle_rules_eq_score_less_shido_wins(GtkWidget *menu_item, gpointer data);
extern void toggle_rules_2017(GtkWidget *menu_item, gpointer data);
extern void toggle_rules_2018(GtkWidget *menu_item, gpointer data);
extern void toggle_confirm_match(GtkWidget *menu_item, gpointer data);
extern void toggle_whitefirst(GtkWidget *menu_item, gpointer data);
extern void toggle_no_texts(GtkWidget *menu_item, gpointer data);
extern void toggle_show_comp(GtkWidget *menu_item, gpointer data);
extern void toggle_judogi_control(GtkWidget *menu_item, gpointer data);
extern void confirm_match_svg(GtkWidget *menu_item, gpointer data);
extern void read_confirm_svg_file(gchar *fname);
extern void select_display_layout(GtkWidget *menu_item, gpointer data);
extern void change_custom_layout(gchar *layout);
extern void set_custom_layout_file_name(GtkWidget *menu_item, gpointer data);
extern void select_name_layout(GtkWidget *menu_item, gpointer data);
extern gpointer client_thread(gpointer args);
extern void voting_result(GtkWidget *w, gpointer data);
extern void reset_display(gint key);
extern gboolean osaekomi_running(void);
extern void set_comment_text(gchar *txt);
extern gboolean set_osaekomi_winner(gint who);
extern void display_big(gchar *txt, gint tmo_sec);
extern void ask_node_ip_address( GtkWidget *w, gpointer data);
extern void show_my_ip_addresses( GtkWidget *w, gpointer data);
extern void ask_video_ip_address( GtkWidget *w, gpointer data);
extern void open_comm_socket(void);
extern void change_language_1(void);
extern gboolean change_language(GtkWidget *eventbox, GdkEventButton *event, void *param);
extern void hajime_inc_func(gdouble fix);
extern void hajime_dec_func(gdouble fix);
extern void osaekomi_inc_func(void);
extern void osaekomi_dec_func(void);
extern void msg_to_queue(struct message *msg);
extern struct message *get_rec_msg(void);
extern void destroy( GtkWidget *widget, gpointer   data );
extern void set_preferences(void);
extern void set_preferences_keyfile(GKeyFile *key_file, gboolean defaults);
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
extern void read_svg_file(gchar *fname);
extern void send_label_msg(struct message *msg);
extern gpointer master_thread(gpointer args);
extern gpointer video_thread(gpointer args);
extern gpointer tvlogo_thread(gpointer args);
extern gpointer sound_thread(gpointer args);
extern void update_label(struct msg_update_label *msg);
extern gboolean send_label(gint bigdsp);
extern void open_sound(void);
extern void close_sound(void);
extern void play_sound(void);
extern void select_sound(GtkWidget *menu_item, gpointer data);
extern void set_gs_text(gchar *txt);
extern void clear_switch_sides(void);
extern void toggle_switch_sides(GtkWidget *menu_item, gpointer data);
extern void light_switch_sides(gboolean yes);
extern void parse_name(const gchar *s, gchar *first, gchar *last, gchar *club, gchar *country);
extern void display_comp_window(gchar *cat, gchar *comp1, gchar *comp2, 
                                gchar *first1, gchar *first2, gchar *country1, gchar *country2,
				gchar *club1, gchar *club2, gint round);
extern gboolean blue_background(void);
extern void close_ad_window(gint pagenum);
extern void display_ask_window(gchar *name, gchar *cat, gchar winner);
extern void close_ask_window(void);
extern void set_competitor_window_rest_time(gint min, gint tsec, gint sec, gboolean rest, gint flags);
extern void set_competitor_window_judogi_control(gboolean control, gint flags);
extern void set_font(gchar *font);
extern void font_dialog(GtkWidget *w, gpointer data);
extern void create_video_window(void);
extern void video_record(gboolean yes);
extern glong hostname_to_addr(gchar *str);
extern void video_init(void);
extern void ask_tvlogo_settings(GtkWidget *w, gpointer data);
extern void write_tv_logo(struct msg_update_label *msg);
extern void tvlogo_send_new_frame(gchar *frame, gint length);

#define ACTIVE (mode != MODE_SLAVE)
extern void set_menu_active(void);

extern void set_menu_white_first(gboolean flag);
extern void activate_slave_mode(void);
extern void set_rules_year(gint year);

extern gint websock_send_msg(gint fd, struct message *msg);
extern void handle_websock(struct jsconn *conn, char *in, gint length, struct message *msg);

#endif
