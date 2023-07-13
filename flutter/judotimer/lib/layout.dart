import 'dart:async';
import 'dart:convert';
import 'dart:core';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:judolib/judolib.dart';
import 'package:judotimer/settings.dart';
import 'package:judotimer/shiai_timer.dart';
import 'package:judotimer/show_comp.dart';
import 'package:judotimer/stopwatch.dart';
import 'package:judotimer/util.dart';
import 'package:judotimer/websocket.dart';
import 'package:judotimer/winner_window.dart';
import 'package:audioplayers/audioplayers.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

import 'global.dart';
import 'label.dart';
import 'lang.dart';
import 'main.dart';
import 'menus.dart';

class Layout extends StatefulWidget {
  final double width, height;
  final List<Label> labels;

  Layout(this.width, this.height, this.labels);

  @override
  State<Layout> createState() => LayoutState();
}

class LayoutState extends State<Layout> {
  late Timer timer, wstimer;
  static var bgcolor = Colors.blue,
      bgcolor_pts = Colors.black,
      bgcolor_points = Colors.black;
  static var current_osaekomi_state = 0;
  static const List<String> num2str = [
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "+"
  ];
  static const List<String> pts2str = ["-", "K", "Y", "W", "I", "S"];
  static bool blue_wins_voting = false, white_wins_voting = false;
  static bool hansokumake_to_blue = FALSE, hansokumake_to_white = FALSE;
  late AudioPlayer player;
  String soundFile = 'AirHorn';
  bool playing = false;
  int playnum = 0;
  bool playsound = false;
  var websockJs = null;
  var websockBroker;
  String big_text = '';
  double toolbarheight = no_frames ? 0 : 40;
  bool shifted = false;
  late FocusNode node;
  bool mainScreen = true;
  late FocusAttachment _nodeAttachment;
  bool showCompetitors = false;
  bool showWinner = false;
  bool _brokerTrafficDetected = false;
  bool setClocks = false;
  var oldPw = jspassword;

  String getUrl() {
    if (mode_slave) return 'ws://${master_name}:${WEBSOCK_PORT + 1}';
    if (kIsWeb) {
      final n = getLocation();
      return 'ws://${n}:$WS_COMM_PORT/tm${tatami}_pw_$jspassword';
    }
    String s = getSsdpAddress('JudoShiai');
    String n = s != '' ? s : node_name;
    return 'ws://${n}:$WS_COMM_PORT/tm${tatami}_pw_$jspassword';
  }

  String getBrokerUrl() {
    String s = getSsdpAddress('JudoShiai');
    String n = s != '' ? s : node_name;
    if (mode_slave) return 'ws://${n}:2317/ts$tatami';
    return 'ws://${n}:$BROKER_PORT/tm$tatami';
  }

  @override
  void initState() {
    print('*** INIT ***');
    //kIsWeb
    super.initState();
    //readSettings();
    initPlayer();
    readCompetitorsSvgString();
    readWinnerSvgStrings();

    const dly = const Duration(seconds: 2);
    wstimer = Timer.periodic(
      dly,
      (Timer timer) {
        if (websockJs == null && !_brokerTrafficDetected) {
          print('WS CONNECT ${getUrl()}');
          websockJs = WebSocketChannel.connect(Uri.parse(getUrl()),
              protocols: ['js']);
          websockJs.stream.listen((event) {
            //print('MSG=$event type=${event.runtimeType}');
            var json = jsonDecode(event);
            var jsonmsg = json['msg'];
            Message msg = Message(jsonmsg);
            //mylog('layout', 160, 'TIMER MSG');
            handle_message(this, msg);
          }, onError: (error) {
            print('websockjs error $error');
            if (websockJs != null) websockJs.sink.close();
            websockJs = null;
          }, onDone: () {
            print('websockjs done');
            if (websockJs != null) websockJs.sink.close();
            websockJs = null;
          });

          if (websockJs != null) {
            var msgout = {
              'msg': [
                COMM_VERSION,
                MSG_ALL_REQ,
                0,
                7777
              ]
            };
            sendMsgToJs(msgout);
          }
        } else if (websockJs != null) {
          if (oldPw != jspassword) {
            print('websockjs password change');
            oldPw = jspassword;
            websockJs.sink.close();
            websockJs = null;
          }
        }

        if (websockBroker == null) {
          //print('WS BROKER CONNECT to ${getBrokerUrl()}');
          websockBroker = WebSocketChannel.connect(Uri.parse(getBrokerUrl()),
              protocols: ['broker']);
          websockBroker.stream.listen((event) {
            _brokerTrafficDetected = true;
            var json = jsonDecode(event);
            var jsonmsg = json['msg'];
            Message msg = Message(jsonmsg);
            //mylog('layout', 160, 'BROKER MSG');
            handle_message(this, msg);
          }, onError: (error) {
            print('BROKER ERROR $error');
            if (websockBroker != null) websockBroker.sink.close();
            websockBroker = null;
          }, onDone: () {
            print('BROKER DONE');
            if (websockBroker != null) websockBroker.sink.close();
            websockBroker = null;
          });
        }
      },
    );

    startTimer();

    node = FocusNode(debugLabel: 'Button');
    node.addListener(_handleFocusChange);
    _nodeAttachment = node.attach(context, onKey: _handleKeyPress);

    ssdp('JudoShiai');

  }

  @override
  void dispose() {
    if (websockJs != null) websockJs.sink.close();
    if (websockBroker != null) websockBroker.sink.close();
    node.removeListener(_handleFocusChange);
    node.dispose();
    wstimer.cancel();
    timer.cancel();
    player.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    _nodeAttachment.reparent();
    List<Widget> w = getLabels();

    return Scaffold(
        appBar: AppBar(
          toolbarHeight: toolbarheight,
          backgroundColor: Colors.black,
          title: Text(toolbarheight > 0 ? 'JudoTimer' : ''),
          leading: Builder(
            builder: (context) => ElevatedButton(
              child: Icon(Icons.menu),
              onPressed: () {
                showPopupMenu(this);
              },
            ),
          ),
          actions: [
            Builder(
              builder: (context) {
                return DropdownButton(
                  dropdownColor: Colors.grey,
                  onChanged: (v) => setState(() {
                    languageCode = v.toString();
                    MyApp.setLocale(context, Locale(v.toString(), ""));
                  }),
                  value: languageCode,
                  items: languageCodes
                      .map<DropdownMenuItem<String>>((String value) {
                    return DropdownMenuItem<String>(
                      value: value,
                      child: Row(children: [
                        Padding(
                            padding: EdgeInsets.all(8.0),
                            child: Image.asset(
                                'packages/judolib/assets/flags-ioc/${languageCodeToIOC[value]}.png')),
                        SizedBox(
                          width: 10,
                        ),
                        Text(languageCodeToLanguage[value] ?? '',
                            style: TextStyle(color: Colors.white)),
                      ]),
                    );
                  }).toList(),
                );
              },
            ),
            Builder(
              builder: (context) => ElevatedButton(
                child: Icon(Icons.settings),
                onPressed: () async {
                  node.unfocus();
                  mainScreen = false;
                  node.unfocus();
                  final old_slave = mode_slave;
                  // Navigate to second route when tapped.
                  final result = await Navigator.push(
                    context,
                    MaterialPageRoute(
                        builder: (context) => SettingsScreen(this)),
                  );
                  mainScreen = true;
                  //print('SLAVE: $old_slave -> $mode_slave');
                  if (old_slave != mode_slave) {
                    websockJs.sink.close();
                    websockJs = null;
                    websockBroker.sink.close();
                    websockBroker = null;
                  }
                  setState(() {});
                },
              ),
            ),
          ],
        ),
        body: Builder(builder: (BuildContext context) {
          //final FocusNode focusNode = Focus.of(context);
          //focusNode.requestFocus();
          if (mainScreen) node.requestFocus();
          return Stack(
            children: w,
          );
        }));
  }

  void _handleFocusChange() {
    //print('FOCUS CHANGE: ${node.hasFocus}');
    if (node.hasFocus != mainScreen) {
      //setState(() {
      //_focused = _node.hasFocus;
      //});
    }
  }

  KeyEventResult _handleKeyPress(FocusNode node, RawKeyEvent event) {
    //if (!mainScreen) return KeyEventResult.ignored;
    shifted = event.isShiftPressed;
    if (event is RawKeyDownEvent) {
      //print('KEY=${event.logicalKey}');
      if (showCompetitors || showWinner) {
        displayMainScreen();
      } else {
        String k = event.logicalKey.keyLabel;
        final Keys? k1 = char2key[k];
        if (k1 != null) clock_key(this, k1, event.isShiftPressed);
      }
    }
    return KeyEventResult.handled;
  }

  Future<void> initPlayer() async {
    player = AudioPlayer();
    /*
    player.playbackEventStream.listen((event) {},
        onError: (Object e, StackTrace stackTrace) {
      print('A stream error occurred: $e');
    });

     */
    /*
    String audioasset = "assets/audio/$soundFile.mp3";
    ByteData bytes = await rootBundle.load(audioasset); //load sound from assets
    Uint8List soundbytes = bytes.buffer.asUint8List(bytes.offsetInBytes, bytes.lengthInBytes);
    final MemoryFileSystem fileSystem = MemoryFileSystem();
    File dataFile = fileSystem.file("alarm.mp3");
    await dataFile.writeAsBytes(soundbytes, flush: true);
    int len = await dataFile.length();
    print('MEM FILE: ${dataFile.absolute} len=${len} ${rootBundle.toString()}');

     */
  }

  void sendMsg(msgout) {
    //print('SEND $msgout');
    final s = jsonEncode(msgout);
    if (websockBroker != null) websockBroker.sink.add(s);
  }

  void sendMsgToJs(msgout) {
    //print('SEND TO JS $msgout');
    final s = jsonEncode(msgout);
    if (websockJs != null) websockJs.sink.add(s);
  }

  void set_timer_value(
      int min, int tsec, int sec, bool rest_time, int rest_flags) {
    //print('SET TIMER $min:$tsec$sec');
    if (!mode_slave)
      send_label_msg(this, SET_TIMER_VALUE,
          [min, tsec, sec, rest_time ? 1 : 0, rest_flags]);

    if (min < 60) {
      widget.labels[Label.t_min].text = min.toString();
      widget.labels[Label.t_tsec].text = tsec.toString();
      widget.labels[Label.t_sec].text = sec.toString();
    } else {
      widget.labels[Label.t_min].text = '-';
      widget.labels[Label.t_tsec].text = '-';
      widget.labels[Label.t_sec].text = '-';
    }
    setState(() {});
  }

  void set_timer_osaekomi_color(int osaekomi_state, int pts, bool orun) {
    Color fg = Colors.white, bg = Colors.black, color = Colors.green;

    if (!mode_slave)
      send_label_msg(
          this, SET_TIMER_OSAEKOMI_COLOR, [osaekomi_state, pts, orun ? 1 : 0]);

    current_orun = orun;

    widget.labels[Label.w1].hide = false;
    widget.labels[Label.y1].hide = false;
    widget.labels[Label.k1].hide = false;
    widget.labels[Label.w2].hide = false;
    widget.labels[Label.y2].hide = false;
    widget.labels[Label.k2].hide = false;

    //print('OCOLOR: state=$osaekomi_state pts=$pts orun=$orun');

    if (pts > 0) {
      if (osaekomi_state == OSAEKOMI_DSP_BLUE ||
          osaekomi_state == OSAEKOMI_DSP_WHITE ||
          osaekomi_state == OSAEKOMI_DSP_UNKNOWN ||
          osaekomi_state == OSAEKOMI_DSP_YES2) {
        var b = Label.k1, w = Label.k2;
        var sb, sw;

        switch (pts) {
          case 2:
            b = Label.k1;
            w = Label.k2;
            break;
          case 3:
            b = Label.y1;
            w = Label.y2;
            break;
          case 4:
            b = Label.w1;
            w = Label.w2;
            break;
        }

        //sb = widget.labels[b].size;
        //sw = widget.labels[w].size;

        widget.labels[b].hide = (osaekomi_state == OSAEKOMI_DSP_BLUE ||
            osaekomi_state == OSAEKOMI_DSP_UNKNOWN);
        widget.labels[w].hide = (osaekomi_state == OSAEKOMI_DSP_WHITE ||
            osaekomi_state == OSAEKOMI_DSP_UNKNOWN);

        //widget.labels[b].size = sb;
        //widget.labels[w].size = sw;
      }
    }

    if (orun) {
      fg = Colors.green;
      bg = bgcolor_points;
    } else {
      switch (osaekomi_state) {
        case OSAEKOMI_DSP_NO:
        case OSAEKOMI_DSP_YES2:
          fg = Colors.grey;
          bg = bgcolor_points;
          break;
        case OSAEKOMI_DSP_YES:
          fg = Colors.green;
          bg = bgcolor_points;
          break;
        case OSAEKOMI_DSP_BLUE:
          fg = Colors.black;
          bg = Colors.white;
          break;
        case OSAEKOMI_DSP_WHITE:
          fg = Colors.white;
          bg = bgcolor;
          break;
        case OSAEKOMI_DSP_UNKNOWN:
          fg = Colors.white;
          bg = bgcolor_points;
          break;
      }
    }

    widget.labels[Label.points].fg = fg;
    widget.labels[Label.points].bg = bg;

    var pts1, pts2;
    pts1 = Label.pts_to_comp2;
    pts2 = Label.pts_to_comp1;

    current_osaekomi_state = osaekomi_state;

    if (orun) {
      color = oclock_run;
      widget.labels[pts1].fg = Colors.white;
      widget.labels[pts1].bg = bgcolor;
      widget.labels[pts2].fg = Colors.black;
      widget.labels[pts2].bg = Colors.white;
    } else {
      color = oclock_stop;
      widget.labels[pts1].fg = Colors.grey;
      widget.labels[pts1].bg = bgcolor_pts;
      widget.labels[pts2].fg = Colors.grey;
      widget.labels[pts2].bg = bgcolor_pts;
    }

    widget.labels[Label.o_tsec].fg = color;
    widget.labels[Label.o_sec].fg = color;

    setState(() {});
  }

  String ptsToStr(int num) {
    if (num < 6) return pts2str[num];
    return ".";
  }

  void setScore(int score) {
    if (!mode_slave) send_label_msg(this, SET_SCORE, [score]);

    //print('score: $score');
    widget.labels[Label.points].text = ptsToStr(score);
    setState(() {});
  }

  List<Widget> getLabels() {
    double w = widget.width;
    double h = widget.height;
    if (window_layout_w <= 0 || window_layout_h <= 0)
      h -= toolbarheight;

    List<Widget> l = [];
    int i;
    int wlen = widget.labels.length;

    if (background_image != null) {
      l.add(background_image!);
    }

    for (i = 0; i < wlen; i++) {
      var a = widget.labels[i];
      if (a.num < Label.padding1 || a.num > Label.padding3)
        continue;
      var wdg = Positioned(
          left: a.x * w,
          top: a.y * h,
          width: a.w * w,
          height: a.h * h,
          child: ColoredBox(
              color: a.bg,
              child: SizedBox(
                width: a.w * w,
                height: a.h * h,
              )));
      l.add(wdg);
    }

    for (i = 0; i < wlen; i++) {
      //for (i = 0; i < Label.padding1; i++) {
      var a = widget.labels[i];
      if (a.num >= Label.padding1 && a.num <= Label.padding3)
        continue;
      if (a.w < 0.01 || a.h < 0.01) continue;
      bool hd = a.hide || (hide_clock_if_osaekomi && current_orun && i >= Label.t_min && i <= Label.t_sec);
      hd = hd || (hide_clock_if_osaekomi && !current_orun && i >= Label.o_tsec && i <= Label.o_sec);
      var wdg = Positioned(
          left: a.x * w,
          top: a.y * h,
          width: a.w * w,
          height: a.h * h,
          child: GestureDetector(
              onTap: () {
                click(a.num, shifted);
              },
              onLongPress: () {
                click(a.num, true);
              },
              onVerticalDragEnd: (DragEndDetails details) {
                if (details.primaryVelocity! > 0) {
                  print('SWIPE DOWN');
                } else if (details.primaryVelocity! < 0) {
                  print('SWIPE UP');
                }
              },
              child: ColoredBox(
                  color: a.bg,
                  child: (i == Label.flag_comp1 || i == Label.flag_comp2)
                    ? ((a.text != null && a.text.length == 3)
                        ? Image(
                            image:
                            AssetImage('packages/judolib/assets/flags-ioc/${a.text}.png'),
                            fit: BoxFit.fitHeight,
                            alignment: Alignment.topLeft,
                          )
                        : Text(''))
                    : (show_shido_cards && (i == Label.s1 || i == Label.s2))
                    ? Image(
                        image: AssetImage(i == Label.s1 ? shido1_png : shido2_png),
                        fit: BoxFit.fitHeight,
                        alignment: Alignment.topLeft,
                      )
                    : Align(
                          alignment: a.xalign < 0
                              ? Alignment.centerLeft
                              : (a.xalign > 0
                                  ? Alignment.centerRight
                                  : Alignment.center),
                          child: Text(hd ? '' : a.text,
                              style: TextStyle(
                                fontWeight: FontWeight.bold,
                                fontSize:
                                    a.h * (a.size == 0.0 ? 0.7 : a.size) * h,
                                color: a.fg,
                                //backgroundColor: a.bg,
                              )))

              )));
      l.add(wdg);
    }

    /*
      l.add(Positioned(
        left: 0,
        top: 0,
        width: w,
        height: h * 0.1,
        child: MouseRegion(
            onHover: (event) { print('HOOVER'); },
            child: ColoredBox(
          color: Colors.transparent,
        ))));
     */

    if (big_text != '' && !no_big_text) {
      l.add(Positioned(
          left: 0,
          top: 0,
          width: w,
          height: h * 0.2,
          child: ColoredBox(
              color: Colors.white,
              child: Center(
                  child: Text(big_text,
                      style: TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 0.16 * h,
                        color: Colors.black,
                      ))))));
    }

    //l.add(SizedBox.shrink());

    if (showCompetitors) {
      l.add(Positioned(
        left: 0,
        top: 0,
        width: w,
        height: h,
        child: ShowCompetitors(this, w, h),
      ));
    } else if (showWinner) {
      l.add(Positioned(
        left: 0,
        top: 0,
        width: w,
        height: h,
        child: ShowWinner(this, w, h),
      ));
    }

    if (setClocks) {
      var a = widget.labels[Label.t_min];
      final isize = a.w * w * 0.5;

      final llist = [Label.t_min, Label.t_tsec, Label.t_sec, Label.o_tsec, Label.o_sec];
      final tlist = [60, 10, 1, -10, -1];
      for (var i = 0; i < 5; i++) {
        a = widget.labels[llist[i]];
        var s = tlist[i];
        //print('ADD ARROW ${a.x},${a.y} ($w x $h): ${a.x * w} ${a.y * h} ${a.w *
        //    w}');
        final x = a.x * w + (a.w * w - isize)*0.5;
        l.add(Positioned(
          left: x,
          top: a.y * h,
          child: GestureDetector(
            onTap: () {
              if (s > 0) hajime_inc_func(this, s);
              else osaekomi_inc_func(this, -s);
            },
            child: Icon(Icons.arrow_upward_outlined, color: Colors.red,
            size: isize,),
        )));

        l.add(Positioned(
          left: x,
          top: (a.y + a.h) * h - isize,
          child: GestureDetector(
            onTap: () {
              if (s > 0) hajime_dec_func(this, s);
              else osaekomi_dec_func(this, -s);
            },
            child: Icon(Icons.arrow_downward_outlined, color: Colors.red,
            size: isize,),
        )));
      }
    }

    return l;
  }

  void adjustClocks() {
    setState(() {
      setClocks = !setClocks;
    });
  }

  void click(int i, bool shift) {
    //print('CLICK $i');
    if (mode_slave) {
      setState(() {
        if (toolbarheight > 30)
          toolbarheight = 0;
        else
          toolbarheight = 40;
      });
      return;
    }

    switch (i) {
      case Label.t_min:
      case Label.colon:
      case Label.t_tsec:
      case Label.t_sec:
        clock_key(this, Keys.GDK_space, shift);
        break;
      case Label.o_tsec:
      case Label.o_sec:
        clock_key(this, Keys.GDK_Return, shift);
        break;
      case Label.sonomama:
        clock_key(this, Keys.GDK_s, shift);
        break;
      case Label.w1:
        if (set_osaekomi_winner(this, BLUE | GIVE_POINTS))
          break;
        clock_key(this, Keys.GDK_F1, shift);
        break;
      case Label.y1:
        if (set_osaekomi_winner(this, BLUE | GIVE_POINTS))
          break;
        clock_key(this, Keys.GDK_F2, shift);
        break;
      case Label.k1:
        if (set_osaekomi_winner(this, BLUE | GIVE_POINTS))
          break;
        clock_key(this, Keys.GDK_F3, shift);
        break;
      case Label.s1:
        clock_key(this, Keys.GDK_F4, shift);
        break;
      case Label.w2:
        if (set_osaekomi_winner(this, WHITE | GIVE_POINTS))
          break;
        clock_key(this, Keys.GDK_F5, shift);
        break;
      case Label.y2:
        if (set_osaekomi_winner(this, WHITE | GIVE_POINTS))
          break;
        clock_key(this, Keys.GDK_F6, shift);
        break;
      case Label.k2:
        if (set_osaekomi_winner(this, WHITE | GIVE_POINTS))
          break;
        clock_key(this, Keys.GDK_F7, shift);
        break;
      case Label.s2:
        clock_key(this, Keys.GDK_F8, shift);
        break;
      case Label.points:
        set_osaekomi_winner(this, 0);
        break;
      case Label.pts_to_comp1:
        set_osaekomi_winner(this, BLUE);
        break;
      case Label.pts_to_comp2:
        set_osaekomi_winner(this, WHITE);
        break;
    }
  }

  void startTimer() {
    const oneSec = const Duration(milliseconds: 100);
    timer = Timer.periodic(
      oneSec,
      (Timer timer) {
        global_tick = timer.tick;
        updateClock(this, global_tick);
        if (playsound) {
          play_sound();
          playsound = false;
        }
      },
    );
  }

  String numToStr(int num) {
    if (num >= 0 && num < 10) return num2str[num];
    return num.toString();
  }

  void set_osaekomi_value(int tsec, int sec) {
    if (!mode_slave) send_label_msg(this, SET_OSAEKOMI_VALUE, [tsec, sec]);

    widget.labels[Label.o_tsec].text = numToStr(tsec);
    widget.labels[Label.o_sec].text = numToStr(sec);
    setState(() {});
  }

  void set_comment_text(String txt) {
    //if (golden_score && (txt == null || txt[0] == 0))
    //  txt = _("Golden Score");

    widget.labels[Label.comment].text = txt;

    //if (big_dialog)
    //  show_big();
  }

  void beep(String txt) {
    //playsound = true;
    play_sound();
  }

  String get_name(int who) {
    if (who == BLUE) return widget.labels[Label.comp1_name_1].text;
    return widget.labels[Label.comp2_name_1].text;
  }

  String get_cat() {
    return widget.labels[Label.cat1].text;
  }

  void set_number(int w, int num) {
    var s = widget.labels[w].text;
    var n = numToStr(num);
    if ((s != null && n != null && s != n) || s == null) {
      widget.labels[w].text = n;
      setState(() {});
    }
  }

  void set_points(List<int> blue, List<int> white) {
    if (!mode_slave) send_label_msg(this, SET_POINTS, blue + white);

    set_number(Label.w1, blue[0]);
    set_number(Label.y1, blue[1]);
    set_number(Label.k1, blue[2]);
    set_number(Label.s1, blue[3]);

    set_number(Label.w2, white[0]);
    set_number(Label.y2, white[1]);
    set_number(Label.k2, white[2]);
    set_number(Label.s2, white[3]);

    switch (blue[3]) {
      case 0: shido1_png = 'assets/shido-none.png'; break;
      case 1: shido1_png = 'assets/shido-yellow1.png'; break;
      case 2: shido1_png = 'assets/shido-yellow2.png'; break;
      case 3: shido1_png = 'assets/shido-red.png'; break;
    }
    switch (white[3]) {
      case 0: shido2_png = 'assets/shido-none.png'; break;
      case 1: shido2_png = 'assets/shido-yellow1.png'; break;
      case 2: shido2_png = 'assets/shido-yellow2.png'; break;
      case 3: shido2_png = 'assets/shido-red.png'; break;
    }

    if (use_ger_u12_rules > 0) {
      widget.labels[Label.comp1_leg_grab].fg = Colors.black;
      widget.labels[Label.comp2_leg_grab].fg = Colors.white;

      String t = widget.labels[Label.comp1_leg_grab].text2;
      if (t != null) widget.labels[Label.comp1_leg_grab].text2 = '';
      t = widget.labels[Label.comp2_leg_grab].text2;
      if (t != null) widget.labels[Label.comp2_leg_grab].text2 = '';

      set_number(Label.comp1_leg_grab,
          blue[0] * use_ger_u12_rules + blue[1] * 2 + blue[2]);
      set_number(Label.comp2_leg_grab,
          white[0] * use_ger_u12_rules + white[1] * 2 + white[2]);
    } else {
      if (blue[4] > 0) {
        widget.labels[Label.comp1_leg_grab].fg = Colors.black;
        widget.labels[Label.comp1_leg_grab].size = 0.4;
      } else {
        widget.labels[Label.comp1_leg_grab].fg = Colors.grey;
        widget.labels[Label.comp1_leg_grab].size = 0.2;
      }

      if (white[4] > 0) {
        widget.labels[Label.comp2_leg_grab].fg = Colors.white;
        widget.labels[Label.comp2_leg_grab].size = 0.4;
      } else {
        widget.labels[Label.comp2_leg_grab].fg = Color(0xff8080ff);
        widget.labels[Label.comp2_leg_grab].size = 0.2;
      }
    }

    setState(() {});

    setValInt('i1', blue[0]);
    setValInt('w1', blue[1]);
    setValInt('s1', blue[3]);
    setValInt('i2', white[0]);
    setValInt('w2', white[1]);
    setValInt('s2', white[3]);
  }

  void show_message(String cat_1, String blue_1, String white_1, String cat_2,
      String blue_2, String white_2, int flags, int rnd) {
    var buf, name;
    var b_tmp = blue_1, w_tmp = white_1;
    var b_first, b_last, b_club, b_country;
    var w_first, w_last, w_club, w_country;

    if (!mode_slave)
      send_label_msg(this, SHOW_MSG,
          [cat_1, blue_1, white_1, cat_2, blue_2, white_2, flags, rnd]);

    b_first = b_last = b_club = b_country = "";
    w_first = w_last = w_club = w_country = "";
    saved_first1 = saved_first2 = saved_last1 = saved_last2 = saved_cat = "";
    saved_country1 = saved_country2 = "";

    var tmp = blue_1.split("\t");
    if (tmp.length >= 2) b_first = tmp[1];
    if (tmp.length >= 1) b_last = tmp[0];
    if (tmp.length >= 4) b_club = tmp[3];
    if (tmp.length >= 3) b_country = tmp[2];
    tmp = white_1.split("\t");
    if (tmp.length >= 2) w_first = tmp[1];
    if (tmp.length >= 1) w_last = tmp[0];
    if (tmp.length >= 4) w_club = tmp[3];
    if (tmp.length >= 3) w_country = tmp[2];

    saved_last1 = b_last;
    saved_last2 = w_last;
    saved_first1 = b_first;
    saved_first2 = w_first;
    saved_cat = cat_1;
    saved_country1 = b_country;
    saved_country2 = w_country;

    widget.labels[Label.cat1].text = cat_1;
    // Show flags. Country must be in IOC format.
    widget.labels[Label.flag_comp1].text = b_country;
    widget.labels[Label.flag_comp2].text = w_country;
    widget.labels[Label.comp1_country].text = b_country;
    widget.labels[Label.comp2_country].text = w_country;

    widget.labels[Label.comp1_club].text = b_club;
    widget.labels[Label.comp2_club].text = w_club;

    name = get_name_by_layout(b_first, b_last, b_club, b_country);
    widget.labels[Label.comp1_name_1].text = name;
    name = get_name_by_layout(w_first, w_last, w_club, w_country);
    widget.labels[Label.comp2_name_1].text = name;

    widget.labels[Label.cat2].text = cat_2;

    tmp = blue_2.split("\t");
    if (tmp.length >= 2) b_first = tmp[1];
    if (tmp.length >= 1) b_last = tmp[0];
    if (tmp.length >= 4) b_club = tmp[3];
    if (tmp.length >= 3) b_country = tmp[2];
    tmp = white_2.split("\t");
    if (tmp.length >= 2) w_first = tmp[1];
    if (tmp.length >= 1) w_last = tmp[0];
    if (tmp.length >= 4) w_club = tmp[3];
    if (tmp.length >= 3) w_country = tmp[2];

    name = get_name_by_layout(b_first, b_last, b_club, b_country);
    widget.labels[Label.comp1_name_2].text = name;

    name = get_name_by_layout(w_first, w_last, w_club, w_country);
    widget.labels[Label.comp2_name_2].text = name;

    if ((flags & MATCH_FLAG_JUDOGI1_NOK) != 0)
      widget.labels[Label.comment].text = "White has a judogi problem.";
    else if ((flags & MATCH_FLAG_JUDOGI2_NOK) != 0)
      widget.labels[Label.comment].text = "Blue has a judogi problem.";
    else
      widget.labels[Label.comment].text = "";

    var t = AppLocalizations.of(context);
    widget.labels[Label.roundnum].text = round2Str(t, rnd);
    saved_round = rnd;

    if (!mode_slave)
      send_label_msg(
          this, SAVED_LAST_NAMES, [saved_last1, saved_last2, saved_cat]);

    setState(() {});
  }

  void display_comp_window(
      String saved_cat,
      String saved_last1,
      String saved_last2,
      String saved_first1,
      String saved_first2,
      String saved_country1,
      String saved_country2,
      int saved_round) async {
    //print('WIN -> COMPETITORS');

    ShowCompetitors.setData(
        saved_cat,
        saved_last1,
        saved_last2,
        saved_first1,
        saved_first2,
        saved_country1,
        saved_country2,
        '',
        '',
        saved_round);

    mainScreen = false;
    node.unfocus();
    setState(() {
      showCompetitors = true;
    });
    if (!mode_slave)
      send_label_msg(this, START_COMPETITORS,
          ['', ShowCompetitors.comp1, ShowCompetitors.comp2, ShowCompetitors.cat, ShowCompetitors.round]);
  }

  void display_winner_window(
      String cat,
      String last,
      String first,
      int winner) async {
    ShowWinner.setData(cat, last, first, winner);

    mainScreen = false;
    node.unfocus();
    setState(() {
      showWinner = true;
    });
    if (!mode_slave)
      send_label_msg(this, START_WINNER,
          ['$last\t$first', cat, winner]);
  }

  displayMainScreen() {
    if (!mode_slave) {
      if (showCompetitors)
        send_label_msg(this, STOP_COMPETITORS, []);
      if (showWinner)
        send_label_msg(this, STOP_WINNER, []);
    }
    mainScreen = true;
    showCompetitors = false;
    showWinner = false;
    node.requestFocus();
    setState(() {});
  }

  void set_gs_text(String txt) {
    widget.labels[Label.gs].text = txt;
  }

  void display_big(String txt, int tmp_sec) {
    setState(() {
      big_text = txt;
    });

    if (!mode_slave) send_label_msg(this, START_BIG, [big_text]);
  }

  void delete_big() {
    setState(() {
      big_text = '';
    });

    if (!mode_slave) send_label_msg(this, STOP_BIG, []);
  }

  void reset_display(Keys key) {
    //mylog('layout.dart', 1005, 'reset_display');
    set_timer_run_color(FALSE, FALSE);
    set_timer_osaekomi_color(OSAEKOMI_DSP_NO, 0, FALSE);
    set_osaekomi_value(0, 0);

    if (golden_score == FALSE) set_points([0, 0, 0, 0, 0], [0, 0, 0, 0, 0]);
  }

  void set_timer_run_color(bool running, bool resttime) {
    if (!mode_slave) {
      //mylog('layout', 1017, 'Sending run color message');
      send_label_msg(
          this, SET_TIMER_RUN_COLOR, [running ? 1 : 0, resttime ? 1 : 0]);
    }

    //mylog('layout.dart', 1019, 'RUN COLOR running=$running rest=$resttime');
    Color color = clock_stop;
    if (running) {
      if (resttime)
        color = Colors.red;
      else
        color = clock_run;
    }

    widget.labels[Label.t_min].fg = color;
    widget.labels[Label.colon].fg = color;
    widget.labels[Label.t_tsec].fg = color;
    widget.labels[Label.t_sec].fg = color;
    setState(() {});
  }

  Future<void> play_sound() async {
    if (playing) return;
    playing = true;
    playnum += 1;
    soundFile = await getVal('sound', 'AirHorn');
    /****
    try {
      var duration = await player
          .setSource(AssetSource('audio/$soundFile.mp3'))
          .catchError((error) {
        //print('SET ASSET $playnum: $error');
      });
      //print('DURATION $playnum = $duration');
    } catch (e) {
      //print("Error loading audio source: $e");
    }
        ****/
    player.play(AssetSource('audio/$soundFile.mp3'));
    playnum -= 1;
    playing = false;
  }

  void voting_result(int data) {
    int selection = data & SELECTION_MASK;
    bool checkippon = (data & NO_IPPON_CHECK) == 0;

    blue_wins_voting = FALSE;
    white_wins_voting = FALSE;
    hansokumake_to_blue = FALSE;
    hansokumake_to_white = FALSE;
    result_hikiwake = FALSE;

    switch (selection) {
      case HANTEI_BLUE:
        widget.labels[Label.comment].text = "White won the voting";
        blue_wins_voting = TRUE;
        break;
      case HANTEI_WHITE:
        widget.labels[Label.comment].text = "Blue won the voting";
        white_wins_voting = TRUE;
        break;
      case HANSOKUMAKE_BLUE:
        widget.labels[Label.comment].text = "Hansoku-make to white";
        hansokumake_to_blue = TRUE;
        break;
      case HANSOKUMAKE_WHITE:
        widget.labels[Label.comment].text = "Hansoku-make to blue";
        hansokumake_to_white = TRUE;
        break;
      case HIKIWAKE:
        widget.labels[Label.comment].text = "Hikiwake";
        result_hikiwake = TRUE;
        break;
      case CLEAR_SELECTION:
        widget.labels[Label.comment].text = "";
        break;
    }

    //if (big_dialog)
    //  show_big();

    if (checkippon) set_hantei_winner(this, data);
  }
}
