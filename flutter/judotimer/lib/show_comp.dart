import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class ShowCompetitors extends StatefulWidget {
  String cat, comp1, comp2, first1, first2, country1, country2, club1, club2;
  int round;
  double width, height;

  ShowCompetitors(
      this.width,
      this.height,
      this.cat,
      this.comp1,
      this.comp2,
      this.first1,
      this.first2,
      this.country1,
      this.country2,
      this.club1,
      this.club2,
      this.round,
      {Key? key})
      : super(key: key);

  @override
  State<ShowCompetitors> createState() => _ShowCompetitorsState();
}

class _ShowCompetitorsState extends State<ShowCompetitors> {
  @override
  Widget build(BuildContext context) {
    final fheight = widget.height * 0.25;
    final btnheight = widget.height * 0.05;
    final boxheight = (widget.height - btnheight) / 3.0;
    final ButtonStyle style = ElevatedButton.styleFrom(
      textStyle: const TextStyle(fontSize: 20),
      primary: Colors.grey,
    );

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
                  child: Text('${widget.cat}',
                      style: TextStyle(
                        color: Colors.white,
                      )))),
          Positioned(
              left: 0,
              top: btnheight + boxheight,
              width: widget.width,
              height: boxheight,
              child: ColoredBox(
                  color: Colors.white,
                  child: Text('${widget.comp1}',
                      style: TextStyle(
                        color: Colors.black,
                      )))),
          Positioned(
              left: 0,
              top: btnheight + boxheight * 2.0,
              width: widget.width,
              height: boxheight,
              child: ColoredBox(
                  color: Colors.blue,
                  child: Text('${widget.comp2}',
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
                        Navigator.pop(context);
                      },
                      child: const Text('OK'),
                    );
                  }))),
        ]));
  }
}
