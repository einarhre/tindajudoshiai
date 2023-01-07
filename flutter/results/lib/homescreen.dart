import 'dart:math';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:form_builder_validators/localization/l10n.dart';
import 'package:provider/provider.dart';
import 'package:results/bloc.dart';
import "package:universal_html/js.dart" as js;
import 'package:judolib/judolib.dart';
import 'package:results/competitors.dart';
import 'package:results/matches.dart';
import 'package:results/results.dart';
import 'package:csslib/parser.dart' as css;
import 'package:results/utils.dart';

import 'coach.dart';
import 'custom_colors.dart';
import 'main.dart';
import 'medals.dart';
import 'statistics.dart';

var tabSelected = 0;

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key, required this.title}) : super(key: key);

  // This widget is the home page of your application. It is stateful, meaning
  // that it has a State object (defined below) that contains fields that affect
  // how it looks.

  // This class is the configuration for the state. It holds the values (in this
  // case the title) provided by the parent (in this case the App widget) and
  // used by the build method of the State. Fields in a Widget subclass are
  // always marked "final".

  final String title;

  @override
  State<HomeScreen> createState() => HomeScreenState();
}

class HomeScreenState extends State<HomeScreen> with TickerProviderStateMixin {
  late double width, height;
  late SvgParse svg;
  late SerialDevice device;
  int tabColumns = 7;
  int oldIndex = 0;
  late TabController tabController;
  late Coach _coach;

  //late final CompetitionModel provider;

  @override
  void initState() {
    super.initState();
    tabController = TabController(vsync: this, length: tabColumns); //<<<<<<
    tabController.addListener(() {
      setState(() {
        oldIndex = tabController.index;
      });
      print("Selected Index: " + tabController.index.toString());
    });
    //provider = Provider.of<CompetitionModel>(context, listen: false);
    Future.delayed(Duration.zero, () {
      savedCompetition = getFullCompetition(context);
    });
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    //savedCompetition = getFullCompetition(context);
  }

  @override
  void didUpdateWidget(covariant HomeScreen oldWidget) {
    super.didUpdateWidget(oldWidget);
  }

  void updateTabs() {
    if (tabColumns != tabController.length) {
      var currentIndex = tabController.index;

      if (oldIndex >= tabController.length)
        currentIndex = oldIndex;

      tabController.dispose();
      tabController = TabController(
        length: tabColumns,
        initialIndex: max(0, min(currentIndex, tabColumns-1)),
        vsync: this,
      );

      tabController.addListener(() {
        setState(() {
          oldIndex = tabController.index;
        });
      });
    }
  }

  @override
  void dispose() {
    tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    width = MediaQuery.of(context).size.width;
    height = MediaQuery.of(context).size.height;
    // Height (without SafeArea)
    var padding = MediaQuery.of(context).viewPadding;
    double height1 = height - padding.top - padding.bottom;
    // Height (without status bar)
    double height2 = height - padding.top;
    // Height (without status and toolbar)
    double height3 = height - padding.top - kToolbarHeight;

    var t = AppLocalizations.of(context);
    //AppLocalizations? t = null;
    print('REDRAW width=$width');

    var provider = Provider.of<CompetitionModel>(context, listen: true);
    var competition = provider.competition;
    var matches = provider.tatamiMatches;
    final String name = competition?.info['name'] ?? '';
    final String date = competition?.info['date'] ?? '';
    final String place = competition?.info['place'] ?? '';

    List<String> tabs = [
      t?.restxt19 ?? '',
      t?.restxt1 ?? '',
      t?.restxt9 ?? '',
      t?.restxt33 ?? '',
      t?.restxt35 ?? '',
      t?.restxt34 ?? '',
      t?.search1334 ?? '',
    ];
    List<Widget> screens = [
      Results(),
      ListCompetitors(listBy: listByName),
      ListCompetitors(listBy: listByClub),
      ListCompetitors(listBy: listByCategory),
      Medals(),
      Statistics(),
      Coach(),
      //Matches(width: width, height: height2, start: 0, count: 1),
    ];

    var numColumns = matches.length;
    print('numColumns=$numColumns');
    tabColumns = 7;
    var count = (width / 200).floor();
    if (count == 0) count = 1;
    var i = 0;
    while (i < numColumns) {
      var last = i + count;
      if (last > numColumns) last = numColumns;
      tabs.add(i + 1 == last ? 'T${i + 1}' : 'T${i + 1}-${last}');
      screens
          .add(Matches(width: width, height: height2, start: i, count: count));
      i += count;
      tabColumns++;
    }
    updateTabs();
    print('TABS=$tabColumns tabs=${tabs.length} screens=${screens.length}');

    return DefaultTabController(
        length: tabColumns,
        child: Scaffold(
          appBar: AppBar(
            backgroundColor: CustomColors.appBar,
            toolbarHeight: kToolbarHeight,
            title: Text('$name $date $place'),
            actions: [
              Builder(
                builder: (context) {
                  return DropdownButton(
                    dropdownColor: Colors.grey,
                    onChanged: (v) => setState(() {
                      languageCode = v.toString();
                      MyApp.setLocale(context, Locale(v.toString(), ""));
                    }),
                    value: languageCode,
                    items: languageCodes
                        .map<DropdownMenuItem<String>>((String value) {
                      return DropdownMenuItem<String>(
                        value: value,
                        child: Row(children: [
                          Padding(
                            padding: EdgeInsets.all(8.0),
                            child: Image.asset(
                              'packages/judolib/assets/flags-ioc/${languageCodeToIOC[value]}.png',
                              height: 24,
                            ),
                          ),
                          SizedBox(
                            width: 10,
                          ),
                          Text(languageCodeToLanguage[value] ?? '',
                              style: TextStyle(color: Colors.white)),
                        ]),
                      );
                    }).toList(),
                  );
                },
              ),
            ],
            bottom: TabBar(
              controller: tabController,
              isScrollable: true,
              onTap: (index) {
                oldIndex = index;
                print('TAB $index selected');
                // Should not used it as it only called when tab options are clicked,
                // not when user swapped
              },
              tabs: tabs.map((e) => Tab(text: e)).toList(),
              /*[
                          Tab(
                            text: t?.restxt19 ?? '',
                          ),
                          Tab(
                            text: t?.restxt1 ?? '',
                          ),
                          Tab(
                            text: t?.restxt9 ?? '',
                          ),
                          Tab(
                            text: t?.restxt33 ?? '',
                          ),
                          Tab(
                            text: t?.restxt35 ?? '',
                          ),
                          Tab(
                            text: t?.restxt34 ?? '',
                          ),
                          Tab(
                            text: t?.restxt32 ?? '',
                          ),
                        ],*/
            ),
          ),
          body: TabBarView(
            controller: tabController,
            children: screens, //screens.map((e) => e).toList(),
            /*[
                        Results(),
                        ListCompetitors(listBy: listByName),
                        ListCompetitors(listBy: listByClub),
                        ListCompetitors(listBy: listByCategory),
                        Medals(),
                        Statistics(),
                        Matches(width: width, height: height2),
                      ],*/
          ),
        ));
  }
}
