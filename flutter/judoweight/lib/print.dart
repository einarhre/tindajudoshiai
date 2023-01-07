// https://pub.dev/documentation/printing/latest/printing/Printing/layoutPdf.html
// https://pub.dev/documentation/printing/latest/printing/printing-library.html
// https://pub.dev/documentation/pdf/latest/pdf/pdf-library.html
// https://pub.dev/documentation/pdf/latest/widgets/SvgImage-class.html
// https://pub.dev/packages/flutter_svg

import 'dart:typed_data';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_svg/avd.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:printing/printing.dart';
import 'package:pdf/pdf.dart';
import 'package:pdf/widgets.dart' as pw;

const List<String> pageSizes = [
  'a3',
  'a4',
  'a5',
  'a6',
  'legal',
  'letter',
  'roll',
];
const List<PdfPageFormat> pageFormats = [
  PdfPageFormat.a3,
  PdfPageFormat.a4,
  PdfPageFormat.a5,
  PdfPageFormat.a6,
  PdfPageFormat.legal,
  PdfPageFormat.letter,
];

String pageSize = 'a4';
int printRollWidth = 50;
String printerName = '';
bool printLabel = false;
bool printNoDialog = false;

Future<Uint8List> generatePdfFromSvg(
    {required String svgstr, PdfPageFormat format = PdfPageFormat.a6}) async {
  final pdf = pw.Document(version: PdfVersion.pdf_1_5, compress: true);
  final svgIcon = pw.SvgImage(svg: svgstr);

  pdf.addPage(
    pw.Page(
      pageFormat: format,
      build: (context) {
        return pw.Container(child: svgIcon);
      },
    ),
  );

  return pdf.save();
}

Future<void> printSvgStr(String svgstr) async {
  if (!printLabel)
    return;

  PdfPageFormat pf = PdfPageFormat.a4;
  if (pageSize == 'roll') {
    pf = PdfPageFormat(printRollWidth.toDouble() * PdfPageFormat.mm,
        double.infinity,
        marginAll: 5 * PdfPageFormat.mm);
  } else {
    final int i = pageSizes.indexOf(pageSize);
    if (i >= 0) pf = pageFormats[i];
  }

  if (printNoDialog) {
    final printers = await Printing.listPrinters();
    for (var p in printers) {
      if (p.name == printerName) {
        await Printing.directPrintPdf(
            printer: p,
            format: pf,
            onLayout: (format) =>
                generatePdfFromSvg(svgstr: svgstr, format: format),
            usePrinterSettings: true,
            dynamicLayout: true);
        return;
      }
    }
  }
  await Printing.layoutPdf(
      format: pf,
      onLayout: (format) => generatePdfFromSvg(svgstr: svgstr, format: format),
      usePrinterSettings: true,
      dynamicLayout: true);
}

Future<void> printPdf(Uint8List data) async {
  final printers = await Printing.listPrinters();
  print(printers);
  await Printing.directPrintPdf(printer: printers.first, onLayout: (_) => data);
}

Future<List<String>> listPrinters() async {
  List<String> lst = [];
  try {
    final printers = await Printing.listPrinters();
    for (var p in printers) {
      lst.add(p.name);
    }
  } catch (e) {
    print('listPrinters: $e');
  }
  return lst;
}
