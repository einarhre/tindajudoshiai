import 'dart:core';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_form_builder/flutter_form_builder.dart';
import 'package:form_builder_validators/form_builder_validators.dart';
import 'package:intl/date_symbol_data_local.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:judolib/judolib.dart';
import 'package:judoweight/comm.dart';

import 'package:judoweight/database.dart';
import 'package:mobile_scanner/mobile_scanner.dart';
import 'package:provider/provider.dart';

import 'package:camera/camera.dart';

import 'bloc.dart';
import 'main.dart';

class WeightEdit extends StatefulWidget {
  final int ix;
  String last, first, regcat;
  int weight;

  WeightEdit(this.ix, this.last, this.first, this.regcat, this.weight) {}

  @override
  _WeightEditState createState() => _WeightEditState();
}

final _formKey = GlobalKey<_WeightEditState>();

class _WeightEditState extends State<WeightEdit> {
  bool autoValidate = true;
  bool readOnly = false;
  bool showSegmentedControl = true;
  final _formKey = GlobalKey<FormBuilderState>();
  bool _yobHasError = false;
  bool _weightHasError = false;
  late var txtCtl1;
  late var txtCtl2;
  late var txtCtl3;
  late var txtCtl4;
  late var txtCtl5;
  late FocusNode ixFocusNode;
  late FocusNode weightFocusNode;

  MobileScannerController cameraController = MobileScannerController();
  bool cameraEnabled = false;

  void _onChanged(dynamic val) => debugPrint(val.toString());

  @override
  void initState() {
    super.initState();
    initializeDateFormatting('en', '');

    txtCtl1 = TextEditingController();
    txtCtl2 = TextEditingController();
    txtCtl3 = TextEditingController();
    txtCtl4 = TextEditingController();
    txtCtl5 = TextEditingController();

    ixFocusNode = FocusNode();
    weightFocusNode = FocusNode();

    //cameraController.stop();
    /*
    txtCtl1.addListener(() {
      final String text = txtCtl1.text.toLowerCase();
      txtCtl1.value = txtCtl1.value.copyWith(
        text: text,
        selection:
        TextSelection(baseOffset: text.length, extentOffset: text.length),
        composing: TextRange.empty,
      );
    });

     */
  }

  @override
  void dispose() {
    ixFocusNode.dispose();
    weightFocusNode.dispose();
    txtCtl1.dispose();
    txtCtl2.dispose();
    txtCtl3.dispose();
    txtCtl4.dispose();
    txtCtl5.dispose();

    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final JudokaListModel ixProvider =
    Provider.of<JudokaListModel>(context, listen: true);
    var t = AppLocalizations.of(context);

    txtCtl1.text = ixProvider.ixtext;
    txtCtl2.text = ixProvider.regcattext;
    txtCtl3.text = ixProvider.lasttext;
    txtCtl4.text = ixProvider.firsttext;
    txtCtl5.text = ixProvider.weighttext;

    txtCtl5.addListener(() {
      final String text = txtCtl5.text;
      print('TEXT=$text');
      //ixProvider.setweight(text);
    });

    return Scaffold(
        body: Container(
            child: Padding(
                padding: const EdgeInsets.all(10),
                child: SingleChildScrollView(
                    child: Column(
                        mainAxisSize: MainAxisSize.max,
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: <Widget>[
                          FormBuilder(
                              key: _formKey,
                              // enabled: false,
                              autovalidateMode: AutovalidateMode.disabled,
                              skipDisabled: true,
                              child: Center(
                                  child: Column(
                                      mainAxisSize: MainAxisSize.max,
                                      mainAxisAlignment: MainAxisAlignment
                                          .center,
                                      children: [
                                        Table(
                                          /*columnWidths: const <int, TableColumnWidth>{
                            0: IntrinsicColumnWidth(),
                            1: IntrinsicColumnWidth(),
                          },*/
                                            defaultVerticalAlignment:
                                            TableCellVerticalAlignment.middle,
                                            children: <TableRow>[
                                              TableRow(children: <Widget>[
                                                FormBuilderTextField(
                                                  autovalidateMode:
                                                  AutovalidateMode.disabled,
                                                  autofocus: true,
                                                  controller: txtCtl1,
                                                  //onChanged: () {ixProvider.setname;},
                                                  focusNode: ixFocusNode,
                                                  name: 'ix',
                                                  enabled: widget.ix > 0
                                                      ? false
                                                      : true,
                                                  //initialValue: widget.ix > 0 ? widget.ix.toString() : '',
                                                  decoration: InputDecoration(
                                                    labelText: t?.iDb718 ?? '',
                                                  ),
                                                  keyboardType: TextInputType
                                                      .number,
                                                  textInputAction: TextInputAction
                                                      .next,
                                                  onEditingComplete: () async {
                                                    print('SUBMITTED ${txtCtl1
                                                        .text}');
                                                    int i =
                                                        int.tryParse(
                                                            txtCtl1.text) ?? 0;
                                                    if (i > 0) {
                                                      Judoka? j = await Provider
                                                          .of<
                                                          JudokaListModel>(
                                                          context,
                                                          listen: false)
                                                          .getJudokaDbOrJs(i,
                                                          save: true);
                                                      if (j != null) {
                                                        ixProvider.setregcat(
                                                            j.regcat);
                                                        ixProvider.setlast(
                                                            j.last);
                                                        ixProvider.setfirst(
                                                            j.first);
                                                      }
                                                    }
                                                    //txtCtl5.text = '';
                                                    ixProvider.setix(
                                                        i.toString());
                                                    ixProvider.setweight('');
                                                    weightFocusNode
                                                        .requestFocus();
                                                  },
                                                ),
                                                FormBuilderTextField(
                                                  autovalidateMode:
                                                  AutovalidateMode.disabled,
                                                  controller: txtCtl2,
                                                  enabled: false,
                                                  name: 'regcat',
                                                  //initialValue: widget.regcat,
                                                  decoration: InputDecoration(
                                                    labelText: t
                                                        ?.regCategory6063 ?? '',
                                                  ),
                                                  //validator: FormBuilderValidators.compose([
                                                  //  FormBuilderValidators.required(),
                                                  //]),
                                                  keyboardType: TextInputType
                                                      .text,
                                                  textInputAction: TextInputAction
                                                      .next,
                                                ),
                                              ]),
                                              TableRow(children: <Widget>[
                                                FormBuilderTextField(
                                                  autovalidateMode:
                                                  AutovalidateMode.disabled,
                                                  controller: txtCtl3,
                                                  enabled: false,
                                                  name: 'last',
                                                  //initialValue: widget.last,
                                                  decoration: InputDecoration(
                                                    labelText: t
                                                        ?.lastName7758 ?? '',
                                                  ),
                                                  //validator: FormBuilderValidators.compose([
                                                  //  FormBuilderValidators.required(),
                                                  //]),
                                                  // initialValue: '12',
                                                  keyboardType: TextInputType
                                                      .name,
                                                  textInputAction: TextInputAction
                                                      .next,
                                                ),
                                                FormBuilderTextField(
                                                  autovalidateMode:
                                                  AutovalidateMode.disabled,
                                                  controller: txtCtl4,
                                                  enabled: false,
                                                  name: 'first',
                                                  //initialValue: widget.first,
                                                  decoration: InputDecoration(
                                                    labelText: t
                                                        ?.firstNamebc91 ?? '',
                                                  ),
                                                  //validator: FormBuilderValidators.compose([
                                                  //  FormBuilderValidators.required(),
                                                  //]),
                                                  // initialValue: '12',
                                                  keyboardType: TextInputType
                                                      .name,
                                                  textInputAction: TextInputAction
                                                      .next,
                                                ),
                                              ]),
                                            ]),
                                        Row(children: [
                                          Spacer(),
                                          SizedBox(
                                              width: 350.0,
                                              height: 108,
                                              child: FormBuilderTextField(
                                                maxLength: 6,
                                                controller: txtCtl5,
                                                focusNode: weightFocusNode,
                                                autovalidateMode:
                                                AutovalidateMode.disabled,
                                                name: 'weight',
                                                //initialValue: ixProvider.weighttext,
                                                inputFormatters: [
                                                  FilteringTextInputFormatter
                                                      .allow(
                                                      RegExp(r'^\d*[\.,]?\d*'))
                                                ],
                                                decoration: InputDecoration(
                                                  labelText: t?.weight0ae0 ??
                                                      '',
                                                  fillColor: Colors.yellow,
                                                  labelStyle: TextStyle(
                                                      color: Colors.black,
                                                      letterSpacing: 1.3,
                                                      fontSize: 32),
                                                ),
                                                validator:
                                                FormBuilderValidators.compose([
                                                  FormBuilderValidators
                                                      .required(),
                                                ]),
                                                keyboardType: TextInputType
                                                    .number,
                                                onEditingComplete: () {
                                                  if (_formKey.currentState
                                                      ?.saveAndValidate() ??
                                                      false) {
                                                    sendAndResetForm(
                                                        _formKey.currentState
                                                            ?.value);
                                                  }
                                                },
                                              )),
                                          Spacer(),

                                          Builder(
                                              builder: (context) {
                                                if (cameraEnabled) {
                                                  //cameraController = MobileScannerController(facing: CameraFacing.values.first);
                                                  //cameraController.facing = CameraFacing.values.last;
                                                  var w = SizedBox(
                                                    width: 192,
                                                    //MediaQuery.of(context).size.width/4,
                                                    height: 108,
                                                    //MediaQuery.of(context).size.height/2,
                                                    child: MobileScanner(
                                                        allowDuplicates: false,
                                                        controller: cameraController,
                                                        fit: BoxFit.contain,
                                                        onDetect: (barcode,
                                                            args) {
                                                          if (barcode
                                                              .rawValue ==
                                                              null) {
                                                            debugPrint(
                                                                'Failed to scan Barcode');
                                                          } else {
                                                            final String code =
                                                            barcode.rawValue!;
                                                            debugPrint(
                                                                'Barcode found! $code');
                                                            var d = code.split(
                                                                '\t');

                                                            if (d.length > 0)
                                                              txtCtl1.text =
                                                              d[0];
                                                            if (d.length > 1)
                                                              txtCtl2.text =
                                                              d[1];
                                                            if (d.length > 2)
                                                              txtCtl3.text =
                                                              d[2];
                                                            if (d.length > 3)
                                                              txtCtl4.text =
                                                              d[3];
                                                            txtCtl5.text = '';

                                                            weightFocusNode
                                                                .requestFocus();
                                                          }
                                                        }),
                                                  );
                                                  //cameraController.stop();
                                                  //cameraController.start();
                                                  return w;
                                                } else return Icon(Icons.desktop_access_disabled);
                                              }),

                                          IconButton(
                                            color: Colors.black,
                                            icon: ValueListenableBuilder(
                                              valueListenable: cameraController.cameraFacingState,
                                              builder: (context, state, child) {
                                                if (!cameraEnabled)
                                                  return const Icon(Icons.camera_alt);

                                                switch (state as CameraFacing) {
                                                  case CameraFacing.front:
                                                    return const Icon(Icons.camera_front);
                                                  case CameraFacing.back:
                                                    return const Icon(Icons.camera_rear);
                                                }
                                              },
                                            ),
                                            iconSize: 32.0,
                                            onPressed: () {
                                              if (!cameraEnabled) {
                                                setState(() {
                                                  cameraEnabled = true;
                                                });
                                              } else cameraController?.switchCamera();
                                            }
                                          ),

                                        ]),
                                      ]))),
                          Row(
                            children: <Widget>[
                              Expanded(
                                child: MaterialButton(
                                  color: Theme
                                      .of(context)
                                      .colorScheme
                                      .secondary,
                                  onPressed: () async {
                                    if (_formKey.currentState
                                        ?.saveAndValidate() ??
                                        false) {
                                      debugPrint(
                                          _formKey.currentState?.value
                                              .toString());
                                      sendAndResetForm(
                                          _formKey.currentState?.value);
                                      //Provider.of<JudokaListModel>(context, listen: false).db.saveWeight(widget.ix, _formKey.currentState?.value);
                                      //Navigator.pop(context, '');
                                    } else {
                                      debugPrint(
                                          _formKey.currentState?.value
                                              .toString());
                                      debugPrint('validation failed');
                                    }
                                    txtCtl1.text = '';
                                    ixFocusNode.requestFocus();
                                  },
                                  child: const Text(
                                    'OK',
                                    style: TextStyle(color: Colors.white),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ])))));
  }

  Future<void> sendAndResetForm(a) async {
    var json = await Provider
        .of<JudokaListModel>(context, listen: false)
        .db
        .saveWeight(widget.ix, a);

    if (json != null) {
      txtCtl2.text = json['regcat'] ?? '';
      txtCtl3.text = json['last'] ?? '';
      txtCtl4.text = json['first'] ?? '';
    } else {
      txtCtl2.text = '';
      txtCtl3.text = '';
      txtCtl4.text = '';
      txtCtl5.text = '';
    }
    txtCtl1.text = '';
    ixFocusNode.requestFocus();
  }
}
