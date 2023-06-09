import 'package:flutter/material.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'package:judoreferee/global.dart';
import 'package:judolib/judolib.dart';
import 'package:provider/provider.dart';
import 'bloc.dart';
import 'layout.dart';
import 'util.dart';

class MatchCard extends StatelessWidget {
  int tatami0, pos;
  double width, height;
  LayoutState layout;
  var cb;
  double k = 0.20;
  final fit = BoxFit.fill;

  MatchCard(
      {required this.layout, required this.width, required this.height, required this.tatami0,
        required this.pos, required this.cb,  Key? key})
      : super(key: key) {
  }

  @override
  Widget build(BuildContext context) {
    var provider = Provider.of<RefereeModel>(context, listen: true);
    var matches = provider.matches;
    var judokainfo = provider.judokaInfo;
    var categoryinfo = provider.categoryInfo;
    var t = AppLocalizations.of(context);
    var color = Colors.white;
    var color1 = tatamiClicked[tatami0] ? Colors.green : Colors.yellow;
    Match m = matches[tatami0].matches[pos];
    Judoka? comp1 = layout.getJudoka(provider, m.comp1);
    Judoka? comp2 = layout.getJudoka(provider, m.comp2);
    CategoryDef? cat = layout.getCategory(provider, m.cat);
    Referee? referee = null, judge1 = null, judge2 = null;

    RefTeam? rt = provider.getMatchRef(m.cat, m.number);
    if (rt != null) {
      referee = provider.getRefereeByName(rt.ref);
      judge1 = provider.getRefereeByName(rt.judge1);
      judge2 = provider.getRefereeByName(rt.judge2);
      if (referee == null) {
        //drawReferees(provider, layout, tatami0+1, false);
      }
    } else if (m.cat != 0) {
      layout.sendGetToUnqlite('m_${m.cat}_${m.number}');
    }
    //print('CARD $tatami $pos ${m.comp1} ${m.comp2}');

    if (pos == 0) {
      return GestureDetector(
          onTap: () {
              layout.setState(() {
                tatamiClicked[tatami0] = true;
              });
          },
        child: Card(
          borderOnForeground: true,
          margin: EdgeInsets.all(6.0),
          elevation: 8.0,
          color: color,
          child:
          Table(children: <TableRow>[
            TableRow(children: <Widget>[
              Container(
                color: Color(0xffccffcc),
                child: Text('Referees',
                  style: TextStyle(fontWeight: FontWeight.bold,
                    fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text(t?.prevWinner4175 ?? '',
                  style: TextStyle(fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text('',
                  style: TextStyle(fontSize: textSize,),
                ),
              ),
            ]),

            TableRow(children: <Widget>[
              Container(
                color: Color(0xffccffcc),
                child: Text('',
                  style: TextStyle(fontWeight: FontWeight.bold,
                    fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text(cat?.name ?? '',
                  style: TextStyle(fontWeight: FontWeight.bold, fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text(
                  comp1?.first ?? '',
                  style: TextStyle(fontSize: textSize,),
                ),
              ),
            ]),

            TableRow(children: <Widget>[
              Container(
                color: Color(0xffccffcc),
                child: Text('',
                  style: TextStyle(fontWeight: FontWeight.bold,
                    fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text('',
                  style: TextStyle(fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text(
                  comp1?.last ?? '',
                  style: TextStyle(fontWeight: FontWeight.bold, fontSize: textSize,),
                ),
              ),
            ]),

            TableRow(children: <Widget>[
              Container(
                color: Color(0xffccffcc),
                child: Text('',
                  style: TextStyle(fontWeight: FontWeight.bold,
                    fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text(
                  '',
                  style: TextStyle(fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text(
                  '${comp1?.club ?? ''}#{comp1?.country ?? ''}',
                  style: TextStyle(fontSize: textSize,),
                ),
              ),
            ]),
          ])));
    }

    return Card(
            borderOnForeground: true,
            margin: EdgeInsets.all(6.0),
            elevation: 8.0,
            color: color,
            child:
            Table(children: <TableRow>[
              TableRow(children: <Widget>[
                Container(
                  color: Color(0xff88ff88),
                  child: TextButton(
                    style: TextButton.styleFrom(
                      alignment: Alignment.centerLeft,
                      textStyle: TextStyle(fontWeight: FontWeight.bold,
                      fontSize: textSize,)),
                    onPressed: () {
                      cb(provider, tatami0, pos, 1);
                    },
                    child: Text('R: ${referee?.name ?? ''}'),
                  ),
                ),
                Container(
                  child: Text(cat?.name ?? '',
                    style: TextStyle(fontWeight: FontWeight.bold,
                        fontSize: textSize,),
                  ),
                ),
                Container(
                  child: Text(round2Str(t, m.round),
                    style: TextStyle(fontSize: textSize,),
                  ),
                ),
              ]),

              TableRow(children: <Widget>[
                Container(
                  color: Color(0xffccffcc),
                  child: TextButton(
                    style: TextButton.styleFrom(
                        alignment: Alignment.centerLeft,
                        textStyle: TextStyle(fontWeight: FontWeight.bold,
                          fontSize: textSize,)),
                    onPressed: () {
                      cb(provider, tatami0, pos, 2);
                    },
                    child: Text("J1: ${judge1?.name ?? ''}"),
                  ),
                ),
                Container(
                  child: Text(comp1?.first ?? '',
                    style: TextStyle(fontSize: textSize,),
                  ),
                ),
                Container(
                  color: Color(0xff0000ff),
                  child: Text(
                    comp2?.first ?? '',
                    style: TextStyle(color: Colors.white, fontSize: textSize,),
                  ),
                ),
              ]),

              TableRow(children: <Widget>[
                Container(
                  color: Color(0xffccffcc),
                  child: TextButton(
                    style: TextButton.styleFrom(
                        alignment: Alignment.centerLeft,
                        textStyle: TextStyle(fontWeight: FontWeight.bold,
                          fontSize: textSize,)),
                    onPressed: () {
                      cb(provider, tatami0, pos, 3);
                    },
                    child: Text("J2: ${judge2?.name ?? ''}"),
                  ),
                ),
                Container(
                  child: Text(comp1?.last ?? '',
                    style: TextStyle(fontWeight: FontWeight.bold, fontSize: textSize,),
                  ),
                ),
                Container(
                  color: Color(0xff0000ff),
                  child: Text(
                    comp2?.last ?? '',
                    style: TextStyle(color: Colors.white,
                        fontWeight: FontWeight.bold, fontSize: textSize,),
                  ),
                ),
              ]),
              TableRow(children: <Widget>[
                Container(
                  color: Color(0xffccffcc),
                  child: Text('',
                    style: TextStyle(fontWeight: FontWeight.bold,
                      fontSize: textSize,),
                  ),
                ),
                Container(
                  child: Text(
                    getClubCountryText(comp1),
                    style: TextStyle(fontSize: textSize,),
                  ),
                ),
                Container(
                  color: Color(0xff0000ff),
                  child: Text(
                    getClubCountryText(comp2),
                    style: TextStyle(color: Colors.white, fontSize: textSize,),
                  ),
                ),
              ]),
            ]));
  }

  String getClubCountryText(Judoka? j) {
    if (j != null) {
      if (j.country.length == 0) return j.club;
      return '${j.country}/${j.club}';
    }
    return '';
  }
}
