import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:judotimer/global.dart';
import 'package:judotimer/stopwatch.dart';
import 'package:judotimer/util.dart';

import 'layout.dart';

class ShowWinner extends StatefulWidget {
  double width, height;
  LayoutState layout;
  String last = '', first = '', cat = '';

  //ShowWinner(this.layout, this.width, this.height, {Key? key}) : super(key: key);
  ShowWinner(
      this.layout, this.width, this.height, this.cat, this.last, this.first,
      {Key? key})
      : super(key: key);

  @override
  State<ShowWinner> createState() => _ShowWinnerState();
}

class _ShowWinnerState extends State<ShowWinner> {
  @override
  Widget build(BuildContext context) {
    final fheight = widget.height * 0.25;
    final btnheight = widget.height * 0.1;
    final boxheight = (widget.height - btnheight) / 3.0;
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

    return DefaultTextStyle(
        style: TextStyle(fontSize: fheight),
        child: Stack(children: [
          Positioned(
              left: 0,
              top: btnheight,
              width: widget.width,
              height: boxheight,
              child: ColoredBox(
                  color: Colors.black,
                  child: Row(children: <Widget>[
                    Expanded(
                        child: Text(widget.cat,
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
              width: widget.width,
              height: boxheight,
              child: ColoredBox(
                  color: Colors.white,
                  child: Text(widget.last,
                      style: TextStyle(
                        color: Color.fromARGB(255, 0, 0, 0),
                      )))),
          Positioned(
              left: 0,
              top: btnheight + boxheight * 2.0,
              width: widget.width,
              height: boxheight,
              child: ColoredBox(
                  color: Color.fromARGB(255, 0, 0, 255),
                  child: Text(widget.first,
                      style: TextStyle(
                        color: Colors.white,
                      )))),
          Positioned(
              top: 0,
              width: widget.width,
              height: btnheight,
              child: Focus(
                  autofocus: true,
                  onKey: (FocusNode node, RawKeyEvent event) {
                    if (event is RawKeyDownEvent &&
                        event.logicalKey.keyLabel == ' ') {
                      reset(widget.layout, Keys.ASK_OK, null);
                      Navigator.pop(context);
                    }
                    return KeyEventResult.handled;
                  },
                  child: Builder(builder: (BuildContext context) {
                    final FocusNode focusNode = Focus.of(context);
                    focusNode.requestFocus();
                    return ElevatedButton(
                      style: style,
                      onPressed: () {
                        reset(widget.layout, Keys.ASK_OK, null);
                        Navigator.pop(context);
                      },
                      child: const Text('OK'),
                    );
                  }))),
        ]));
  }
}
