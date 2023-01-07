import 'dart:async';
import 'dart:collection';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

import 'package:flutter/material.dart';
import 'package:judoinfo/match_card.dart';
import 'package:judolib/judolib.dart';

import 'global.dart';
import 'main.dart';
import 'menus.dart';
import 'settings.dart';

class Layout extends StatefulWidget {
  final double width, height;

  Layout(this.width, this.height);

  @override
  State<Layout> createState() => LayoutState();
}

class LayoutState extends State<Layout> {
  var websockJs = null;
  double toolbarheight = 40;
  late Timer wstimer;
  Map<int, DateTime> asked = HashMap();
  bool updateDisplay = false;
  var oldPw = jspassword;

  @override
  void initState() {
    print('*** INIT ***');
    super.initState();
    readSettings();

    const dly = const Duration(seconds: 1);
    wstimer = Timer.periodic(
        dly,
            (Timer timer) {
          if (websockJs == null) {
            print('WS CONNECT ${getUrl2()}');
            asked = {};
            websockJs = WebSocketChannel.connect(Uri.parse(getUrl2()),
                protocols: ['js']);
            websockJs.stream.listen((event) {
              //print('MSG=$event type=${event.runtimeType}');
              var json = jsonDecode(event);
              var jsonmsg = json['msg'];
              Message msg = Message(jsonmsg);
              handle_message(msg);
            }, onError: (error) {
              if (websockJs != null) websockJs.sink.close();
              //print('WEBSOCK ERROR $error');
              websockJs = null;
            }, onDone: () {
              //print('WEBSOCK DONE');
              if (websockJs != null) websockJs.sink.close();
              websockJs = null;
            });

            if (websockJs != null) {
              var msgout = {
                'pw': jspassword,
                'msg': [COMM_VERSION, MSG_ALL_REQ, 0, 7780]
              };
              sendMsgToJs(msgout);
            }
          } else {
            if (oldPw != jspassword) {
              oldPw = jspassword;
              websockJs.sink.close();
              websockJs = null;
            }
          }

          if (updateDisplay) {
            setState(() {
              //print('UPDATE DSP');
              updateDisplay = false;
            });
          }
        }
    );
  }

  @override
  void dispose() {
    if (websockJs != null) websockJs.sink.close();
  }

  @override
  Widget build(BuildContext context) {
    if (!kIsWeb) {
      if (fullscreen)
        goFullScreen();
      else
        exitFullScreen();
    }

    return Scaffold(
        backgroundColor: Color(0xffcccccc),
        appBar: AppBar(
            toolbarHeight: toolbarheight,
            backgroundColor: Colors.black,
            title: Text('JudoInfo'),
            actions: [
              Builder(
                builder: (context) {
                  return DropdownButton(
                    dropdownColor: Colors.grey,
                    onChanged: (v) =>
                        setState(() {
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
                builder: (context) =>
                    ElevatedButton(
                      child: Icon(Icons.settings),
                      onPressed: () async {
                        final result = await Navigator.push(
                          context,
                          MaterialPageRoute(
                              builder: (context) => SettingsScreen(this)),
                        );
                        setState(() {});
                      },
                    ),
              ),

            ]),
        body: Builder(builder: (BuildContext context) {
          //final FocusNode focusNode = Focus.of(context);
          //focusNode.requestFocus();
          return Table(
            border: TableBorder(
              //horizontalInside: BorderSide(width: 1.0, color: Colors.black),
              verticalInside: BorderSide(width: 3.0, color: Colors.black),
            ),
            children: getTableRows(),
          );
        }));
  }

  List<TableRow> getTableRows() {
    List<TableRow> rows = [];

    var numColumns = 0;
    for (var t = 0; t < 20; t++)
      if (tatamis[t]) numColumns++;

    if (numColumns == 0) {
      tatamis[0] = true;
      numColumns = 1;
    }

    List<Widget> row = [];
    var t = mirror ? 19 : 0;
    while (t >= 0 && t < 20) {
      if (tatamis[t]) {
        row.add(Text(
          'Tatami ${t + 1}',
          textAlign: TextAlign.center,
          style: TextStyle(
            fontWeight: FontWeight.bold,
            fontSize: 20.0,
          ),
        ));
      }
      if (mirror) t--;
      else t++;
    }
    rows.add(TableRow(children: row));

    for (var p = 0; p < 11; p++) {
      row = [];
      t = mirror ? 19 : 0;
      while (t >= 0 && t < 20) {
        if (tatamis[t]) {
          row.add(MatchCard(layout: this,
              width: widget.width / numColumns,
              height: widget.height / 10,
              tatami: t,
              pos: p));
        }
        if (mirror) t--;
        else t++;
      }
      rows.add(TableRow(children: row));
    }

    return rows;
  }

  String getUrl2() {
    if (kIsWeb) {
      final n = getLocation();
      return 'ws://${n}:$WS_COMM_PORT/info_pw_$jspassword';
    }
    String s = getSsdpAddress('JudoShiai');
    String n = s != '' ? s : node_name;
    return 'ws://${n}:$WS_COMM_PORT/info_pw_$jspassword';
  }

  void setMatch(MsgMatchInfo m) {
    var t = m.tatami - 1;
    var p = m.position;
    if (p == 0) {
      Match old = matches[t].matches[p];
      if (old.cat != m.category || old.number != m.number)
        tatamiClicked[t] = false;
    }
    Match match = Match(m.category, m.number, m.comp1, m.comp2, m.round);
    //print('SET MATCH $t $p ${m.comp1}');
    matches[t].matches[p] = match;
  }

  void handle_message(Message msg) {
    //print('MSG TYPE=${msg.type}');
    if (msg.type == MSG_MATCH_INFO) {
      var m = MsgMatchInfo(msg.message);
      setMatch(m);
      updateDisplay = true;
    } else if (msg.type == MSG_11_MATCH_INFO) {
      var m = MsgMatchInfo11(msg.message);
      for (var i = 0; i < 11; i++) {
        var info = m.getMatchInfo(i);
        setMatch(info);
      }
      updateDisplay = true;
    } else if (msg.type == MSG_NAME_INFO) {
      var m = MsgNameInfo(msg.message);
      var ix = m.ix;
      //print('NAME INFO $ix = ${m.last}');
      if (ix < 10000)
        judokaInfo[ix] = Judoka(ix, m.first, m.last, m.club, '');
      else
        categoryInfo[ix] = CategoryDef(ix, m.last);
      updateDisplay = true;
    }
  }

  void sendMsgToJs(msgout) {
    //print('SEND TO JS $msgout');
    final s = jsonEncode(msgout);
    if (websockJs != null) websockJs.sink.add(s);
  }

  bool doNotAskAgain(int ix) {
    const age = Duration(seconds: 3);
    DateTime? d = asked[ix];
    if (d != null) {
      final now = DateTime.now();
      if (now.difference(d) > age)
        return false;
      return true;
    }
    return false;
  }

  Judoka? getJudoka(int ix) {
    if (ix == 0) return null;
    Judoka? j = judokaInfo[ix];
    if (j != null) return j;
    //print('getJudoka $ix doNotAskAgain=${doNotAskAgain(ix)}');
    if (doNotAskAgain(ix)) return null;
    var msgout = {
      'pw': jspassword,
      'msg': [COMM_VERSION, MSG_NAME_REQ, 0, 7778, ix]
    };
    sendMsgToJs(msgout);
    asked[ix] = DateTime.now();
    return null;
  }

  CategoryDef? getCategory(int ix) {
    if (ix == 0) return null;
    CategoryDef? j = categoryInfo[ix];
    if (j != null) return j;
    if (doNotAskAgain(ix)) return null;
    var msgout = {
      'pw': jspassword,
      'msg': [COMM_VERSION, MSG_NAME_REQ, 0, 7779, ix]
    };
    sendMsgToJs(msgout);
    asked[ix] = DateTime.now();
    return null;
  }


}

