import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'bloc.dart';
import 'global.dart';
import 'layout.dart';
import 'match_card.dart';
import 'util.dart';

class TatamiPage extends StatefulWidget {
  LayoutState layout;
  int tatami;
  final double width, height;

  TatamiPage(
      {Key? key,
      required LayoutState this.layout,
      required int this.tatami,
      required this.width,
      required this.height})
      : super(key: key);

  @override
  State<TatamiPage> createState() => _TatamiPageState();
}

class _TatamiPageState extends State<TatamiPage> {
  int _selectedIndex = 0;
  late List<TatamiMatches> matches;
  late Map matchref;
  Referee? _selectedReferee = null;

  @override
  Widget build(BuildContext context) {
    LayoutState layout = widget.layout;
    int tatami = widget.tatami;
    var provider = Provider.of<RefereeModel>(context, listen: true);
    var listedReferees = provider.listedReferees;
    matches = provider.matches;
    matchref = provider.matchRef;

    List<Referee> tatamiRefs = [];
    for (var r in listedReferees) if (r.tatami == tatami) tatamiRefs.add(r);

    return Scaffold(
        appBar: AppBar(
            title: getIconWithText('tatami', '${tatami}'),
            actions: <Widget>[
          ElevatedButton(
            child: getIcon('icon6'),
            style: ElevatedButton.styleFrom(
              elevation: 8,
            ),
            onPressed: () {
              drawReferees(provider, layout, tatami, true);
            },
          ),
          ElevatedButton(
            child: getIcon('icon7'),
            style: ElevatedButton.styleFrom(
              elevation: 8,
            ),
            onPressed: () {
              for (var i = 1; i < 11; i++) {
                Match m = matches[tatami - 1].matches[i];
                provider.putMatchRef(m.cat, m.number, RefTeam('', '', ''));
                layout.sendPutToUnqlite(
                    'm_${m.cat}_${m.number}', {'r': '', 'j1': '', 'j2': ''});
              }
            },
          ),
        ]),
        body: SingleChildScrollView(
            child: Row(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Expanded(
              child: Column(children: [
                Container(
                  child: Text(
                    'Referees',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: textSize,
                    ),
                  ),
                ),
                ListView.builder(
                    shrinkWrap: true,
                    scrollDirection: Axis.vertical,
                    itemCount: tatamiRefs.length,
                    itemBuilder: (BuildContext context, int index) {
                      var r = tatamiRefs[index];
                      return ListTile(
                        focusColor: Colors.yellow,
                        isThreeLine: true,
                        title: Text(r.name),
                        subtitle: Text(
                            '${r.club} / ${r.country}\nRef: ${r.ref}, Judge: ${r.judg1 + r.judg2}'),
                        selected: index == _selectedIndex,
                        trailing: IconButton(
                          icon: Icon(Icons.not_interested),
                          onPressed: () {
                            setState(() {
                              r.active = false;
                              r.tatami = 0;
                              for (var i = 1; i < 11; i++) {
                                Match m = matches[tatami - 1].matches[i];
                                RefTeam? rt = matchref[m.cat + m.number];
                                if (rt != null) {
                                  if (rt.ref == r.name) rt.ref = '';
                                  if (rt.judge1 == r.name) rt.judge1 = '';
                                  if (rt.judge2 == r.name) rt.judge2 = '';
                                  provider.putMatchRef(m.cat, m.number, rt);
                                  layout.sendPutToUnqlite(
                                      'm_${m.cat}_${m.number}',
                                      {'r': '', 'j1': '', 'j2': ''});
                                }
                              }
                            });
                          },
                        ),
                        onTap: () {
                          setState(() {
                            _selectedIndex = index;
                            _selectedReferee = tatamiRefs[index];
                          });
                        },
                      );
                    }),
              ]),
            ),
            Expanded(
                child: Table(
              border: const TableBorder(
                //horizontalInside: BorderSide(width: 1.0, color: Colors.black),
                verticalInside: BorderSide(width: 3.0, color: Colors.black),
              ),
              children: getTableRows(),
            )),
          ],
        )));
  }

  List<TableRow> getTableRows() {
    int tatami = widget.tatami;
    List<TableRow> rows = [];

    var numColumns = 1;
    List<Widget> row = [];
    var t = tatami - 1;

    for (var p = 0; p < 11; p++) {
      row = [];
      t = tatami - 1;
      row.add(MatchCard(
        layout: widget.layout,
        width: widget.width / numColumns,
        height: widget.height / 10,
        tatami0: t,
        pos: p,
        cb: matchInfoCb,
      ));
      rows.add(TableRow(children: row));
    }

    return rows;
  }

  void matchInfoCb(provider, int t, int p, int refnum) {
    if (t < 0) return;
    Match m = matches[t].matches[p];

    RefTeam? rt = matchref[m.cat + m.number];
    if (rt != null) {
      if (rt.ref == _selectedReferee?.name) rt.ref = '';
      if (rt.judge1 == _selectedReferee?.name) rt.judge1 = '';
      if (rt.judge2 == _selectedReferee?.name) rt.judge2 = '';
    } else {
      rt = RefTeam('', '', '');
    }

    if (refnum == 1) {
      rt.ref = _selectedReferee?.name ?? '';
    } else if (refnum == 2) {
      rt.judge1 = _selectedReferee?.name ?? '';
    } else if (refnum == 3) {
      rt.judge2 = _selectedReferee?.name ?? '';
    }

    provider.putMatchRef(m.cat, m.number, rt);
    widget.layout.sendPutToUnqlite('m_${m.cat}_${m.number}',
        {'r': rt.ref, 'j1': rt.judge1, 'j2': rt.judge2});
  }

  List<Widget> getRefereeRows(listedReferees) {
    int tatami = widget.tatami;
    double textSize = 15.0;
    List<Widget> rows = [];

    int index = 0;
    for (var r in listedReferees) {
      if (r.tatami != tatami) continue;
      rows.add(ListTile(
        selected: index == _selectedIndex,
        focusColor: Colors.yellow,
        isThreeLine: true,
        title: Text(r.name),
        subtitle: Text(
            '${r.club} / ${r.country}\nRef: ${r.ref}, Judge: ${r.judg1 + r.judg2}'),
        onTap: () {
          setState(() {
            print('SEL IX= $index');
            _selectedIndex = index;
          });
        },
      ));
      index++;
    }
    return rows;
  }


  void deleteReferees() {
    int tatami = widget.tatami;

    for (int i = 0; i < NUM_REFEREES; i++) {
      Referee r = allReferees[tatami].referees[i];
      r.name = '';
      r.club = '';
      r.country = '';
      widget.layout.sendRefToUnqlite(tatami, i, r);
    }
  }
}
