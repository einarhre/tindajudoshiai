import 'package:flutter/material.dart';
import 'package:format/format.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'package:judolib/judolib.dart';

import 'utils.dart';

var textSize = 14.0;

class MatchCard extends StatelessWidget {
  int tatami;
  double width, height;
  Match m;
  double k = 0.20;
  final fit = BoxFit.fill;
  var dummy = Match(0, '', '', '', '', '', '', '');

  MatchCard(
      {required this.width, required this.height, required this.tatami, required this.m, Key? key})
      : super(key: key);

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(context);
    var color = Colors.white;
    var color1 = Colors.yellow;
    int pos = 0;
    try {
      pos = m.number;
    } catch(e) {
      m = dummy;
    }

    if (pos == 0) {
      return GestureDetector(
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
                    child: Text(m.category,
                      style: TextStyle(fontWeight: FontWeight.bold, fontSize: textSize,),
                    ),
                  ),
                  Container(
                    color: color1,
                    child: Text(
                      m.first1,
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
                      m.last1,
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
                      m.club1,
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
              child: Text(m.category,
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
              child: Text(m.first1,
                style: TextStyle(fontSize: textSize,),
              ),
            ),
            Container(
              color: Color(0xff0000ff),
              child: Text(
                m.first2,
                style: TextStyle(color: Colors.white, fontSize: textSize,),
              ),
            ),
          ]),

          TableRow(children: <Widget>[
            Container(
              child: Text(m.last1,
                style: TextStyle(fontWeight: FontWeight.bold, fontSize: textSize,),
              ),
            ),
            Container(
              color: Color(0xff0000ff),
              child: Text(
                m.last2,
                style: TextStyle(color: Colors.white,
                  fontWeight: FontWeight.bold, fontSize: textSize,),
              ),
            ),
          ]),
          TableRow(children: <Widget>[
            Container(
              child: Text(
                m.club1,
                style: TextStyle(fontSize: textSize,),
              ),
            ),
            Container(
              color: Color(0xff0000ff),
              child: Text(
                m.club2,
                style: TextStyle(color: Colors.white, fontSize: textSize,),
              ),
            ),
          ]),
        ]));
  }
}
