import 'package:flutter/material.dart';
import 'dart:ui' as ui;
import 'utils.dart';

class CompetitionModel extends ChangeNotifier {
  Competition? _competition = null;
  List<CategoryImage> _categoryImages = [];
  Image? _png = null;

  Competition? get competition => _competition;
  setCompetition(Competition c) {
    _competition = c;
    notifyListeners();
  }

  List<TatamiMatches> get tatamiMatches => _competition?.matches ?? [];
  setTatamiMatches(List<TatamiMatches> m) {
    print('SET MATCHES len=${m.length} _competition=$_competition');
    _competition?.matches = m;
    notifyListeners();
  }

  List<CategoryImage> get categoryImages => _categoryImages;
  setCategoryImages(List<CategoryImage> imgs) {
    _categoryImages = imgs;
    notifyListeners();
  }

  Image? get png => _png;
  setPng(Image png) {
    _png = png;
    notifyListeners();
  }
  /***
  void getPng(BuildContext context, double width, double height, String cat, int page) async {
    _png = await getPngImage(context, width, height, cat, page);
    notifyListeners();
  }
      ***/
}
