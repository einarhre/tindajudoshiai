import 'dart:html';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';

import '../judolib.dart';

class WidgetIFramer extends StatefulWidget {
  WidgetIFramer({
    required this.cat,
    required this.page,
    required this.ext,
    required this.width,
    required this.height,
    Key? key,
  }) : super(key: key);

  String cat, ext;
  int page;
  double width;
  double height;

  @override
  _WidgetIFramerState createState() => _WidgetIFramerState();
}

class _WidgetIFramerState extends State<WidgetIFramer> {
  final IFrameElement _iframeElement = IFrameElement();
  late Widget _iframeWidget;

  @override
  void initState() {
    super.initState();
    _iframeElement.height = '${widget.height.toInt()}';
    _iframeElement.width = '${widget.width.toInt()}';

    _iframeElement.src = categoryToUrl(widget.cat, widget.page, widget.ext);
    _iframeElement.style.border = 'none';

    _platformViewRegistry.registerViewFactory(
      categoryToIframeElement(widget.cat, widget.page, widget.ext),
          (int viewId) => _iframeElement,
    );

    _iframeWidget = HtmlElementView(
      key: UniqueKey(),
      viewType: categoryToIframeElement(widget.cat, widget.page, widget.ext),
    );
  }

  @override
  Widget build(BuildContext context) =>
      SizedBox(
          width: widget.width,
          height: widget.height,
          child: _iframeWidget);
}

// ignore: camel_case_types
class _platformViewRegistry {
  // ignore: always_declare_return_types
  static registerViewFactory(String viewId, dynamic cb) {
    // ignore:undefined_prefixed_name
    ui.platformViewRegistry.registerViewFactory(viewId, cb);
  }
}
