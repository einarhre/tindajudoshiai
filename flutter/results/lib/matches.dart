import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:provider/provider.dart';

import 'bloc.dart';
import 'match_card.dart';
import 'utils.dart';

class Matches extends StatefulWidget {
  final double width, height;
  final start, count;

  const Matches(
      {required this.width,
      required this.height,
      required this.start,
      required this.count,
      Key? key})
      : super(key: key);

  @override
  State<Matches> createState() => _MatchesState();
}

class _MatchesState extends State<Matches> {
  //bool _running = true;
  late Timer _timer;

  @override
  void initState() {
    super.initState();
    const oneSec = const Duration(seconds: 30);
    _timer = Timer.periodic(
      oneSec,
      (Timer timer) {
        getMatches(context);
      },
    );
  }

  @override
  void dispose() {
    //_running = false;
    _timer.cancel();
    //controller.close();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(context);
    SystemChannels.textInput.invokeMethod('TextInput.hide');
    var provider2 = Provider.of<CompetitionModel>(context, listen: true);
    List<TatamiMatches> matches = provider2.tatamiMatches;

    return Scaffold(
        body: Container(
            child: Scaffold(
                body: SingleChildScrollView(
                    scrollDirection: Axis.vertical,
                    child: Table(
                      border: TableBorder(
                        //horizontalInside: BorderSide(width: 1.0, color: Colors.black),
                        verticalInside:
                            BorderSide(width: 3.0, color: Colors.black),
                      ),
                      children: getTableRows(matches),
                    )))));
  }

  List<TableRow> getTableRows(List<TatamiMatches> matches) {
    List<TableRow> rows = [];

    var numColumns = matches.length;
    if (numColumns == 0) return rows;

    List<Widget> row = [];
    var last = widget.start + widget.count;
    if (last > numColumns) last = numColumns;
    for (var t = widget.start; t < last; t++) {
      var tatami = matches[t];
      row.add(Text(
        'Tatami ${tatami.tatami}',
        textAlign: TextAlign.center,
        style: TextStyle(
          fontWeight: FontWeight.bold,
          fontSize: 20.0,
        ),
      ));
    }
    rows.add(TableRow(children: row));

    for (var p = 0; p < 11; p++) {
      row = [];
      for (var t = widget.start; t < last; t++) {
        Match m1;
        try {
          m1 = matches[t].matches[p];
        } catch (e) {
          m1 = Match(t, '', '', '', '', '', '', '');
        }
        row.add(MatchCard(
            width: widget.width / numColumns,
            height: widget.height / 10,
            tatami: t,
            m: m1));
      }
      rows.add(TableRow(children: row));
    }

    return rows;
  }
}
