import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:http/http.dart' as http;
import 'package:judotimer/label.dart';
import 'package:judotimer/layout.dart';
import 'package:judotimer/stopwatch.dart';
import 'package:judotimer/winner_window.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

import 'global.dart';
//import 'web_specific.dart';
import 'linux_specific.dart';
import 'main.dart';
import 'message.dart';

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

Future<String> getHostName() async {
  return await getHostName1();
}


Future<List<Label>> getTimerCustom() async {
  var host = await getHostName();
  var lbs = <Label>[];

  try {
    /***
    var response = await http.get(
      Uri.parse('http://$host:8088/timer-custom.txt'),
    );
        ***/
    var response = await rootBundle.loadString('assets/timer-custom.txt');
    if (response.length > 0) {
      LineSplitter ls = new LineSplitter();
      List<String> lines = ls.convert(response);
      for (var l in lines) {
        if (l.length < 1 || l.codeUnitAt(0) < '0'.codeUnitAt(0) ||
            l.codeUnitAt(0) > '9'.codeUnitAt(0))
          continue;
        final pattern = RegExp('\\s+');
        final l2 = l.replaceAll(pattern, ' ');
        var a = l2.split(' ');
        var num = int.parse(a[0]);
        if (num < 100) {
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
        }
      }
    }
  } catch (e) {
    print("HTTP get timer_custom.txt error $e");
    rethrow;
  }
  return lbs;
}

void handle_message(LayoutState layout, Message msg) {
  //print('MSG TYPE=${msg.type}');
  if (msg.type == MSG_NEXT_MATCH) {
    MsgNextMatch nm = MsgNextMatch(msg.message);
    if (nm.tatami != tatami) return;
    if (clock_running()) return;

    layout.show_message(
        nm.cat_1,
        nm.blue_1,
        nm.white_1,
        nm.cat_2,
        nm.blue_2,
        nm.white_2,
        nm.flags,
        nm.round);
    if (nm.minutes > 0 && automatic)
      reset(layout, Keys.GDK_0, nm);
    if (golden_score)
      layout.set_comment_text("Golden Score");
    if (current_category != nm.category ||
        current_match != nm.match) {
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
        layout.display_comp_window(m.cat_1, m.blue_1, m.white_1, '', '', '', '', m.round);
        break;
      case STOP_COMPETITORS:
        print('STOP COMP');
        StopCompetitors m = ml.stop_competitors;
        Navigator.pop(layout.context);
        break;
      case START_WINNER:
        print('START WINNER rules_confirm_match=$rules_confirm_match');
        StartWinner m = ml.start_winner;
        if (rules_confirm_match)
          Navigator.push(layout.context,
          MaterialPageRoute(builder: (context) {
              return ShowWinner(layout, layout.widget.width, layout.widget.height, m.cat, m.last, m.first);
          }));
        break;
      case STOP_WINNER:
        print('STOP WINNER');
        //Navigator.of(layout.context).pushAndRemoveUntil(MaterialPageRoute(builder: (context) =>
        //    MyHomePage(title: 'JudoTimer')), (Route<dynamic> route) => false);
        if (Navigator.canPop(layout.context))
          Navigator.pop(layout.context);
        break;
      case SAVED_LAST_NAMES:
        SavedLastNames m = ml.saved_last_names;
        saved_last1 = m.last1;
        saved_last2 = m.last2;
        saved_cat = m.cat;
        break;
      case SHOW_MSG:
        ShowMsg m = ml.show_msg;
        layout.show_message(m.cat_1, m.blue_1, m.white_1, m.cat_2, m.blue_2, m.white_2, m.flags, m.round);
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
        print('OKOMI COLOR ${m.osaekomi_state} ${m.pts} ${m.orun}');
        layout.set_timer_osaekomi_color(m.osaekomi_state, m.pts, m.orun);
        break;
      case SET_TIMER_RUN_COLOR:
        SetTimerRunColor m = ml.set_timer_run_color;
        layout.set_timer_run_color(m.running, m.resttime);
        break;
    }
  }
}

String get_name_by_layout(String first, String last, String club,
    String country) {
  switch (selected_name_layout) {
    case 0:
      if (country == null || country[0] == 0)
        return '$first $last, $club';
      else if (club == null || club[0] == 0)
        return '$first $last, $country';
      return '$first $last, $country/$club';
    case 1:
      if (country == null || country[0] == 0)
        return '$last, $first, $club';
      else if (club == null || club[0] == 0)
        return '$last, $first, $country';
      return '$last, $first, $country/$club';
    case 2:
      if (country == null || country[0] == 0)
        return '$club, $last, $first';
      else if (club == null || club[0] == 0)
        return '$country $last, $first';
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

String round_to_str(int round) {
  if (round == 0)
    return "";

  switch (round & ROUND_TYPE_MASK) {
    case 0:
      return 'Round ${round & ROUND_MASK}';
    case ROUND_ROBIN:
      return "Round Robin";
    case ROUND_REPECHAGE:
      return "Repechage";
    case ROUND_SEMIFINAL:
      if ((round & ROUND_UP_DOWN_MASK) == ROUND_UPPER)
        return 'Semifinal 1';
      else if ((round & ROUND_UP_DOWN_MASK) == ROUND_LOWER)
        return 'Semifinal 2';
      else
        return 'Semifinal';
    case ROUND_BRONZE:
      if ((round & ROUND_UP_DOWN_MASK) == ROUND_UPPER)
        return 'Bronze 1';
      else if ((round & ROUND_UP_DOWN_MASK) == ROUND_LOWER)
        return 'Bronze 2';
      else
        return 'Bronze';
    case ROUND_SILVER:
      return "Silver medal match";
    case ROUND_FINAL:
      return "Final";
  }
  return "";
}

int array2int(List<int> pts) {
  int x = 0;
  x = (pts[0] << 16) | (pts[1] << 12) | (pts[2] << 8) | pts[3];
  return x;
}

const languageCodeToLanguage = {"fi": "Suomi", "sv": "Svensk", "en": "English", "es": "Español", "et": "Eesti",
  "uk": "Українська", "is": "Íslenska", "nb": "Norsk", "pl": "Polski", "sk": "Slovenčina",
  "nl": "Nederlands", "cs": "Čeština", "de": "Deutsch", "da": "Dansk", "he": "עברית", "fr": "Français", "fa": "فارسی"};

const languageCodeToIOC = {"fi": "FIN", "sv": "SWE", "en": "GBR", "es": "ESP", "et": "EST",
  "uk": "UKR", "is": "ISL", "nb": "NOR", "pl": "POL", "sk": "SLO",
  "nl": "NED", "cs": "CZE", "de": "GER", "da": "DEN", "he": "ISR", "fr": "FRA", "fa": "IRI"};

