import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:judoreferee/icons.dart';
import 'package:provider/provider.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'package:judolib/judolib.dart';
import 'package:judoreferee/layout.dart';

import 'all_referees.dart';
import 'bloc.dart';
import 'global.dart';
import 'util.dart';

class RefereesPage extends StatefulWidget {
  LayoutState layout;

  RefereesPage({Key? key, required LayoutState this.layout}) : super(key: key);

  @override
  State<RefereesPage> createState() => _RefereesPageState();
}

class _RefereesPageState extends State<RefereesPage> {
  @override
  Widget build(BuildContext context) {
    LayoutState layout = widget.layout;
    var provider = Provider.of<RefereeModel>(context, listen: true);
    var listedReferees = provider.listedReferees;
    var t = AppLocalizations.of(layout.context);

    List<String> movelist = ['|'];
    List<DropdownMenuItem<String>> menuItems = [];
    for (var i = 0; i < numTatamis; i++) {
      movelist.add('-> Tatami ${i + 1}');
      menuItems
          .add(DropdownMenuItem<String>(child: Text('-> Tatami ${i + 1}')));
    }
    String dropdownValue = '-> Tatami 1';
    int selectedMenu = 1;

    return Scaffold(
        appBar: AppBar(title: getIcon('referees'), actions: <Widget>[
          Tooltip(
              message: t?.new03c2 ?? 'New',
              waitDuration: Duration(seconds: 1),
              child: ElevatedButton(
                child: getIcon('icon1'), //Text(t?.new03c2 ?? 'New'),
                onPressed: () {
                  newRef(context, provider, listedReferees);
                },
              )),
          PopupMenuButton<int>(
            icon: getIcon('icon2'),
            iconSize: 142,
            //initialValue: 1,
            // Callback that sets the selected popup menu item.
            onSelected: (int item) {
              setState(() {
                selectedMenu = item;
                for (var r in listedReferees) {
                  if (r.selected) {
                    r.tatami = item;
                    r.active = true;
                    r.selected = false;
                  }
                }
                provider.setListedReferees(listedReferees);
              });
            },
            itemBuilder: (BuildContext context) {
              List<PopupMenuEntry<int>> lst = [];
              for (var i = 0; i < numTatamis; i++)
                lst.add(PopupMenuItem<int>(
                  value: i + 1,
                  child: Text('-> Tatami ${i + 1}'),
                ));
              return lst;
            },
          ),
          Tooltip(
              message: t?.fileImport0209 ?? 'File import',
              waitDuration: Duration(seconds: 1),
              child: ElevatedButton(
                child: getIcon(
                    'icon3'), //Text(t?.fileImport0209 ?? 'File import'),
                onPressed: () {
                  readTextFileDialog((String s) {
                    listedReferees = [];
                    LineSplitter ls = new LineSplitter();
                    List<String> lines = ls.convert(s);
                    for (int i = 0; i < lines.length; i++) {
                      var words = lines[i].split(',');
                      if (words.length == 3)
                        listedReferees.add(Referee(
                            words[0].trim(), words[1].trim(), words[2].trim()));
                    }
                    listedReferees.sort(refereeCompareTo);
                    provider.setListedReferees(listedReferees);
                    layout.sendPutToUnqlite(
                        'listedRefs', {'refs': listedReferees});
                    setState(() {});
                  });
                },
              )),
          Tooltip(
              message: t?.distribute8e25 ?? 'Distribute',
              waitDuration: Duration(seconds: 1),
              child: ElevatedButton(
                  child: getIcon(
                      'icon4'), //Text(t?.distribute8e25 ?? 'Distribute'),
                  onPressed: () {
                    listedReferees.sort(refereeCompareTo);
                    // zero
                    for (var r in listedReferees) r.tatami = 0;
                    var t = 1, n = 0;
                    // first round
                    for (var r in listedReferees) {
                      if (r.active == false || r.refereeOk == false) continue;
                      r.tatami = t++;
                      if (t > numTatamis) {
                        t = 1;
                        n++;
                        if (n >= refsPerTatami) break;
                      }
                    }
                    // second round
                    if (n < refsPerTatami) {
                      for (var r in listedReferees) {
                        if (r.active == false ||
                            r.tatami > 0 ||
                            (r.refereeOk == false && r.judgeOk == false))
                          continue;
                        r.tatami = t++;
                        if (t > numTatamis) {
                          t = 1;
                          n++;
                          if (n >= refsPerTatami) break;
                        }
                      }
                    }
                    provider.setListedReferees(listedReferees);
                    layout.sendPutToUnqlite(
                        'listedRefs', {'refs': listedReferees});
                    setState(() {});
                  })),
          Tooltip(
              message: 'Delete for ever',
              waitDuration: Duration(seconds: 1),
              child: ElevatedButton(
                  child: getIcon('icon5'),
                  onPressed: () {
                    showAlertDialog(
                        context, 'Confirm', 'Delete all?', 'Yes', 'No', () {
                      provider.setListedReferees([]);
                      layout.sendDelToUnqlite('listedRefs');
                      layout.sendDelAllToUnqlite('m_');
                      setState(() {});
                    }, () {});
                  })),
          /**
          IconButton(
            icon: const Icon(Icons.add_alert),
            tooltip: 'Show Snackbar',
            onPressed: () {
              ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('This is a snackbar')));
            },
          ),**/
        ]),
        body: Container(
          child: ListView(
            key: UniqueKey(),
            scrollDirection: Axis.vertical,
            shrinkWrap: true,
            children: getRefereeList(provider, listedReferees),
          ),
        ));
  }

  List<EditableListTile> getRefereeList(provider, listedReferees) {
    var rows = <EditableListTile>[];
    print('Get ref list');
    for (var r in listedReferees) {
      rows.add(EditableListTile(
          model: r,
          onChanged: (Referee updatedModel) {
            /// Gets called when the save Icon is tapped on the List Tile.
            provider.setListedReferees(listedReferees);
            widget.layout
                .sendPutToUnqlite('listedRefs', {'refs': listedReferees});
          })); //???
    }
    return rows;
  }

  void newRef(context, provider, listedReferees) {
    showDialog(
        context: context,
        builder: (BuildContext context) {
          var nameController = TextEditingController();
          var clubController = TextEditingController();
          var countryController = TextEditingController();
          return AlertDialog(
            scrollable: true,
            title: Text('New referee'),
            content: Padding(
              padding: const EdgeInsets.all(8.0),
              child: Form(
                child: Column(
                  children: <Widget>[
                    TextFormField(
                      controller: nameController,
                      decoration: InputDecoration(
                        labelText: 'Name',
                        icon: Icon(Icons.account_box),
                      ),
                    ),
                    TextFormField(
                      controller: clubController,
                      decoration: InputDecoration(
                        labelText: 'Club',
                        icon: Icon(Icons.email),
                      ),
                    ),
                    TextFormField(
                      controller: countryController,
                      validator: (String? value) {
                        return (value != null && value.length != 3)
                            ? 'Country must be a 3 letter acronym.'
                            : null;
                      },
                      decoration: InputDecoration(
                        labelText: 'Country',
                        icon: Icon(Icons.flag),
                      ),
                    ),
                  ],
                ),
              ),
            ),
            actions: [
              ElevatedButton(
                  child: Text("Cancel"),
                  onPressed: () {
                    Navigator.of(context).pop();
                  }),
              ElevatedButton(
                  child: Text("Submit"),
                  onPressed: () {
                    var name = nameController.text;
                    var club = clubController.text;
                    var country = countryController.text;
                    Navigator.of(context).pop();
                    listedReferees.insert(0, Referee(name, club, country));
                    provider.setListedReferees(listedReferees);
                    widget.layout.sendPutToUnqlite(
                        'listedRefs', {'refs': listedReferees});
                    setState(() {});
                  })
            ],
          );
        });
  }
}
