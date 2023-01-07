import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:format/format.dart';
import 'package:provider/provider.dart';
import 'package:results/custom_colors.dart';

import 'bloc.dart';
import 'utils.dart';

class Statistics extends StatefulWidget {
  const Statistics({Key? key}) : super(key: key);

  @override
  State<Statistics> createState() => _StatisticsState();
}

class _StatisticsState extends State<Statistics> {
  static const padding = 8.0;

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
            child: Table(
                        defaultColumnWidth: IntrinsicColumnWidth(),
                        children: getStatisticsList(competition, t),
  )));
  }

  Widget strCell(String val, bool right) {
    return Container(
        alignment: right ? Alignment.centerRight : Alignment.centerLeft,
        padding: EdgeInsets.all(padding),
        child: Text(val, style: TextStyle(fontWeight: FontWeight.bold)),
        );
  }

  Widget strVCell(String val, bool right) {
    return Container(
      alignment: right ? Alignment.centerRight : Alignment.centerLeft,
      padding: EdgeInsets.all(padding),
      child: RotatedBox(quarterTurns: 1, child: Text(val, style: TextStyle(fontWeight: FontWeight.bold))),
    );
  }


  Widget numCell(int val) {
    return Container(
      alignment: Alignment.centerRight,
      padding: EdgeInsets.all(padding),
      child: Text(val.toString(),
          //style: TextStyle(fontWeight: FontWeight.bold)),
    ));
  }

  Widget timeCell(int val) {
    var min = ((val/60).floor());
    var sec = (val%60);

    return Container(
        alignment: Alignment.centerRight,
        padding: EdgeInsets.all(padding),
        child: Text(format('{:d}:{:02d}', min, sec),
          //style: TextStyle(fontWeight: FontWeight.bold)),
        ));
  }

  List<TableRow> getStatisticsList(Competition? competition, AppLocalizations? t) {
    //const hdr = '{:2}{:1} {:20} {:>3s} {:>3s} {:>3s}   {:}';
    const hdr = '{:2}{:1} {:20} {:3s} {:3s} {:3s}   {:}';
    const fmt = '{:2d}{:1} {:20} {:3d} {:3d} {:3d}   {:3d}';

    var rows = <TableRow>[];
    if (competition == null) return rows;

    var len = competition.statistics.length;

    List<Widget> row = [
      strVCell(t?.restxt31 ?? 'Category', false),
      strVCell(t?.restxt1 ?? 'Competitors', true),
      strVCell(t?.restxt10 ?? 'Matches', true),
      strVCell('Ippon', true),
      strVCell('Waza-ari', true),
      strVCell('Shido', true),
      strVCell('Golden score', true),
      strVCell(t?.restxt20 ?? 'Time', true),
    ];
    rows.add(TableRow(
        decoration: BoxDecoration(
          color: CustomColors.groupHdr,
          shape: BoxShape.rectangle,
          border: const Border(
              bottom : BorderSide(
                  color: Colors.black87,
                  width: 1,
                  style: BorderStyle.solid
              )
          ),
        ),
        children: row));

    for (int i = 0; i < len; i++) {
      rows.add(TableRow(
          children: [
        strCell(competition.statistics[i].name, false),
        numCell(competition.statistics[i].competitors),
        numCell(competition.statistics[i].matches),
        numCell(competition.statistics[i].ippons),
        numCell(competition.statistics[i].wazaaris),
        numCell(competition.statistics[i].shidowins),
        numCell(competition.statistics[i].gs),
        timeCell(competition.statistics[i].time),
      ]));
    }

    return rows;
  }
}
