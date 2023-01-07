
class SerialDevice {
  static int selected_baudrate = 0;
  static int serialType = 0;
  static String serialDevice = '';
  final void Function(String) setWeight;

  SerialDevice(this.setWeight);

  static void initSerialPort() {}
  static void disposeSerialPort() {}
  static void restartSerialPort() {}
  static List<String> getAvailablePorts() {
    return [];
  }

  void readInput() {}
}
