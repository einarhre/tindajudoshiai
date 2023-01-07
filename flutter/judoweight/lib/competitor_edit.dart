

import 'package:flutter/material.dart';
import 'package:flutter_form_builder/flutter_form_builder.dart';
import 'package:form_builder_validators/form_builder_validators.dart';
import 'package:intl/date_symbol_data_local.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:judolib/judolib.dart';

import 'package:judoweight/database.dart';
import 'package:provider/provider.dart';

import 'bloc.dart';

class CompetitorEdit extends StatefulWidget {
  final Judoka? compOrNull;
  final List<String>? categoryNames;
  late Judoka competitor;

  CompetitorEdit(this.compOrNull, this.categoryNames) {
    competitor = compOrNull ?? Judoka(
        ix: 0,
        last: '',
        first: '',
        club: '',
        country: '',
        regcat: '',
        category: '',
        id: '',
        coachid: '',
        flags: 0,
        birthyear: 0,
        comment: '',
        belt: 0,
        weight: 0,
        clubseeding: 0,
        seeding: 0,
        gender: 0,
    );
  }

  @override
  _CompetitorEditState createState() => _CompetitorEditState();
}

final _formKey = GlobalKey<_CompetitorEditState>();

class _CompetitorEditState extends State<CompetitorEdit> {
  bool autoValidate = true;
  bool readOnly = false;
  bool showSegmentedControl = true;
  final _formKey = GlobalKey<FormBuilderState>();
  bool _yobHasError = false;
  bool _weightHasError = false;
  bool _genderHasError = false;

  void _onChanged(dynamic val) => debugPrint(val.toString());

  @override
  void initState() {
    super.initState();
    initializeDateFormatting('en', '');
  }

  @override
  Widget build(BuildContext context) {
    var t = AppLocalizations.of(context);
    return Scaffold(
        body: Container(
            child: Padding(
              padding: const EdgeInsets.all(10),
              child: SingleChildScrollView(
                child: Column(
                  children: <Widget>[
                    FormBuilder(
                      key: _formKey,
                      // enabled: false,
                      autovalidateMode: AutovalidateMode.disabled,
                      skipDisabled: true,
                      child: Column(
                        children: <Widget>[
                          const SizedBox(height: 15),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'last',
                            initialValue: widget.competitor.last,
                            decoration: InputDecoration(
                              labelText: t?.lastNamea76b ?? '',
                            ),
                            // valueTransformer: (text) => num.tryParse(text),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(),
                            ]),
                            // initialValue: '12',
                            keyboardType: TextInputType.name,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'first',
                            initialValue: widget.competitor.first,
                            decoration: InputDecoration(
                              labelText: t?.firstName962c ?? '',
                            ),
                            // valueTransformer: (text) => num.tryParse(text),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(),
                            ]),
                            // initialValue: '12',
                            keyboardType: TextInputType.name,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'yob',
                            initialValue: widget.competitor.birthyear.toString(),
                            decoration: InputDecoration(
                              labelText: t?.yearOfBirthe2c1 ?? '',
                              suffixIcon: _yobHasError
                                  ? const Icon(Icons.error, color: Colors.red)
                                  : const Icon(Icons.check, color: Colors.green),
                            ),
                            onChanged: (val) {
                              setState(() {
                                _yobHasError = !(_formKey.currentState?.fields['yob']
                                    ?.validate() ??
                                    false);
                              });
                            },
                            valueTransformer: (text) => num.tryParse(text!),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(),
                              FormBuilderValidators.numeric(),
                                  (val) {
                                var number = int.tryParse(val ?? '');
                                if (number == 0 || (number! >= 1930 && number <= 2100))
                                  return null;
                                return 'Year must be 0 or between 1930 - 2100';
                              },
                            ]),
                            // initialValue: '12',
                            keyboardType: TextInputType.number,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderDropdown<String>(
                            // autovalidate: true,
                            name: 'grade',
                            decoration: InputDecoration(
                              labelText: t?.grade17a6 ?? '',
                            ),
                            // initialValue: 'Male',
                            allowClear: true,
                            hint: const Text('Select Grade'),
                            items: getGradeOptions()
                                .map((grade) => DropdownMenuItem(
                              value: grade,
                              child: Text(grade),
                            ))
                                .toList(),
                            initialValue: getGradeOptions()[widget.competitor.belt],
                            valueTransformer: (val) => val?.toString(),
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'club',
                            initialValue: widget.competitor.club,
                            decoration: InputDecoration(
                              labelText: t?.club84bd ?? '',
                            ),
                            // valueTransformer: (text) => num.tryParse(text),
                            validator: FormBuilderValidators.compose([
                              //FormBuilderValidators.required(),
                            ]),
                            keyboardType: TextInputType.name,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'country',
                            initialValue: widget.competitor.country,
                            decoration: InputDecoration(
                              labelText: t?.countryf64b ?? '',
                            ),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.max(3),
                            ]),
                            keyboardType: TextInputType.name,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'regcat',
                            initialValue: widget.competitor.regcat,
                            decoration: InputDecoration(
                              labelText: t?.regCategoryd49a ?? '',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            // autovalidate: true,
                            name: 'category',
                            decoration: InputDecoration(
                              labelText: t?.category56a8 ?? '',
                            ),
                            // initialValue: 'Male',
                            initialValue: widget.competitor.category,
                            valueTransformer: (val) => val?.toString(),
                            enabled: false,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'weight',
                            initialValue: weightVal2Str(widget.competitor.weight),
                            decoration: InputDecoration(
                              labelText: t?.weight0ae0 ?? '',
                              suffixIcon: _weightHasError
                                  ? const Icon(Icons.error, color: Colors.red)
                                  : const Icon(Icons.check, color: Colors.green),
                            ),
                            onChanged: (val) {
                              setState(() {
                                _weightHasError = !(_formKey.currentState?.fields['weight']
                                    ?.validate() ??
                                    false);
                              });
                            },
                            // valueTransformer: (text) => num.tryParse(text),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(),
                              FormBuilderValidators.numeric(),
                              FormBuilderValidators.max(300),
                              FormBuilderValidators.min(0),
                            ]),
                            keyboardType: TextInputType.number,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderDropdown<String>(
                            // autovalidate: true,
                            name: 'seeding',
                            initialValue: getSeedingOptions()[widget.competitor.seeding],
                            decoration: InputDecoration(
                              labelText: t?.seedingd950 ?? '',
                            ),
                            allowClear: true,
                            hint: const Text('Select Seeding'),
                            items: getSeedingOptions()
                                .map((s) => DropdownMenuItem(
                              value: s,
                              child: Text(s),
                            ))
                                .toList(),
                            valueTransformer: (val) => val?.toString(),
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'clubseeding',
                            initialValue: widget.competitor.clubseeding.toString(),
                            decoration: InputDecoration(
                              labelText: t?.clubSeeding9b65 ?? '',
                            ),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(),
                              FormBuilderValidators.numeric(),
                              FormBuilderValidators.max(10),
                              FormBuilderValidators.min(0),
                            ]),
                            keyboardType: TextInputType.number,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'id',
                            initialValue: widget.competitor.id,
                            decoration: InputDecoration(
                              labelText: t?.id2b3e ?? '',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'coachid',
                            initialValue: widget.competitor.coachid,
                            decoration: InputDecoration(
                              labelText: t?.coachIda174 ?? '',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderDropdown<String>(
                            // autovalidate: true,
                            name: 'gender',
                            initialValue: getGenderStr(widget.competitor.flags),
                            decoration: InputDecoration(
                              labelText: t?.gendercaa5 ?? '',
                              suffix: _genderHasError
                                  ? const Icon(Icons.error)
                                  : const Icon(Icons.check),
                            ),
                            // initialValue: 'Male',
                            allowClear: true,
                            hint: const Text('Select Gender'),
                            validator: FormBuilderValidators.compose(
                                [FormBuilderValidators.required()]),
                            items: getGenderOptions()
                                .map((gender) => DropdownMenuItem(
                              value: gender,
                              child: Text(gender),
                            ))
                                .toList(),
                            onChanged: (val) {
                              setState(() {
                                _genderHasError = !(_formKey
                                    .currentState?.fields['gender']
                                    ?.validate() ??
                                    false);
                              });
                            },
                            valueTransformer: (val) => val?.toString(),
                          ),
                          FormBuilderDropdown<String>(
                            // autovalidate: true,
                            name: 'control',
                            decoration: InputDecoration(
                              labelText: t?.control2732 ?? '',
                            ),
                            // initialValue: 'Male',
                            hint: const Text('Select Control'),
                            validator: FormBuilderValidators.compose(
                                [FormBuilderValidators.required()]),
                            initialValue: getControlStr(widget.competitor.flags),
                            items: getControlOptions()
                                .map((c) => DropdownMenuItem(
                              value: c,
                              child: Text(c),
                            ))
                                .toList(),
                            valueTransformer: (val) => val?.toString(),
                          ),
                          FormBuilderCheckbox(
                            name: 'hansokumake',
                            initialValue: getHansokumake(widget.competitor.flags),
                            onChanged: _onChanged,
                            title: RichText(
                              text: const TextSpan(
                                children: [
                                  TextSpan(
                                    text: 'Hansokumake',
                                    style: TextStyle(color: Colors.black),
                                  ),
                                ],
                              ),
                            ),
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'comment1',
                            initialValue: '',
                            decoration: InputDecoration(
                              labelText: t?.comment240f ?? '',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'comment2',
                            initialValue: '',
                            decoration: InputDecoration(
                              labelText: t?.comment240f ?? '',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'comment3',
                            initialValue: '',
                            decoration: InputDecoration(
                              labelText: t?.comment240f ?? '',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderCheckbox(
                            name: 'hide',
                            initialValue: getNoShow(widget.competitor.flags),
                            onChanged: _onChanged,
                            title: RichText(
                              text: TextSpan(
                                children: [
                                  TextSpan(
                                    text: t?.hideName3823 ?? '',
                                    style: TextStyle(color: Colors.black),
                                  ),
                                ],
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                    Row(
                      children: <Widget>[
                        Expanded(
                          child: MaterialButton(
                            color: Theme.of(context).colorScheme.secondary,
                            onPressed: () async {
                              if (_formKey.currentState?.saveAndValidate() ?? false) {
                                debugPrint(_formKey.currentState?.value.toString());
                                //widget.competitor.saveValue(_formKey.currentState?.value);
                                //await widget.competitor.save();
                                Provider.of<JudokaListModel>(context, listen: false).db.saveValue(widget.competitor, _formKey.currentState?.value);
                                Navigator.pop(context, '');
                              } else {
                                debugPrint(_formKey.currentState?.value.toString());
                                debugPrint('validation failed');
                              }
                            },
                            child: Text(
                              t?.oKe0aa ?? '',
                              style: TextStyle(color: Colors.white),
                            ),
                          ),
                        ),
                        const SizedBox(width: 20),
                        Expanded(
                          child: OutlinedButton(
                            onPressed: () {
                              _formKey.currentState?.reset();
                            },
                            // color: Theme.of(context).colorScheme.secondary,
                            child: Text(
                              t?.resetToDefaults6795 ?? '',
                              style: TextStyle(
                                  color: Theme.of(context).colorScheme.secondary),
                            ),
                          ),
                        ),
                        const SizedBox(width: 20),
                        Expanded(
                          child: OutlinedButton(
                            onPressed: () {
                              Navigator.pop(context);
                            },
                            // color: Theme.of(context).colorScheme.secondary,
                            child: Text(
                              t?.cancelea47 ?? '',
                              style: TextStyle(
                                  color: Theme.of(context).colorScheme.secondary),
                            ),
                          ),
                        ),
                        const SizedBox(width: 20),
                        Expanded(
                          child: OutlinedButton(
                            onPressed: () {
                              Provider.of<JudokaListModel>(context, listen: false).db
                                  .deleteJudoka(widget.competitor.toCompanion(true));
                              Navigator.pop(context, '');
                            },
                            // color: Theme.of(context).colorScheme.secondary,
                            child: Icon(Icons.delete),
                            ),
                          ),
                      ],
                    ),
                  ],
                ),
              ),
            )
        )
    );
  }
}
