
class Match {
  int comp1, comp2, cat, number, round;
  Match(this.cat, this.number, this.comp1, this.comp2, this.round);
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

String node_name = 'judoshiai.local';
var tatamis = new List.filled(20, false, growable: false);
var judokaInfo = new Map();
var categoryInfo = new Map();
var matches = new List.generate(20, (_) => TatamiMatches());
var textSize = 18.0;
var tatamiClicked = new List.filled(20, false, growable: false);
bool fullscreen = false;
bool mirror = false;
var jspassword = '';
