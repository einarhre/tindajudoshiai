import 'dart:html' as html;
import 'package:shared_preferences/shared_preferences.dart';

Future<String> getHostName(String key) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  if (prefs != null) {
    var host = prefs.getString(key);
    if (host != null && host != '0.0.0.0') {
      return host;
    }
  }
  String loc = html.window.location.toString();
  var i = loc.indexOf('//', 0);
  var j = loc.indexOf(':', i+2);
  return '${loc.substring(i+2, j)}';
}

String getSsdpAddress(String lookfor) {
  return '';
}

void ssdp(String lookfor) {
}
