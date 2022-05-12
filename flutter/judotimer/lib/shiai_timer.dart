import 'dart:async';
import 'dart:convert';
import 'dart:core';
import 'package:flutter/material.dart';
import 'package:format/format.dart';
import 'label.dart';


class ShiaiTimer extends StatefulWidget {
  late double width, height;
  late Label lbl1, lbl2, lbl3, lbl4;
  ShiaiTimer(double w, double h, Label l1, Label l2, Label l3, Label l4, {Key? key}) : super(key: key) {
    this.width = w;
    this.height = h;
    this.lbl1 = l1;
    this.lbl2 = l2;
    this.lbl3 = l3;
    this.lbl4 = l4;
  }

  @override
  State<ShiaiTimer> createState() => _ShiaiTimerState();
}

class _ShiaiTimerState extends State<ShiaiTimer> {
  late Timer _timer;
  int _start = 10;

  @override
  void initState() {
    super.initState();
    startTimer();
  }

  @override
  void dispose() {
    _timer.cancel();
    super.dispose();
  }

  void startTimer() {
    const oneSec = const Duration(seconds: 1);
    _timer = Timer.periodic(
      oneSec,
          (Timer timer) {
        if (_start == 0) {
          //setState(() {
          timer.cancel();
          print('TIMER IS ACTIVE: ${timer.isActive}');
          //});
          print('TIMER CANCELLED');
        } else {
          setState(() {
            _start--;
          });
          print('_start=$_start');
          //widget.lbl4.text = _start.toString();
        }
      },
    );
  }


  Positioned getWidget(Label a) {
    double w = widget.width;
    double h = widget.height;
    return Positioned(
        left: a.x * w,
        top: a.y * h,
        width: a.w * w,
        height: a.h * h,
        child: Text('X', style: TextStyle(
        fontSize: a.h * a.size * h,
        color: a.fg,
        backgroundColor: a.bg,
    )));
  }

  @override
  Widget build(BuildContext context) {
    return Text(format('X:Y{}', _start));
  }
}
