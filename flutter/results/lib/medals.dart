import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:format/format.dart';
import 'package:provider/provider.dart';
import 'package:results/custom_colors.dart';

import 'bloc.dart';
import 'utils.dart';

class Medals extends StatefulWidget {
  const Medals({Key? key}) : super(key: key);

  @override
  State<Medals> createState() => _MedalsState();
}

class _MedalsState extends State<Medals> {

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
                        children: getMedalsList(competition, t),
                )));
  }

  List<Card> getMedalsList(Competition? competition, AppLocalizations? t) {
    //const hdr = '{:2}{:1} {:20} {:>3s} {:>3s} {:>3s}   {:}';
    const hdr = '{:2}{:1} {:20} {:3s} {:3s} {:3s}   {:}';
    const fmt = '{:2d}{:1} {:20} {:3d} {:3d} {:3d}   {:3d}';

    var rows = <Card>[
      Card(
          elevation: 2,
          margin: EdgeInsets.all(4),
          color: Colors.white,
          child: ListTile(
            tileColor: CustomColors.groupHdr,
            isThreeLine: false,
            title:
                Text(format(hdr, '', '', t?.restxt9, '  I', ' II', 'III', t?.restxt1),
                    style: TextStyle(fontFamily: 'RobotoMono')),
          ))
    ];

    if (competition == null) return rows;

    var len = competition.medals.length;
    for (int i = 0; i < len; i++) {
      var m = competition.medals[i];
      var txt =
          format(fmt, m.pos, '.', m.club, m.p1, m.p2, m.p3, m.num);

      Card row = Card(
          elevation: 2,
          margin: EdgeInsets.all(4),
          color: Colors.white,
          child: ListTile(
            isThreeLine: false,
            title: Text(txt, style: TextStyle(fontFamily: 'RobotoMono')),
            selected: false,
          ));
      rows.add(row);
    }
    return rows;
  }
}
