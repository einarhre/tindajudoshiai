import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:judotimer/layout.dart';
import 'package:judotimer/message.dart';
import 'package:judotimer/util.dart';
import 'package:judotimer/winner_window.dart';
import 'global.dart';
import 'layout.dart';
import 'menus.dart';

void update_display(LayoutState layout) {
  int t, min, sec, oSec;
  int orun, score1;

  if (elap == 0)
    t = total;
  else if (total == 0)
    t = elap;
  else {
    t = total - elap + 9;
    if (t == 0 && total > elap) t = 1;
  }

  t = (t / 10).toInt();
  min = (t / 60).toInt();
  sec = t - min * 60;
  oSec = (oElap / 10).toInt();

  if (t != last_m_time) {
    last_m_time = t;
    layout.set_timer_value(
        min, (sec / 10).toInt(), sec % 10, rest_time, rest_flags);
  }

  if (oSec != last_m_otime) {
    last_m_otime = oSec;
    layout.set_osaekomi_value((oSec / 10).toInt(), oSec % 10);
  }

  if (running != last_m_run) {
    last_m_run = running;
    layout.set_timer_run_color(last_m_run, rest_time);
  }

  if (oRunning) {
    //orun = oRunning ? OSAEKOMI_DSP_YES : OSAEKOMI_DSP_NO;
    if (score >= 2) {
      if (osaekomi_winner == BLUE)
        orun = OSAEKOMI_DSP_BLUE;
      else if (osaekomi_winner == WHITE)
        orun = OSAEKOMI_DSP_WHITE;
      else
        orun = OSAEKOMI_DSP_UNKNOWN;

      if (++flash > 5) orun = OSAEKOMI_DSP_YES2;
      if (flash > 10) flash = 0;
    } else {
      orun = OSAEKOMI_DSP_YES;
    }

    score1 = score;
  } else if (stackdepth > 0) {
    if (stackval[stackdepth - 1].who == BLUE)
      orun = OSAEKOMI_DSP_BLUE;
    else if (stackval[stackdepth - 1].who == WHITE)
      orun = OSAEKOMI_DSP_WHITE;
    else
      orun = OSAEKOMI_DSP_UNKNOWN;

    if (++flash > 5) orun = OSAEKOMI_DSP_YES2;
    if (flash > 10) flash = 0;

    score1 = stackval[stackdepth - 1].points;
  } else {
    orun = OSAEKOMI_DSP_NO;
    score1 = 0;
  }

  if (orun != last_m_orun) {
    last_m_orun = orun;
    layout.set_timer_osaekomi_color(last_m_orun, score1, oRunning);
  }

  if (last_score != score1) {
    last_score = score1;
    layout.setScore(last_score);
  }
}

void updateClock(LayoutState layout, int tick) {
  now = tick;

  if (running) {
    elap = tick - startTime;
    if (total > 0 && elap >= total) {
      //running = false;
      elap = total;
      if (!oRunning) {
        running = false;
        if (rest_time == false) show_soremade = true;
        if (rest_time) {
          elap = 0;
          oRunning = false;
          oElap = 0;
          rest_time = false;
        }
      }
    }
  }

  if (running && oRunning) {
    oElap = tick - oStartTime;
    score = 0;
    if (oElap >= wazaari) {
      score = 3;
      if (((osaekomi_winner == 1 && comp1pts[W] > 0) ||
          (osaekomi_winner == 2 && comp2pts[W] > 0))) {
        running = false;
        oRunning = false;
        //give_osaekomi_score();
        //approve_osaekomi_score(0);
      }
    }
    if (oElap >= ippon) {
      score = 4;
      running = false;
      oRunning = false;
      int tmp = osaekomi_winner;
      //give_osaekomi_score();
      if (tmp > 0) {
        //approve_osaekomi_score(0);
      }
    }
  }

  update_display(layout);
}

void toggle(LayoutState layout) {
  if (running) {
    running = false;
    elap = now - startTime;
    if (total > 0 && elap >= total) elap = total;
    if (oRunning) {
      //oRunning = FALSE;
      //oElap = 0;
      //give_osaekomi_score();
    } else {
      oElap = 0;
      score = 0;
    }
    update_display(layout);
  } else {
    if (total > 9000) // don't let clock run if dashes in display '-:--'
      return;

    startTime = now - elap;
    running = true;
    if (oRunning) {
      oStartTime = now - oElap;
      //set_comment_text("");
    }
    update_display(layout);
  }
}

void oToggle(LayoutState layout) {
  if (oRunning) {
    oRunning = false;
    oElap = 0;
    //set_comment_text("");
    update_display(layout);
    if (total > 0.0 && elap >= total) {
      //layout.beep("SOREMADE");
    }
  } else {
    if (!running) startTime = now - elap;
    running = true;
    //osaekomi = ' ';
    oRunning = true;
    oStartTime = now;
    update_display(layout);
  }
}

bool set_osaekomi_winner(LayoutState layout, int who) {
  if (oRunning == 0) {
    return approve_osaekomi_score(layout, who & CMASK);
  }

  if ((who & GIVE_POINTS) != 0) return false;

  osaekomi_winner = who;

  if (who == BLUE)
    layout.set_comment_text("Points going to blue");
  else if (who == WHITE)
    layout.set_comment_text("Points going to white");
  else
    layout.set_comment_text("");

  return TRUE;
}

bool approve_osaekomi_score(LayoutState layout, int who) {
  if (stackdepth == 0) return FALSE;

  stackdepth--;

  if (who == 0) who = stackval[stackdepth].who;

  switch (stackval[stackdepth].points) {
    case 1: // no koka
      break;
    case 2:
      if (who == BLUE)
        incdecpts(bluepts, Y, false);
      else if (who == WHITE) incdecpts(whitepts, Y, false);
      break;
    case 4:
      if (who == BLUE)
        incdecpts(bluepts, I, false);
      else if (who == WHITE) incdecpts(whitepts, I, false);
      break;
    case 3:
      if (who == BLUE)
        incdecpts(bluepts, W, false);
      else if (who == WHITE) incdecpts(whitepts, W, false);
      break;
  }

  check_ippon(layout);

  return TRUE;
}

void give_osaekomi_score() {
  if (score > 0) {
    stackval[stackdepth].who = osaekomi_winner;
    stackval[stackdepth].points = score;
    if (stackdepth < (STACK_DEPTH - 1)) stackdepth++;
  }

  score = 0;
  osaekomi_winner = 0;
}

void incdecpts(List<int> p, int s, bool decrement) {
  if (decrement) {
    big_displayed = FALSE;
    if (p[s] > 0) p[s]--;
  } else if (p[s] < 99) p[s]++;
}

const INC = false;
const DEC = true;
const SHIDOMAX = 3;

void check_ippon(LayoutState layout) {
  if (use_ger_u12_rules > 0) {
    if (bluepts[W] >= 2) {
      bluepts[I]++;
      bluepts[W] = 0;
    }
    if (whitepts[W] >= 2) {
      whitepts[I]++;
      whitepts[W] = 0;
    }
    if (bluepts[I] > 2) bluepts[I] = 2;
    if (whitepts[I] > 2) whitepts[I] = 2;
  } else {
    if (bluepts[I] > 1) bluepts[I] = 1;
    if (whitepts[I] > 1) whitepts[I] = 1;

    if (bluepts[W] >= 2) {
      bluepts[I] = 1;
      bluepts[W] = 0;
    }
    if (whitepts[W] >= 2) {
      whitepts[I] = 1;
      whitepts[W] = 0;
    }
  }

  layout.set_points(bluepts, whitepts);

  if (use_ger_u12_rules > 0) {
    int p1 = bluepts[I] * use_ger_u12_rules + bluepts[W] * 2 + bluepts[Y];
    int p2 = whitepts[I] * use_ger_u12_rules + whitepts[W] * 2 + whitepts[Y];
    if (p1 >= 2 * use_ger_u12_rules || p2 >= 2 * use_ger_u12_rules) {
      if (rules_stop_ippon_2 == TRUE) {
        if (oRunning) oToggle(layout);
        if (running) toggle(layout);
      }
      layout.beep("SOREMADE");
    }
  } else if (whitepts[I] > 0 || bluepts[I] > 0) {
    if (rules_stop_ippon_2 == TRUE) {
      if (oRunning) oToggle(layout);
      if (running) toggle(layout);
    }
    //voting_result(NULL, gint_to_ptr(CLEAR_SELECTION | NO_IPPON_CHECK));
    if (whitepts[S] >= SHIDOMAX || bluepts[S] >= SHIDOMAX)
      layout.beep("HANSOKUMAKE");
    else
      layout.beep("IPPON");

    var name = layout.get_name(whitepts[I] > 0 ? WHITE : BLUE);
    if (name == null || name.length == 0) name = whitepts[I] > 0 ? "white" : "blue";
  } else if (golden_score && get_winner(false) > 0) {
    layout.beep("Golden Score");
  } else if (whitepts[S] >= SHIDOMAX && bluepts[S] >= SHIDOMAX) {
    //voting_result(NULL, gint_to_ptr(HIKIWAKE | NO_IPPON_CHECK));
    layout.beep("HANSOKUMAKE");
  } else if ((whitepts[S] >= SHIDOMAX || bluepts[S] >= SHIDOMAX) &&
      rules_stop_ippon_2) {
    if (oRunning) oToggle(layout);
    if (running) toggle(layout);
    layout.beep("HANSOKUMAKE");
    String name = layout.get_name(whitepts[I] > 0 ? WHITE : BLUE);
    if (name == null || name[0] == 0) name = whitepts[I] > 0 ? "white" : "blue";
  }
}

int get_winner(bool final_result) {
  // final_result is true when result is sent to judoshiai. It is up to
  // the user to ensure result is correct i.e. shidos determine the winner correctly.
  int winner = 0;

  if (use_ger_u12_rules > 0) {
    int p1 = bluepts[I] * use_ger_u12_rules + bluepts[W] * 2 + bluepts[Y];
    int p2 = whitepts[I] * use_ger_u12_rules + whitepts[W] * 2 + whitepts[Y];
    if (p1 > p2)
      winner = BLUE;
    else if (p1 < p2) winner = WHITE;
  } else if (bluepts[I] > whitepts[I])
    winner = BLUE;
  else if (bluepts[I] < whitepts[I])
    winner = WHITE;
  else if (bluepts[W] > whitepts[W])
    winner = BLUE;
  else if (bluepts[W] < whitepts[W])
    winner = WHITE;
  else if (bluepts[Y] > whitepts[Y])
    winner = BLUE;
  else if (bluepts[Y] < whitepts[Y])
    winner = WHITE;
  else if (LayoutState.blue_wins_voting)
    winner = BLUE;
  else if (LayoutState.white_wins_voting)
    winner = WHITE;
  else if (LayoutState.hansokumake_to_white)
    winner = BLUE;
  else if (LayoutState.hansokumake_to_blue)
    winner = WHITE;
  else if (bluepts[S] > 0 || whitepts[S] > 0) {
    if (bluepts[S] >= 3 && whitepts[S] >= 3) return 0;
    if (bluepts[S] >= 3)
      winner = WHITE;
    else if (whitepts[S] >= 3) winner = BLUE;
  }

  return winner;
}

void clock_key(LayoutState layout, Keys key, bool shift) {
  bool ctl = false;
  if (key == Keys.GDK_c && ctl) {
    //manipulate_time(main_window, (gpointer)4 );
    return;
  }
  if (key == Keys.GDK_b && ctl) {
    //voting_result(NULL,(gpointer)HANTEI_WHITE);
    return;
  }
  if (key == Keys.GDK_w && ctl) {
    //voting_result(NULL,(gpointer)HANTEI_BLUE);
    return;
  }
  if (key == Keys.GDK_e && ctl) {
    //voting_result(NULL,(gpointer)CLEAR_SELECTION);
    return;
  }

  if (key == Keys.GDK_t) {
    /*
    display_comp_window(saved_cat, saved_last1, saved_last2,
        saved_first1, saved_first2, saved_country1, saved_country2,
        saved_club1, saved_club2, saved_round);

     */
    return;
  }

  if (key == Keys.GDK_z && ctl) {
    update_display(layout); //THESE LINES CAUSE PROBLEMS!
    check_ippon(layout);
    return;
  }

  action_on = true;

  switch (key) {
    case Keys.GDK_0:
      automatic = TRUE;
      reset(layout, Keys.GDK_7, null);
      break;
    case Keys.GDK_1:
    case Keys.GDK_2:
    case Keys.GDK_3:
    case Keys.GDK_4:
    case Keys.GDK_5:
    case Keys.GDK_6:
    case Keys.GDK_9:
      automatic = FALSE;
      reset(layout, key, null);
      break;
    case Keys.GDK_space:
      toggle(layout);
      break;
    case Keys.GDK_s:
    // sonomama removed
      break;
    case Keys.GDK_Up:
      set_osaekomi_winner(layout, BLUE);
      break;
    case Keys.GDK_Down:
      set_osaekomi_winner(layout, WHITE);
      break;
    case Keys.GDK_Return:
      oToggle(layout);
      if (!oRunning) {
        give_osaekomi_score();
      }
      break;
    case Keys.GDK_F5:
      if (whitepts[I] > 0 && !shift && use_ger_u12_rules == 0) break;
      incdecpts(whitepts, I, shift);
      check_ippon(layout);
      break;
    case Keys.GDK_F6:
      incdecpts(whitepts, W, shift);
      check_ippon(layout);
      break;
    case Keys.GDK_F7:
    case Keys.GDK_l:
      break;
    case Keys.GDK_F8:
      if (shift) {
        if (whitepts[S] >= SHIDOMAX) {
          bluepts[I] = 0;
        }
        incdecpts(whitepts, S, DEC);
        last_shido_to = 0;
      } else {
        incdecpts(whitepts, S, INC);

        if (whitepts[S] >= SHIDOMAX) {
          if (bluepts[S] >= SHIDOMAX) {
            bluepts[I] = 0;
            whitepts[I] = 0;
          } else {
            bluepts[I] = 1;
          }
        }
        last_shido_to = WHITE;
      }
      break;
    case Keys.GDK_F1:
      if (bluepts[I] > 0 && !shift && use_ger_u12_rules == 0) break;
      incdecpts(bluepts, I, shift);
      check_ippon(layout);
      break;
    case Keys.GDK_F2:
      incdecpts(bluepts, W, shift);
      check_ippon(layout);
      break;
    case Keys.GDK_F3:
    case Keys.GDK_k:
      break;
    case Keys.GDK_F4:
      if (shift) {
        if (bluepts[S] >= SHIDOMAX) {
          whitepts[I] = 0;
        }
        incdecpts(bluepts, S, DEC);
        last_shido_to = 0;
      } else {
        incdecpts(bluepts, S, INC);
        if (bluepts[S] >= SHIDOMAX) {
          if (whitepts[S] >= SHIDOMAX) {
            bluepts[I] = 0;
            whitepts[I] = 0;
          } else {
            whitepts[I] = 1;
          }
        }
        last_shido_to = BLUE;
      }
      break;
    default:
      ;
  }

  /* check for shido amount of points */
  if (bluepts[S] > SHIDOMAX) bluepts[S] = SHIDOMAX;

  if (whitepts[S] > SHIDOMAX) whitepts[S] = SHIDOMAX;

  check_ippon(layout);
}

Future<void> reset(LayoutState layout, Keys key, MsgNextMatch? msg0) async {
  bool asked = FALSE;

  if (key == Keys.ASK_OK) {
    key = key_pending;
    key_pending = Keys.None;
    asking = FALSE;
    asked = TRUE;
  } else if (key == Keys.ASK_NOK) {
    key_pending = Keys.None;
    asking = FALSE;
    return;
  }

  if ((running && rest_time == FALSE) || oRunning || asking)
    return;

  layout.set_gs_text("");

  if (key != Keys.GDK_0) {
    golden_score = FALSE;
  }

  int bp;
  int wp;
  bp = array2int(bluepts);
  wp = array2int(whitepts);

  bp &= ~0xf;
  wp &= ~0xf;

  //print('key=$key msg0=$msg0');

  if (key == Keys.GDK_9 ||
      (bp == wp && result_hikiwake == FALSE &&
          blue_wins_voting == white_wins_voting &&
          total > 0 && elap >= total && key != Keys.GDK_0)) {
    asking = TRUE;
    await ask_for_golden_score(layout);
    print('GS=$golden_score');
    asking = FALSE;
    if (key == Keys.GDK_9 && golden_score == FALSE)
      return;
    key = Keys.GDK_9;
    match_time = elap;
    layout.set_gs_text("GOLDEN SCORE");
  } else if (asked == FALSE &&
      ((bluepts[I] == 0 && whitepts[I] == 0 &&
          elap > 0 && elap < total) ||
          (running && rest_time) ||
          rules_confirm_match) &&
      key != Keys.GDK_0) {
    key_pending = key;
    asking = TRUE;

    final winner = get_winner(true);
    var last_wname = '', first_wname = '';
    if (winner == BLUE) {
      last_wname = saved_last1;
      first_wname = saved_first1;
    } else if (winner == WHITE) {
      last_wname = saved_last2;
      first_wname = saved_first2;
    }

    final result = Navigator.push(
        layout.context,
        MaterialPageRoute(builder: (context) =>
            ShowWinner(layout, layout.widget.width, layout.widget.height,
                saved_cat, last_wname, first_wname)));
    return;
  }

  /**
      if (key != Keys.GDK_0 && rest_time) {
      if ((rest_flags & MATCH_FLAG_BLUE_REST) != 0)
      cancel_rest_time(TRUE, FALSE);
      else if ((rest_flags & MATCH_FLAG_WHITE_REST) != 0)
      cancel_rest_time(FALSE, TRUE);
      }
   **/

  rest_time = FALSE;

  if (key != Keys.GDK_0 && golden_score == FALSE) {
    last_reported_category = current_category;
    last_reported_number = current_match;

    // set bit if golden score
    if (gs_cat == current_category && gs_num == current_match) legend |= 0x100;

    send_result(layout, bluepts, whitepts, blue_wins_voting, white_wins_voting,
        hansokumake_to_blue, hansokumake_to_white, legend, result_hikiwake);
    match_time = 0;
    gs_cat = gs_num = 0;
  }

  if (golden_score == FALSE) {
    bluepts = [0, 0, 0, 0, 0];
    whitepts = [0, 0, 0, 0, 0];
  }

  blue_wins_voting = false;
  white_wins_voting = false;
  hansokumake_to_blue = false;
  hansokumake_to_white = false;
  if (key != Keys.GDK_0) result_hikiwake = false;
  osaekomi_winner = 0;
  big_displayed = FALSE;

  running = FALSE;
  elap = 0;
  oRunning = FALSE;
  oElap = 0;

  //memset(&(stackval), 0, sizeof(stackval));
  stackdepth = 0;

  if (use_2017_rules || use_2018_rules) {
    koka = 0;
    yuko = 0;
    wazaari = 100;
    ippon = 200;
  } else {
    koka = 0;
    yuko = 100;
    wazaari = 150;
    ippon = 200;
  }

  switch (key) {
    case Keys.GDK_0:
      if (msg0 != null) {
        /*print("is-gs=$golden_score match=${msg0.match_time} gs=${msg0
            .gs_time} rep=${msg0.rep_time} flags=${msg0.flags} rest=${msg0
            .rest_time}");*/
        if ((msg0.flags & MATCH_FLAG_REPECHAGE) > 0 && msg0.rep_time > 0) {
          total = msg0.rep_time*10;
          golden_score = TRUE;
        } else if (golden_score)
          total = msg0.gs_time*10;
        else
          total = msg0.match_time*10;

        koka = msg0.pin_time_koka*10;
        yuko = msg0.pin_time_yuko*10;
        wazaari = msg0.pin_time_wazaari*10;
        ippon = msg0.pin_time_ippon*10;

        jcontrol = FALSE;
        if (require_judogi_ok &&
            ((msg0.flags & MATCH_FLAG_JUDOGI1_OK) == 0 ||
                (msg0.flags & MATCH_FLAG_JUDOGI2_OK) == 0)) {
          final name = (msg0.flags & MATCH_FLAG_JUDOGI1_OK) == 0
              ? layout.get_name(BLUE)
              : layout.get_name(WHITE);
          layout.display_big('CONTROL ${name}', 2);
          jcontrol = TRUE;
          jcontrol_flags = msg0.flags;
        } else if (msg0.rest_time > 0) {
          final name = (msg0.flags & MATCH_FLAG_JUDOGI1_OK) == 0
              ? layout.get_name(BLUE)
              : layout.get_name(WHITE);
          rest_time = TRUE;
          rest_flags = msg0.minutes;
          total = msg0.rest_time*10;
          startTime = now;
          running = TRUE;
          layout.display_big('REST TIME ${name}', msg0.rest_time);
          return;
        }
      }
      break;
    case Keys.GDK_1:
      total = golden_score ? gs_time : 1200;
      koka = 0;
      if (use_2017_rules || use_2018_rules) {
        yuko = 0;
        wazaari = 100;
      } else {
        yuko = 50;
        wazaari = 100;
      }
      ippon = 150;
      break;
    case Keys.GDK_2:
      total = golden_score ? gs_time : 1200;
      break;
    case Keys.GDK_3:
      total = golden_score ? gs_time : 1800;
      break;
    case Keys.GDK_4:
      total = golden_score ? gs_time : 2400;
      break;
    case Keys.GDK_5:
      total = golden_score ? gs_time : 3000;
      break;
    case Keys.GDK_6:
      total = golden_score ? gs_time : 1500;
      break;
    case Keys.GDK_7:
      total = 100000;
      break;
    case Keys.GDK_9:
      if (golden_score)
        total = gs_time;
      break;
  }

  action_on = false;

  if (key != Keys.GDK_0) {
    layout.set_comment_text("");
    layout.delete_big();
  }

  layout.reset_display(key);
  update_display(layout);

  if (key != Keys.GDK_0) {
    //display_ad_window();
  }
}

void send_result(LayoutState layout, List<int> bluepts, List<int> whitepts,
    bool blue_vote, bool white_vote,
    bool blue_hansokumake, bool white_hansokumake, int legend,
    bool hikiwake) {
  var msgout = {'msg': [COMM_VERSION, MSG_RESULT, 0, 7777,
    tatami, current_category, current_match, get_match_time(),
    array2int(bluepts), array2int(whitepts),
    hikiwake ? 1 : (blue_vote ? 1 : 0), hikiwake ? 1 : (white_vote ? 1 : 0),
    blue_hansokumake ? 1 : 0, white_hansokumake ? 1 : 0, legend]};

  layout.sendMsg(msgout);
}

bool clock_running() {
  return action_on;
}

int get_match_time() {
  return ((elap + match_time)/10).toInt();
}

void set_hantei_winner(LayoutState layout, int cmd) {
  if (cmd == CLEAR_SELECTION)
    big_displayed = FALSE;

  if (cmd == HANSOKUMAKE_BLUE)
    whitepts[I] = 1;
  else if (cmd == HANSOKUMAKE_WHITE)
    bluepts[I] = 1;

  check_ippon(layout);
}
