import 'dart:convert';

import 'package:http/http.dart' as http;
import 'package:judolib/judolib.dart';
import 'package:judoweight/settings.dart';
import 'database.dart';

Future<int> sendJudokaData(Judoka c) async {
  var host = await getHostName('jsip');
  var arg = judoka2json(c);
  arg['op'] = 'setcomp';
  arg['pw'] = jspassword;
  print('SENDING ${json.encode(arg)}');
  try {
    var response = await http.post(
      Uri.parse('http://$host:8088/json'),
      body: json.encode(arg),
    );
    if (response.statusCode == 200) {
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      print('RESP=${json}');
      return json['ix'];
    }
  } catch (e) {
    print("HTTP error $e");
    //rethrow;
  }
  return -1;
}

Future<Map<String, dynamic>?> sendWeightData(int ix, int weight) async {
  var host = await getHostName('jsip');
  try {
    var response = await http.post(
      Uri.parse('http://$host:8088/json'),
      body: json.encode({"op": "setweight", "pw": jspassword, "ix": ix, "weight": weight}),
    );
    if (response.statusCode == 200) {
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      print('RESP=${json} type=${json.runtimeType}');
      return json;
    }
  } catch (e) {
    print("HTTP error $e");
    //rethrow;
  }
  return null;
}

Future<Map<String, dynamic>?> getJudokaJs(int ix) async {
  var host = await getHostName('jsip');
  try {
    var response = await http.post(
      Uri.parse('http://$host:8088/json'),
      body: json.encode({"op": "getcomp", "pw": jspassword, "ix": ix}),
    );
    if (response.statusCode == 200) {
      var json = jsonDecode(utf8.decode(response.bodyBytes));
      print('getJudokaJs RESP=${json} type=${json.runtimeType}');
      return json;
    }
  } catch (e) {
    print("HTTP error $e");
    //rethrow;
  }
  return null;
}
