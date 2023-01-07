import 'package:flutter/material.dart';
import 'package:format/format.dart';
import 'package:judolib/judolib.dart';
import 'package:judoweight/homescreen.dart';
import 'package:provider/provider.dart';

import 'bloc.dart';
import 'competitor_edit.dart';
import 'database.dart';

class JudokaCard extends StatelessWidget {
  final Judoka c;
  static const txtFormat = '{:4d} {:6.2f} kg  {:6} {:-20} {:3} {}';

  JudokaCard(this.c) : super(key: ObjectKey(c.ix));

  @override
  Widget build(BuildContext context) {
    final JudokaListModel myProvider = Provider.of<JudokaListModel>(context, listen: false);
    var color = Colors.white;
    if (c.weight > 0) {
      if ((c.flags & DB_SAVED) != 0) color = Colors.green;
      else color = Colors.yellow;
    }
    return Card(
      color: color,
      child: ListTile(
        title: Row(
          mainAxisSize: MainAxisSize.max,
          children: <Widget>[
            Text(format(txtFormat, c.ix, c.weight/1000,
            c.regcat, '${c.last}, ${c.first}', c.country, c.club),
                style: TextStyle(fontFamily: 'RobotoMono',
                color: overWeight(c.weight, c.regcat)
                    ? Colors.red : Colors.black,
                ),
            ),
            Spacer(),
            IconButton(
              alignment: Alignment.centerRight,
              icon: const Icon(Icons.scale),
              color: Colors.blueGrey,
              onPressed: () {
                tabController.animateTo(0);
                myProvider.setix(c.ix.toString());
                myProvider.setregcat(c.regcat);
                myProvider.setlast(c.last);
                myProvider.setfirst(c.first);
                myProvider.setweight((c.weight/1000).toString());
              },
            ),
            IconButton(
              alignment: Alignment.centerRight,
              icon: const Icon(Icons.edit),
              color: Colors.blueGrey,
              onPressed: () async {
                print('PRESSED');
                final result = await Navigator.push(
                  context,
                  MaterialPageRoute(builder: (context) => CompetitorEdit(c, null)),
                );
                print('EDITED');
              },
            ),
          ],
        ),
      ),
    );
  }

  bool overWeight(int weight, String cat) {
    if (cat == null)
      return false;
    final i = cat.indexOf('-');
    if (i >= 0) {
      final w = int.tryParse(cat.substring(i+1));
      if (w != null) {
        if (weight > w*1000)
          return true;
      }
    }
    return false;
  }
}
