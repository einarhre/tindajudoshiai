import 'package:flutter/material.dart';
import 'package:flutter/foundation.dart' show kIsWeb;
import 'package:judolib/judolib.dart';
import 'package:judoweight/homescreen.dart';

import 'global.dart';
import 'package:card_settings/card_settings.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'print.dart';

var jspassword = '';
final GlobalKey<FormState> _formKey = GlobalKey<FormState>();

Future<void> readSettings() async {
  languageCode = await getVal('languageCode', 'en');
  countryCode = await getVal('countryCode', '');
  jspassword = await getVal('jspassword', '');
  pageSize = await getVal('pagesize', 'a4');
  printerName = await getVal('printer', '');
  printLabel = await getValBool('printlabel', false);
  printNoDialog = await getValBool('printnodialog', false);
  printRollWidth = await getValInt('rollwidth', 50);
  SerialDevice.selected_baudrate = await getValInt('baudrate', 0);
  SerialDevice.serialType = await getValInt('serialType', 0);
  SerialDevice.serialDevice = await getVal('serialDevice', '');
}

const List<PickerModel> baudrates = <PickerModel>[
  PickerModel("1200,N81", code: 0),
  PickerModel("9600,N81", code: 1),
  PickerModel("19200,N81", code: 2),
  PickerModel("38400,N81", code: 3),
  PickerModel("115200,N81", code: 4),
];

const List<PickerModel> serialTypes = <PickerModel>[
  PickerModel("Normal", code: DEV_TYPE_NORMAL),
  PickerModel("Stathmos/Allvåg", code: DEV_TYPE_STATHMOS),
  PickerModel("AP-1", code: DEV_TYPE_AP1),
  PickerModel("My Weight", code: DEV_TYPE_MYWEIGHT),
];

/********************************************/

class SettingsScreen extends StatefulWidget {
  HomeScreenState layout;

  SettingsScreen(this.layout, {Key? key}) : super(key: key);

  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

final GlobalKey<FormState> _printerNameKey = GlobalKey<FormState>();
final GlobalKey<FormState> _printLabelKey = GlobalKey<FormState>();
final GlobalKey<FormState> _printNoDialogKey = GlobalKey<FormState>();
final GlobalKey<FormState> _pageSizeKey = GlobalKey<FormState>();
final GlobalKey<FormState> _rollWidthKey = GlobalKey<FormState>();
final GlobalKey<FormState> _nodeNameKey = GlobalKey<FormState>();
final GlobalKey<FormState> _baudrateKey = GlobalKey<FormState>();
final GlobalKey<FormState> _serialTypeKey = GlobalKey<FormState>();
final GlobalKey<FormState> _serialPortKey = GlobalKey<FormState>();
final GlobalKey<FormState> _jspasswordKey = GlobalKey<FormState>();

class _SettingsScreenState extends State<SettingsScreen> {
  Future<List<CardSettingsSection>>? _settings;
  AutovalidateMode _autoValidateMode = AutovalidateMode.onUserInteraction;

  Future<List<CardSettingsSection>> runGetSettings() async {
    var t = AppLocalizations.of(widget.layout.context);
    var availablePorts = SerialDevice.getAvailablePorts();
    List<CardSettingsSection> lst = [];

    if (!kIsWeb) {
      lst.add(CardSettingsSection(
          header: CardSettingsHeader(
            label: t?.communicationNode09de ?? '',
          ),
          children: <CardSettingsWidget>[
            CardSettingsText(
              key: _nodeNameKey,
              label: t?.iPAddressf82d ?? '',
              initialValue: node_name,
              maxLength: 64,
              validator: (value) {
                if (value == null || value.isEmpty)
                  return 'IP address is required.';
              },
              onSaved: (value) {
                node_name = value ?? '';
                //title = value!;
                setVal('jsip', value);
              },
            ),
          ]));
    }

    lst.add(CardSettingsSection(
        header: CardSettingsHeader(
          label: t?.printer37fe ?? '',
        ),
        children: <CardSettingsWidget>[
          CardSettingsPassword(
            key: _jspasswordKey,
            hintText: t?.passwordb341 ?? 'Password',
            label: t?.passwordb341 ?? 'Password',
            initialValue: jspassword,
            maxLength: 64,
            onSaved: (value) {
              jspassword = value ?? '';
              setVal('jspassword', value);
            },
          ),
          CardSettingsSwitch(
            key: _printLabelKey,
            label: 'Print label',
            initialValue: printLabel,
            onSaved: (value) {
              setValBool('printLabel', value ?? false);
              setState(() {
                printLabel = value ?? false;
              });
            },
          ),
          CardSettingsSwitch(
            key: _printNoDialogKey,
            label: 'No print dialog',
            initialValue: printNoDialog,
            onSaved: (value) {
              setValBool('printnodialog', value ?? false);
              setState(() {
                printNoDialog = value ?? false;
              });
            },
          ),
          CardSettingsRadioPicker<String>(
            key: _printerNameKey,
            label: t?.printer37fe ?? '',
            initialItem: printerName,
            hintText: t?.selecte062 ?? '',
            items: await listPrinters(),
            onSaved: (value) {
              printerName = value ?? '';
              setVal('printer', value);
            },
          ),
          CardSettingsRadioPicker<String>(
            key: _pageSizeKey,
            label: 'Page Size',
            initialItem: pageSize,
            hintText: t?.selecte062 ?? '',
            items: pageSizes,
            onSaved: (value) {
              pageSize = value ?? '';
              setVal('pagesize', value);
            },
          ),
          CardSettingsInt(
            key: _rollWidthKey,
            label: 'Roll width',
            initialValue: printRollWidth,
            autovalidateMode: _autoValidateMode,
            validator: (value) {
              if (value != null) {
                if (value > 100) return 'Number must be 20 - 100';
                if (value < 20) return 'Number must be 20 - 100';
              }
              return null;
            },
            onSaved: (value) {
              setValInt('rollwidth', value ?? 50);
              setState(() {
                printRollWidth = value ?? 50;
              });
            },
          ),
        ]));

    if (!kIsWeb) {
      lst.add(CardSettingsSection(
          header: CardSettingsHeader(
            label: t?.serialCommunicationc0a1 ?? '',
          ),
          children: <CardSettingsWidget>[
            CardSettingsRadioPicker<String>(
              key: _serialPortKey,
              label: t?.devicee0ac ?? '',
              initialItem: SerialDevice.serialDevice,
              hintText: 'Select One',
              items: availablePorts,
              onSaved: (value) {
                SerialDevice.serialDevice = value ?? '';
                setVal('serialDevice', value);
                SerialDevice.restartSerialPort();
              },
            ),
            CardSettingsRadioPicker<PickerModel>(
              key: _baudrateKey,
              label: t?.baudratece01 ?? '',
              initialItem: baudrates.firstWhere(
                  (element) => element.code == SerialDevice.selected_baudrate),
              hintText: 'Select One',
              items: baudrates,
              onSaved: (PickerModel? value) {
                SerialDevice.selected_baudrate = value?.code as int;
                setValInt('baudrate', SerialDevice.selected_baudrate);
                SerialDevice.restartSerialPort();
              },
            ),
            CardSettingsRadioPicker<PickerModel>(
              key: _serialTypeKey,
              label: t?.typea1fa ?? '',
              initialItem: serialTypes.firstWhere(
                  (element) => element.code == SerialDevice.serialType),
              hintText: 'Select One',
              items: serialTypes,
              onSaved: (PickerModel? value) {
                SerialDevice.serialType = value?.code as int;
                setValInt('serialType', SerialDevice.serialType);
                SerialDevice.restartSerialPort();
              },
            ),
          ]));
    }

    lst.add(CardSettingsSection(
        header: CardSettingsHeader(
          label: '',
        ),
        children: <CardSettingsWidget>[
          CardSettingsButton(
            onPressed: () => savePressed(),
            label: 'Save',
          )
        ]));

    return lst;
  }

  Future savePressed() async {
    final form = _formKey.currentState;
    if (form == null) return;
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
    _settings = runGetSettings() as Future<List<CardSettingsSection>>?;
  }

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(widget.layout.context);
    return Scaffold(
        appBar: AppBar(title: Text('Settings UI')),
        body: Form(
            key: _formKey,
            child: FutureBuilder<List<CardSettingsSection>>(
                initialData: null,
                future: _settings,
                builder: (BuildContext context,
                    AsyncSnapshot<List<CardSettingsSection>> snapshot) {
                  print('State: ${snapshot.connectionState}');
                  if (snapshot.connectionState == ConnectionState.waiting) {
                    return CircularProgressIndicator();
                  } else if (snapshot.connectionState == ConnectionState.done) {
                    if (snapshot.hasError) {
                      print('ERROR1 ${snapshot.error}');
                      return Text(t?.error902b ?? '');
                    } else if (snapshot.hasData) {
                      print('Settings HAS DATA');
                      return CardSettings(
                          children: snapshot.data as List<CardSettingsSection>);
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
