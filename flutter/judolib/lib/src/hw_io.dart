import 'dart:io';
import 'package:shared_preferences/shared_preferences.dart';

Future<String> getHostName(String key) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  if (prefs != null) {
    var host = prefs.getString(key);
    if (host != null && host != '0.0.0.0') {
      return host;
    }
  }
  return 'localhost';
}

String getLocation() {
  return '';
}

String getUrl() {
  return 'http://localhost:8000';
}

var ssdpAddress = new Map();

String getSsdpAddress(String lookfor) {
  String? n = ssdpAddress[lookfor];
  if (n != null) return n;
  return '';
}

void ssdp(String lookfor) {
  InternetAddress multicastAddress = new InternetAddress("239.255.255.250");
  int multicastPort = 1900;
  RawDatagramSocket.bind(InternetAddress.anyIPv4, multicastPort)
      .then((RawDatagramSocket socket) {
    print('Datagram socket ready to receive');
    print('${socket.address.address}:${socket.port}');

    socket.joinMulticast(multicastAddress);
    print('Multicast group joined');

    socket.listen((RawSocketEvent e) {
      Datagram? d = socket.receive();
      if (d == null) return;

      String message = new String.fromCharCodes(d.data).trim();
      //print('Datagram from ${d.address.address}:${d.port}');
      if (message.contains(lookfor) && d.address.type == InternetAddressType.IPv4) {
        //print('JudoSHiai addr = ${d.address.address}, type1=${d.address}, type2=${d.address.address.runtimeType}');
        if (ssdpAddress != d.address.address) {
          //print('SSDP ADDRESS FOR $lookfor: ${d.address.address}');
          ssdpAddress[lookfor] = d.address.address;
        }
      }
    });
  });
}

void goFullScreen() {
}

void exitFullScreen() {
}

void showPdf(List<int> pdf) {
}

void saveTextFile(String text, String filename) {

}