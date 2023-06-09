import 'dart:async';
import 'dart:collection';
import 'dart:convert';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/foundation.dart';
import 'package:judoreferee/help.dart';
import 'package:judoreferee/refeerees.dart';
import 'package:provider/provider.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

import 'package:flutter/material.dart';
import 'package:judoreferee/match_card.dart';
import 'package:judolib/judolib.dart';

import 'all_referees.dart';
import 'bloc.dart';
import 'global.dart';
import 'main.dart';
import 'settings.dart';
import 'tatami.dart';
import 'util.dart';

class Layout extends StatefulWidget {
  final double width, height;

  Layout(this.width, this.height);

  @override
  State<Layout> createState() => LayoutState();
}

class LayoutState extends State<Layout> with SingleTickerProviderStateMixin {
  late List<Tab> myTabs;
  var websockJs = null;
  var websockUnqlite = null;
  double toolbarheight = 40;
  late Timer wstimer;
  Map<int, DateTime> asked = HashMap();
  bool updateDisplay = false;
  var oldPw = jspassword;
  late TabController _tabController;

  @override
  void initState() {
    print('*** INIT ***');
    var provider = Provider.of<RefereeModel>(context, listen: false);
    super.initState();
    readSettings();

    myTabs = [];
    myTabs.add(Tab(icon: getIcon('referees')));
    for (int i = 0; i < numTatamis; i++) myTabs.add(Tab(icon: getIconWithText('tatami', '${i + 1}') /*text: 'T${i + 1}'*/));
    myTabs.add(Tab(icon: getIcon('manual')));
    _tabController = TabController(vsync: this, length: myTabs.length);

    const dly = const Duration(seconds: 1);
    wstimer = Timer.periodic(dly, (Timer timer) {
      if (websockJs == null) {
        print('WS CONNECT ${getUrl2()}');
        asked = {};
        websockJs =
            WebSocketChannel.connect(Uri.parse(getUrl2()), protocols: ['js']);
        websockJs.stream.listen((event) {
          //print('MSG=$event type=${event.runtimeType}');
          var json = jsonDecode(event);
          var jsonmsg = json['msg'];
          Message msg = Message(jsonmsg);
          handle_message(provider, msg);
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

      if (websockUnqlite == null) {
        print('WS Unqlite CONNECT to ${getUnqliteUrl()}');
        websockUnqlite = WebSocketChannel.connect(Uri.parse(getUnqliteUrl()),
            protocols: ['unqlite']);
        websockUnqlite.stream.listen((event) {
          //print('UNQLITE MSG=$event type=${event.runtimeType}');
          var op = event.substring(0, 1);
          var eq = event.indexOf('=');
          if (eq > 1) {
            try {
              var key = event.substring(1, eq);
              var json = {};
              final String val = event.substring(eq + 1);
              if (val.length > 1) json = jsonDecode(val);
              handleUnqliteMsg(provider, op, key, json);
            } catch (e) {
              print('CORRUPTED MESSAGE $e');
            }
          } else {
            var key = event.substring(1);
            handleUnqliteMsg(provider, op, key, {});
          }
        }, onError: (error) {
          print('Unqlite ERROR $error');
          if (websockUnqlite != null) websockUnqlite.sink.close();
          websockUnqlite = null;
        }, onDone: () {
          print('Unqlite DONE');
          if (websockUnqlite != null) websockUnqlite.sink.close();
          websockUnqlite = null;
        });

        if (websockUnqlite != null) {
          sendGetToUnqlite('listedRefs');
          /*
          for (int i = 0; i < NUM_REFEREES; i++) {
            sendGetToUnqlite('ref_${tatami}_${i}');
          }*/
        }
      }

      if (updateDisplay) {
        updateDisplay = false;
        //setState(() {
        //print('UPDATE DSP');
        //});
      }
    });
  }

  @override
  void dispose() {
    if (websockJs != null) websockJs.sink.close();
    _tabController.dispose();
  }

  @override
  Widget build(BuildContext context) {
    var provider = Provider.of<RefereeModel>(context, listen: true);

    if (!kIsWeb) {
      if (fullscreen)
        goFullScreen();
      else
        exitFullScreen();
    }

    return Scaffold(
        backgroundColor: const Color(0xffcccccc),
        appBar: AppBar(
            toolbarHeight: toolbarheight,
            backgroundColor: Colors.black,
            bottom: TabBar(
              controller: _tabController,
              isScrollable: true,
              tabs: myTabs,
            ),
            title: const Text('JudoReferee'),
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
                              padding: const EdgeInsets.all(8.0),
                              child: Image.asset(
                                  'packages/judolib/assets/flags-ioc/${languageCodeToIOC[value]}.png')),
                          const SizedBox(
                            width: 10,
                          ),
                          Text(languageCodeToLanguage[value] ?? '',
                              style: const TextStyle(color: Colors.white)),
                        ]),
                      );
                    }).toList(),
                  );
                },
              ),
              Builder(
                builder: (context) => ElevatedButton(
                  child: const Icon(Icons.settings),
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
        body: TabBarView(
          controller: _tabController,
          children: tabList(),
        ));
  }

  List<Widget> tabList() {
    List<Widget> tabs = [];

    tabs.add(RefereesPage(layout: this));

    for (int i = 0; i < numTatamis; i++) {
      tabs.add(TatamiPage(
          layout: this,
          tatami: i + 1,
          width: widget.width,
          height: widget.height));
    }

    tabs.add(HelpPage());

    return tabs;
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

  String getUnqliteUrl() {
    String s = getSsdpAddress('JudoShiai');
    String n = s != '' ? s : node_name;
    return 'ws://${n}:$WS_UNQLITE_PORT';
  }

  void setMatch(provider, MsgMatchInfo m) {
    var t = m.tatami - 1;
    var p = m.position;
    if (p == 0) {
      Match old = provider.matches[t].matches[p];
      if (old.cat != m.category || old.number != m.number) {
        tatamiClicked[t] = false;
        sendDelToUnqlite('m_${m.category}_${m.number}');
      }
    }
    Match match = Match(m.category, m.number, m.comp1, m.comp2, m.round);
    //print('SET MATCH $t $p ${m.comp1}');
    provider.putMatch(match, t, p);
    RefTeam? rt = provider.getMatchRef(m.category, m.number);
    if (rt == null || rt.ref.length == 0) {
      Future.delayed(const Duration(milliseconds: 3000), () {
        drawReferees(provider, this, m.tatami, false);
        setState(() {});
      });
    }
    //matches[t].matches[p] = match;
  }

  void handle_message(provider, Message msg) {
    //print('MSG TYPE=${msg.type}');
    if (msg.type == MSG_MATCH_INFO) {
      var m = MsgMatchInfo(msg.message);
      setMatch(provider, m);
      updateDisplay = true;
    } else if (msg.type == MSG_11_MATCH_INFO) {
      var m = MsgMatchInfo11(msg.message);
      for (var i = 0; i < 11; i++) {
        var info = m.getMatchInfo(i);
        setMatch(provider, info);
      }
      updateDisplay = true;
    } else if (msg.type == MSG_NAME_INFO) {
      var m = MsgNameInfo(msg.message);
      var ix = m.ix;
      //print('NAME INFO $ix = ${m.last}');
      if (ix < 10000) {
        var a = m.club.indexOf('/');
        if (a == 3) {
          final country = m.club.substring(0, a);
          final club = m.club.substring(a + 1);
          provider.putJudokaInfo(
              ix, Judoka(ix, m.first, m.last, club, country));
        } else {
          provider.putJudokaInfo(ix, Judoka(ix, m.first, m.last, m.club, ''));
        }
      } else
        provider.putCategoryInfo(ix, CategoryDef(ix, m.last));
      updateDisplay = true;
    }
  }

  void handleUnqliteMsg(provider, String op, String key, json) {
    //print('REC MSG $op $key = $json');
    if (op == 'P' || op == 'G') {
      var a = key.split('_');
      if (a[0] == 'listedRefs') {
        List<Referee> listedReferees = [];

        try {
          var ls = json['refs'];
          for (int i = 0; i < ls.length; i++) {
            var l = ls[i];
            Referee r = Referee(l['name'], l['club'], l['country']);
            try {
              r.tatami = l['tatami'];
            } catch (e) {
              r.tatami = 0;
            }
            ;
            try {
              r.ref = l['numref'];
            } catch (e) {
              r.ref = 0;
            }
            ;
            try {
              r.judg1 = l['numjudg1'];
            } catch (e) {
              r.judg1 = 0;
            }
            ;
            try {
              r.judg2 = l['numjudg2'];
            } catch (e) {
              r.judg2 = 0;
            }
            ;
            try {
              r.active = l['active'];
            } catch (e) {
              r.active = true;
            }
            ;
            try {
              r.flags = l['flags'];
            } catch (e) {
              r.flags = REFEREE_OK | JUDGE_OK;
            }
            ;
            listedReferees.add(r);
          }
        } catch (e) {
          print('ERROR listedRefs $e');
        }
        provider.setListedReferees(listedReferees);
      } else if (a[0] == 'm') {
        try {
          int cat = int.parse(a[1]);
          int num = int.parse(a[2]);
          var r = json['r'];
          var j1 = json['j1'];
          var j2 = json['j2'];
          RefTeam rt = RefTeam(r, j1, j2);
          provider.putMatchRef(cat, num, rt);
        } catch (e) {}
      }
    } else if (op == 'D') {
      var a = key.split('_');
      if (a[0] == 'm') {
        int cat = int.parse(a[1]);
        int num = int.parse(a[2]);
        provider.delMatchRef(cat, num);
      }
    } else if (op == 'X') {}
  }

  void sendMsgToJs(msgout) {
    //print('SEND TO JS $msgout');
    final s = jsonEncode(msgout);
    if (websockJs != null) websockJs.sink.add(s);
  }

  void sendPutToUnqlite(key, json) {
    final s = 'P$key=${jsonEncode(json)}';
    print('SEND PUT To Unqlite: $key');
    if (websockUnqlite != null) websockUnqlite.sink.add(s);
  }

  void sendGetToUnqlite(String key) {
    final s = 'G$key';
    print('SEND GET To Unqlite: $s');
    if (websockUnqlite != null) websockUnqlite.sink.add(s);
  }

  void sendDelToUnqlite(String key) {
    final s = 'D$key';
    print('SEND DEL To Unqlite: $s');
    if (websockUnqlite != null) websockUnqlite.sink.add(s);
  }
  void sendDelAllToUnqlite(String key) {
    final s = 'X$key';
    print('SEND DEL ALL To Unqlite: $s');
    if (websockUnqlite != null) websockUnqlite.sink.add(s);
  }

  void sendRefToUnqlite(int t, int p, Referee r) {
    sendPutToUnqlite('ref_${t}_${p}',
        {'name': r.name, 'club': r.club, 'country': r.country});
  }

  bool doNotAskAgain(int ix) {
    const age = Duration(seconds: 3);
    DateTime? d = asked[ix];
    if (d != null) {
      final now = DateTime.now();
      if (now.difference(d) > age) return false;
      return true;
    }
    return false;
  }

  Judoka? getJudoka(provider, int ix) {
    if (ix == 0) return null;
    Judoka? j = provider.getJudokaInfo(ix);
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

  CategoryDef? getCategory(provider, int ix) {
    if (ix == 0) return null;
    CategoryDef? j = provider.getCategoryInfo(ix);
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
