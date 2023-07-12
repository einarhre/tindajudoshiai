import 'package:flutter/material.dart';
import 'package:hive/hive.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'package:judolib/judolib.dart';
import 'package:judotimer/util.dart';

const STACK_DEPTH = 8;
const I = 0;
const W = 1;
const Y = 2;
const S = 3;
const L = 5;
const OSAEKOMI_DSP_NO      = 0;
const OSAEKOMI_DSP_YES     = 1;
const OSAEKOMI_DSP_YES2    = 2;
const OSAEKOMI_DSP_BLUE    = 3;
const OSAEKOMI_DSP_WHITE   = 4;
const OSAEKOMI_DSP_UNKNOWN = 5;
const BLUE = 1;
const WHITE = 2;
const CMASK = 3;
const GIVE_POINTS = 4;
const CLEAR_SELECTION   = 0;
const HANTEI_BLUE       = 1;
const HANTEI_WHITE      = 2;
const HANSOKUMAKE_BLUE  = 3;
const HANSOKUMAKE_WHITE = 4;
const HIKIWAKE          = 5;
const NO_IPPON_CHECK    = 16;
const SELECTION_MASK    = 15;

const MODE_NORMAL = 0;
const MODE_MASTER = 1;
const MODE_SLAVE  = 2;

const START_BIG           = 128;
const STOP_BIG            = 129;
const START_ADVERTISEMENT = 130;
const START_COMPETITORS   = 131;
const STOP_COMPETITORS    = 132;
const START_WINNER        = 133;
const STOP_WINNER         = 134;
const SAVED_LAST_NAMES    = 135;
const SHOW_MSG            = 136;
const SET_SCORE           = 137;
const SET_POINTS          = 138;
const SET_OSAEKOMI_VALUE  = 139;
const SET_TIMER_VALUE     = 140;
const SET_TIMER_OSAEKOMI_COLOR = 141;
const SET_TIMER_RUN_COLOR = 142;
const MAX_LABEL_NUMBER    = 143;

class StackVal {
  int who;
  int points;
  StackVal(this.who, this.points);
}

int osaekomi_winner = 0;
var action_on = false;
bool running = false, oRunning = false;
int now = 0, startTime = 0, oStartTime = 0, elap = 0, oElap = 0;
int score = 0;
List<int> comp1pts = [0,0,0,0,0];
List<int> comp2pts = [0,0,0,0,0];
var big_displayed = false;
int match_time = 0;
List<StackVal> stackval = List.filled(8, StackVal(0, 0), growable: false);
int stackdepth = 0;
int last_m_time = 0;
bool last_m_run = false;
int  last_m_otime = 0;
int last_m_orun = 0;
int  last_score = 0;
var osaekomi = ' ';

int koka = 0;
int yuko = 0;
int wazaari = 100;
int ippon = 200;
int total = 2400;
int gs_time = 0;
var tatami = 1;
bool automatic = true;
var golden_score = false;
bool rest_time = false;
int  rest_flags = 0;
int gs_cat = 0, gs_num = 0;
bool jcontrol = false;
int jcontrol_flags = 0;
int last_shido_to = 0;

int flash = 0;
var bluepts = [0, 0, 0, 0, 0];
var whitepts = [0, 0, 0, 0, 0];
bool asking = false;
Keys key_pending = Keys.None;

String saved_first1 = '', saved_first2 = '', saved_last1 = '', saved_last2 = '', saved_cat = '';
String saved_country1 = '', saved_country2 = '', saved_club1 = '', saved_club2 = '';
int  saved_round = 0;

int      current_index = 10;
int      current_category_index = 10000;
int      current_category = 0;
int      current_match = 0;
int      my_address = 0;
bool     automatic_sheet_update = FALSE;
bool     automatic_web_page_update = FALSE;
bool     print_svg = FALSE;
bool     weights_in_sheets = FALSE;
bool     grade_visible = FALSE;
int      name_layout = 0;
bool     pool_style = FALSE;
bool     belt_colors = FALSE;
bool     cleanup_import = FALSE;
bool     result_hikiwake = FALSE;
bool     blue_wins_voting = FALSE, white_wins_voting = FALSE;
bool     hansokumake_to_blue = FALSE, hansokumake_to_white = FALSE;

int last_reported_category = 0;
int last_reported_number = 0;
int legend = 0;

int msg_out_start_time = 0, msg_out_err_time = 0;
int msg_out_addr = 0;
int msg_out_text_set_tmo = 0;

bool tv_logo = false;
bool current_orun = false;

Color clock_run = Colors.white;
Color clock_stop = Colors.yellow;
Color? clock_bg = Colors.black;

Color oclock_run = Colors.green;
Color oclock_stop = Colors.black;
Color? oclock_bg = Colors.black;

bool hide_clock_if_osaekomi = false;
bool no_frames = false;
int show_shidos = 0;
bool activate_slave_mode = false;
bool hide_scores_if_zero = false;
int hide_zero_osaekomi_points = 0;
bool show_shido_cards = false;
int window_layout_x = -1;
int window_layout_y = -1;
int window_layout_w = -1;
int window_layout_h = -1;
Image? background_image = null;
String display_font = 'Arial Bold';
String shido1_png = 'assets/shido-none.png';
String shido2_png = 'assets/shido-none.png';

int global_tick = 0;

/* settings */
int mode = 0;
bool use_2017_rules = false;
bool use_2018_rules = true;
bool short_pin_times = false;
bool rules_confirm_match = true;
var use_ger_u12_rules = 0;
bool rules_stop_ippon_2 = false;
int language = 0;
bool no_big_text = false;
bool show_competitor_names = true;
bool show_soremade = false;
bool require_judogi_ok = false;
bool fullscreen = false;
int selected_name_layout = 7;
bool mode_slave = false;
bool showflags = true, showletter = false;
double  flagsize = 0, namesize = 0;
String node_name = 'judoshiai.local';
String master_name = 'judotimer1.local';
String sound = 'IndutrialAlarm';
var jspassword = '';
bool showcompetitors = true;
bool showwinner = true;
/*****/

// Hive
Box? box = null;