import 'package:drift/drift.dart';

LazyDatabase openConnection() {
  throw UnsupportedError(
      'No suitable database implementation was found on this platform.');
}

Future<void> deleteDB() async {
  throw UnsupportedError(
      'No suitable database implementation was found on this platform.');
}
