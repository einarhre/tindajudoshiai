import 'dart:convert';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:form_builder_validators/localization/l10n.dart';
import 'package:judolib/judolib.dart';
import 'package:judoweight/bloc.dart';
import 'package:judoweight/competitor_edit.dart';
import 'package:judoweight/judoka_list.dart';
import 'package:judoweight/svg.dart';
import 'package:judoweight/weight_edit.dart';
import 'package:judoweight/weightform.dart';
import 'package:provider/provider.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:mobile_scanner/mobile_scanner.dart';

//import 'lang.dart';
import 'database.dart';
import 'judoka_card.dart';
import 'main.dart';
import 'menus.dart';
import 'settings.dart';

late TabController tabController;

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

class HomeScreenState extends State<HomeScreen> with SingleTickerProviderStateMixin {
  late double width, height;
  late SvgParse svg;
  late SerialDevice device;

  @override
  void initState() {
    super.initState();

    tabController = TabController(vsync: this, length: 2);
    readSvgString();
    SerialDevice.initSerialPort();
    device = SerialDevice(Provider.of<JudokaListModel>(context, listen: false).setweight);
    device.readInput();
  }

  @override
  void dispose() {
    SerialDevice.disposeSerialPort();
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

    return DefaultTabController(
        length: 2,
        child: Scaffold(
          appBar: AppBar(
            toolbarHeight: kToolbarHeight,
            backgroundColor: Colors.black,
            title: Text('JudoWeight'),
            leading: Builder(
              builder: (context) => ElevatedButton(
                child: Icon(Icons.menu),
                onPressed: () {
                  showPopupMenu(context);
                },
              ),
            ),
            actions: [
              Builder(
                builder: (context) => ElevatedButton(
                  child: Icon(Icons.add),
                  onPressed: () async {
                    final result = await Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => CompetitorEdit(null, null)),
                    );
                    setState(() {});
                  },
                ),
              ),
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
                                  'packages/judolib/assets/flags-ioc/${languageCodeToIOC[value]}.png')),
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
              Builder(
                builder: (context) => ElevatedButton(
                  child: Icon(Icons.settings),
                  onPressed: () async {
                    final result = await Navigator.push(
                      context,
                      MaterialPageRoute(
                          builder: (context) => SettingsScreen(this)),
                    );
                    setState(() {});
                  },
                ),
              ),
            ],
            bottom: TabBar(
              controller: tabController,
              tabs: [
                Tab(icon: Icon(Icons.scale)),
                Tab(icon: Icon(Icons.list)),
              ],
            ),
          ),
          body: TabBarView(
            controller: tabController,
            children: [
              WeightEdit(0, '', '', '', 0),
              JudokaList(),
            ],
          ),
        ));
  }
}
