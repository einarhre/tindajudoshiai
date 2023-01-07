import 'package:flutter/material.dart';
import 'package:flutter_form_builder/flutter_form_builder.dart';
import 'package:form_builder_validators/form_builder_validators.dart';
//import 'package:intl/date_symbol_data_http_request.dart';
import 'package:intl/intl.dart';
import 'package:intl/date_symbol_data_local.dart';
import 'my_classes.dart';
import 'data.dart';

class CompetitorEdit extends StatefulWidget {
  final JSCompetitor competitor;
  final List<String> categoryNames;

  const CompetitorEdit(this.competitor, this.categoryNames);

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
                      /*
              initialValue: const {
                'movie_rating': 5,
                'best_language': 'Dart',
                'age': '13',
                'gender': 'Male'
              },
              */
                      skipDisabled: true,
                      child: Column(
                        children: <Widget>[
                          const SizedBox(height: 15),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'last',
                            initialValue: widget.competitor.last,
                            decoration: InputDecoration(
                              labelText: 'Last Name',
                            ),
                            // valueTransformer: (text) => num.tryParse(text),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(context),
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
                              labelText: 'First Name',
                            ),
                            // valueTransformer: (text) => num.tryParse(text),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(context),
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
                              labelText: 'Year of Birth',
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
                              FormBuilderValidators.required(context),
                              FormBuilderValidators.numeric(context),
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
                              labelText: 'Grade',
                            ),
                            // initialValue: 'Male',
                            allowClear: true,
                            hint: const Text('Select Grade'),
                            items: widget.competitor.gradeOptions
                                .map((grade) => DropdownMenuItem(
                              value: grade,
                              child: Text(grade),
                            ))
                                .toList(),
                            initialValue: widget.competitor.gradeOptions[widget.competitor.belt],
                            valueTransformer: (val) => val?.toString(),
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'club',
                            initialValue: widget.competitor.club,
                            decoration: InputDecoration(
                              labelText: 'Club',
                            ),
                            // valueTransformer: (text) => num.tryParse(text),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(context),
                            ]),
                            keyboardType: TextInputType.name,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'country',
                            initialValue: widget.competitor.country,
                            decoration: InputDecoration(
                              labelText: 'Country',
                            ),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(context),
                              FormBuilderValidators.minLength(context, 3),
                              FormBuilderValidators.max(context, 3),
                            ]),
                            keyboardType: TextInputType.name,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'regcategory',
                            initialValue: widget.competitor.regcategory,
                            decoration: InputDecoration(
                              labelText: 'Reg.Category',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderDropdown<String>(
                            // autovalidate: true,
                            name: 'category',
                            decoration: InputDecoration(
                              labelText: 'Category',
                            ),
                            // initialValue: 'Male',
                            allowClear: true,
                            hint: const Text('Select Category'),
                            items: widget.categoryNames
                                .map((name) => DropdownMenuItem(
                              value: name,
                              child: Text(name),
                            ))
                                .toList(),
                            initialValue: widget.competitor.category,
                            valueTransformer: (val) => val?.toString(),
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'weight',
                            initialValue: widget.competitor.getWeightStr(),
                            decoration: InputDecoration(
                              labelText: 'Weight',
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
                              FormBuilderValidators.required(context),
                              FormBuilderValidators.numeric(context),
                              FormBuilderValidators.max(context, 300),
                              FormBuilderValidators.min(context, 0),
                            ]),
                            keyboardType: TextInputType.number,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderDropdown<String>(
                            // autovalidate: true,
                            name: 'seeding',
                            initialValue: widget.competitor.seedingOptions[widget.competitor.seeding],
                            decoration: InputDecoration(
                              labelText: 'Seeding',
                            ),
                            allowClear: true,
                            hint: const Text('Select Seeding'),
                            items: widget.competitor.seedingOptions
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
                              labelText: 'Club Seeding',
                            ),
                            validator: FormBuilderValidators.compose([
                              FormBuilderValidators.required(context),
                              FormBuilderValidators.numeric(context),
                              FormBuilderValidators.max(context, 10),
                              FormBuilderValidators.min(context, 0),
                            ]),
                            keyboardType: TextInputType.number,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'id',
                            initialValue: widget.competitor.id,
                            decoration: InputDecoration(
                              labelText: 'Id',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'coachid',
                            initialValue: widget.competitor.coachid,
                            decoration: InputDecoration(
                              labelText: 'Coach Id',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderDropdown<String>(
                            // autovalidate: true,
                            name: 'gender',
                            initialValue: widget.competitor.getGenderStr(),
                            decoration: InputDecoration(
                              labelText: 'Gender',
                              suffix: _genderHasError
                                  ? const Icon(Icons.error)
                                  : const Icon(Icons.check),
                            ),
                            // initialValue: 'Male',
                            allowClear: true,
                            hint: const Text('Select Gender'),
                            validator: FormBuilderValidators.compose(
                                [FormBuilderValidators.required(context)]),
                            items: widget.competitor.genderOptions
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
                              labelText: 'Control',
                            ),
                            // initialValue: 'Male',
                            hint: const Text('Select Control'),
                            validator: FormBuilderValidators.compose(
                                [FormBuilderValidators.required(context)]),
                            initialValue: widget.competitor.getControlStr(),
                            items: widget.competitor.controlOptions
                                .map((c) => DropdownMenuItem(
                              value: c,
                              child: Text(c),
                            ))
                                .toList(),
                            valueTransformer: (val) => val?.toString(),
                          ),
                          FormBuilderCheckbox(
                            name: 'hansokumake',
                            initialValue: widget.competitor.getHansokumake(),
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
                              labelText: 'Comment',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'comment2',
                            initialValue: '',
                            decoration: InputDecoration(
                              labelText: 'Comment',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderTextField(
                            autovalidateMode: AutovalidateMode.disabled,
                            name: 'comment3',
                            initialValue: '',
                            decoration: InputDecoration(
                              labelText: 'Comment',
                            ),
                            keyboardType: TextInputType.text,
                            textInputAction: TextInputAction.next,
                          ),
                          FormBuilderCheckbox(
                            name: 'hide',
                            initialValue: widget.competitor.getNoShow(),
                            onChanged: _onChanged,
                            title: RichText(
                              text: const TextSpan(
                                children: [
                                  TextSpan(
                                    text: 'Hide name',
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
                                print('TYPE=${_formKey.currentState?.value.runtimeType}');
                                widget.competitor.saveValue(_formKey.currentState?.value);
                                await widget.competitor.save();
                                Navigator.pop(context, '');
                              } else {
                                debugPrint(_formKey.currentState?.value.toString());
                                debugPrint('validation failed');
                              }
                            },
                            child: const Text(
                              'Submit',
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
                              'Reset',
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
                              'Cancel',
                              style: TextStyle(
                                  color: Theme.of(context).colorScheme.secondary),
                            ),
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
