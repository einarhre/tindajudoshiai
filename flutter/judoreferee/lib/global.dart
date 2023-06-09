
class Referee {
  String name, club, country;
  int ref = 0, judg1 = 0, judg2 = 0;
  int tatami = 0;
  bool selected = false;
  bool active = true;
  int flags = REFEREE_OK | JUDGE_OK;

  Referee(this.name, this.club, this.country);

  bool get refereeOk => (flags & REFEREE_OK) != 0;
  bool get judgeOk => (flags & JUDGE_OK) != 0;

  void setRefereeOk(bool ok) {
    if (ok) flags |= REFEREE_OK;
    else flags &= ~REFEREE_OK;
  }

  void setJudgeOk(bool ok) {
    if (ok) flags |= JUDGE_OK;
    else flags &= ~JUDGE_OK;
  }

  toJson() => {
    'name': name,
    'club': club,
    'country': country,
    'tatami': tatami,
    'numref': ref,
    'numjudg1': judg1,
    'numjudg2': judg2,
    'active': active,
    'flags': flags,
  };
/*
  @override
  int compareTo(Referee other) {
    int a = country.compareTo(other.country);
    int b = club.compareTo(other.club);
    if (a == 0) return b;
    return a;
  }

 */
}

const REFEREE_OK = 1;
const JUDGE_OK = 2;

int refereeCompareTo(Referee mine, Referee other) {
  int a = mine.country.compareTo(other.country);
  int b = mine.club.compareTo(other.club);
  if (a == 0) return b;
  return a;
}

class TatamiReferees {
  var referees = new List.generate(NUM_REFEREES, (_) => Referee('', '', ''));
}

class Match {
  int comp1, comp2, cat, number, round;
  //Referee? referee = null, judge1 = null, judge2 = null;

  Match(this.cat, this.number, this.comp1, this.comp2, this.round);
  /*
  void setReferee(Referee ref) {
    this.referee = ref;
  }
  void setJudge1(Referee ref) {
    this.judge1 = ref;
  }
  void setJudge2(Referee ref) {
    this.judge2 = ref;
  }
*/
  toJson() => {
    'c': cat,
    'n': number,
    'c1': comp1,
    'c2': comp2,
    /*
    'r': referee?.name ?? '',
    'j1': judge1?.name ?? '',
    'j2': judge2?.name ?? '',
     */
  };
}

class TatamiMatches {
  var matches = new List.generate(11, (_) => Match(0, 0, 0, 0, 0));
}

class Judoka {
  int ix;
  String first, last, club, country;
  Judoka(this.ix, this.first, this.last, this.club, this.country);
}

class CategoryDef {
  int ix;
  String name;
  CategoryDef(this.ix, this.name);
}

class RefTeam {
  String ref, judge1, judge2;
  RefTeam(this.ref, this.judge1, this.judge2);
}

String node_name = 'judoshiai.local';
//var tatami = 1;
//var judokaInfo = new Map();
//var categoryInfo = new Map();
//var matches = new List.generate(20, (_) => TatamiMatches());
var textSize = 18.0;
var tatamiClicked = new List.filled(20, false, growable: false);
bool fullscreen = false;
bool mirror = false;
var jspassword = '';
var numTatamis = 3;
var refsPerTatami = 5;
const int NUM_REFEREES = 5;
var allReferees = new List.generate(20, (_) => TatamiReferees());

