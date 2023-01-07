import 'package:flutter/material.dart';
import 'package:format/format.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:provider/provider.dart';

import 'bloc.dart';
import 'compinfo.dart';
import 'utils.dart';

const pngWidth = 630;
const pngHeight = 891;

class Coach extends StatefulWidget {
  Coach({Key? key}) : super(key: key);

  @override
  State<Coach> createState() => _CoachState();
}

class _CoachState extends State<Coach>
    with AutomaticKeepAliveClientMixin<Coach> {
  late TextEditingController controller;
  late FocusNode ixFocusNode;

  String textToSearch = '';
  String category = 'AP-66';

  @override
  bool get wantKeepAlive => true;

  @override
  void initState() {
    super.initState();
    //SystemChannels.textInput.invokeMethod('TextInput.show');
    ixFocusNode = FocusNode();
    controller = TextEditingController();
  }

  @override
  void dispose() {
    ixFocusNode.unfocus();
    ixFocusNode.dispose();
    controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(context);
    var provider = Provider.of<CompetitionModel>(context, listen: true);
    var competition = provider.competition;

    return Scaffold(
        //resizeToAvoidBottomInset: false,
        body: Container(
          child: ListView(
            children: getCompetitorList(competition, t, textToSearch),
          ),
        ));
  }

  List<Widget> getCompetitorList(
      Competition? competition, AppLocalizations? t, String id) {
    final firstlast = 0;
    var rows = <Widget>[
      Card(
        elevation: 2,
        margin: EdgeInsets.all(4),
        color: Colors.white,
        child: TextField(
          controller: controller,
          focusNode: ixFocusNode,
          autofocus: true,
          onChanged: (String value) {
            setState(() {
              textToSearch = value;
            });
          },
          decoration: InputDecoration(
            border: OutlineInputBorder(),
            labelText: '${t?.restxt7 ?? "Name"}/${t?.coachIda174 ?? "Coach ID:"}',
          ),
        ),
      ),
    ];

    if (competition == null || textToSearch == '') {
      print('NOTHING TO SHOW, text=$textToSearch');
      return rows;
    }

    var clist = competition.competitors.where((c) {
      final name = c.last.toLowerCase();
      final test = textToSearch.toLowerCase();
      //print('c=>${name}< >${test}< bool=${name.startsWith(test)}');
      return name.startsWith(test) || c.coachid == textToSearch;
    }).toList();
    print('COMP LIST LEN=${clist.length}');

    var len = clist.length;
    for (int i = 0; i < len; i++) {
      Competitor c = clist[i];

      var name =
          firstlast == 0 ? '${c.last}, ${c.first}' : '${c.first} ${c.last}';
      if (c.belt != '') name += '  (${c.belt})';

      Card row = Card(
          elevation: 2,
          margin: EdgeInsets.all(4),
          color: Colors.white,
          child: ListTile(
            isThreeLine: false,
            dense: true,
            leading: Text(format('{:6}', c.category),
                style: TextStyle(fontFamily: 'RobotoMono')),
            title: Text(name),
            subtitle: Text(c.club),
            selected: false,
            onTap: () {
              setState(() {
                ixFocusNode.unfocus();
                //SystemChannels.textInput.invokeMethod('TextInput.hide');
                category = c.category;
                Navigator.push(context, MaterialPageRoute(builder: (context) => CompInfo(category: c.category, competitor: c)));
              });
            },
          ));
      rows.add(row);
    }

    ixFocusNode.requestFocus();

    return rows;
  }
}
