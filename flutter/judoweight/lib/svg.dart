import 'package:flutter/services.dart';
import 'package:intl/intl.dart';
import 'package:judolib/judolib.dart';
import 'package:judoweight/print.dart';

List<SvgTextSegment> svgSegments = [];

Future<void> printSvg(int ix, int weight, String last, String first,
    String club, String country) async {
  DateTime eventDate = DateTime.now();
  String formattedDate = DateFormat('yyyy-MM-dd kk:mm:ss').format(eventDate);
  String r = '';

  for (var txt in svgSegments) {
    var a = txt.args;
    if (a.length == 0) {
      r += txt.after;
      continue;
    }
    var code = a[0];
    if (code == 'd') {
      r += formattedDate;
    } else if (code == 'c') {
      for (var i = 1; i < a.length; i++) {
        var t = a[i];
        if (t == 'weight') r += '${weight / 1000}';
        else if (t == 'last') r += last;
        else if (t == 'first') r += first;
        else if (t == 'club') r += club;
        else if (t == 'country') r += country;
        else if (t == 's') r += ' ';
      }
    }
    r += txt.after;
  }

  printSvgStr(r);
}

Future<void> readSvgString() async {
  final svgstr = await rootBundle.loadString('assets/weight-label.svg');
  var s = SvgParse(svgstr);
  svgSegments = s.varTexts;
}
