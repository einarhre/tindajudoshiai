import 'dart:ui';

import 'package:flutter/material.dart';

class Label {
  static const comp1_name_1 = 0;
  static const comp2_name_1 = 1;
  static const comp1_name_2 = 2;
  static const comp2_name_2 = 3;
  static const comp1_club = 4;
  static const comp2_club = 5;
  static const match1 = 6;
  static const match2 = 7;
  static const wazaari = 8;
  static const yuko = 9;
  static const koka = 10;
  static const shido = 11;
  static const padding = 12;
  static const sonomama = 13;
  static const w1 = 14;
  static const y1 = 15;
  static const k1 = 16;
  static const s1 = 17;
  static const w2 = 18;
  static const y2 = 19;
  static const k2 = 20;
  static const s2 = 21;
  static const t_min = 22;
  static const colon = 23;
  static const t_tsec = 24;
  static const t_sec = 25;
  static const o_tsec = 26;
  static const o_sec = 27;
  static const points = 28;
  static const pts_to_comp1 = 29;
  static const pts_to_comp2 = 30;
  static const comment = 31;
  static const cat1 = 32;
  static const cat2 = 33;
  static const gs = 34;
  static const flag_comp1 = 35;
  static const flag_comp2 = 36;
  static const roundnum = 37;
  static const comp1_country = 38;
  static const comp2_country = 39;
  static const comp1_leg_grab = 40;
  static const comp2_leg_grab = 41;
  static const padding1 = 42;
  static const padding2 = 43;
  static const padding3 = 44;
  static const screen_bg_color = 100;
  static const clock_colors = 101;
  static const osaekomi_colors = 102;
  static const misc_settings = 103;
  static const window_layout = 104;
  static const bg_image = 105;
  static const option = 106;

  int num;
  double x, y;
  double w, h;
  String text = '';
  String text2 = '';
  double size;
  int xalign;
  Color fg = const Color(0xFFFFFFFF);
  Color bg = Colors.transparent;
  bool hide = false;

  Label(this.num, this.x, this.y, this.w, this.h, this.size, this.xalign,
      double fg_r, double fg_g, double fg_b,
      double bg_r, double bg_g, double bg_b) {
    fg = Color.fromARGB(255, (fg_r*255.0).toInt(), (fg_g*255.0).toInt(), (fg_b*255.0).toInt());
    if (bg_r >= 0)
      bg = Color.fromARGB(255, (bg_r*255.0).toInt(), (bg_g*255.0).toInt(), (bg_b*255.0).toInt());
    switch (num) {
      case match1: text = 'Match:';
      break;
      case match2: text = 'Next:';
      break;
      case wazaari: text = 'I';
      break;
      case yuko: text = 'W';
      break;
      case koka: text = 'Y';
      break;
      case shido: text = 'S';
      break;
      case gs: text = 'Golden Score';
      break;
    }
    if (num >= w1 && num <= o_sec) {
      if (num == colon)
        text = ':';
      else
        text = '0';
    }
  }

}
