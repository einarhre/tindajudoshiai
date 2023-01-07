import 'package:flutter/material.dart';

void registerIframeElement() {
}

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

    @override
    Widget build(BuildContext context) =>
        SizedBox(
            width: widget.width,
            height: widget.height,
        );
}
