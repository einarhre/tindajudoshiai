import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:card_settings/card_settings.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'package:judolib/judolib.dart';
import 'global.dart';
import 'layout.dart';

final GlobalKey<FormState> _formKey = GlobalKey<FormState>();

Future<void> readSettings() async {
  node_name = await getHostName('jsip');
  jspassword = await getVal('jspassword', '');
  languageCode = await getVal('languageCode', 'en');
  countryCode = await getVal('countryCode', '');
  fullscreen = await getValBool('fullscreen', false);
  mirror = await getValBool('mirror', false);

  var n = 0;
  for (var i = 0; i < 20; i++) {
    tatamis[i] = await getValBool('tatami${i + 1}', false);
    if (tatamis[i]) n++;
  }
  if (n == 0) tatamis[0] = true;
}

class SettingsScreen extends StatefulWidget {
  LayoutState layout;

  SettingsScreen(this.layout, {Key? key}) : super(key: key);

  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

final GlobalKey<FormState> _nodeNameKey = GlobalKey<FormState>();
final GlobalKey<FormState> _jspasswordKey = GlobalKey<FormState>();
final GlobalKey<FormState> _textSizeKey = GlobalKey<FormState>();
final GlobalKey<FormState> _fullscreenKey = GlobalKey<FormState>();
final GlobalKey<FormState> _mirrorKey = GlobalKey<FormState>();
final List<GlobalKey<FormState>> _tatami_keys = new List.generate(20, (_) => GlobalKey<FormState>());

class _SettingsScreenState extends State<SettingsScreen> {
  Future<CardSettingsSection>? _settings;
  AutovalidateMode _autoValidateMode = AutovalidateMode.onUserInteraction;

  Future<CardSettingsSection> runGetSettings() async {
    var t = AppLocalizations.of(widget.layout.context);
    List<CardSettingsWidget> lst = [];
    if (!kIsWeb) {
      lst = [
        CardSettingsText(
          key: _nodeNameKey,
          label: 'JudoShiai address',
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
      ];
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
              setVal('jspassword', value);
            },
          ),

          CardSettingsSwitch(
            key: _fullscreenKey,
            label: t?.fullScreenMode89ed ?? '',
            initialValue: fullscreen,
            onSaved: (value) {
              setValBool('fullscreen', value ?? false);
              setState(() {
                fullscreen = value ?? false;
              });
            },
          ),

          CardSettingsSwitch(
            key: _mirrorKey,
            label: t?.mirrorTatamiOrder9066 ?? '',
            initialValue: mirror,
            onSaved: (value) {
              setValBool('mirror', value ?? false);
              setState(() {
                mirror = value ?? false;
              });
            },
          ),

          CardSettingsSlider(
            key: _textSizeKey,
            label: 'Text size',
            min: 8.0,
            max: 30.0,
            initialValue: textSize,
            onSaved: (value) {
              setState(() {
                textSize = value ?? 14.0;
              });
              setValDouble('textSize', textSize);
            },
          ),
        ];

    for (var i = 0; i < 20; i++) {
      lst.add(CardSettingsSwitch(
        key: _tatami_keys[i],
        label: '${t?.showTatami9539 ?? "Show Tatami"} ${i+1}',
        initialValue: tatamis[i],
        onSaved: (value) {
          setValBool('tatami${i+1}', value ?? false);
          setState(() {
            tatamis[i] = value ?? false;
          });
        },
      ));
    }

    lst.add(CardSettingsButton(
      onPressed: () => savePressed(),
      label: 'Save',
    ));

    return CardSettingsSection(
        header: CardSettingsHeader(
          label: 'JudoInfo',
        ),
        children: lst);
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
                  if (snapshot.connectionState == ConnectionState.waiting) {
                    return CircularProgressIndicator();
                  } else if (snapshot.connectionState == ConnectionState.done) {
                    if (snapshot.hasError) {
                      return Text(t?.error902b ?? '');
                    } else if (snapshot.hasData) {
                      return CardSettings(children: <CardSettingsSection>[
                        snapshot.data as CardSettingsSection
                      ]);
                    } else {
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
