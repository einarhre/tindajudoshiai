import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:judolib/judolib.dart';
import 'package:judotimer/layout.dart';
import 'global.dart';
import 'util.dart';
import 'lang.dart';
import 'package:card_settings/card_settings.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/services.dart';
import 'dart:typed_data';

final GlobalKey<FormState> _formKey = GlobalKey<FormState>();

Future<void> readSettings() async {
  tatami = await getValInt('tatami', 1);
  mode_slave = await getValBool('modeslave', false);
  rules_stop_ippon_2 = await getValBool('rulesstopippon', false);
  rules_confirm_match = await getValBool('rulesconfirm', true);
  require_judogi_ok = await getValBool('judogicontrol', false);
  no_big_text = await getValBool('nobigtext', false);
  master_name = await getVal('masterip', 'judotimer$tatami.local');
  node_name = await getHostName('jsip');
  jspassword = await getVal('jspassword', '');
  sound = await getVal('sound', 'IndutrialAlarm');
  selected_name_layout = await getValInt('namelayout', 7);
  languageCode = await getVal('languageCode', 'en');
  countryCode = await getVal('countryCode', '');
  tv_logo = await getValBool('tvlogo', false);

  oldCategory = await getValInt('category', 0);
  oldNumber = await getValInt('number', 0);
  oldClock = await getValInt('clock', 0);
  oldOsaekomi = await getValInt('osaekomi', 0);
  oldI1 = await getValInt('i1', 0);
  oldW1 = await getValInt('w1', 0);
  oldS1 = await getValInt('s1', 0);
  oldI2 = await getValInt('i2', 0);
  oldW2 = await getValInt('w2', 0);
  oldS2 = await getValInt('s2', 0);
}

var oldClock = 0;
var oldOsaekomi = 0;
var oldI1 = 0, oldW1 = 0, oldS1 = 0;
var oldI2 = 0, oldW2 = 0, oldS2 = 0;
var oldCategory = 0, oldNumber = 0;

/********************************************/

class SettingsScreen extends StatefulWidget {
  LayoutState layout;

  SettingsScreen(this.layout, {Key? key}) : super(key: key);

  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

final GlobalKey<FormState> _soundFileKey = GlobalKey<FormState>();
final GlobalKey<FormState> _stopClockOnIpponKey = GlobalKey<FormState>();
final GlobalKey<FormState> _confirmNewMatchKey = GlobalKey<FormState>();
final GlobalKey<FormState> _judogiControlKey = GlobalKey<FormState>();
final GlobalKey<FormState> _noBigTextKey = GlobalKey<FormState>();
final GlobalKey<FormState> _nodeNameKey = GlobalKey<FormState>();
final GlobalKey<FormState> _jspasswordKey = GlobalKey<FormState>();
final GlobalKey<FormState> _masterNameKey = GlobalKey<FormState>();
final GlobalKey<FormState> _nameLayoutKey = GlobalKey<FormState>();
final GlobalKey<FormState> _tatamiKey = GlobalKey<FormState>();
final GlobalKey<FormState> _modeslaveKey = GlobalKey<FormState>();
final GlobalKey<FormState> _tvlogoKey = GlobalKey<FormState>();

class _SettingsScreenState extends State<SettingsScreen> {
  Future<CardSettingsSection>? _settings;
  AutovalidateMode _autoValidateMode = AutovalidateMode.onUserInteraction;

  Future<CardSettingsSection> runGetSettings() async {
    var t = AppLocalizations.of(widget.layout.context);
    List<PickerModel> nameLayouts = <PickerModel>[
      PickerModel(t?.nameSurnameCountryClub02ec ?? "Name Surname, Country/Club", code: 0),
      PickerModel(t?.surnameNameCountryClub727f ?? "Surname, Name, Country/Club", code: 1),
      PickerModel(t?.countryClubSurnameNamef2a8 ?? "Country/Club  Surname, Name", code: 2),
      PickerModel(t?.countrySurnameName1e6f ?? "Country  Surname, Name", code: 3),
      PickerModel(t?.clubSurnameName9377 ?? "Club  Surname, Name", code: 4),
      PickerModel(t?.countrySurnameffdc ?? "Country Surname", code: 5),
      PickerModel(t?.clubSurnamedd0c ?? "Club Surname", code: 6),
      PickerModel(t?.surnameName7e75 ?? "Surname, Name", code: 7),
      PickerModel(t?.surname8e55 ?? "Surname", code: 8),
      PickerModel(t?.country5971 ?? "Country", code: 9),
      PickerModel(t?.clubaeef ?? "Club", code: 10),
    ];

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
        onSaved:
        /*onFieldSubmitted:*/ (value) {
          node_name = value ?? '';
          //title = value!;
          setVal('jsip', value);
        },
      ));
    }

    lst = lst +
        [
          CardSettingsPassword(
            hintText: t?.passwordb341 ?? 'Password',
            key: _jspasswordKey,
            label: t?.passwordb341 ?? 'Password',
            initialValue: jspassword,
            maxLength: 64,
            onSaved: (value) {
              jspassword = value ?? '';
              setVal('jspassword', value);
            },
          ),

          CardSettingsInt(
            key: _tatamiKey,
            label: t?.contestArea9836 ?? '',
            initialValue: tatami,
            autovalidateMode: _autoValidateMode,
            validator: (value) {
              if (value != null) {
                if (value > 20) return 'Number must be 0 - 20';
                if (value < 0) return 'Number must be 0 - 20';
              }
              return null;
            },
            /*onSaved: (value) {
              tatami = value ?? 0;
              print('SAVED tatami $tatami');
              },*/
            onSaved: (value) {
              setValInt('tatami', value ?? 0);
              setState(() {
                tatami = value ?? 0;
              });
            },
          ),
          CardSettingsSwitch(
            key: _stopClockOnIpponKey,
            label: t?.stopClockOnIppond5e8 ?? '',
            initialValue: rules_stop_ippon_2,
            onSaved: (value) {
              setValBool('rulesstopippon', value ?? false);
              setState(() {
                rules_stop_ippon_2 = value ?? false;
              });
            },
          ),
          CardSettingsSwitch(
            key: _confirmNewMatchKey,
            label: t?.confirmNewMatch67e6 ?? '',
            initialValue: rules_confirm_match,
            onSaved: (value) {
              setValBool('rulesconfirm', value ?? true);
              setState(() {
                rules_confirm_match = value ?? true;
              });
            },
          ),
          CardSettingsSwitch(
            key: _judogiControlKey,
            label: t?.requireJudogiControl6a61 ?? '',
            initialValue: require_judogi_ok,
            onSaved: (value) {
              setValBool('judogicontrol', value ?? false);
              setState(() {
                require_judogi_ok = value ?? false;
              });
            },
          ),
          CardSettingsSwitch(
            key: _noBigTextKey,
            label: t?.noSOREMADEIPPONTexts0385 ?? '',
            initialValue: no_big_text,
            onSaved: (value) {
              setValBool('nobigtext', value ?? false);
              setState(() {
                no_big_text = value ?? false;
              });
            },
          ),
          CardSettingsSwitch(
            key: _tvlogoKey,
            label: 'TV logo',
            initialValue: tv_logo,
            onSaved: (value) {
              setValBool('tvlogo', value ?? false);
              setState(() {
                tv_logo = value ?? false;
              });
            },
          ),
          CardSettingsRadioPicker<PickerModel>(
            key: _nameLayoutKey,
            label: t?.nameFormatca21 ?? '',
            initialItem: nameLayouts
                .firstWhere((element) => element.code == selected_name_layout),
            hintText: 'Select One',
            items: nameLayouts,
            onSaved: (PickerModel? value) {
              selected_name_layout = value?.code as int;
              setValInt('namelayout', selected_name_layout);
              setState(() {});
            },
          ),
          CardSettingsRadioPicker<String>(
            key: _soundFileKey,
            label: t?.sound9d07 ?? '',
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
          ),
          CardSettingsText(
            key: _masterNameKey,
            label: 'JudoTimer ${t?.iPAddressf82d ?? "IP address"}',
            initialValue: master_name,
            maxLength: 64,
            validator: (value) {
              if (value == null || value.isEmpty)
                return 'IP address is required.';
            },
            onSaved: (value) {
              master_name = value ?? '';
              //title = value!;
              setVal('masterip', value);
            },
          ),
          CardSettingsSwitch(
            key: _modeslaveKey,
            label: t?.slaveMode4a90 ?? '',
            initialValue: mode_slave,
            onSaved: (value) {
              setValBool('modeslave', value ?? false);
              setState(() {
                mode_slave = value ?? false;
              });
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
  }
}
