
import 'package:flutter/material.dart';

import 'comm.dart';
import 'database.dart';

// Provider.of<JudokaListModel>(context, listen: false).db.createOrUpdateJudoka(c.toCompanion(true));

class JudokaListModel extends ChangeNotifier {
  final JudoDatabase db;
  late final Stream<List<Judoka>> _currentEntries;

  Stream<List<Judoka>> get homeScreenEntries => _currentEntries;

  String _ixtext = '';
  String get ixtext => _ixtext;
  setix(String ixstr) {
    _ixtext = ixstr;
    notifyListeners();
  }
  String _lasttext = '';
  String get lasttext => _lasttext;
  setlast(String str) {
    _lasttext = str;
    notifyListeners();
  }
  String _firsttext = '';
  String get firsttext => _firsttext;
  setfirst(String str) {
    _firsttext = str;
    notifyListeners();
  }
  String _regcattext = '';
  String get regcattext => _regcattext;
  setregcat(String str) {
    _regcattext = str;
    notifyListeners();
  }
  String _weighttext = '';
  String get weighttext => _weighttext;
  setweight(String str) {
    _weighttext = str;
    notifyListeners();
  }

  String _svgstr = '';
  String get svgstr => _svgstr;
  setsvgstr(String str) {
    _svgstr = str;
    notifyListeners();
  }

  JudokaListModel() : db = JudoDatabase() {
    _currentEntries = db.watchJudokaEntries();
    _ixtext = '';
  }

  void removeAll() {
    db.deleteEverything();
  }

  void removeDatabase() {
    db.removeDB();
  }

  void close() {
    db.close();
  }

  Future<Judoka?> getJudoka(int ix) async {
    return await db.getJudoka(ix);
  }

  Future<Judoka?> getJudokaDbOrJs(int ix, {bool? save}) async {
    Judoka? j = await db.getJudoka(ix);
    if (j == null) {
      var json = await getJudokaJs(ix);
      //{index: 71, last: ALATALO, first: Rasmus 1, club: Finndai, country: FIN, regcat: M-60, cat: M-60, weight: 59000, ecat: M-60}
      if (json != null) {
        j = getNewJudoka(json);
        if (save == true) {
          db.createOrUpdateJudoka(j.toCompanion(false));
        }
      }
    }
    return j;
  }
}
