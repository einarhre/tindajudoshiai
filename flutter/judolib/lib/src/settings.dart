import 'package:shared_preferences/shared_preferences.dart';

Future<String> getVal(String key, String dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getString(key) ?? dflt;
}

Future<void> setVal(String key, String? t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setString(key, t == null ? '' : t);
}

Future<bool> getValBool(String key, bool dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getBool(key) ?? dflt;
}

Future<void> setValBool(String key, bool t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setBool(key, t);
}

Future<int> getValInt(String key, int dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getInt(key) ?? dflt;
}

Future<void> setValInt(String key, int t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setInt(key, t);
}

Future<double> getValDouble(String key, double dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getDouble(key) ?? dflt;
}

Future<void> setValDouble(String key, double t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setDouble(key, t);
}
