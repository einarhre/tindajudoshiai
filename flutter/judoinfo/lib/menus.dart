import 'package:flutter/material.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:judolib/judolib.dart';

import 'global.dart';
import 'layout.dart';
import 'settings.dart';

Future<void> showPopupMenu(LayoutState layout) async {
  var t = AppLocalizations.of(layout.context);
  double left = 0.0; //offset.dx;
  double top = 20.0;
  double h = 32.0;
  await showMenu(
    context: layout.context,
    position: RelativeRect.fromLTRB(left, top, 0, 0),
    items: [
      CheckedPopupMenuItem<String>(
          height: h,
          checked: fullscreen,
          child: Text(t?.fullScreenMode89ed ?? ''), value: 'fullscreen'),
    ],
    elevation: 8.0,
  ).then((value) async {
    switch (value) {
      case 'fullscreen':
        goFullScreen();
        break;
    }
  });
}
