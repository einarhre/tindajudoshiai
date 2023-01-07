import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_svg/svg.dart';
import 'package:judotimer/global.dart';
import 'package:judotimer/layout.dart';
import 'package:xml_parser/xml_parser.dart';

import 'package:judolib/judolib.dart';

class Flag {
  String name;
  double x = 0, y = 0, w = 0, h = 0;

  Flag(this.name, this.x, this.y, this.w, this.h);
}

List<Flag> flags = [];
List<SvgTextSegment> competitorsSvgSegments = [];
List<String> elements = ['flag1', 'flag2'];

class ShowCompetitors extends StatelessWidget {
  LayoutState layout;
  static String cat = '',
      comp1 = '',
      comp2 = '',
      first1 = '',
      first2 = '',
      country1 = '',
      country2 = '',
      club1 = '',
      club2 = '';
  static int round = 0;
  double width, height;

  ShowCompetitors(this.layout, this.width, this.height, {Key? key})
      : super(key: key);

  static void setData(
      String cat,
      String comp1,
      String comp2,
      String first1,
      String first2,
      String country1,
      String country2,
      String club1,
      String club2,
      int round) {
    ShowCompetitors.cat = cat;
    ShowCompetitors.comp1 = comp1;
    ShowCompetitors.comp2 = comp2;
    ShowCompetitors.first1 = first1;
    ShowCompetitors.first2 = first2;
    ShowCompetitors.country1 = country1;
    ShowCompetitors.country2 = country2;
    ShowCompetitors.club1 = club1;
    ShowCompetitors.club2 = club2;
    ShowCompetitors.round = round;
  }

  List<Widget> getWidgets(String snapshot) {
    final btnheight = height * 0.05;
    final ButtonStyle style = ElevatedButton.styleFrom(
      textStyle: const TextStyle(fontSize: 20),
      primary: Colors.grey,
    );

    List<Widget> l = [
      Positioned(
        left: 0,
        top: btnheight,
        width: width,
        height: height - btnheight,
        child: SvgPicture.string(snapshot, fit: BoxFit.fill),
      ),
      Positioned(
        top: 0,
        width: width,
        height: btnheight,
        child: Focus(
            autofocus: true,
            onKey: (FocusNode node, RawKeyEvent event) {
              if (event is RawKeyDownEvent &&
                  event.logicalKey.keyLabel == ' ') {
                layout.displayMainScreen();
              }
              return KeyEventResult.handled;
            },
            child: Builder(builder: (BuildContext context) {
              final FocusNode focusNode = Focus.of(context);
              focusNode.requestFocus();
              return ElevatedButton(
                style: style,
                onPressed: () {
                  layout.displayMainScreen();
                  //Navigator.pop(context);
                },
                child: const Text('OK'),
              );
            })),
      ),
    ];

    for (var f in flags) {
      var c = (f.name == 'flag1') ? country1 : country2;
      if (c.length == 3) {
        l.add(Positioned(
            left: f.x * width,
            top: f.y * (height - btnheight) + btnheight,
            width: f.w * width * 3,
            height: f.h * (height - btnheight),
            child: Image(
              image: AssetImage('packages/judolib/assets/flags-ioc/${c}.png'),
              fit: BoxFit.fitHeight,
              alignment: Alignment.topLeft,
            )));
      }
    }

    return l;
  }

  @override
  Widget build(BuildContext context) {
    return FutureBuilder<String>(
        future: getSvgText(),
        builder: (context, AsyncSnapshot<String> snapshot) {
          if (!snapshot.hasData) {
            return CircularProgressIndicator();
          } else {
            return Stack(children: getWidgets(snapshot.data!));
          }
        });
  }
}

Flag? getFlagById(XmlDocument? xmlDocument, String id) {
  double viewX = 0, viewY = 0, viewW = 297, viewH = 167;

  if (xmlDocument == null) return null;

  XmlElement? svg = xmlDocument.getElementWhere(name: 'svg');
  if (svg != null) {
    String? viewBox = svg.getAttribute('viewBox');
    if (viewBox != null) {
      var vals = viewBox.split(' ');
      if (vals.length == 4) {
        viewX = double.parse(vals[0]);
        viewY = double.parse(vals[1]);
        viewW = double.parse(vals[2]);
        viewH = double.parse(vals[3]);
      }
    }
  }

  XmlElement? flag = xmlDocument.getElementWhere(id: id);
  if (flag != null) {
    String? x, y, w, h;
    x = flag.getAttribute('x');
    y = flag.getAttribute('y');
    w = flag.getAttribute('width');
    h = flag.getAttribute('height');
    final f = Flag(
        id,
        x != null ? double.parse(x) / viewW : 0,
        y != null ? double.parse(y) / viewH : 0,
        w != null ? double.parse(w) / viewW : 0,
        h != null ? double.parse(h) / viewH : 0);
    return f;
  }

  return null;
}

Future<void> readCompetitorsSvgString() async {
  final svgstr = await rootBundle.loadString('assets/timer-competitors.svg');

  XmlDocument? xmlDocument = XmlDocument.from(svgstr);
  if (xmlDocument != null) {
    for (var id in elements) {
      Flag? flag = getFlagById(xmlDocument, id);
      if (flag != null) flags.add(flag);
    }
  }

  var s = SvgParse(svgstr);
  competitorsSvgSegments = s.varTexts;
}

Future<String> flagToBase64(String country) async {
  String flagasset = "packages/judolib/assets/flags-ioc/${country}.png";
  ByteData bytes = await rootBundle.load(flagasset);
  Uint8List flagbytes =
      bytes.buffer.asUint8List(bytes.offsetInBytes, bytes.lengthInBytes);
  String base64string = base64.encode(flagbytes);
  return base64string;
}

Future<String> getSvgText() async {
  String r = '';

  for (var txt in competitorsSvgSegments) {
    var a = txt.args;
    if (a.length == 0) {
      r += txt.after;
      continue;
    }
    var code = a[0];
    if (code == 'C') {
      r += ShowCompetitors.cat;
    } else if (code == 'R') {
      //r  += round;
    } else if (code == 'last' && a.length == 2) {
      if (a[1] == '1')
        r += ShowCompetitors.comp1;
      else
        r += ShowCompetitors.comp2;
    } else if (code == 'first' && a.length == 2) {
      if (a[1] == '1')
        r += ShowCompetitors.first1;
      else
        r += ShowCompetitors.first2;
    } else if (code == 'club' && a.length == 2) {
      if (a[1] == '1')
        r += ShowCompetitors.club1;
      else
        r += ShowCompetitors.club2;
    } else if (code == 'country' && a.length == 2) {
      if (a[1] == '1')
        r += ShowCompetitors.country1;
      else
        r += ShowCompetitors.country2;
    } else if (code == 'f' && a.length == 2) {
      if (a[1] == '1' && ShowCompetitors.first1.length > 1)
        r += '${ShowCompetitors.first1[0]}.';
      else if (ShowCompetitors.first2.length > 1)
        r += '${ShowCompetitors.first2[0]}.';
    } else if (code == 'flag' && a.length == 2) {
      String c =
          a[1] == '1' ? ShowCompetitors.country1 : ShowCompetitors.country2;
      if (c.length != 3) {
        print('EMPTY DATA FLAG');
        r +=
            'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAABHNCSVQICAgIfAhkiAAAAAtJREFUCJljYAACAAAFAAFiVTKIAAAAAElFTkSuQmCC';
      } else {
        String flag64 = await flagToBase64(c);
        r += 'data:image/png;base64,${flag64}';
        //r += 'http://${node_name}:8088/flags-ioc/${c}.png';
      }
    }
    r += txt.after;
  }
  return r;
}


/******
    return DefaultTextStyle(
    style: TextStyle(fontSize: fheight),
    child: Stack(children: [
    Positioned(
    left: 0,
    top: btnheight,
    width: width,
    height: boxheight,
    child: ColoredBox(
    color: Colors.black,
    child: Text(ShowCompetitors.cat,
    style: TextStyle(
    color: Colors.white,
    )))),
    Positioned(
    left: 0,
    top: btnheight + boxheight,
    width: width,
    height: boxheight,
    child: ColoredBox(
    color: Colors.white,
    child: Text(ShowCompetitors.comp1,
    style: TextStyle(
    color: Colors.black,
    )))),
    Positioned(
    left: 0,
    top: btnheight + boxheight * 2.0,
    width: width,
    height: boxheight,
    child: ColoredBox(
    color: Colors.blue,
    child: Text(ShowCompetitors.comp2,
    style: TextStyle(
    color: Colors.white,
    )))),
    Positioned(
    top: 0,
    width: width,
    height: btnheight,
    child: Focus(
    autofocus: true,
    onKey: (FocusNode node, RawKeyEvent event) {
    if (event is RawKeyDownEvent &&
    event.logicalKey.keyLabel == ' ') {
    layout.displayMainScreen();
    }
    return KeyEventResult.handled;
    },
    child: Builder(builder: (BuildContext context) {
    final FocusNode focusNode = Focus.of(context);
    focusNode.requestFocus();
    return ElevatedButton(
    style: style,
    onPressed: () {
    print('OK PRESSED');
    layout.displayMainScreen();
    //Navigator.pop(context);
    },
    child: const Text('OK'),
    );
    }))),
    ]));
 ****/
