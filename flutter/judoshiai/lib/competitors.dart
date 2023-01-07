import 'dart:convert';
import 'dart:ui';
import 'package:flutter/cupertino.dart';
import 'package:flutter/foundation.dart';
import 'package:data_table_plus/data_table_plus.dart';
import 'package:flutter/material.dart';
import 'package:adaptive_scrollbar/adaptive_scrollbar.dart';
import 'package:flutter/rendering.dart';
import 'package:format/format.dart';
import 'package:judolib/judolib.dart';
import 'package:judoshiai/global.dart';
import 'competitor_edit.dart';
import 'main.dart';
import 'settings.dart';
import 'my_classes.dart';
import 'utils.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:url_launcher/url_launcher.dart';
import 'package:http/http.dart' as http;
import 'package:url_launcher/url_launcher_string.dart';
//import 'package:file/file.dart';
import 'package:path_provider/path_provider.dart';

//import 'package:popup_menu/popup_menu.dart';
//import 'package:intl/intl.dart';
//import 'package:json_table/json_table.dart';
import 'package:file_saver/file_saver.dart';
import 'package:rflutter_alert/rflutter_alert.dart';

class Competitors extends StatefulWidget {
  const Competitors({Key? key}) : super(key: key);

  @override
  CompetitorsState createState() => CompetitorsState();
}

void doMenu(int val) {
  print('SELECTED $val');
  switch (val) {
    case 1:
      break;
  }
}

class CompetitorsState extends State<Competitors> {
  Future<String>? _compStr;
  bool toggle = true;
  Future<JSCategories>? _cats;
  int _currentSortColumn = 0;
  bool _isSortAsc = true;
  final ScrollController horizontalScroll = ScrollController();
  final ScrollController verticalScroll = ScrollController();
  final double width = 20;
  double toolbarheight = 40;

  Future<JSCategories> getStuff() async {
    var host = await getHostName('jsip');
    var jscats = JSCategories();
    JSCategory cat = JSCategory(
        999999, '?', 0, 0, 0, 0, 0, 0, 0, 0, 0, '');
    jscats.addnew(cat);

    try {
      var response = await http.post(
        Uri.parse('http://$host:8088/json'),
        body: '{"op":"categories", "pw": "$jspassword"}',
      );
      if (response.statusCode == 200) {
        //print('RESP1=${response.body}');
        var json = jsonDecode(utf8.decode(response.bodyBytes));
        //print('json=$json');
        //_cats = JSCategories() as Future<JSCategories>?;
        var len = json.length;
        int i;
        for (i = 1; i < len; i++) {
          var lst = json[i];
          //print('lst=$lst');
          JSCategory cat = JSCategory(
              lst['index'],
              lst['category'],
              lst['tatami'],
              lst['group'],
              lst['system'],
              lst['numcomp'],
              lst['table'],
              lst['wishsys'],
              lst['status'],
              lst['count'],
              lst['matchedcnt'],
              lst['sysdescr']);
          jscats.addnew(cat);
        }
      }

      response = await http.post(
        Uri.parse('http://$host:8088/json'),
        body: '{"op":"sql", "pw": "$jspassword", "cmd":"select * from competitors"}',
      );
      if (response.statusCode == 200) {
        final catslen = jscats.categories.length;
        //print('RESP1=${response.body}');
        var json = jsonDecode(utf8.decode(response.bodyBytes));
        //print('json=$json');
        //_cats = JSCategories() as Future<JSCategories>?;
        var len = json.length;
        int i;
        for (i = 1; i < len; i++) {
          var lst = json[i];
          //print('2: lst=$lst');
          JSCompetitor c = JSCompetitor(
              int.parse(lst[0]),
              lst[1],
              lst[2],
              int.parse(lst[3]),
              int.parse(lst[4]),
              lst[5],
              lst[6],
              int.parse(lst[7]),
              int.parse(lst[8]),
              lst[9],
              int.parse(lst[10]),
              lst[11],
              lst[12],
              int.parse(lst[13]),
              int.parse(lst[14]),
              lst[15],
              lst[16]);

          for (var cat in jscats.categories) {
            if (cat.category == c.category) {
              cat.addnew(c);
              break;
            }
          }
        }
      }
    } catch (e) {
      print("HTTP cats error $e");
      //rethrow;
    }
    jscats.sort();
    for (var e in jscats.categories) { e.sort();}
    await jscats.getExpanded();
    return jscats;
  }

  void _getSelectedRowInfo() {
    print('Selected Item Row Name Here...');
  }

  void _showPopupMenu(int ix, Offset offset) async {
    print('IX=$ix');
    double left = 0.0; //offset.dx;
    double top = offset.dy;
    await showMenu(
      context: context,
      position: RelativeRect.fromLTRB(left, top, 0, 0),
      items: [
        PopupMenuItem<String>(
            child: const Text('Move Competitors Here'), value: 'move'),
        PopupMenuItem<String>(child: const Text('Edit'), value: 'edit'),
        PopupMenuItem<String>(child: const Text('Show Sheet'), value: 'sheet'),
        PopupMenuItem<String>(
            child: const Text('Draw Selected'), value: 'draw'),
        PopupMenuItem<String>(
            child: const Text('Print Selected Accreditation Cards'),
            value: 'accr'),
      ],
      elevation: 8.0,
    ).then((value) async {
      print('VALUE=$value');
      var dest = '?';
      switch (value) {
        case 'move':
          JSCategories jc = await _cats as JSCategories;
          var sel = jc.getSelected();
          dest = jc.catIxToStr(ix);
          var js = json.encode({'op': 'movcat', 'pw': jspassword, 'comps': sel, 'dest': dest});
          try {
            var host = await getHostName('jsip');
            var response = await http.post(
              Uri.parse('http://${host}:8088/json'),
              body: js,
            );
            print('status=${response.statusCode} type=${response
                .headers['content-type']} body=${response.body}');
            var msgs = jsonDecode(utf8.decode(response.bodyBytes));
            int len = msgs.length;
            if (len > 0) {
              int i;
              for (i = 0; i < len; i++) {
                var m = msgs[i];
                Alert(context: context,
                    title: "ERROR",
                    desc: "${m['first']} ${m['last']}: ${m['msg']}")
                    .show();
              }
            } else {
              await jc.saveExpanded();
              Navigator.pushReplacement(
                  context,
                  MaterialPageRoute(
                      builder: (BuildContext context) => super.widget));
            }
          } catch(e) {
            print("HTTP move error $e");
            //rethrow;
          }
          setState(() {
            jc.unselectAll();
          });
          break;
        case 'accr':
          JSCategories jc = await _cats as JSCategories;
          var sel = jc.getSelected();
          var js = json.encode({'op': 'accrcard', 'pw': jspassword, 'comps': sel, 'what': 0});
          var host = await getHostName('jsip');
          try {
            var response = await http.post(
              Uri.parse('http://${host}:8088/json'),
              body: js,
            );
            if (response.headers['content-type'] == 'application/pdf') {
              showPdf(response.bodyBytes);
            } else {
              Alert(context: context, title: "ERROR", desc: "Cannot download.")
                  .show();
            }
          } catch (e) {
            print("HTTP accr error $e");
            //rethrow;
          }
          setState(() {
            jc.unselectAll();
          });
          break;
        case 'edit':
          JSCategories jc = await _cats as JSCategories;
          for (var cat in jc.categories) {
            for (var c in cat.competitors) {
              if (c.selected) {
                Navigator.push(
                  context,
                  MaterialPageRoute(builder: (context) => CompetitorEdit(c, [])),
                );
                break;
              }
            }
          }
          break;
        case 'sheet':
          var host = await getHostName('jsip');
          var _url = 'http://${host}:8088/web?op=5&s=2&c=$ix';
          if (!await launch(_url)) throw 'Could not launch $_url';
          break;
        case 'draw':
          break;
      }
    });
  }

  List<ExpansionPanel> getPanelList(JSCategories? jscats) {
    final _horizontalScrollController = ScrollController();
    var panels = <ExpansionPanel>[];
    if (jscats == null)
      return panels;
    int i;
    var catlen = jscats.categories.length;

    for (i = 0; i < catlen; i++) {
      var rows = <DataRow>[];
      var cat = jscats.categories[i];
      int j;
      var len = cat.competitors.length;

      for (j = 0; j < len; j++) {
        var c = cat.competitors[j];
        DataRow row =
        DataRow(
          selected: c.selected,
          onSelectChanged: (selected) {
            setState(() {
              c.selected = selected!;
            });
          },
          cells: [
            //DataCell(Text(c.category)),
            DataCell(
              GestureDetector(
                onTap: () async {
                  final result = await Navigator.push(
                    context,
                    MaterialPageRoute(builder: (context) => CompetitorEdit(c, jscats.getCategoryNames())),
                  );
                  print('EDIT OK');
                  await jscats.saveExpanded();
                  setState(() {

                  });
                },
                child: Text(c.last),
              ),
              //showEditIcon: true,
              //placeholder: false,
              //onTap: _getSelectedRowInfo,
            ),
            DataCell(Text(c.first,)),
            DataCell(Text(c.birthyear.toString())),
            DataCell(Text(c.belt.toString())),
            DataCell(Text(c.club)),
            DataCell(Text(c.country)),
            DataCell(Text(c.regcategory)),
            DataCell(Text(c.weight.toString())),
            DataCell(Text(c.id)),
            DataCell(Text(c.seeding.toString())),
            DataCell(Text(c.clubseeding.toString())),
          ],);
        c.row = row;
        rows.add(row);
      }

      var b = DataTablePlus(
        columns: <DataColumn>[
          //DataColumn(label: Text('Edit')),
          DataColumn(
            label: Text('Last Name'),

            onSort: (columnIndex, ascending) {
              setState(() {
                _currentSortColumn = columnIndex;
                if (ascending) {
                  cat.competitors.sort((a, b) => b.last.compareTo(a.last));
                } else {
                  cat.competitors.sort((a, b) => a.last.compareTo(b.last));
                }
                _isSortAsc = ascending;
              });
            },

          ),
          DataColumn(label: Text('First Name')),
          DataColumn(label: Text('Year of Birth'), numeric: true),
          DataColumn(label: Text('Grade')),
          DataColumn(label: Text('Club')),
          DataColumn(label: Text('Country')),
          DataColumn(label: Text('Reg.Category')),
          DataColumn(label: Text('Weight'), numeric: true),
          DataColumn(label: Text('Id')),
          DataColumn(label: Text('Seeding')),
          DataColumn(label: Text('Club Seeding')),
        ],
        rows: rows,
        headingTextStyle: TextStyle(
          fontWeight: FontWeight.bold,
          color: Colors.black,
        ),
        sortColumnIndex: _currentSortColumn,
        sortAscending: _isSortAsc,
      );

      var p = ExpansionPanel(
        headerBuilder: (BuildContext context, bool isExpanded) {
          return ListTile(
            title: Text(format('{:-6} [{:3}]  T{}  Group {}  {}',
                cat.category, cat.numcomp, cat.tatami, cat.group,
                cat.sysdescr),
              style: TextStyle(
                  fontFamily: 'RobotoMono', fontWeight: FontWeight.w700),),
            tileColor: cat.getCatColor(),
            leading: GestureDetector(
                onTapDown: (TapDownDetails details) {
                  _showPopupMenu(cat.index, details.globalPosition);
                },
                child: Icon(Icons.menu)
              //Container(child: Text("Press Me")),
            ),
          );
        },
        body:  b,
        isExpanded: cat.isExpanded,
        canTapOnHeader: true,
      );

      panels.add(p);
    }

    return panels;
  }


  @override
  void initState() {
    super.initState();
    readSettings();
    _cats = getStuff() as Future<JSCategories>?;
  }


  @override
  Widget build(BuildContext context) {
    final _verticalScrollController = ScrollController();
    final _horizontalScrollController = ScrollController();

    return Scaffold(
        appBar: AppBar(
          toolbarHeight: toolbarheight,
          backgroundColor: Colors.black,
          title: Text('JudoTimer'),
          /***
          leading: Builder(
            builder: (context) => ElevatedButton(
              child: Icon(Icons.menu),
              onPressed: () {
                showPopupMenu(this);
              },
            ),
          ),
              ***/
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
                  final result = await Navigator.push(
                    context,
                    MaterialPageRoute(
                        builder: (context) => SettingsScreen(this)),
                  );
                  setState(() {});
                },
              ),
            ),
          ],
        ),

        body: Container(
            child: FutureBuilder<JSCategories>(
                initialData: null,
                future: _cats,
                builder: (BuildContext context,
                    AsyncSnapshot<JSCategories> snapshot) {
                  print('State: ${snapshot.connectionState}');
                  if (snapshot.connectionState == ConnectionState.waiting) {
                    return CircularProgressIndicator();
                  } else if (snapshot.connectionState == ConnectionState.done) {
                    if (snapshot.hasError) {
                      print('ERROR ${snapshot.error}');
                      return const Text('Error');
                    } else if (snapshot.hasData) {
                      print('HAS DATA');
                      return Scaffold(
                        body: SingleChildScrollView(
                          scrollDirection: Axis.vertical,
                          child: Column(
                            children: getPanelList2(snapshot.data),
                          ),
                        ),
                      );
                    } else {
                      print('NO DATA');
                      return Center(
                        child: CircularProgressIndicator(
                          strokeWidth: 3,
                        ),
                      );
                    }
                  } else {
                    return Text('State: ${snapshot.connectionState}');
                  }
                })));
  }

  String getPrettyJSONString(jsonObject) {
    JsonEncoder encoder = JsonEncoder.withIndent('  ');
    String jsonString = encoder.convert(json.decode(jsonObject));
    return jsonString;
  }


  List<Widget> getPanelList2(JSCategories? jscats) {
    var panels = <ExpansionTile>[];
    if (jscats == null)
      return panels;
    int i;
    var catlen = jscats.categories.length;

    for (i = 0; i < catlen; i++) {
      var rows = <Card>[];
      var cat = jscats.categories[i];
      int j;
      var len = cat.competitors.length;

      for (j = 0; j < len; j++) {
        var c = cat.competitors[j];

        Card row = Card(
          elevation: 2,
          margin: EdgeInsets.all(4),
          color: c.selected ? Colors.grey : Colors.white,
          child: ListTile(
            isThreeLine: true,
            //leading: Icon(Icons.),
            title: Row(children: [
              GestureDetector(
                child: Text(format('{:-20}    ', '${c.last}, ${c.first}'),
                    style: TextStyle(fontFamily: 'RobotoMono')),
                onTap: () {
                  setState(() {
                    cat.sort();
                  });
                },
              ),
              GestureDetector(
                child: Text(format('{:6.2f} kg ', c.weight/1000),
                    style: TextStyle(fontFamily: 'RobotoMono')),
                onTap: () {
                  setState(() {
                    cat.sortByWeight();
                  });
                },
              ),
              GestureDetector(
                child: Text(format(' {:3} ', c.country),
                    style: TextStyle(fontFamily: 'RobotoMono')),
                onTap: () {
                  setState(() {
                    cat.sortByCountry();
                  });
                },
              ),
              GestureDetector(
                child: Text(format('{}', c.club),
                    style: TextStyle(fontFamily: 'RobotoMono')),
                onTap: () {
                  setState(() {
                    cat.sortByClub();
                  });
                },
              ),
            ],
            ),
            subtitle: Text('${c.regcategory} \n ${c.belt}'),
            selected: c.selected,
            onTap: () {
              setState(() {
                c.selected = !c.selected;
              });

            },
          ),
        );

        rows.add(row);
      }

      var p = ExpansionTile(
        collapsedBackgroundColor: cat.getCatColor(),
        backgroundColor: cat.getCatColor(),
        title: Text(
          format('{:-6} [{:3}]  T{}  Group {}  {}', cat.category, cat.numcomp,
              cat.tatami, cat.group, cat.sysdescr),
          style:
          TextStyle(fontFamily: 'RobotoMono', fontWeight: FontWeight.w700),
        ),
        leading: GestureDetector(
            onTapDown: (TapDownDetails details) {
              _showPopupMenu(cat.index, details.globalPosition);
            },
            child: Icon(Icons.menu)
          //Container(child: Text("Press Me")),
        ),
        children: rows,
      );
      panels.add(p);
    }
    return panels;
  }
}
