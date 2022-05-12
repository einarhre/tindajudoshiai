import 'dart:convert';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:judotimer/settings.dart';
import 'package:judotimer/show_comp.dart';
import 'package:judotimer/stopwatch.dart';
import 'package:judotimer/util.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'global.dart';
import 'label.dart';
import 'layout.dart';

final navigatorKey = GlobalKey<NavigatorState>();

void main() {
  // https://stackoverflow.com/questions/65307961/button-to-change-the-language-flutter
  // To solve problem (ServicesBinding.defaultBinaryMessenger was accessed before the binding was initialized)
  WidgetsFlutterBinding.ensureInitialized();

  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  static void setLocale(BuildContext context, Locale newLocale) async {
    _MyAppState? state = context.findAncestorStateOfType<_MyAppState>();

    var prefs = await SharedPreferences.getInstance();
    prefs.setString('languageCode', newLocale.languageCode);
    prefs.setString('countryCode', "");

    state?.setState(() {
      state._locale = newLocale;
    });
  }

  @override
  _MyAppState createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  Locale _locale = Locale('en', '');

  @override
  void initState() {
    super.initState();
    this._fetchLocale().then((locale) {
      setState(() {
        this._locale = locale;
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      locale: _locale,
      title: 'JudoTimer',
      localizationsDelegates: [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        const FallbackCupertinoLocalisationsDelegate(),
      ],
      supportedLocales: [
        // 'en' is the language code. We could optionally provide a
        // a country code as the second param, e.g.
        // Locale('en', 'US'). If we do that, we may want to
        // provide an additional app_en_US.arb file for
        // region-specific translations.
        const Locale('fi', ''),
        const Locale('sv', ''),
        const Locale('en', ''),
        const Locale('es', ''),
        const Locale('et', ''),
        const Locale('uk', ''),
        const Locale('is', ''),
        const Locale('nb', ''),
        const Locale('pl', ''),
        const Locale('sk', ''),
        const Locale('nl', ''),
        const Locale('cs', ''),
        const Locale('de', ''),
        const Locale('da', ''),
        const Locale('he', ''),
        const Locale('fr', ''),
        const Locale('fa', ''),
      ],
      theme: ThemeData(
        // This is the theme of your application.
        //
        // Try running your application with "flutter run". You'll see the
        // application has a blue toolbar. Then, without quitting the app, try
        // changing the primarySwatch below to Colors.green and then invoke
        // "hot reload" (press "r" in the console where you ran "flutter run",
        // or simply save your changes to "hot reload" in a Flutter IDE).
        // Notice that the counter didn't reset back to zero; the application
        // is not restarted.
        primarySwatch: Colors.blue,
      ),
      home: const MyHomePage(title: 'JudoTimer'),
    );
  }

  Future<Locale> _fetchLocale() async {
    var prefs = await SharedPreferences.getInstance();

    String languageCode = prefs.getString('languageCode') ?? 'en';
    String countryCode = prefs.getString('countryCode') ?? '';

    return Locale(languageCode, countryCode);
  }

}

class MyHomePage extends StatefulWidget {
  const MyHomePage({Key? key, required this.title}) : super(key: key);

  // This widget is the home page of your application. It is stateful, meaning
  // that it has a State object (defined below) that contains fields that affect
  // how it looks.

  // This class is the configuration for the state. It holds the values (in this
  // case the title) provided by the parent (in this case the App widget) and
  // used by the build method of the State. Fields in a Widget subclass are
  // always marked "final".

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  late double width, height;
  Future<List<Label>>? labels;

  //Future<String>? _layout;

  @override
  void initState() {
    super.initState();
    labels = getTimerCustom();
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

    return FutureBuilder<List<Label>>(
        //initialData: null,
        future: labels,
        builder: (BuildContext context, AsyncSnapshot<List<Label>> snapshot) {
          print('State: ${snapshot.connectionState}');
          if (snapshot.connectionState == ConnectionState.waiting) {
            return CircularProgressIndicator();
          } else if (snapshot.connectionState == ConnectionState.done) {
            if (snapshot.hasError) {
              print('ERROR ${snapshot.error}');
              return const Text('Error');
            } else if (snapshot.hasData) {
              return Layout(width, height2, snapshot.data!);
            }
          }
          return Text('State: ${snapshot.connectionState}');
        });
  }
}

/*
  To solve problem of hold press on inputs
 */
class FallbackCupertinoLocalisationsDelegate extends LocalizationsDelegate<CupertinoLocalizations> {
  const FallbackCupertinoLocalisationsDelegate();

  @override
  bool isSupported(Locale locale) => true;

  @override
  Future<CupertinoLocalizations> load(Locale locale) =>
      DefaultCupertinoLocalizations.load(locale);

  @override
  bool shouldReload(FallbackCupertinoLocalisationsDelegate old) => false;
}
