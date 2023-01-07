
import 'package:flutter/material.dart';
import 'package:judotimer/show_comp.dart';
import 'package:judotimer/util.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'global.dart';
import 'layout.dart';
import 'stopwatch.dart';

Future<void> showPopupMenu(LayoutState layout) async {
  var t = AppLocalizations.of(layout.context);
  double left = 0.0; //offset.dx;
  double top = 20.0;
  double h = 32.0;
  await showMenu(
    context: layout.context,
    position: RelativeRect.fromLTRB(left, top, 0, 0),
    items: [
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.matchDurationAutomatic3c72 ?? ''), value: 'm0'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.matchDuration2MinShortPinTimes7d0d ?? ''),
          value: 'm1'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.matchDuration2Minf9d4 ?? ''), value: 'm2'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.matchDuration3Min05b8 ?? ''), value: 'm3'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.matchDuration4Min92cc ?? ''), value: 'm4'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.matchDuration5Mina174 ?? ''), value: 'm5'),
      PopupMenuItem<String>(
          height: h ,
          child: Text((t?.goldenScore7836 ?? '') + '...'), value: 'gs'),
      PopupMenuItem<String>(
          height: 12,
          child: const Divider(), value: 'x1'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.hanteiWhiteWins8376 ?? ''), value: 'h1'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.hanteiBlueWinse030 ?? ''), value: 'h2'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.hansokuMakeToWhite689e ?? ''), value: 'hm1'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.hansokuMakeToBlued834 ?? ''), value: 'hm2'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.hikiwake2c14 ?? ''), value: 'hw'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.clearSelection80ff ?? ''), value: 'cl'),
      PopupMenuItem<String>(
          height: 12,
          child: const Divider(), value: 'x2'),
      PopupMenuItem<String>(
          height: h,
          child: Text(t?.showCompetitors83b3 ?? ''), value: 'showcomp'),
      PopupMenuItem<String>(
          height: 12,
          child: const Divider(), value: 'x2'),
      CheckedPopupMenuItem<String>(
        height: h,
        checked: layout.setClocks,
        child: Text(t?.setClocks81f5 ?? ''), value: 'setclocks'),
    ],
    elevation: 8.0,
  ).then((value) async {
    //print('VALUE=$value');
    switch (value) {
      case 'showcomp':
        layout.display_comp_window(
            saved_cat,
            saved_last1,
            saved_last2,
            saved_first1,
            saved_first2,
            saved_country1,
            saved_country2,
            saved_round);
        /****
        layout.mainScreen = false;
        layout.node.unfocus();
        final result = await Navigator.push(
          layout.context,
          MaterialPageRoute(
              builder: (context) => ShowCompetitors(
                layout,
                  layout.widget.width,
                  layout.widget.height,
                  saved_cat,
                  saved_last1,
                  saved_last2,
                  saved_first1,
                  saved_first2,
                  saved_country1,
                  saved_country2,
                  '',
                  '',
                  saved_round)),
        );
        layout.mainScreen = true;
            ****/
        break;
      case 'm0':
        clock_key(layout, Keys.GDK_0, false);
        break;
      case 'm1':
        clock_key(layout, Keys.GDK_1, false);
        break;
      case 'm2':
        clock_key(layout, Keys.GDK_2, false);
        break;
      case 'm3':
        clock_key(layout, Keys.GDK_3, false);
        break;
      case 'm4':
        clock_key(layout, Keys.GDK_4, false);
        break;
      case 'm5':
        clock_key(layout, Keys.GDK_5, false);
        break;
      case 'gs':
        clock_key(layout, Keys.GDK_9, false);
        break;
      case 'h1':
        layout.voting_result(HANTEI_BLUE);
        break;
      case 'h2':
        layout.voting_result(HANTEI_WHITE);
        break;
      case 'hm1':
        layout.voting_result(HANSOKUMAKE_BLUE);
        break;
      case 'hm2':
        layout.voting_result(HANSOKUMAKE_WHITE);
        break;
      case 'hw':
        layout.voting_result(HIKIWAKE);
        break;
      case 'cl':
        layout.voting_result(CLEAR_SELECTION);
        break;
      case 'setclocks':
        layout.adjustClocks();
    }
  });
}

Future<void> ask_for_golden_score(LayoutState layout) async {
  var t = AppLocalizations.of(layout.context);
  double left = 0.0; //offset.dx;
  double top = 20.0;
  golden_score = false;
  gs_cat = gs_num = 0;

  await showMenu(
    context: layout.context,
    position: RelativeRect.fromLTRB(left, top, 0, 0),
    items: [
      PopupMenuItem<String>(
          child: Text(t?.cancelea47 ?? ''), value: 'cancel'),
      PopupMenuItem<String>(
          child: Text('Auto'), value: 'auto'),
      PopupMenuItem<String>(
          child: Text(t?.noLimitc9b0 ?? ''), value: 'nolimit'),
      PopupMenuItem<String>(
          child: const Text('1:00 min'), value: '100'),
      PopupMenuItem<String>(
          child: const Text('1:30 min'), value: '130'),
      PopupMenuItem<String>(
          child: const Text('2:00 min'), value: '200'),
      PopupMenuItem<String>(
          child: const Text('2:30 min'), value: '230'),
      PopupMenuItem<String>(
          child: const Text('3:00 min'), value: '300'),
      PopupMenuItem<String>(
          child: const Text('4:00 min'), value: '400'),
      PopupMenuItem<String>(
          child: const Text('5:00 min'), value: '500'),
    ],
    elevation: 8.0,
  ).then((value) async {
    print('VALUE=$value');
    if (value != 'cancel') {
      switch (value) {
        case 'nolimit':
          gs_time = 0;
          break;
        case '100':
          gs_time = 600;
          break;
        case '130':
          gs_time = 900;
          break;
        case '200':
          gs_time = 1200;
          break;
        case '230':
          gs_time = 1500;
          break;
        case '300':
          gs_time = 1800;
          break;
        case '400':
          gs_time = 2400;
          break;
        case '500':
          gs_time = 3000;
          break;
        default:
          gs_time = 100000;
          break;
      }
      last_shido_to = 0;
      golden_score = true;
      gs_cat = current_category;
      gs_num = current_match;
      if (value == 'auto')
        automatic = true;
      else
        automatic = false;

    }
    return golden_score;
  });
}

