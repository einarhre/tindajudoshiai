import 'dart:async';
import 'dart:convert';

import 'package:judotimer/message.dart';
import 'package:judotimer/util.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

import 'global.dart';
import 'layout.dart';

AutoReconnectWebSocket? websock = null;

AutoReconnectWebSocket? websockComm(LayoutState layout) {
  String host = node_name;
  //print('HOSTNAME=$host');
  var url = 'ws://${node_name}:2315';
  if (mode_slave)
    url = 'ws://${master_name}:2316';
  if (websock != null) {
    //print('WEBSOCK ALREADY OPEN');
    websock?.reconnect(Uri.parse(url));
  } else {
    //print('WEBSOCK OPEN to $url');
    websock = AutoReconnectWebSocket(Uri.parse(url));
  }
  var stream = websock?.stream;
  stream?.listen((event) {
    //print('EVENT=$event');
    var json = jsonDecode(event);
    var jsonmsg = json['msg'];
    Message msg = Message(jsonmsg);
    handle_message(layout, msg);
  },
  onError: (error) { websock?.close();},
  onDone: () { websock?.close();});
  return websock;
}

class AutoReconnectWebSocket {
  Uri _endpoint;
  bool _ok = false;
  WebSocketChannel? webSocketChannel = null;

  get stream => webSocketChannel?.stream;

  get ok => _ok;

  AutoReconnectWebSocket(this._endpoint) {
    webSocketChannel = WebSocketChannel.connect(_endpoint);
    _ok = true;
  }

  void send(msg) {
    print('WEBSOCK SEND $msg');
    if (webSocketChannel == null) print('ERROR: WEBSOCK');
    webSocketChannel?.sink.add(msg);
  }

  void close() {
    _ok = false;
    webSocketChannel?.sink.close();
    websock = null;
  }

  void reconnect(Uri new_endpoint) {
    _ok = false;
    _endpoint = new_endpoint;
    webSocketChannel?.sink.close();
    webSocketChannel = WebSocketChannel.connect(_endpoint);
    _ok = true;
  }
}

/****
class AutoReconnectWebSocket1 {
  Uri _endpoint;
  final int delay;
  final StreamController<dynamic> _recipientCtrl = StreamController<dynamic>();
  final StreamController<dynamic> _sentCtrl = StreamController<dynamic>();

  static WebSocketChannel? webSocketChannel = null;

  get stream => _recipientCtrl.stream;

  get sink => _sentCtrl.sink;

  AutoReconnectWebSocket(this._endpoint, {this.delay = 5}) {
    _sentCtrl.stream.listen((event) {
      webSocketChannel!.sink.add(event);
    });
    _connect();
  }

  void _connect() {
    webSocketChannel = WebSocketChannel.connect(_endpoint);
    webSocketChannel!.stream.listen((event) {
      _recipientCtrl.add(event);
    }, onError: (e) async {
      _recipientCtrl.addError(e);
      await Future.delayed(Duration(seconds: delay));
      _connect();
    }, onDone: () async {
      await Future.delayed(Duration(seconds: delay));
      _connect();
    }, cancelOnError: true);
  }

  void send(msg) {
    webSocketChannel?.sink.add(msg);
  }

  void close() {
    webSocketChannel?.sink.close();
    websock = null;
  }

  void reconnect(Uri new_endpoint) {
    _endpoint = new_endpoint;
    webSocketChannel?.sink.close();
  }
}
****/