import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:http/http.dart' as http;
import 'package:format/format.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:judolib/judolib.dart';
import 'package:provider/provider.dart';
import 'package:results/launch.dart';

import 'bloc.dart';
import 'compinfo.dart';
import 'custom_colors.dart';
import 'utils.dart';

var resultsExpanded = true;

class Results extends StatefulWidget {
  const Results({Key? key}) : super(key: key);

  @override
  State<Results> createState() => _ResultsState();
}

class _ResultsState extends State<Results> {
  //Future<Competition>? _competition;

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
        children: getPanelList3(competition, t),
      ),
    ));
  }

  List<Card> getPanelList3(Competition? competition, AppLocalizations? t) {
    var rows = <Card>[];
    if (competition == null) return rows;

    var catlen = competition.categories.length;
    for (var i = 1; i < catlen; i++) {
      var cat = competition.categories[i];
      var catname = cat.category;
      var numcomp = cat.numcomp;
      var firstlast = cat.firstlast;
      var comps = cat.competitors;
      var complen = comps.length;

      rows.add(Card(
        //color: Colors.blue,
        child: ListTile(
          tileColor: CustomColors.groupHdr,
          title: Text(catname),
          trailing: Text(complen.toString()),
          onTap: () {
            Navigator.push(context, MaterialPageRoute(builder: (context) => CompInfo(category: catname, competitor: null)));

            //launchWebPage('${str2hex(catname)}.png');
            /*
            Navigator.push(
              context,
              MaterialPageRoute(builder: (context) => WebPage(filename: '${str2hex(catname)}.pdf')),
            );*/
          },
        ),
      ));

      for (var j = 0; j < complen; j++) {
        var comp = comps[j];
        var pos = comp.pos;
        if (pos == 0) continue;
        var first = comp.first;
        var last = comp.last;
        var club = comp.club;
        var country = comp.country;
        var name = firstlast == 0
            ? format('{}. {:-20}  {:-20}  {:3}', pos, '${last}, ${first}', club,
                country)
            : format('{}. {:-20}  {:-20}  {:3}', pos, '${first}, ${last}', club,
                country);

        Card row = Card(
            elevation: 2,
            margin: EdgeInsets.all(4),
            color: Colors.white,
            child: ListTile(
              isThreeLine: false,
              leading: Text('${pos.toString()}.'),
              trailing: Text(country),
              dense: true,
              //leading: Icon(Icons.add_a_photo),
              title: Text('$last, $first'),
              subtitle: Text('${club}'),
              selected: false,
              onTap: () {
                Navigator.push(context, MaterialPageRoute(builder: (context) => CompInfo(category: catname, competitor: comp)));
              },
            ));
        rows.add(row);
      }
    }
    return rows;
  }

  List<Widget> getPanelList2(Competition? competition, AppLocalizations? t) {
    var panels = <ExpansionTile>[];
    if (competition == null) return panels;

    var catlen = competition.categories.length;
    for (var i = 1; i < catlen; i++) {
      var rows = <Card>[];
      var cat = competition.categories[i];
      var catname = cat.category;
      var numcomp = cat.numcomp;
      var firstlast = cat.firstlast;
      var comps = cat.competitors;
      var complen = comps.length;

      for (var j = 0; j < complen; j++) {
        var comp = comps[j];
        var pos = comp.pos;
        if (pos == 0) continue;
        var first = comp.first;
        var last = comp.last;
        var club = comp.club;
        var country = comp.country;
        var name = firstlast == 0
            ? format('{}. {:-20}  {:-20}  {:3}', pos, '${last}, ${first}', club,
                country)
            : format('{}. {:-20}  {:-20}  {:3}', pos, '${first}, ${last}', club,
                country);

        Card row = Card(
            elevation: 2,
            margin: EdgeInsets.all(4),
            color: Colors.white,
            child: ListTile(
              isThreeLine: false,
              leading: Text(pos.toString()),
              trailing: Text(club),
              //leading: Icon(Icons.add_a_photo),
              title: Row(children: [
                GestureDetector(
                  child: Text('$pos. $last, $first',
                      style: TextStyle(fontFamily: 'RobotoMono')),
                  onTap: () {
                    setState(() {});
                  },
                ),
                Spacer(flex: 4),
                Text(club),
                Spacer(),
                Text(country),
              ]),
              //subtitle: Text('${club} ${country}'),
              selected: false,
              /*onTap: () {
                setState(() {});
              },*/
            ));
        rows.add(row);
      }

      var p = ExpansionTile(
        collapsedTextColor: Colors.black,
        collapsedBackgroundColor: Colors.white,
        backgroundColor: Colors.grey,
        textColor: Colors.white,
        title: Text(
          format('{:-6} ({:3})', catname, numcomp),
          style:
              TextStyle(fontFamily: 'RobotoMono', fontWeight: FontWeight.w700),
        ),
        /****
            leading: GestureDetector(
            onTapDown: (TapDownDetails details) {
            _showPopupMenu(details.globalPosition, t);
            },
            child: Icon(Icons.menu)
            //Container(child: Text("Press Me")),
            ),
         ***/
        initiallyExpanded: resultsExpanded,
        children: rows,
      );

      panels.add(p);
    }

    return panels;
  }

  void _showPopupMenu(Offset offset, AppLocalizations? t) async {
    double left = 0.0; //offset.dx;
    double top = offset.dy;
    await showMenu(
      context: context,
      position: RelativeRect.fromLTRB(left, top, 0, 0),
      items: [
        PopupMenuItem<String>(
            child: Text(t?.expandAll5ffd ?? 'Expand all'), value: 'expand'),
        PopupMenuItem<String>(
            child: Text(t?.collapseAllb56c ?? 'Collapse all'),
            value: 'collapse'),
      ],
      elevation: 8.0,
    ).then((value) async {
      resultsExpanded = !resultsExpanded;
    });
  }
}
