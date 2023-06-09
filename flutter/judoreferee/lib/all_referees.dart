import 'package:editable/editable.dart';
import 'package:flutter/material.dart';
import 'package:judoreferee/global.dart';

class EditableListTile extends StatefulWidget {
  final Referee model;
  final Function(Referee listModel) onChanged;
  EditableListTile({Key? key, required this.model, required this.onChanged})
      : assert(model != null),
        super(key: key);

  @override
  _EditableListTileState createState() => _EditableListTileState();
}

class _EditableListTileState extends State<EditableListTile> {
  late Referee model;
  bool _isEditingMode = false;

  TextEditingController? _nameEditingController = null,
      _clubEditingController = null,
      _countryEditingController = null;

  @override
  void initState() {
    super.initState();
    this.model = widget.model;
    this._isEditingMode = false;
  }

  @override
  Widget build(BuildContext context) {

    return Card(
        elevation: 2,
        margin: EdgeInsets.all(4),
        color: model.selected ? Colors.yellow : Colors.white,
        child: ListTile(
          title: Row(
            children: [
              Expanded(child: nameWidget),
              Expanded(child: clubWidget),
              Expanded(child: countryWidget),
              /*Expanded(
                  child:*/ Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Container(
                          color: Colors.amber,
                          width: 150,
                height: 40,
                child: CheckboxListTile(
                  title: const Text('Referee'),
                    value: model.refereeOk,
                    onChanged: (bool? value) {
                      setState(() {
                        model.setRefereeOk(!model.refereeOk);
                        if (widget.onChanged != null) {
                          widget.onChanged(this.model);
                        }
                      });
                    })),
                    Container(
                        color: Colors.amber,
                      width: 150,
                        height: 40,
                    child: CheckboxListTile(
                      title: Text('Judge'),
                        value: model.judgeOk,
                        onChanged: (bool? value) {
                          setState(() {
                            model.setJudgeOk(!model.judgeOk);
                            if (widget.onChanged != null) {
                              widget.onChanged(this.model);
                            }
                          });
                        })),
              ]),
              //Expanded(child: Text(model.tatami.toString())),
            ],
          ),
          subtitle: tatamiWidget,
          selected: false,
          leading: IconButton(
            icon: model.active
                ? Icon(Icons.check_box)
                : Icon(Icons.not_interested),
            onPressed: () {
              setState(() {
                model.active = !model.active;
                if (widget.onChanged != null) {
                  widget.onChanged(this.model);
                }
              });
            },
          ),
          trailing: tralingButton,
          onTap: () {
            setState(() {
              model.selected = !model.selected;
            });
          },
        ));
  }

  Widget get nameWidget {
    if (_isEditingMode) {
      _nameEditingController = TextEditingController(text: model.name);
      return TextField(
        controller: _nameEditingController,
      );
    } else
      return Text(model.name);
  }

  Widget get clubWidget {
    if (_isEditingMode) {
      _clubEditingController = TextEditingController(text: model.club);
      return TextField(
        controller: _clubEditingController,
      );
    } else
      return Text(model.club);
  }

  Widget get countryWidget {
    if (_isEditingMode) {
      _countryEditingController = TextEditingController(text: model.country);
      return TextField(
        controller: _countryEditingController,
      );
    } else
      return Text(model.country);
  }

  Widget get tatamiWidget {
    return Text(model.tatami > 0 ? 'Tatami ${model.tatami}' : 'Tatami ?');
  }

  Widget get tralingButton {
    if (_isEditingMode) {
      return IconButton(
        icon: Icon(Icons.check),
        onPressed: saveChange,
      );
    } else
      return IconButton(
        icon: Icon(Icons.edit),
        onPressed: _toggleMode,
      );
  }

  void _toggleMode() {
    setState(() {
      _isEditingMode = !_isEditingMode;
    });
  }

  void saveChange() {
    this.model.name = _nameEditingController?.text ?? '';
    this.model.club = _clubEditingController?.text ?? '';
    this.model.country = _countryEditingController?.text ?? '';
    _toggleMode();
    if (widget.onChanged != null) {
      widget.onChanged(this.model);
    }
  }
}
