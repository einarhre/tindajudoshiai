import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:judolib/judolib.dart';

import 'global.dart';

class RefereeModel extends ChangeNotifier {
  List<Referee> _listedReferees = [];
  List<TatamiMatches> _matches = new List.generate(20, (_) => TatamiMatches());
  Map _judokaInfo = new Map();
  Map _categoryInfo = new Map();
  Map _matchRef = new Map();

  /**/
  Map get matchRef => _matchRef;

  putMatchRef(int cat, int num, RefTeam rt) {
    _matchRef[cat+num] = rt;
    notifyListeners();
  }

  RefTeam? getMatchRef(int cat, int num) {
    return _matchRef[cat+num];
  }

  void delMatchRef(int cat, int num) {
    _matchRef.remove(cat+num);
    notifyListeners();
  }
  /**/
  List<Referee> get listedReferees => _listedReferees;

  setListedReferees(List<Referee> r) {
    _listedReferees = r;
    notifyListeners();
  }

  Referee? getRefereeByName(String name) {
    for (var r in _listedReferees) {
      if (r.name == name) return r;
    }
    return null;
  }
  /**/
  List<TatamiMatches> get matches => _matches;

  setMatches(List<TatamiMatches> ms) {
    _matches = ms;
    notifyListeners();
  }

  putMatch(Match m, int t, int p) {
    _matches[t].matches[p] = m;
    notifyListeners();
  }
  /**/
  Map get judokaInfo => _judokaInfo;

  Judoka? getJudokaInfo(int ix) {
    return _judokaInfo[ix];
  }

  putJudokaInfo(int ix, Judoka j) {
    _judokaInfo[ix] = j;
    notifyListeners();
  }
  /**/
  Map get categoryInfo => _categoryInfo;

  CategoryDef? getCategoryInfo(int ix) {
    return _categoryInfo[ix];
  }

  putCategoryInfo(int ix, CategoryDef c) {
    _categoryInfo[ix] = c;
    notifyListeners();
  }
  /**/
}
