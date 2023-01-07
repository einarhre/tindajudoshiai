import 'package:flutter/material.dart';
import 'package:format/format.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'package:judoinfo/global.dart';
import 'package:judolib/judolib.dart';
import 'layout.dart';

class MatchCard extends StatelessWidget {
  int tatami, pos;
  double width, height;
  LayoutState layout;
  double k = 0.20;
  final fit = BoxFit.fill;

  MatchCard(
      {required this.layout, required this.width, required this.height, required this.tatami, required this.pos, Key? key})
      : super(key: key) {
  }

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(context);
    var color = Colors.white;
    var color1 = tatamiClicked[tatami] ? Colors.green : Colors.yellow;
    Match m = matches[tatami].matches[pos];
    Judoka? comp1 = layout.getJudoka(m.comp1);
    Judoka? comp2 = layout.getJudoka(m.comp2);
    CategoryDef? cat = layout.getCategory(m.cat);
    //print('CARD $tatami $pos ${m.comp1} ${m.comp2}');

    if (pos == 0) {
      return GestureDetector(
          onTap: () {
              layout.setState(() {
                tatamiClicked[tatami] = true;
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
                color: color1,
                child: Text(
                  '',
                  style: TextStyle(fontSize: textSize,),
                ),
              ),
              Container(
                color: color1,
                child: Text(
                  comp1?.club ?? '',
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
                  child: Text(
                    comp1?.club ?? '',
                    style: TextStyle(fontSize: textSize,),
                  ),
                ),
                Container(
                  color: Color(0xff0000ff),
                  child: Text(
                    comp2?.club ?? '',
                    style: TextStyle(color: Colors.white, fontSize: textSize,),
                  ),
                ),
              ]),
            ]));
  }
}
