import 'package:flutter/material.dart';
import 'global.dart';
import 'util.dart';
import 'package:card_settings/card_settings.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/services.dart';
import 'dart:typed_data';

String title = "Spheria";
String author = "Cody Leet";
String url = "http://www.codyleet.com/spheria";

final GlobalKey<FormState> _formKey = GlobalKey<FormState>();

Future<String> getVal(String key, String dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getString(key) ?? dflt;
}

Future<void> setVal(String key, String? t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setString(key, t == null ? '' : t);
}

Future<bool> getValBool(String key, bool dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getBool(key) ?? dflt;
}

Future<void> setValBool(String key, bool t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setBool(key, t);
}
Future<int> getValInt(String key, int dflt) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  return prefs.getInt(key) ?? dflt;
}

Future<void> setValInt(String key, int t) async {
  final SharedPreferences prefs = await SharedPreferences.getInstance();
  prefs.setInt(key, t);
}

Future<void> readSettings() async {
  tatami = await getValInt('tatami', 1);
  mode_slave = await getValBool('modeslave', false);
  rules_stop_ippon_2 = await getValBool('rulesstopippon', false);
  rules_confirm_match = await getValBool('rulesconfirm', true);
  require_judogi_ok = await getValBool('judogicontrol', false);
  no_big_text = await getValBool('nobigtext', false);
  master_name = await getVal('masterip', 'judotimer$tatami.local');
  node_name = await getHostName();
  sound = await getVal('sound', 'IndutrialAlarm');
  selected_name_layout = await getValInt('namelayout', 0);
  languageCode = await getVal('languageCode', 'fi');
  countryCode = await getVal('countryCode', 'FI');
}

/********************************************/

const List<PickerModel> nameLayouts = <PickerModel>[
  PickerModel("Name Surname, Country/Club", code: 0),
  PickerModel("Surname, Name, Country/Club", code: 1),
  PickerModel("Country/Club  Surname, Name", code: 2),
  PickerModel("Country  Surname, Name", code: 3),
  PickerModel("Club  Surname, Name", code: 4),
  PickerModel("Country Surname", code: 5),
  PickerModel("Club Surname", code: 6),
  PickerModel("Surname, Name", code: 7),
  PickerModel("Surname", code: 8),
  PickerModel("Country", code: 9),
  PickerModel("Club", code: 10),
];

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({Key? key}) : super(key: key);

  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

 final GlobalKey<FormState> _soundFileKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _stopClockOnIpponKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _confirmNewMatchKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _judogiControlKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _noBigTextKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _nodeNameKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _masterNameKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _nameLayoutKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _tatamiKey = GlobalKey<FormState>();
 final GlobalKey<FormState> _modeslaveKey = GlobalKey<FormState>();

class _SettingsScreenState extends State<SettingsScreen> {
  Future<CardSettingsSection>? _settings;
  AutovalidateMode _autoValidateMode = AutovalidateMode.onUserInteraction;


  Future<CardSettingsSection> runGetSettings() async {
    return CardSettingsSection(
      header: CardSettingsHeader(
        label: 'JudoShiai',
      ),
      children: <CardSettingsWidget>[
        CardSettingsInt(
          key: _tatamiKey,
          label: 'Contest area',
          initialValue: tatami,
          autovalidateMode: _autoValidateMode,
          validator: (value) {
            if (value != null) {
              if (value > 20) return 'Number must be 0 - 20';
              if (value < 0) return 'Number must be 0 - 20';
            }
            return null;
          },
          onSaved: (value) => tatami = value ?? 0,
          onChanged: (value) {
            setValInt('tatami', value ?? 0);
            setState(() {
              tatami = value ?? 0;
            });
          },
        ),

        CardSettingsText(
          key: _nodeNameKey,
          label: 'JudoShiai address',
          initialValue: node_name,
          maxLength: 64,
          validator: (value) {
            if (value == null || value.isEmpty)
              return 'IP address is required.';
          },
          onFieldSubmitted: (value) {
            node_name = value;
            //title = value!;
              setVal('jsip', value);
          },
        ),

        CardSettingsText(
          key: _masterNameKey,
          label: 'JudoTimer address',
          initialValue: master_name,
          maxLength: 64,
          validator: (value) {
            if (value == null || value.isEmpty)
              return 'IP address is required.';
          },
          onFieldSubmitted: (value) {
            master_name = value;
            //title = value!;
            setVal('masterip', value);
          },
        ),

        CardSettingsSwitch(
          key: _stopClockOnIpponKey,
          label: 'Stop clock on Ippon',
          initialValue: rules_stop_ippon_2,
          onSaved: (value) {
            rules_stop_ippon_2 = value!;
          },
          onChanged: (value) {
            setValBool('rulesstopippon', value);
            setState(() {
              rules_stop_ippon_2 = value;
            });
          },
        ),

        CardSettingsSwitch(
          key: _confirmNewMatchKey,
          label: 'Confirm New Match',
          initialValue: rules_confirm_match,
          onSaved: (value) {
            rules_confirm_match = value!;
          },
          onChanged: (value) {
            setValBool('rulesconfirm', value);
            setState(() {
              rules_confirm_match = value;
            });
          },
        ),

        CardSettingsSwitch(
          key: _judogiControlKey,
          label: 'Require judogi control',
          initialValue: require_judogi_ok,
          onSaved: (value) {
            require_judogi_ok = value!;
          },
          onChanged: (value) {
            setValBool('judogicontrol', value);
            setState(() {
              require_judogi_ok = value;
            });
          },
        ),

        CardSettingsSwitch(
          key: _noBigTextKey,
          label: 'No SOREMADE/IPPON texts',
          initialValue: no_big_text,
          onSaved: (value) {
              no_big_text = value!;
          },
          onChanged: (value) {
            setValBool('nobigtext', value);
            setState(() {
              no_big_text = value;
            });
          },
        ),

        CardSettingsRadioPicker<PickerModel>(
          key: _nameLayoutKey,
          label: 'Name format',
          initialItem: nameLayouts.firstWhere((element) => element.code == selected_name_layout),
          hintText: 'Select One',
          items: nameLayouts,
          onSaved: (value) {
          },
          onChanged: (PickerModel value) {
            selected_name_layout = value.code as int;
            setValInt('namelayout', selected_name_layout);
            setState(() {
            });
          },
        ),

        CardSettingsRadioPicker<String>(
          key: _soundFileKey,
          label: 'Sound',
          initialItem: sound,
          hintText: 'Select One',
          items: [
            'AirHorn',
            'BikeHorn',
            'CarAlarm',
            'Doorbell',
            'IndutrialAlarm',
            'IntruderAlarm',
            'RedAlert',
            'TrainHorn',
            'TwoToneDoorbell'
          ],
          onSaved: (value) {
            sound = value ?? '';
            setVal('sound', value);
          },
          onChanged: (value) {
            sound = value;
              setVal('sound', value);
          },
        ),

        CardSettingsSwitch(
          key: _modeslaveKey,
          label: 'Slave mode',
          initialValue: mode_slave,
          onSaved: (value) {
            mode_slave = value!;
          },
          onChanged: (value) {
            setValBool('modeslave', value);
            setState(() {
              mode_slave = value;
            });
          },
        ),

        CardSettingsButton(
          onPressed: () => null,
          label: 'Save',
        ),
      ],
    );
  }

  @override
  void initState() {
    super.initState();
    _settings = runGetSettings() as Future<CardSettingsSection>?;
  }

  @override
  Widget build(BuildContext context) {
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
  }
}
