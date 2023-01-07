import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:http/http.dart' as http;
import 'package:format/format.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:provider/provider.dart';

import 'bloc.dart';
import 'compinfo.dart';
import 'custom_colors.dart';
import 'homescreen.dart';
import 'utils.dart';

const listByName = 0;
const listByClub = 1;
const listByCategory = 2;

class ListCompetitors extends StatefulWidget {
  int listBy = listByName;

  ListCompetitors({required this.listBy, Key? key}) : super(key: key);

  @override
  State<ListCompetitors> createState() => _ListCompetitorsState();
}

class _ListCompetitorsState extends State<ListCompetitors> {

  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(context);
    var provider = Provider.of<CompetitionModel>(context, listen: true);
    var competition = provider.competition;
    SystemChannels.textInput.invokeMethod('TextInput.hide');

    return Scaffold(
        body: Container(
            child: ListView(
                        children: getCompetitorList(competition, t, widget.listBy),
                      )));
  }

  List<Card> getCompetitorList(Competition? competition, AppLocalizations? t, int listBy) {
    final firstlast = 0;
    var rows = <Card>[];
    if (competition == null) return rows;

    if (listBy == listByClub)
      competition.sortCompetitorsByClub();
    else if (listBy == listByCategory)
      competition.sortCompetitorsByCategory();
    else
      competition.sortCompetitorsByName();

    var oldCat = '';
    var oldClub = '';
    var len = competition.competitors.length;
    for (int i = 0; i < len; i++) {
      Competitor c = competition.competitors[i];

      var pos = c.pos == 0 ? '-' : c.pos.toString();
      var name = firstlast == 0
          ? '${c.last}, ${c.first}'
          : '${c.first} ${c.last}';
      if (c.belt != '') name += '  (${c.belt})';

      if (listBy == listByCategory) {
        if (c.category != oldCat) {
          Card row = Card(
              elevation: 2,
              margin: EdgeInsets.all(4),
              color: Colors.white,
              child: ListTile(
                tileColor: CustomColors.groupHdr,
                isThreeLine: false,
                dense: false,
                title: Text(
                    c.category, style: TextStyle(fontWeight: FontWeight.bold)),
                onTap: () {
                  Navigator.push(context, MaterialPageRoute(builder: (context) => CompInfo(category: c.category, competitor: null)));
                },
              ));
          rows.add(row);
          oldCat = c.category;
        }
      } else if (listBy == listByClub) {
          if (c.club != oldClub) {
            Card row = Card(
                elevation: 2,
                margin: EdgeInsets.all(4),
                color: Colors.white,
                child: ListTile(
                  tileColor: CustomColors.groupHdr,
                  isThreeLine: false,
                  dense: false,
                  title: Text(c.club, style: TextStyle(fontWeight: FontWeight.bold)),
                ));
            rows.add(row);
            oldClub = c.club;
          }
        }

        Card row = Card(
          elevation: 2,
          margin: EdgeInsets.all(4),
          color: Colors.white,
          child: ListTile(
            isThreeLine: false,
            dense: true,
            leading: listBy == listByCategory
                ? Text('')
                : Text(format('{:6}', c.category), style: TextStyle(fontFamily: 'RobotoMono')),
            title: Text(name),
            subtitle: Text(listBy == listByClub ? '' : c.club),
            trailing: Text(pos),
            selected: false,
            onTap: () {
              Navigator.push(context, MaterialPageRoute(builder: (context) => CompInfo(category: c.category, competitor: c)));
              },
          ));
      rows.add(row);
    }

    return rows;
  }

    List<TableRow> xxxxxcompTableRows(Competition? competition, AppLocalizations? t, bool listByClub) {
    var hdrStyle = TextStyle(fontWeight: FontWeight.bold);

    List<TableRow> rows = [
      TableRow(
        decoration: BoxDecoration(
            color: Colors.green.shade100,
            shape: BoxShape.rectangle,
            border: const Border(
                bottom : BorderSide( color: Colors.black87,
                    width: 1, style: BorderStyle.solid
                )
            )
        ),
        children: [
          TableCell(child: Text(t?.restxt7 ?? 'Name', style: hdrStyle,)),
          TableCell(child: Text(t?.restxt8 ?? 'Grade', style: hdrStyle,)),
          TableCell(child: Text(listByClub ? '' : (t?.restxt9 ?? 'Club'), style: hdrStyle,)),
          TableCell(child: Text(t?.restxt31 ?? 'Category', style: hdrStyle,)),
          TableCell(child: Text(t?.restxt23 ?? 'Pos', style: hdrStyle,)),
        ]
      )
    ];
    if (competition == null) return rows;

    if (listByClub) {
      var len = competition.clubs.length;
      for (var i = 0; i < len; i++) {
        var club = competition.clubs[i];
        var complen = club.competitors.length;

        TableRow row = TableRow(
          decoration: BoxDecoration(
              color: Color(0xffeeeeee),
              shape: BoxShape.rectangle,
              border: const Border(
                  bottom: BorderSide(color: Colors.black87,
                      width: 1, style: BorderStyle.solid
                  ),
                  top: BorderSide(color: Colors.black87,
                      width: 1, style: BorderStyle.solid
                  )
              )
          ),

          children: <Widget>[
            Text(club.club, style: hdrStyle,),
            Text(club.competitors.length.toString()),
            Text(''),
            Text(''),
            Text(''),
          ],
        );
        rows.add(row);

        for (var j = 0; j < complen; j++) {
          var c = club.competitors[j];
          TableRow row = TableRow(
            decoration: BoxDecoration(
                color: Color(0xffffffff),
                shape: BoxShape.rectangle,
                border: const Border(
                    /*bottom: BorderSide(color: Colors.black87,
                        width: 1, style: BorderStyle.solid
                    )*/
                )
            ),

            children: <Widget>[
              Text('${c.last}, ${c.first}  '),
              Text('${c.belt}  '),
              Text(''),
              Text(c.category),
              Text(c.pos == 0 ? '-' : c.pos.toString()),
            ],
          );
          rows.add(row);
        }
      }
    } else {
      var len = competition.competitors.length;
      for (int i = 0; i < len; i++) {
        Competitor c = competition.competitors[i];
        TableRow row = TableRow(
          decoration: BoxDecoration(
              color: i & 1 == 0 ? Color(0xffeeeeee) : Color(0xffdddddd),
              shape: BoxShape.rectangle,
              border: const Border(
                  bottom: BorderSide(color: Colors.black87,
                      width: 1, style: BorderStyle.solid
                  )
              )
          ),

          children: <Widget>[
            Text('${c.last}, ${c.first}  '),
            Text('${c.belt}  '),
            Text(c.club),
            Text(c.category),
            Text(c.pos == 0 ? '-' : c.pos.toString()),
          ],
        );
        rows.add(row);
      }
    }

    return rows;
  }
}
