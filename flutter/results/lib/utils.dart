import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:image/image.dart' as img;
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:judolib/judolib.dart';
import 'package:provider/provider.dart';
import 'package:image_size_getter/image_size_getter.dart';
import 'dart:ui' as ui;
import 'bloc.dart';

int competitorComparison(Competitor a, Competitor b) {
  final last = a.last.toLowerCase().compareTo(b.last.toLowerCase());
  if (last != 0) return last;
  return a.first.toLowerCase().compareTo(b.first.toLowerCase());
}

int competitorComparisonByClub(Competitor a, Competitor b) {
  final club = a.club.toLowerCase().compareTo(b.club.toLowerCase());
  if (club != 0) return club;
  final last = a.last.toLowerCase().compareTo(b.last.toLowerCase());
  if (last != 0) return last;
  return a.first.toLowerCase().compareTo(b.first.toLowerCase());
}

int competitorComparisonByCategory(Competitor a, Competitor b) {
  final cat = a.category.compareTo(b.category);
  if (cat != 0) return cat;
  final last = a.last.toLowerCase().compareTo(b.last.toLowerCase());
  if (last != 0) return last;
  return a.first.toLowerCase().compareTo(b.first.toLowerCase());
}

class Category {
  String category;
  int numcomp;
  int clubformat;
  int firstlast;
  List<Competitor> competitors = [];

  Category(this.category, this.numcomp, this.clubformat, this.firstlast);

  void addCompetitor(Competitor comp) {
    competitors.add(comp);
  }

  void sortCompetitors() {
    competitors.sort(competitorComparison);
  }
}

class Competitor {
  String last;
  String first;
  String club;
  String country;
  String belt = '';
  String category = '';
  int pos;
  int index = 0;
  String id;
  String coachid;
  int tatami;
  int waittime;
  int matchnum;
  int round;

  Competitor(this.last, this.first, this.club, this.country, this.pos, this.id, this.coachid,
      this.tatami, this.waittime, this.matchnum, this.round);

  void setIndex(int ix) {
    index = ix;
  }
}

class Club {
  String club;
  String country;
  List<Competitor> competitors = [];

  Club(this.club, this.country);

  void addCompetitor(Competitor comp) {
    competitors.add(comp);
  }

  void sortCompetitors() {
    competitors.sort(competitorComparison);
  }
}

class ClubMedals {
  String club;
  int pos = 0, p1 = 0, p2 = 0, p3 = 0, num = 0;

  ClubMedals(this.club, this.pos, this.p1, this.p2, this.p3, this.num);
}

class CatStatistics {
  String name;
  int competitors;
  int matches;
  int ippons;
  int wazaaris;
  int shidowins;
  int gs;
  int time;

  CatStatistics(this.name, this.competitors, this.matches, this.ippons, this.wazaaris, this.shidowins, this.gs, this.time);
}

class Match {
  int number;
  String category;
  int round = 0;
  String first1, last1, club1;
  String first2, last2, club2;

  Match(this.number, this.category, this.first1, this.last1, this.club1, this.first2, this.last2, this.club2);
}

class TatamiMatches {
  int tatami;
  List<Match> matches = [];

  TatamiMatches(this.tatami);

  void add(Match m) {
    matches.add(m);
  }
}

class Competition {
  List<Category> categories = [];
  List<Competitor> competitors = [];
  List<Club> clubs = [];
  List<ClubMedals> medals = [];
  List<CatStatistics> statistics = [];
  List<TatamiMatches> matches = [];
  var info = {};

  void addCategory(Category cat) {
    categories.add(cat);
  }

  void addCompetitor(Competitor comp) {
    competitors.add(comp);
    var clublist = clubs.where((e) => e.club == comp.club);
    if (clublist.length > 0) {
      clublist.first.addCompetitor(comp);
    } else {
      var club = Club(comp.club, '');
      club.addCompetitor(comp);
      clubs.add(club);
    }
  }

  void addMedals(ClubMedals m) {
    medals.add(m);
  }

  void addStatistics(CatStatistics stat) {
    statistics.add(stat);
  }

  void addMatches(TatamiMatches m) {
    matches.add(m);
  }

  void sortCompetitorsByName() {
    competitors.sort(competitorComparison);
  }
  void sortCompetitorsByCategory() {
    competitors.sort(competitorComparisonByCategory);
  }
  void sortCompetitorsByClub() {
    competitors.sort(competitorComparisonByClub);
  }
}

Future<Competition>? savedCompetition = null;
late String hostUrl;

Future<void> getResults(Competition competition) async {
  competition.categories = [];
  try {
    var response = await http.get(
      Uri.parse('$hostUrl/results.json?a=${DateTime.now().millisecondsSinceEpoch}'),
    );
    if (response.statusCode == 200) {
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      var catlen = json.length;
      for (var i = 0; i < catlen; i++) {
        var cat = json[i];
        var catname = cat['category'];
        var numcomp = cat['numcomp'];
        var clubform = cat['clubtext'];
        var firstlast = cat['nameord'];
        var comps = cat['competitors'];
        var complen = comps.length;
        Category ctg = Category(catname, numcomp, clubform, firstlast);

        for (var j = 0; j < complen; j++) {
          var comp = comps[j];
          var first = comp['first'] ?? '';
          var last = comp['last'] ?? '';
          var club = comp['club'] ?? '';
          var country = comp['country'] ?? '';
          var id = comp['id'] ?? '';
          var coachid = comp['coachid'] ?? '';
          int pos = comp['pos'] ?? 0;
          int ix = comp['ix'] ?? 0;

          var competitor = Competitor(last, first, club, country, pos, id, coachid,
          0, -1, 0, 0);
          competitor.setIndex(ix);
          ctg.addCompetitor(competitor);
        }

        competition.addCategory(ctg);
      }
    }
  } catch (e) {
    print("HTTP results.json error $e");
    //rethrow;
  }
}

Future<void> getCompetitors(Competition competition) async {
  competition.competitors = [];
  try {
    var response = await http.get(
      Uri.parse('$hostUrl/competitors.json?a=${DateTime.now().millisecondsSinceEpoch}'),
    );
    if (response.statusCode == 200) {
      //print('COMPS: ${utf8.decode(response.bodyBytes)}');
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      var len = json.length;
        for (var j = 0; j < len; j++) {
          var comp = json[j];
          var first = comp['first'] ?? '';
          var last = comp['last'] ?? '';
          var club = comp['club'] ?? '';
          var country = comp['country'] ?? '';
          var belt = comp['belt'] ?? '';
          var category = comp['category'] ?? '';
          var id = comp['id'] ?? '';
          var coachid = comp['coachid'] ?? '';

          int pos = comp['pos'] ?? 0;
          int index = comp['ix'] ?? 0;

          int tatami = comp['tatami'] ?? 0;
          int waittime = comp['waittime'] ?? -1;
          int matchnum = comp['matchnum'] ?? 0;
          int round = comp['round'] ?? 0;

          var competitor = Competitor(last, first, club, country, pos, id, coachid,
          tatami, waittime, matchnum, round);
          competitor.index = index;
          competitor.belt = belt;
          competitor.category = category;
          competition.addCompetitor(competitor);

        }
    }
  } catch (e) {
    print("HTTP competitors.json error $e");
    //rethrow;
  }
}

Future<void> getMedals(Competition competition) async {
  competition.medals = [];
  try {
    var response = await http.get(
      Uri.parse('$hostUrl/medals.json?a=${DateTime.now().millisecondsSinceEpoch}'),
    );
    if (response.statusCode == 200) {
      //print('COMPS: ${utf8.decode(response.bodyBytes)}');
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      var len = json.length;
      for (var j = 0; j < len; j++) {
        var m = json[j];
        var club = m['club'] ?? '';
        var pos = m['pos'] ?? 0;
        var p1 = m['p1'] ?? 0;
        var p2 = m['p2'] ?? 0;
        var p3 = m['p3'] ?? 0;
        var num = m['numcompetitors'] ?? 0;

        var medals = ClubMedals(club, pos, p1, p2, p3, num);
        competition.addMedals(medals);
      }
    }
  } catch (e) {
    print("HTTP medals.json error $e");
    //rethrow;
  }
}

var statisticsExist = false;

Future<void> getStatistics(Competition competition, t) async {
  statisticsExist = false;
  competition.statistics = [];
  try {
    var response = await http.get(
      Uri.parse('$hostUrl/statistics.json?a=${DateTime.now().millisecondsSinceEpoch}'),
    );
    if (response.statusCode == 200) {
      //print('COMPS: ${utf8.decode(response.bodyBytes)}');
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      var len = json.length;
      for (var j = 0; j < len; j++) {
        var m = json[j];
        var name = m['cat'] ?? '';
        if (name == 'total') name = t?.restxt36 ?? 'Total';
        var numcomp = m['competitors'] ?? 0;
        var matches = m['matches'] ?? 0;
        var ippons = m['ippons'] ?? 0;
        var wazaaris = m['wazaaris'] ?? 0;
        var shidoswins = m['shidowins'] ?? 0;
        var gs = m['goldenscores'] ?? 0;
        var time = m['time'] ?? 0;

        var stat = CatStatistics(name, numcomp, matches, ippons,
            wazaaris, shidoswins, gs, time);
        competition.addStatistics(stat);
      }
      if (len > 0)
        statisticsExist = true;
    }
  } catch (e) {
    print("HTTP statistics.json error $e");
    //rethrow;
  }
}

Future<void> getInfo(Competition competition, t) async {
  try {
    var response = await http.get(
      Uri.parse('$hostUrl/info.json?a=${DateTime.now().millisecondsSinceEpoch}'),
    );
    if (response.statusCode == 200) {
      //print('COMPS: ${utf8.decode(response.bodyBytes)}');
      Map<String, dynamic> json = jsonDecode(utf8.decode(response.bodyBytes));
      for (var k in json.keys) {
        var value = json[k];
        print('$k = $value');
        competition.info[k] = value;
      }
    }
  } catch (e) {
    print("HTTP statistics.json error $e");
    //rethrow;
  }
}

Future<void> getMatches(BuildContext context) async {
  var provider = Provider.of<CompetitionModel>(context, listen: false);
  provider.setTatamiMatches([]);

  try {
    var response = await http.get(
      Uri.parse('$hostUrl/nextmatches.json?a=${DateTime.now().millisecondsSinceEpoch}'),
    );
    if (response.statusCode == 200) {
      //print('MATCHES: ${utf8.decode(response.bodyBytes)}');
      List<TatamiMatches> lst = [];
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      var len = json.length;
      for (var j = 0; j < len; j++) {
        var t = json[j];
        var tatami = t['tatami'] ?? 0;
        if (tatami > 0) {
          var tm = TatamiMatches(tatami);
          var matches = t['matches'];
          var mlen = matches.length;
          for (var k = 0; k < mlen; k++) {
            var m = matches[k];
            var num = m['num'] ?? 0;
            tm.add(Match(num, m['cat'] ?? '', m['first1'] ?? '', m['last1'] ?? '', m['club1'] ?? '', m['first2'] ?? '', m['last2'] ?? '', m['club2'] ?? ''));
          }
          lst.add(tm);
        }
      }
      provider.setTatamiMatches(lst);
    }
  } catch (e) {
    print("HTTP nextmatches.json error $e");
    //rethrow;
  }
}

Future<Competition> getFullCompetition(BuildContext context) async {
  Competition competition = Competition();
  var t = AppLocalizations.of(context);
  var provider = Provider.of<CompetitionModel>(context, listen: false);
  provider.setCompetition(competition);

  hostUrl = getUrl();
  print('hostUrl=$hostUrl');

  await getInfo(competition, t);
  await getResults(competition);
  await getCompetitors(competition);
  await getMedals(competition);
  await getStatistics(competition, t);
  await getMatches(context);

  return competition;
}

Future<void> readSettings() async {
  languageCode = await getVal('languageCode', 'en');
}

class NamePosition {
  int x1, y1, x2, y2, ix;
  NamePosition(this.x1, this.y1, this.x2, this.y2, this.ix);
}

class CategoryImage {
  double width = 0, height = 0;
  Image? netImg;
  img.Image? imgImg;
  ui.Image? uiImg;
  List<NamePosition> positions;
  Uint8List? bytes;

  CategoryImage(this.netImg, this.imgImg, this.uiImg, this.bytes, this.positions);

  void setSize(Size s) {
    width = s.width.toDouble();
    height = s.height.toDouble();
  }
}

Future<List<NamePosition>> getNamePositions(String cat, int page) async {
  List<NamePosition> lst = [];
  var map = categoryToUrl(cat, page, 'map') + '?a=${DateTime.now().millisecondsSinceEpoch}';

  try {
    var response = await http.get(
      Uri.parse(map),
    );
    if (response.statusCode == 200) {
      String data = utf8.decode(response.bodyBytes);
      //print('LINES=$data');
      LineSplitter ls = new LineSplitter();
      List<String> lines = ls.convert(data);
      for (var i = 0; i < lines.length; i++) {
        List<String> s = lines[i].split(',');
        var n = NamePosition(int.parse(s[0]), int.parse(s[1]), int.parse(s[2]), int.parse(s[3]), int.parse(s[4]));
        lst.add(n);
      }
    }
  } catch (e) {
    print("HTTP map error $e");
  }
  return lst;
}

Future<img.Image?> getImage(String cat, int page) async {
  var png = (page == 0)
      ? '$hostUrl/${str2hex(cat)}.png'
      : '$hostUrl/${str2hex(cat)}-$page.png';

  try {
    var response = await http.get(
      Uri.parse(png),
    );
    if (response.statusCode == 200) {
      img.Image? image = img.decodeImage(response.bodyBytes);
      return image;
    }
  } catch (e) {
    print("HTTP map error $e");
  }
  return null;
}

/***
Future<Image?> getPngImage(BuildContext context, double width, double height, String cat, int page) async {
  var png = (page == 0)
      ? '$hostUrl/${str2hex(cat)}.png'
      : '$hostUrl/${str2hex(cat)}-$page.png';
  try {
    var response = await http.get(Uri.parse(png));
    if (response.statusCode == 200) {
      return Image.memory(response.bodyBytes, width: width, height: height,
          alignment: Alignment(-1.0, -1.0), fit: BoxFit.contain);
    }
  } catch (e) {
    print("HTTP png error $e");
  }
  return null;
}
***/

Future<Uint8List?> getNetBytes(BuildContext context, double width, double height, String cat, int page) async {
  var png = categoryToUrl(cat, page, 'png') + '?a=${DateTime
      .now()
      .millisecondsSinceEpoch}';

  try {
    var response = await http.get(Uri.parse(png));
    if (response.statusCode == 200) {
      return response.bodyBytes;
    }
  } catch (e) {
    print("HTTP png error $e");
  }
  return null;
}

Future<Image?> getNetImage(BuildContext context, double width, double height, String cat, int page) async {
  var png = categoryToUrl(cat, page, 'png') + '?a=${DateTime.now().millisecondsSinceEpoch}';

  try {
    var response = await http.get(Uri.parse(png));
    if (response.statusCode == 200) {
      return Image.memory(response.bodyBytes, width: width, height: height,
          alignment: Alignment(-1.0, -1.0), fit: BoxFit.contain);
    }
  } catch (e) {
    print("HTTP png error $e");
  }

  /*
  try {
    var svg = (page == 0)
        ? '$hostUrl/${str2hex(cat)}.svg'
        : '$hostUrl/${str2hex(cat)}-$page.svg';

    var response = await http.get(
      Uri.parse(svg),
    );
    print('RESPONSE CODE = ${response.statusCode}');
    if (response.statusCode == 200) {
      Uint8List s = await svgToPng(context, utf8.decode(response.bodyBytes));
      return Image.memory(s);
    }
  } catch (e) {
    print("HTTP svg error $e");
  }
   */

  return null;
}

Future<void> getCatgoryImage(BuildContext context, double width, double height, cat) async {
  var provider = Provider.of<CompetitionModel>(context, listen: false);
  List<CategoryImage> ilist = [];
  var i = 0;
  while (i < 10) {
    //Image? netImg = await getNetImage(context, width, height, cat, i);
    Uint8List? bytes = await getNetBytes(context, width, height, cat, i);
    if (bytes == null)
      break;

    List<NamePosition> positions = await getNamePositions(cat, i);

    Image netImg = Image.memory(bytes);
    var ci = CategoryImage(netImg, null, null, bytes, positions);
    Size s = ImageSizeGetter.getSize(MemoryInput(bytes));
    ci.setSize(s);
    ilist.add(ci);
    i++;
  }
  provider.setCategoryImages(ilist);
}

