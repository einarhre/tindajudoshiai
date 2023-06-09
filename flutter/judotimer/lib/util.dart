import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:hive/hive.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'package:judolib/judolib.dart';
import 'package:judotimer/label.dart';
import 'package:judotimer/layout.dart';
import 'package:judotimer/settings.dart';
import 'package:judotimer/stopwatch.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'global.dart';

enum Keys {
  None,
  GDK_0,
  GDK_1,
  GDK_2,
  GDK_3,
  GDK_4,
  GDK_5,
  GDK_6,
  GDK_7,
  GDK_9,
  GDK_b,
  GDK_c,
  GDK_Down,
  GDK_e,
  GDK_F1,
  GDK_F2,
  GDK_F3,
  GDK_F4,
  GDK_F5,
  GDK_F6,
  GDK_F7,
  GDK_F8,
  GDK_k,
  GDK_KP,
  GDK_l,
  GDK_Return,
  GDK_s,
  GDK_space,
  GDK_t,
  GDK_Up,
  GDK_v,
  GDK_w,
  GDK_z,
  ASK_OK,
  ASK_NOK
}

const char2key = {
  '0': Keys.GDK_0,
  '1': Keys.GDK_1,
  '2': Keys.GDK_2,
  '3': Keys.GDK_3,
  '4': Keys.GDK_4,
  '5': Keys.GDK_5,
  '6': Keys.GDK_6,
  '7': Keys.GDK_7,
  '9': Keys.GDK_9,
  'b': Keys.GDK_b,
  'c': Keys.GDK_c,
  'Arrow Down': Keys.GDK_Down,
  'e': Keys.GDK_e,
  'F1': Keys.GDK_F1,
  'F2': Keys.GDK_F2,
  'F3': Keys.GDK_F3,
  'F4': Keys.GDK_F4,
  'F5': Keys.GDK_F5,
  'F6': Keys.GDK_F6,
  'F7': Keys.GDK_F7,
  'F8': Keys.GDK_F8,
  'k': Keys.GDK_k,
  'KP': Keys.GDK_KP,
  'l': Keys.GDK_l,
  'Enter': Keys.GDK_Return,
  's': Keys.GDK_s,
  ' ': Keys.GDK_space,
  't': Keys.GDK_t,
  'Arrow Up': Keys.GDK_Up,
  'v': Keys.GDK_v,
  'w': Keys.GDK_w,
  'z': Keys.GDK_z,
};

Future<List<Label>> getTimerCustom() async {
  var host = await getHostName('jsip');
  var lbs = <Label>[];
  background_image = null;

  await readSettings();

  await Hive.initFlutter();

  try {
    /***
        var response = await http.get(
        Uri.parse('http://$host:8088/timer-custom.txt'),
        );
     ***/
    //var response = await rootBundle.loadString('assets/timer-custom.txt');
    var response = await rootBundle.loadString(tv_logo ? 'assets/timer-tv-logo.txt' : 'assets/timer-custom.txt');
    if (response.length > 0) {
      LineSplitter ls = new LineSplitter();
      List<String> lines = ls.convert(response);
      for (var l in lines) {
        if (l.length < 1 ||
            l.codeUnitAt(0) < '0'.codeUnitAt(0) ||
            l.codeUnitAt(0) > '9'.codeUnitAt(0)) continue;
        final pattern = RegExp('\\s+');
        final l2 = l.replaceAll(pattern, ' ');
        var a = l2.split(' ');
        //print('LINE=$l len=${a.length}');
        var num = int.parse(a[0]);
        if (num < 100 && a.length >= 13) {
          var label = Label(
              num,
              double.parse(a[1]),
              double.parse(a[2]),
              double.parse(a[3]),
              double.parse(a[4]),
              double.parse(a[5]),
              int.parse(a[6]),
              double.parse(a[7]),
              double.parse(a[8]),
              double.parse(a[9]),
              double.parse(a[10]),
              double.parse(a[11]),
              double.parse(a[12]));
          lbs.add(label);
        } else if (num == Label.misc_settings) {
          hide_clock_if_osaekomi = a[1] == '1';
          no_frames = a[2] == '1';
          hide_zero_osaekomi_points = int.parse(a[3]);
          activate_slave_mode = a[4] == '1';
          hide_scores_if_zero = a[5] == '1';
          show_shido_cards = a[6] == '1';
          if (activate_slave_mode) mode_slave = true;
        } else if (num == Label.window_layout) {
          window_layout_x = int.parse(a[1]);
          window_layout_y = int.parse(a[2]);
          window_layout_w = int.parse(a[3]);
          window_layout_h = int.parse(a[4]);
        } else if (num == Label.bg_image) {
          if (window_layout_w > 0 && window_layout_h > 0) {
            background_image = Image(
              image: AssetImage('assets/${a[1]}'),
              width: window_layout_w as double,
              height: window_layout_h as double,
            );
          } else {
            background_image = Image(
              image: AssetImage('assets/${a[1]}'),
              fit: BoxFit.fill,
            );
          }
        }
      }
    }
  } catch (e) {
    print("HTTP get timer_custom.txt error $e");
    //rethrow;
  }

  box = await Hive.openBox('timer');

  return lbs;
}

void handle_message(LayoutState layout, Message msg) {
  var t = AppLocalizations.of(layout.context);
  //print('MSG TYPE=${msg.type}');
  if (msg.type == MSG_NEXT_MATCH) {
    MsgNextMatch nm = MsgNextMatch(msg.message);
    if (nm.tatami != tatami) return;
    if (clock_running()) return;

    layout.show_message(nm.cat_1, nm.blue_1, nm.white_1, nm.cat_2, nm.blue_2,
        nm.white_2, nm.flags, nm.round);
    if (nm.minutes > 0 && automatic) reset(layout, Keys.GDK_0, nm);
    if (golden_score) layout.set_comment_text(t?.goldenScore7836 ?? '');
    if (current_category != nm.category || current_match != nm.match) {
      layout.display_comp_window(
          saved_cat,
          saved_last1,
          saved_last2,
          saved_first1,
          saved_first2,
          saved_country1,
          saved_country2,
          saved_round);
      current_category = nm.category;
      current_match = nm.match;
      print('OLD=$oldCategory/$oldNumber NEW=$current_category/$current_match');
      if (!clock_running() &&
          current_category == oldCategory &&
          current_match == oldNumber &&
          (oldClock != 0 || oldOsaekomi != 0)) {
        action_on = true;
        set_clocks(layout, oldClock, oldOsaekomi);
      }

      print('SET CAT $current_category/$current_match');
      setValInt('category', current_category);
      setValInt('number', current_match);
    }
  } else if (msg.type == MSG_UPDATE_LABEL) {
    MsgUpdateLabel ml = MsgUpdateLabel(msg.message);
    //print('UPDATE LABEL type ${ml.label_num}');
    switch (ml.label_num) {
      case START_BIG:
        StartBig m = ml.start_big;
        layout.display_big(m.big_text, 1000);
        break;
      case STOP_BIG:
        layout.display_big('', 0);
        break;
      case START_ADVERTISEMENT:
        break;
      case START_COMPETITORS:
        print('START COMP');
        StartCompetitors m = ml.start_competitors;
        layout.display_comp_window(
            m.cat_1, m.blue_1, m.white_1, '', '', '', '', m.round);
        break;
      case STOP_COMPETITORS:
        print('STOP COMP');
        StopCompetitors m = ml.stop_competitors;
        layout.displayMainScreen();
        //Navigator.pop(layout.context);
        break;
      case START_WINNER:
        print('START WINNER rules_confirm_match=$rules_confirm_match');
        StartWinner m = ml.start_winner;
        if (rules_confirm_match)
          layout.display_winner_window(m.cat, m.last, m.first, m.winner);
        /*
          Navigator.push(layout.context,
          MaterialPageRoute(builder: (context) {
              return ShowWinner(layout, layout.widget.width, layout.widget.height, m.cat, m.last, m.first);
          }));
         */
        break;
      case STOP_WINNER:
        print('STOP WINNER');
        layout.displayMainScreen();
        /*if (Navigator.canPop(layout.context))
          Navigator.pop(layout.context);*/
        break;
      case SAVED_LAST_NAMES:
        SavedLastNames m = ml.saved_last_names;
        saved_last1 = m.last1;
        saved_last2 = m.last2;
        saved_cat = m.cat;
        break;
      case SHOW_MSG:
        ShowMsg m = ml.show_msg;
        layout.show_message(m.cat_1, m.blue_1, m.white_1, m.cat_2, m.blue_2,
            m.white_2, m.flags, m.round);
        break;
      case SET_SCORE:
        SetScore m = ml.set_score;
        layout.setScore(m.score);
        break;
      case SET_POINTS:
        SetPoints m = ml.set_points;
        layout.set_points(m.pts1, m.pts2);
        break;
      case SET_OSAEKOMI_VALUE:
        SetOsaekomiValue m = ml.set_osaekomi_value;
        layout.set_osaekomi_value(m.tsec, m.sec);
        break;
      case SET_TIMER_VALUE:
        SetTimerValue m = ml.set_timer_value;
        layout.set_timer_value(m.min, m.tsec, m.sec, m.isrest, m.flags);
        break;
      case SET_TIMER_OSAEKOMI_COLOR:
        SetTimerOsaekomiColor m = ml.set_timer_osaekomi_color;
        //print('OKOMI COLOR ${m.osaekomi_state} ${m.pts} ${m.orun}');
        layout.set_timer_osaekomi_color(m.osaekomi_state, m.pts, m.orun);
        break;
      case SET_TIMER_RUN_COLOR:
        SetTimerRunColor m = ml.set_timer_run_color;
        //mylog('util.dart', 249, 'MSG SET_TIMER_RUN_COLOR running=${m.running}, rest=${m.resttime}');
        layout.set_timer_run_color(m.running, m.resttime);
        break;
    }
  }
}

String get_name_by_layout(
    String first, String last, String club, String country) {
  switch (selected_name_layout) {
    case 0:
      if (country == null || country.length == 0)
        return '$first $last, $club';
      else if (club == null || club.length == 0) return '$first $last, $country';
      return '$first $last, $country/$club';
    case 1:
      if (country == null ||  country.length == 0)
        return '$last, $first, $club';
      else if (club == null || club.length == 0) return '$last, $first, $country';
      return '$last, $first, $country/$club';
    case 2:
      if (country == null || country.length == 0)
        return '$club, $last, $first';
      else if (club == null || club.length == 0) return '$country $last, $first';
      return '$country/$club $last, $first';
    case 3:
      return '$country $last, $first';
    case 4:
      return '$club $last, $first';
    case 5:
      return '$country $last';
    case 6:
      return '$club $last';
    case 7:
      return '$last, $first';
    case 8:
      return last;
    case 9:
      return country;
    case 10:
      return club;
  }

  return '';
}

int array2int(List<int> pts) {
  int x = 0;
  x = (pts[0] << 16) | (pts[1] << 12) | (pts[2] << 8) | pts[3];
  return x;
}

void send_label_msg(LayoutState layout, int label, List<dynamic> m) {
  List<dynamic> hdr = [
    COMM_VERSION,
    MSG_UPDATE_LABEL,
    0,
    7779,
    label,
  ];

  var msgout = {
    'msg': hdr + m,
  };

  layout.sendMsg(msgout);
}
