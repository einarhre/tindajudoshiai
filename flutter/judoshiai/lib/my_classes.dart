// [["index","last","first","birthyear","belt","club","regcategory","weight","visible","category","deleted","country","id","seeding","clubseeding","comment","coachid"],

import 'dart:collection';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:judolib/judolib.dart';
import 'global.dart';
import 'settings.dart';
import 'package:http/http.dart' as http;
import 'utils.dart';
import 'package:shared_preferences/shared_preferences.dart';

class JSCompetitor {
  int index;
  String last;
  String first;
  int birthyear;
  int belt;
  String club;
  String regcategory;
  int weight;
  int visible;
  String category;
  int deleted;
  String country;
  String id;
  int seeding;
  int clubseeding;
  String comment;
  String coachid;
  bool selected;
  late DataRow row;

  final List<String> genderOptions = ['?', 'Male', 'Female'];
  final List<String> gradeOptions = ['?', '6.kyu', '5.kyu', '4.kyu', '3.kyu',
    '2.kyu', '1.kyu', '1.dan', '2.dan', '3.dan', '4.dan',
    '5.dan', '6.dan', '7.dan', '8.dan', '9.dan'];
  final List<String> seedingOptions = ['No seeding', '1', '2', '3', '4', '5', '6', '7', '8'];
  final List<String> controlOptions = ['?', 'OK', 'NOK'];

  JSCompetitor(
      this.index, //0
      this.last,
      this.first,
      this.birthyear,
      this.belt,
      this.club, //5
      this.regcategory,
      this.weight,
      this.visible,
      this.category,
      this.deleted, //10
      this.country,
      this.id,
      this.seeding,
      this.clubseeding,
      this.comment, //15
      this.coachid,
      {this.selected = false});

  String getGenderStr() {
    if ((deleted & 0x80) != 0) return 'Male';
    if ((deleted & 0x100) != 0) return 'Female';
    return '?';
  }

  void setGenderStr(String g) {
    deleted = deleted & ~0x180;
    if (g == 'Male') deleted |= 0x80;
    else if (g == 'Female') deleted |= 0x100;
  }

  String getControlStr() {
    if ((deleted & 0x20) != 0) return 'OK';
    if ((deleted & 0x40) != 0) return 'NOK';
    return '?';
  }

  void setControlStr(String g) {
    deleted = deleted & ~0x60;
    if (g == 'OK') deleted |= 0x20;
    else if (g == 'NOK') deleted |= 0x40;
  }

  bool getHansokumake() {
    return (deleted & 2) != 0;
  }

  void setHansokumake(bool a) {
    deleted = deleted & ~2;
    if (a) deleted |= 2;
  }

  bool getNoShow() {
    return (deleted & 0x400) != 0;
  }

  void setNoShow(bool a) {
    deleted = deleted & ~0x400;
    if (a) deleted |= 0x400;
  }

  String getWeightStr() {
    return (weight/1000).toString();
  }

  void setWeightStr(String w) {
    var kg = double.parse(w);
    weight = (kg*1000).toInt();
  }

  Future<void> save() async {
    var hostname = await getHostName('jsip');
    var response = await http.post(
      Uri.parse('http://$hostname:8088/json'),
      body: json.encode({
        'op': 'setcomp', 'pw': jspassword, 'ix': index, 'last': last, 'first': first,
        'birthyear': birthyear, 'belt': belt, 'club': club,
        'country': country, 'regcat': regcategory, 'category': category,
        'weight': weight, 'seeding': seeding, 'clubseeding': clubseeding,
        'id': id, 'coachid': coachid, 'flags': deleted, 'comment': comment
      }),
    );
    if (response.statusCode == 200) {
      print('SEND OK');
    } else{
      print('SEND FAILED');
    }
  }

  void saveValue(Map<String, dynamic>? a) {
    last = a?['last'] ?? '';
    first = a?['first'] ?? '';
    club = a?['club'] ?? '';
    country = a?['country'] ?? '';
    regcategory = a?['regcat'] ?? '';
    category = a?['category'] ?? '';
    id = a?['id'] ?? '';
    coachid = a?['coachid'] ?? '';
    setGenderStr(a?['gender'] ?? '?');
    setControlStr(a?['control'] ?? '?');
    setHansokumake(a?['hansokumake'] ?? false);
    setNoShow(a?['hide'] ?? false);
    setWeightStr(a?['weight'] ?? '0');
  }
}

class JSCategory {
  int index;
  String category;
  int tatami;
  int group;
  int system;
  int numcomp;
  int table;
  int wishsys;
  int status;
  int count;
  int matchedcnt;
  String sysdescr;
  bool isExpanded = false;
  List<JSCompetitor> competitors = [];

  JSCategory(this.index, this.category, this.tatami, this.group,
      this.system, this.numcomp, this.table, this.wishsys, this.status,
      this.count, this.matchedcnt, this.sysdescr) {
    /***
        print('CAT ${this.index}, ${this.category}, ${this.tatami}, ${this.group},'
        '${this.system}, ${this.numcomp}, ${this.table}, ${this.wishsys}, ${this.status},'
        '${this.count}, ${this.matchedcnt}, ${this.sysdescr}');
     ***/
  }

  void addnew(JSCompetitor comp) {
    competitors.add(comp);
    //print('cats len=${categories.length}');
  }

  Color getCatColor() {
    const int MATCH_EXISTS = 1;
    const int MATCH_MATCHED = 2;
    const int MATCH_UNMATCHED = 4;
    const int CAT_PRINTED = 8;
    const int REAL_MATCH_EXISTS = 16;
    const int SYSTEM_DEFINED = 32;

    if (((status & SYSTEM_DEFINED) != 0 && (status & MATCH_UNMATCHED) == 0)) {
      return Colors.green;
    } else if ((status & MATCH_MATCHED) != 0) {
      return Colors.orange;
    } else if ((status & REAL_MATCH_EXISTS) != 0) {
      return Colors.yellow;
    }
    return Colors.white;
  }

  void sort() {
    competitors.sort((a, b) {
      if (a.last == b.last)
        return a.first.compareTo(b.first);
      return a.last.compareTo(b.last);
    });
  }

  void sortByWeight() {
    competitors.sort((a, b) => a.weight.compareTo(b.weight));
  }

  void sortByClub() {
    competitors.sort((a, b) => a.club.compareTo(b.club));
  }

  void sortByCountry() {
    competitors.sort((a, b) => a.country.compareTo(b.country));
  }
}

class JSCategories {
  List<JSCategory> categories = [];
  int _currentSortColumn = 0;
  bool _isSortAsc = true;

  void addnew(JSCategory cat) {
    categories.add(cat);
    //print('cats len=${categories.length}');
  }

  List<int> getSelected() {
    var sel = <int>[];
    var len = categories.length;
    int i;
    for (i = 0; i < len; i++) {
      JSCategory cat = categories[i];
      int clen = cat.competitors.length;
      int j;
      for (j = 0; j < clen; j++) {
        JSCompetitor c = cat.competitors[j];
        if (c.selected) {
          sel.add(c.index);
        }
      }
    }
    return sel;
  }

  void unselectAll() {
    var len = categories.length;
    int i;
    for (i = 0; i < len; i++) {
      JSCategory cat = categories[i];
      int clen = cat.competitors.length;
      int j;
      for (j = 0; j < clen; j++) {
        JSCompetitor c = cat.competitors[j];
        c.selected = false;
      }
    }
  }

  String catIxToStr(int ix) {
    var len = categories.length;
    int i;
    for (i = 0; i < len; i++) {
      JSCategory cat = categories[i];
      if (cat.index == ix) return cat.category;
    }
    return '';
  }

  List<String> getCategoryNames() {
    List<String> names = [];
    var len = categories.length;
    int i;
    for (i = 0; i < len; i++) {
      JSCategory cat = categories[i];
      names.add(cat.category);
    }
    return names;
  }

  void sort() {
    categories.sort((a, b) => a.category.compareTo(b.category));
  }

  Future<void> saveExpanded() async {
    final prefs = await SharedPreferences.getInstance();
    var e = <String>[];
    var len = categories.length;
    int i;
    for (i = 0; i < len; i++) {
      JSCategory cat = categories[i];
      if (cat.isExpanded) e.add(cat.category);
    }
    await prefs.setStringList('expandedCats', e);
  }

  Future<void> getExpanded() async {
    final prefs = await SharedPreferences.getInstance();
    final List<String>? e = prefs.getStringList('expandedCats');
    if (e != null) {
      var len = categories.length;
      int i;
      for (i = 0; i < len; i++) {
        var ix = e.indexOf(categories[i].category);
        if (ix >= 0)
          categories[i].isExpanded = true;
      }
    }
  }
}
