



import 'package:flutter/material.dart';

import 'global.dart';

int sameCountry(Referee r, Judoka? c1, Judoka? c2) {
  int res = 0;

  if (c1 != null) {
    if (r.country.length > 0 &&
        c1.country.length > 0 &&
        r.country == c1.country) res += 2;

    if (r.club.length > 0 && c1.club.length > 0 && r.club == c1.club)
      res += 1;
  }

  if (c2 != null) {
    if (r.country.length > 0 &&
        c2.country.length > 0 &&
        r.country == c2.country) res += 2;

    if (r.club.length > 0 && c2.club.length > 0 && r.club == c2.club)
      res += 1;
  }

  return res;
}

RefTeam getBestRefTeam(provider, layout, Match m, List<Referee> rs) {
  Judoka? comp1 = layout.getJudoka(provider, m.comp1);
  Judoka? comp2 = layout.getJudoka(provider, m.comp2);
  Referee? bestRef = null, bestJudge1 = null, bestJudge2 = null;
  List<int> same = [];
  for (var r in rs) {
    same.add(sameCountry(r, comp1, comp2));
  }

  int bestvalR = 100000;
  for (var r in rs) {
    var v = sameCountry(r, comp1, comp2) * 1000;
    v += r.ref * 4 + r.judg1 + r.judg2 + (r.refereeOk ? 0 : 10000);
    if (v < bestvalR) {
      bestvalR = v;
      bestRef = r;
    }
  }

  var bestvalJ1 = 100000;
  for (var r in rs) {
    var v = sameCountry(r, comp1, comp2) * 1000;
    v += r.ref * 4 + r.judg1 + r.judg2 + (r.judgeOk ? 0 : 10000);
    if (v < bestvalJ1 && r != bestRef) {
      bestvalJ1 = v;
      bestJudge1 = r;
    }
  }

  var bestvalJ2 = 100000;
  for (var r in rs) {
    var v = sameCountry(r, comp1, comp2) * 1000;
    v += r.ref * 4 + r.judg1 + r.judg2 + (r.judgeOk ? 0 : 10000);
    if (v < bestvalJ2 && r != bestRef && r != bestJudge1) {
      bestvalJ2 = v;
      bestJudge2 = r;
    }
  }
  print("Bestvals ${bestRef?.name ?? 'x'} $bestvalR $bestvalJ1 $bestvalJ2");
  bestRef?.ref++;
  bestJudge1?.ref++;
  bestJudge2?.ref++;
  return RefTeam(
      bestRef?.name ?? '', bestJudge1?.name ?? '', bestJudge2?.name ?? '');
}

void drawReferees(provider, layout, int tatami, bool all) {
  var refs = provider.listedReferees;
  var matches = provider.matches;

  int t = tatami - 1;
  List<Referee> rs = [];

  for (var r in refs) {
    if (r.tatami == t + 1) {
      rs.add(r);
    }
  }

  var lenrs = rs.length;
  var a = 0;
  if (lenrs > 0) {
    for (int i = 1; i < 11; i++) {
      Match m = matches[t].matches[i];
      if (!all) {
        RefTeam? rt1 = provider.getMatchRef(m.cat, m.number);
        if (rt1 != null && rt1.ref.length > 0) continue;
      }
      RefTeam rt = getBestRefTeam(provider, layout, m, rs);
      provider.putMatchRef(m.cat, m.number, rt);
      layout.sendPutToUnqlite('m_${m.cat}_${m.number}',
          {'r': rt.ref, 'j1': rt.judge1, 'j2': rt.judge2});
    }
  }
}

Widget getIcon(String assetName, [double height = 48]) {
  return Image.asset('assets/png/$assetName.png');
}

Widget getIconWithText(String assetName, String txt) {
  return Stack(
      alignment: Alignment.center,
      children: <Widget>[
        Image.asset('assets/png/$assetName.png'),
        Center(child: Text(txt, style: TextStyle(fontSize: 25, color: Colors.white),)),
      ]
  );
}