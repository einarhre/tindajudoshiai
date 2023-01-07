import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:judolib/judolib.dart';
import 'package:judoshiai/competitors.dart';
import 'global.dart';
import 'utils.dart';
import 'package:settings_ui/settings_ui.dart';
import 'package:card_settings/card_settings.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

final GlobalKey<FormState> _formKey = GlobalKey<FormState>();

Future<void> readSettings() async {
  jspassword = await getVal('jspassword', '');
  node_name = await getHostName('jsip');
}

Future<String> getVal(String key, String dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getString(key) ?? dflt;
}

Future<void> setVal(String key, String t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setString(key, t);
}

class SettingsScreen extends StatefulWidget {
  CompetitorsState layout;

  SettingsScreen(this.layout, {Key? key}) : super(key: key);

  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

final GlobalKey<FormState> _jspasswordKey = GlobalKey<FormState>();
final GlobalKey<FormState> _nodeNameKey = GlobalKey<FormState>();

class _SettingsScreenState extends State<SettingsScreen> {
  Future<CardSettingsSection>? _settings;
  AutovalidateMode _autoValidateMode = AutovalidateMode.onUserInteraction;

  Future<CardSettingsSection> runGetSettings() async {
    var t = AppLocalizations.of(widget.layout.context);
    List<CardSettingsWidget> lst = [];

    if (!kIsWeb) {
      lst.add(CardSettingsText(
        key: _nodeNameKey,
        label: t?.addressOfTheCommunicationNodebceb ?? 'JudoShiai address',
        initialValue: node_name,
        maxLength: 64,
        validator: (value) {
          if (value == null || value.isEmpty) return 'IP address is required.';
        },
        onSaved: /*onFieldSubmitted:*/ (value) {
          node_name = value ?? '';
          //title = value!;
          setVal('jsip', node_name);
        },
      ));
    }

    lst = lst +
        [
          CardSettingsPassword(
            key: _jspasswordKey,
            hintText: t?.passwordb341 ?? 'Password',
            label: t?.passwordb341 ?? 'Password',
            initialValue: jspassword,
            maxLength: 64,
            onSaved: (value) {
              jspassword = value ?? '';
              setVal('jspassword', jspassword);
            },
          ),
          CardSettingsButton(
            onPressed: () => savePressed(),
            label: 'Save',
          ),
        ];

    return CardSettingsSection(
      header: CardSettingsHeader(
        label: 'JudoShiai',
      ),
      children: lst,
    );
  }

  Future savePressed() async {
    final form = _formKey.currentState;
    if (form == null)
      return;
    if (form.validate()) {
      form.save();
      Navigator.pop(context);
    } else {
      setState(() => _autoValidateMode = AutovalidateMode.onUserInteraction);
    }
  }

  @override
  void initState() {
    super.initState();
    _settings = runGetSettings() as Future<CardSettingsSection>?;
  }

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(widget.layout.context);
    return Scaffold(
        appBar: AppBar(title: Text('Settings UI')),
        body: Form(
            key: _formKey,
            child: FutureBuilder<CardSettingsSection>(
                initialData: null,
                future: _settings,
                builder: (BuildContext context,
                    AsyncSnapshot<CardSettingsSection> snapshot) {
                  print('State: ${snapshot.connectionState}');
                  if (snapshot.connectionState == ConnectionState.waiting) {
                    return CircularProgressIndicator();
                  } else if (snapshot.connectionState == ConnectionState.done) {
                    if (snapshot.hasError) {
                      print('ERROR ${snapshot.error}');
                      return Text(t?.error902b ?? '');
                    } else if (snapshot.hasData) {
                      print('Settings HAS DATA');
                      return CardSettings(
                          labelWidth: 200.0,
                          children: <CardSettingsSection>[
                            snapshot.data as CardSettingsSection
                          ]);
                    } else {
                      print('NO DATA');
                      return Center(
                        child: CircularProgressIndicator(
                          strokeWidth: 3,
                        ),
                      );
                    }
                  } else {
                    return Text('State: ${snapshot.connectionState}');
                  }
                })));



    /***
    return Scaffold(
        appBar: AppBar(title: Text('Settings UI')),
        body: Form(
            key: _formKey,
            child: FutureBuilder<CardSettingsSection>(
                initialData: null,
                future: _settings,
                builder: (BuildContext context,
                    AsyncSnapshot<CardSettingsSection> snapshot) {
                  print('State: ${snapshot.connectionState}');
                  if (snapshot.connectionState == ConnectionState.waiting) {
                    return CircularProgressIndicator();
                  } else if (snapshot.connectionState == ConnectionState.done) {
                    if (snapshot.hasError) {
                      print('ERROR ${snapshot.error}');
                      return const Text('Error');
                    } else if (snapshot.hasData) {
                      print('Settings HAS DATA');
                      return CardSettings(children: <CardSettingsSection>[
                        snapshot.data as CardSettingsSection
                      ]);
                    } else {
                      print('NO DATA');
                      return Center(
                        child: CircularProgressIndicator(
                          strokeWidth: 3,
                        ),
                      );
                    }
                  } else {
                    return Text('State: ${snapshot.connectionState}');
                  }
                })));
        ****/
  }
}
