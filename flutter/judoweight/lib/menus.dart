import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:judolib/judolib.dart';
import 'package:judoweight/bloc.dart';
import 'package:judoweight/database.dart';
import 'package:http/http.dart' as http;
import 'package:judoweight/homescreen.dart';
import 'package:judoweight/print.dart';
import 'package:provider/provider.dart';

import 'main.dart';
import 'settings.dart';

Future<void> showPopupMenu(context) async {
  var t = AppLocalizations.of(context);
  double left = 0.0; //offset.dx;
  double top = 20.0;
  double h = 32.0;
  await showMenu(
      context: context,
      position: RelativeRect.fromLTRB(left, top, 0, 0),
      items: [
        PopupMenuItem<String>(
            height: h,
            child: Text('Load competitors'),
            value: 'load'),
        PopupMenuItem<String>(
            height: h,
            child: Text('Delete all'),
            value: 'deleteall'),
        PopupMenuItem<String>(
            height: h,
            child: Text('Delete Database'),
            value: 'deletedb'),
        PopupMenuItem<String>(
            height: h,
            child: Text('Upload'),
            value: 'upload'),
        PopupMenuItem<String>(
            height: h,
            child: Text('Save to file'),
            value: 'save'),
        PopupMenuItem<String>(
            height: h,
            child: Text('Print'),
            value: 'print'),
        PopupMenuItem<String>(
            height: h,
            child: Text('Exit'),
            value: 'exit'),

      ]).then((value) async {
    print('VALUE=$value');
    switch (value) {
      case 'load':
        loadCompetitors(context);
        break;
      case 'deleteall':
        Provider.of<JudokaListModel>(context, listen: false).removeAll();
        break;
      case 'deletedb':
        Provider.of<JudokaListModel>(context, listen: false).removeDatabase();
        Navigator.pushReplacement(
            context,
            MaterialPageRoute(
                builder: (BuildContext context) => MyApp()));
        break;
      case 'save':
        Provider.of<JudokaListModel>(context, listen: false).db.saveToFile();
        break;
      case 'upload':
        Provider.of<JudokaListModel>(context, listen: false).db.uploadToJudoshiai();
        break;
      case 'print':
        print('${await listPrinters()}');
        break;
      case 'exit':
        SystemChannels.platform.invokeMethod('SystemNavigator.pop');
        break;
    }
  });
}

Future<void> loadCompetitors(context) async {
  var host = await getHostName('jsip');
  try {
    //print('WEB REQ host=$host');
    var response = await http.post(
      Uri.parse('http://$host:8088/json'),
      body: '{"op":"sql", "pw": "${jspassword}", "cmd":"select * from competitors"}',
    );
    if (response.statusCode == 200) {
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      //print('json=$json');
      //_cats = JSCategories() as Future<JSCategories>?;
      var len = json.length;
      int i;
      for (i = 1; i < len; i++) {
        var lst = json[i];
        var f = int.parse(lst[10]);
        if ((f & DELETED) != 0) continue; // deleted

        int ix = int.parse(lst[0]);
        int weight = int.parse(lst[7]);
        int saved = DB_SAVED;
        Judoka? j = await Provider.of<JudokaListModel>(context, listen: false).getJudoka(ix);
        if (j != null && j.weight > 0) {
          weight = j.weight;
          saved = 0;
        }

        //print('2: lst=$lst');
        Judoka c = Judoka(
            ix: ix,
            last: lst[1],
            first: lst[2],
            birthyear: int.parse(lst[3]),
            belt: int.parse(lst[4]),
            club: lst[5],
            regcat: lst[6],
            weight: weight,
            category: lst[9],
            flags: f | saved,
            country: lst[11],
            id: lst[12],
            seeding: int.parse(lst[13]),
            clubseeding: int.parse(lst[14]),
            gender: (f & GENDER_MALE) != 0 ? IS_MALE : ((f & GENDER_FEMALE) != 0 ? IS_FEMALE : 0),
            comment: lst[15],
            coachid: lst[16]);
        Provider.of<JudokaListModel>(context, listen: false).db.createOrUpdateJudoka(c.toCompanion(true));
        //db.addJudoka(c.toCompanion(true));
      }
    }

  } catch (e) {
    print("HTTP cats error $e");
    //rethrow;
  }

}