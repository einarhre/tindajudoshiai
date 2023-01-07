import 'dart:math';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:http/http.dart' as http;
import 'package:format/format.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:judolib/judolib.dart';
import 'package:provider/provider.dart';
import 'package:results/custom_colors.dart';
import 'dart:ui' as ui;
import 'package:pinch_zoom/pinch_zoom.dart';
import 'package:image/image.dart' as img;

import 'bloc.dart';
import 'launch.dart';
import 'utils.dart';

const pngWidth = 630;
const pngHeight = 891;

class CompInfo extends StatefulWidget {
  CompInfo({required this.category, this.competitor, Key? key}) : super(key: key);

  Competitor? competitor = null;
  String category;

  @override
  State<CompInfo> createState() => _CompInfoState();
}

class _CompInfoState extends State<CompInfo> with TickerProviderStateMixin {
  late TabController tabController;
  int tabColumns = 0;
  int _page = 0;

  List<CategoryImage> catImgs = [];

  @override
  void initState() {
    super.initState();
    tabController = TabController(vsync: this, length: tabColumns);
    Future.delayed(Duration.zero, () {
      final width = MediaQuery.of(context).size.width;
      final height = MediaQuery.of(context).size.height;
      var padding = MediaQuery.of(context).viewPadding;
      double height3 = height - padding.top - kToolbarHeight;
      getCatgoryImage(context, width, height3, widget.category);
    });
  }

  @override
  void dispose() {
    tabController.dispose();
    super.dispose();
  }

  void updateTabs() {
    if (tabColumns != tabController.length) {
      final oldIndex = tabController.index;
      tabController.dispose();
      tabController = TabController(
        length: tabColumns,
        initialIndex: max(0, min(oldIndex, tabColumns - 1)),
        vsync: this,
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final width = MediaQuery.of(context).size.width;
    final height = MediaQuery.of(context).size.height;
    var padding = MediaQuery.of(context).viewPadding;
    double height3 = height - padding.top - kToolbarHeight;
    print('SIZE= $width x $height3');

    SystemChannels.textInput.invokeMethod('TextInput.hide');
    var t = AppLocalizations.of(context);
    var provider = Provider.of<CompetitionModel>(context, listen: true);
    catImgs = provider.categoryImages;
    tabColumns = catImgs.length;
    final firstlast = 0;


    return Scaffold(
      appBar: AppBar(
        backgroundColor: CustomColors.appBar,
        toolbarHeight: kToolbarHeight,
        title: Text(widget.category),
        titleSpacing: 20,
        actions: [
          statisticsExist
          ? ElevatedButton(
              onPressed: () {
                launchWebPage('${str2hex(widget.category)}.pdf');
              },
              child: Icon(Icons.picture_as_pdf))
          : Text(''),
          SizedBox(
            width: 20,
          ),
          ElevatedButton(
              onPressed: () {
                if (_page > 0)
                  setState(() {
                    _page--;
                  });
              },
              child: Icon(_page > 0 ? Icons.arrow_circle_left : Icons.arrow_left_outlined)),
          Text('${_page + 1}', style: TextStyle(fontSize: kToolbarHeight)),
          ElevatedButton(
              onPressed: () {
                if (_page < catImgs.length - 1)
                  setState(() {
                    _page++;
                  });
              },
              child: Icon(_page < catImgs.length - 1 ? Icons.arrow_circle_right : Icons.arrow_right_outlined))
        ],
        //bottom:
      ),
      body: LayoutBuilder(builder: (context, constraints) {
        Widget w;
        var screenSize = MediaQuery.of(context).size;
        print(
            'XXXXX PAGE=$_page Max height: ${constraints.maxHeight}, max width: ${constraints.maxWidth}');

        if (catImgs.length < _page + 1) return CircularProgressIndicator();

        Size cardSize = Size(10.0, 10.0);

        if (widget.competitor != null) {
          Competitor c = widget.competitor!;
          var name =
          firstlast == 0 ? '${c.last}, ${c.first}' : '${c.first} ${c.last}';
          if (c.belt != '') name += '  (${c.belt})';
          var subtitle = c.club;
          if (c.tatami > 0 && c.waittime >= 0)
            subtitle += '\n${round2Str(t, c.round)}: ${t?.restxt47 ?? "Match after"} ${c.waittime} ${t?.restxt48 ?? "matches"} (Tatami ${c.tatami})';

          w = MeasureSize(
              onChange: (size) {
                //setState(() {
                cardSize = size;
                print('CALCULATED SIZE: $size');
                //});
              },
              child: Card(
                  elevation: 2,
                  margin: EdgeInsets.all(4),
                  color: Colors.white,
                  child: ListTile(
                    isThreeLine: true,
                    dense: true,
                    leading: Text(format('{:6}', c.category),
                        style: TextStyle(fontFamily: 'RobotoMono')),
                    title: Text(name),
                    subtitle: Text(subtitle),
                    selected: false,
                    onTap: () {
                      setState(() {
                        //SystemChannels.textInput.invokeMethod('TextInput.hide');
                      });
                    },
                  ))
          );
        } else {
          w = MeasureSize(
            onChange: (size) {
              //setState(() {
              cardSize = size;
              print('CALCULATED SIZE 2: $size');
              //});
            },
            child: SizedBox(width: 0, height: 0,));
        }
        var pageImg = catImgs[_page];
        return ListView(
          children: [
            w,
            GestureDetector(
              onPanUpdate: (details) {
                // Swiping in right direction.
                if (details.delta.dx > 0) {
                  if (_page > 0)
                    setState(() {
                      _page--;
                    });
                }
                // Swiping in left direction.
                if (details.delta.dx < 0) {
                  if (_page < catImgs.length - 1)
                    setState(() {
                      _page++;
                    });
                }
              },
            child: SizedBox(
                width: constraints.maxWidth,
                height: constraints.maxHeight - cardSize.height,
                child: InteractiveViewer(
                    panEnabled: false,
                    // Set it to false
                    boundaryMargin: EdgeInsets.all(10),
                    minScale: 0.5,
                    maxScale: 2,
                    child: Stack(
                        children: getImagePage(pageImg, widget.competitor?.index ?? 0,
                            constraints.maxWidth, constraints.maxHeight - cardSize.height)))))
          ],
        );
      }),
    );
  }

  List<Widget> getImagePage(
      CategoryImage ci, int index, double maxWidth, double maxHeight) {
    Image img = Image.memory(ci.bytes!,
        width: maxWidth,
        height: maxHeight,
        alignment: Alignment(-1.0, -1.0),
        fit: BoxFit.contain);
    List<Widget> lst = [Positioned(left: 0.0, top: 0.0, child: img)];

    //print('POSITIONS: $positions');
    double? w = ci.width;
    double? h = ci.height;
    print('SIZE=${ci.width}x${ci.height}');

    double scalew = 1.0, scaleh = 1.0, scale = 1.0;
    if (w != null && w > 0) scalew = maxWidth / w;
    if (h != null && h > 0) scaleh = maxHeight / h;
    scale = min(scalew, scaleh);

    for (var i = 0; i < ci.positions.length; i++) {
      var pos = ci.positions[i];
      if (pos.ix == index) {
        lst.add(Positioned(
            left: pos.x1 * scale,
            top: pos.y1 * scale,
            child: Container(
              width: (pos.x2 - pos.x1) * scale,
              height: (pos.y2 - pos.y1) * scale,
              decoration: BoxDecoration(
                border: Border.all(width: 2.0, color: Colors.red),
                shape: BoxShape.rectangle,
              ),
            )));
      }
    }
    return lst;
  }
}

class CategoryPainter extends CustomPainter {
  ui.Image catImage;
  img.Image imgImg;

  //double width, height;
  int ix = 0;
  List<NamePosition> namePositions = [];

  CategoryPainter(this.imgImg, this.catImage, this.namePositions, this.ix) {}

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..strokeWidth = 2
      ..color = Colors.red
      ..style = PaintingStyle.stroke;

    canvas.drawImage(catImage, Offset(0.0, 0.0), Paint());

    for (var i = 0; i < namePositions.length; i++) {
      var pos = namePositions[i];
      if (pos.ix == ix) {
        final rect = Path();
        rect.moveTo(pos.x1.toDouble(), pos.y1.toDouble());
        rect.lineTo(pos.x2.toDouble(), pos.y1.toDouble());
        rect.lineTo(pos.x2.toDouble(), pos.y2.toDouble());
        rect.lineTo(pos.x1.toDouble(), pos.y2.toDouble());
        rect.close();
        canvas.drawPath(rect, paint);
      }
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
