import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';

import 'const.dart';


class SerialDevice {
  static int selected_baudrate = 0;
  static int serialType = 0;
  static String serialDevice = '';
  static SerialPort? serialPort = null;
  static SerialPortReader? serialPortReader = null;
  late DateTime last_send;
  final void Function(String) setWeight;
  late int ascii_0;

  int weight = 0;
  int decimal = 0;

  SerialDevice(this.setWeight) {
    last_send = DateTime.now();
    ascii_0 = '0'.codeUnitAt(0);
  }

  static void initSerialPort() {
    try {
      serialPort = SerialPort(serialDevice);
      print('Serial port $serialDevice = $serialPort');
    } catch(e) {
      serialPort = null;
    }

    if (serialPort != null) {
      serialPort!.open(mode: SerialPortMode.readWrite);

      SerialPortConfig config = SerialPortConfig();
      config.baudRate = baudrate2int[selected_baudrate];
      config.stopBits = 1;
      config.bits = 8;
      config.parity = SerialPortParity.none;
      config.setFlowControl(SerialPortFlowControl.none);
      serialPort!.config = config;

      serialPortReader = SerialPortReader(serialPort!);
    }
  }

  static void disposeSerialPort() {
    if (serialPortReader != null) {
      serialPortReader?.close();
      serialPortReader = null;
    }

    if (serialPort != null) {
      serialPort?.close();
      serialPort?.dispose();
      serialPort = null;
    }
  }

  static void restartSerialPort() {
    disposeSerialPort();
    initSerialPort();
  }

  static List<String> getAvailablePorts() {
    return SerialPort.availablePorts;
  }

  int handleCharacter(int c) {
    if (c == '.'.codeUnitAt(0) || c == ','.codeUnitAt(0)) {
      decimal = 1;
    } else if (serialType == DEV_TYPE_AP1 && c == 0x06) {
      // ACK
      return 0x11; // DC1
    } else if (c < ascii_0 || c > '9'.codeUnitAt(0)) {
      if ((serialType == DEV_TYPE_NORMAL && c == 13) ||
          (serialType == DEV_TYPE_STATHMOS && c == 0x1e) ||
          (serialType == DEV_TYPE_AP1 &&
              (c == 'k'.codeUnitAt(0) || c == 0x04 || c == 0x03)) ||
          (serialType == DEV_TYPE_MYWEIGHT &&
              (c == 'k'.codeUnitAt(0) || c == 13))) {
        print('WEIGHT READY=$weight');
        setWeight((weight / 1000.0).toString());

        weight = 0;
        decimal = 0;
      }
    } else if (serialType == DEV_TYPE_STATHMOS) {
      weight = 10 * weight + (c - ascii_0);
    } else if (decimal == 0) {
      weight = 10 * weight + 1000 * (c - ascii_0);
    } else {
      switch (decimal) {
        case 1:
          weight += 100 * (c - ascii_0);
          break;
        case 2:
          weight += 10 * (c - ascii_0);
          break;
        case 3:
          weight += c - ascii_0;
          break;
      }
      decimal++;
    }
    print("weight=$weight");
    return 0;
  }

  int send_chars() {
    if (serialType == DEV_TYPE_NORMAL) return 0;
    final now = DateTime.now();
    if (now.difference(last_send) < Duration(seconds: 1)) return 0;
    last_send = now;
    if (serialType == DEV_TYPE_STATHMOS) return 0x05;
    if (serialType == DEV_TYPE_AP1) return 0x05;
    if (serialType == DEV_TYPE_MYWEIGHT) return 0x0d;
    return 0;
  }

  void readInput() {
    Uint8List l = Uint8List(1);
    int ch = 0;

    if (serialPortReader != null) {
      serialPortReader!.stream.listen((d) {
        print("${d} ${d.runtimeType}");
        for (var c in d) {
          var ch2 = handleCharacter(c);
          if (ch2 > 0 && ch == 0) ch = ch2;
        }

        if (ch == 0) ch = send_chars();

        if (ch > 0) {
          l[0] = ch;
          serialPort!.write(l);
        }
      });
    }
  }
}
