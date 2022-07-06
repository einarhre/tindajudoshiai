class SvgTextSegment {
  List<String>args;
  String after;

  SvgTextSegment(this.args, this.after);
}

class SvgParse {
  SvgParse(String txt) {
    analyzeString(txt);
  }

  List<SvgTextSegment> _varTexts = [];

  List<SvgTextSegment> get varTexts => _varTexts;

  List<String> splitArg(String s) {
    List<String> r = [];
    bool wasnum = false,
        isnum = false;
    int start = 0;

    for (var i = 0; i < s.length; i++) {
      var c = s[i];
      isnum =
          ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'].indexOf(c) >= 0;
      if (c == '-') {
        r.add(s.substring(start, i));
        start = i + 1;
      } else if (start != i && isnum && !wasnum) {
        r.add(s.substring(start, i));
        start = i;
      } else if (!isnum && wasnum) {
        r.add(s.substring(start, i));
        start = i;
      } else if (i == s.length - 1) {
        r.add(s.substring(start, s.length));
      }
      wasnum = isnum;
    }

    return r;
  }

  void analyzeString(String s) {
    final re = RegExp(r'^([dCMR][0-9]*|[-a-z0-9#]*)');
    final flagRe1 = RegExp(r'href=".*[^%]flag-1.png"');
    final flagRe2 = RegExp(r'href=".*[^%]flag-2.png"');

    s = s.replaceAll(flagRe1, 'href="%flag-1"');
    s = s.replaceAll(flagRe2, 'href="%flag-2"');

    List<String> slist = s.split('%');

    var len = slist.length;

    for (var i = 0; i < len; i++) {
      var a = slist[i];
      List<String> args = [];
      SvgTextSegment t;
      String after = a;

      if (i > 0) {
        final match = re.firstMatch(a);
        if (match != null) {
          final s = match.group(0) ?? '';
          after = a.substring(s.length);
          //args = s.split('-');
          args = splitArg(s);
        }
      }

      t = SvgTextSegment(args, after);
      _varTexts.add(t);
    }
/*
    for (var t in _varTexts) {
      print('${t.args} === ${t.after}');
    }
 */
  }
}
