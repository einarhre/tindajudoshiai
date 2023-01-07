import 'dart:convert';
import 'dart:html' as html;
import 'dart:html';
import 'package:shared_preferences/shared_preferences.dart';

Future<String> getHostName(String key) async {
  /*
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  if (prefs != null) {
    var host = prefs.getString(key);
    if (host != null && host != '0.0.0.0') {
      return host;
    }
  }
   */
  String loc = html.window.location.toString();
  var i = loc.indexOf('//', 0);
  var j = loc.indexOf(':', i+2);
  return '${loc.substring(i+2, j)}';
}

String getLocation() {
  String loc = html.window.location.toString();
  var i = loc.indexOf('//', 0);
  var j = loc.indexOf(':', i+2);
  return '${loc.substring(i+2, j)}';
}

String getUrl() {
  String r = '';
  String loc = html.window.location.toString();
  final len = loc.length;
  if (loc[len-1] == '/') {
    loc = loc.substring(0, len-1);
  }
  final last = loc.lastIndexOf('/');
  if (last > 0)
    r = loc.substring(0, last);
  else
    r = loc;
  return r;
  /*
  var i = loc.indexOf('//', 0);
  var j = loc.indexOf('/', i+2);
  return '${loc.substring(0, j)}';

   */
}

String getSsdpAddress(String lookfor) {
  return '';
}

void ssdp(String lookfor) {
}

void goFullScreen() {
  document.documentElement?.requestFullscreen();
}

void exitFullScreen() {
  document.exitFullscreen();
}

void showPdf(List<int> pdf) {
  final blob = html.Blob([pdf], 'application/pdf');
  final url = html.Url.createObjectUrlFromBlob(blob);
  html.window.open(url, '_blank');
  html.Url.revokeObjectUrl(url);
}

void saveTextFile(String text, String filename) {
  html.AnchorElement()
    ..href = '${Uri.dataFromString(text, mimeType: 'text/plain', encoding: utf8)}'
    ..download = filename
    ..style.display = 'none'
    ..click();
}

