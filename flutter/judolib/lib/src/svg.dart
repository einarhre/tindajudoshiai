import 'dart:typed_data';
import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:xml/xml.dart';

Future<Uint8List> svgToPng(BuildContext context, String svgString,
    {int? svgWidth, int? svgHeight}) async {
  print('SVG START');
  DrawableRoot svgDrawableRoot = await svg.fromSvgString(svgString, 'key');
  print('SVG FROM STRING');
  // Parse the SVG file as XML
  XmlDocument document = XmlDocument.parse(svgString);

  // Getting size of SVG
  final svgElement = document.findElements("svg").first;
  final svgWidth = double.parse(svgElement.getAttribute("width")!);
  final svgHeight = double.parse(svgElement.getAttribute("height")!);
  // toPicture() and toImage() don't seem to be pixel ratio aware, so we calculate the actual sizes here
  double devicePixelRatio = MediaQuery.of(context).devicePixelRatio;

  double width = svgHeight * devicePixelRatio;
  double height = svgWidth * devicePixelRatio;
  print('SVG SIZE = $width x $height');

  // Convert to ui.Picture
  final picture = svgDrawableRoot.toPicture(size: Size(width, height));

  // Convert to ui.Image. toImage() takes width and height as parameters
  // you need to find the best size to suit your needs and take into account the screen DPI
  final image = await picture.toImage(width.toInt(), height.toInt());
  ByteData? bytes = await image.toByteData(format: ImageByteFormat.png);

  return bytes!.buffer.asUint8List();
}
