import 'package:drift/drift.dart';
import 'package:drift/wasm.dart';
import 'package:http/http.dart' as http;
import 'package:sqlite3/wasm.dart';

LazyDatabase openConnection() {
  return LazyDatabase(() async {
    // Load wasm bundle
    final response = await http.get(Uri.parse('sqlite3.wasm'));
    // Create a virtual file system backed by IndexedDb with everything in
    // `/drift/my_app/` being persisted.
    final fs = await IndexedDbFileSystem.open(dbName: 'judoweight');
    var files = fs.files;
    print('EXISTING FILES=$files');

    final sqlite3 = await WasmSqlite3.load(
      response.bodyBytes,
      SqliteEnvironment(fileSystem: fs),
    );
    // Then, open a database inside that persisted folder.
    return WasmDatabase(sqlite3: sqlite3, path: 'app.db');
  });
}

Future<void> deleteDB() async {
  final fs = await IndexedDbFileSystem.open(dbName: 'judoweight');
  var files = fs.files;
  print('FILES=$files');
  try {
    fs.deleteFile('/app.db');
  } catch (e) {
    print('DB FILE DELETE ERROR $e');
  }
}