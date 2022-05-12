import 'package:shared_preferences/shared_preferences.dart';

Future<String> getHostName1() async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  if (prefs != null) {
    var host = prefs.getString('jsip');
    if (host != null && host != '0.0.0.0') {
      print('host=$host');
      return host;
    }
  }
  return 'judoshiai.local';
}
