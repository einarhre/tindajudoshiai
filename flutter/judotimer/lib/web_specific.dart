import 'package:shared_preferences/shared_preferences.dart';
import 'dart:html' as html;

Future<String> getHostName1() async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  if (prefs != null) {
    var host = prefs.getString('jsip');
    if (host != null && host != '0.0.0.0') {
      print('host=$host');
      return host;
    }
  }
  String loc = html.window.location.toString();
  print('loc=$loc');
  var i = loc.indexOf('//', 0);
  var j = loc.indexOf(':', i+2);
  print('r=${loc.substring(i+2, j)}');
  return '${loc.substring(i+2, j)}';
}
