library judolib;

export 'src/lang.dart';
export 'src/const.dart';
export 'src/util.dart';
export 'src/svg_parse.dart';
export 'src/settings.dart';
export 'src/message.dart';

export 'src/hw_none.dart' // Stub implementation
  if (dart.library.io) 'src/hw_io.dart' // dart:io implementation
  if (dart.library.html) 'src/hw_html.dart'; // dart:html implementation

export 'src/serial_none.dart' // Stub implementation
  if (dart.library.io) 'src/serial_io.dart' // dart:io implementation
  if (dart.library.html) 'src/serial_html.dart'; // dart:html implementation
