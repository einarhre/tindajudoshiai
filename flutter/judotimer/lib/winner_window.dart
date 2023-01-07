import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:judolib/judolib.dart';
import 'package:judotimer/global.dart';
import 'package:judotimer/stopwatch.dart';
import 'package:judotimer/util.dart';
import 'package:flutter_svg/svg.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'layout.dart';

List<SvgTextSegment> winnerSvgSegments1 = [];
List<SvgTextSegment> winnerSvgSegments2 = [];

class ShowWinner extends StatelessWidget {
  double width, height;
  LayoutState layout;
  static String last = '', first = '', cat = '';
  static int winner = 0;

  //ShowWinner(this.layout, this.width, this.height, {Key? key}) : super(key: key);
  ShowWinner(
      this.layout, this.width, this.height,
      {Key? key})
      : super(key: key);

  static void setData(String cat, String last, String first, int winner) {
    ShowWinner.last = last;
    ShowWinner.first = first;
    ShowWinner.cat = cat;
    ShowWinner.winner = winner;
  }

  @override
  Widget build(BuildContext context) {
    final fheight = height * 0.25;
    final btnheight = height * 0.1;
    final boxheight = (height - btnheight) / 3.0;
    final ButtonStyle style = ElevatedButton.styleFrom(
      textStyle: const TextStyle(fontSize: 20),
      primary: Colors.black,
    );
    final winner = get_winner(true);
    var last_wname = '', first_wname = '';
    if (winner == BLUE) {
      last_wname = saved_last1;
      first_wname = saved_first1;
    } else if (winner == WHITE) {
      last_wname = saved_last2;
      first_wname = saved_first2;
    }

    List<Widget> getWidgets(String snapshot) {
      List<Widget> l = [
        Positioned(
          left: 0,
          top: btnheight,
          width: width,
          height: height - btnheight,
          child: SvgPicture.string(snapshot, fit: BoxFit.fill),
        ),
        Positioned(
            top: 0,
            width: width/2,
            height: btnheight,
            child: Focus(
                autofocus: true,
                onKey: (FocusNode node, RawKeyEvent event) {
                  if (event is RawKeyDownEvent &&
                      event.logicalKey.keyLabel == ' ') {
                    reset(layout, Keys.ASK_OK, null);
                    layout.displayMainScreen();
                  }
                  return KeyEventResult.handled;
                },
                child: Builder(builder: (BuildContext context) {
                  final FocusNode focusNode = Focus.of(context);
                  focusNode.requestFocus();
                  return ElevatedButton(
                    style: style,
                    onPressed: () {
                      reset(layout, Keys.ASK_OK, null);
                      layout.displayMainScreen();
                    },
                    child: const Text('OK'),
                  );
                }))),
        Positioned(
            top: 0,
            left: width/2,
            width: width/2,
            height: btnheight,
            child: Focus(
                autofocus: true,
                onKey: (FocusNode node, RawKeyEvent event) {
                  if (event is RawKeyDownEvent &&
                      event.logicalKey.keyLabel == ' ') {
                    reset(layout, Keys.ASK_OK, null);
                    layout.displayMainScreen();
                  }
                  return KeyEventResult.handled;
                },
                child: Builder(builder: (BuildContext context) {
                  final FocusNode focusNode = Focus.of(context);
                  focusNode.requestFocus();
                  return ElevatedButton(
                    style: style,
                    onPressed: () {
                      reset(layout, Keys.ASK_NOK, null);
                      layout.displayMainScreen();
                    },
                    child: const Icon(Icons.cancel),
                  );
                }))),

      ];
      return l;
    }

      return FutureBuilder<String>(
        future: getWinnerSvgText(context),
        builder: (context, AsyncSnapshot<String> snapshot) {
          if (!snapshot.hasData) {
            return CircularProgressIndicator();
          } else {
            return Stack(children: getWidgets(snapshot.data!));
          }
        });

  }
}

Future<String> getWinnerSvgText(context) async {
  String r = '';
  var segs = ShowWinner.winner == 1 ? winnerSvgSegments1 : winnerSvgSegments2;
  var t = AppLocalizations.of(context);

  for (var txt in segs) {
    var a = txt.args;
    if (a.length == 0) {
      r += txt.after;
      continue;
    }
    var code = a[0];
    if (code == 'C') {
      r += ShowWinner.cat;
    } else if (code == 'last') {
      r += ShowWinner.last;
    } else if (code == 'first') {
      r += ShowWinner.first;
    } else if (code == 'winner') {
      r += t?.wINNERb5a1 ?? 'WINNER';
    }
    r += txt.after;
  }
  return r;
}

Future<void> readWinnerSvgStrings() async {
  var svgstr = await rootBundle.loadString('assets/timer-winner-1.svg');
  var s = SvgParse(svgstr);
  winnerSvgSegments1 = s.varTexts;

  svgstr = await rootBundle.loadString('assets/timer-winner-2.svg');
  s = SvgParse(svgstr);
  winnerSvgSegments2 = s.varTexts;
}

/*******
    return DefaultTextStyle(
    style: TextStyle(fontSize: fheight),
    child: Stack(children: [
    Positioned(
    left: 0,
    top: btnheight,
    width: width,
    height: boxheight,
    child: ColoredBox(
    color: Colors.black,
    child: Row(children: <Widget>[
    Expanded(
    child: Text(cat,
    style: TextStyle(
    color: Colors.white,
    ))),
    Expanded(
    child: Text('WINNER',
    style: TextStyle(
    color: Colors.white,
    ))),
    ]))),
    Positioned(
    left: 0,
    top: btnheight + boxheight,
    width: width,
    height: boxheight,
    child: ColoredBox(
    color: Colors.white,
    child: Text(last,
    style: TextStyle(
    color: Color.fromARGB(255, 0, 0, 0),
    )))),
    Positioned(
    left: 0,
    top: btnheight + boxheight * 2.0,
    width: width,
    height: boxheight,
    child: ColoredBox(
    color: Color.fromARGB(255, 0, 0, 255),
    child: Text(first,
    style: TextStyle(
    color: Colors.white,
    )))),
    Positioned(
    top: 0,
    width: width,
    height: btnheight,
    child: Focus(
    autofocus: true,
    onKey: (FocusNode node, RawKeyEvent event) {
    if (event is RawKeyDownEvent &&
    event.logicalKey.keyLabel == ' ') {
    reset(layout, Keys.ASK_OK, null);
    layout.displayMainScreen();
    }
    return KeyEventResult.handled;
    },
    child: Builder(builder: (BuildContext context) {
    final FocusNode focusNode = Focus.of(context);
    focusNode.requestFocus();
    return ElevatedButton(
    style: style,
    onPressed: () {
    reset(layout, Keys.ASK_OK, null);
    layout.displayMainScreen();
    },
    child: const Text('OK'),
    );
    }))),
    ]));
 ******/
