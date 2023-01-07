import 'dart:convert';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_gen/gen_l10n/app_localizations.dart';
import 'package:form_builder_validators/localization/l10n.dart';
import 'package:judolib/judolib.dart';
import 'package:judoweight/bloc.dart';
import 'package:judoweight/competitor_edit.dart';
import 'package:judoweight/weight_edit.dart';
import 'package:judoweight/weightform.dart';
import 'package:provider/provider.dart';
import 'package:shared_preferences/shared_preferences.dart';

//import 'lang.dart';
import 'database.dart';
import 'judoka_card.dart';
import 'main.dart';
import 'menus.dart';
import 'settings.dart';

class JudokaList extends StatefulWidget {
  const JudokaList({Key? key}) : super(key: key);

  // This widget is the home page of your application. It is stateful, meaning
  // that it has a State object (defined below) that contains fields that affect
  // how it looks.

  // This class is the configuration for the state. It holds the values (in this
  // case the title) provided by the parent (in this case the App widget) and
  // used by the build method of the State. Fields in a Widget subclass are
  // always marked "final".

  final String title = 'JudoWeight';

  @override
  State<JudokaList> createState() => JudokaListState();
}

class JudokaListState extends State<JudokaList> {
  late double width, height;
  late FocusNode ixFocusNode;

  @override
  void initState() {
    super.initState();
    ixFocusNode = FocusNode();
  }

  @override
  void dispose() {
    // Clean up the focus node when the Form is disposed.
    ixFocusNode.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    /***
    width = MediaQuery.of(context).size.width;
    height = MediaQuery.of(context).size.height;
    // Height (without SafeArea)
    var padding = MediaQuery.of(context).viewPadding;
    double height1 = height - padding.top - padding.bottom;
    // Height (without status bar)
    double height2 = height - padding.top;
    // Height (without status and toolbar)
    double height3 = height - padding.top - kToolbarHeight;
        ***/
    var t = AppLocalizations.of(context);
    final fieldText = TextEditingController();

    return Column(
        children: [
          Row(children: [
            Text(t?.iDe0f0 ?? ''),
            Expanded(
                child: TextField(
              autofocus: true,
              focusNode: ixFocusNode,
              controller: fieldText,
              onSubmitted: (val) async {
                int? ix = int.tryParse(val);
                if (ix != null) {
                  Judoka? c =
                      await Provider.of<JudokaListModel>(context, listen: false)
                          .getJudoka(ix);
                  final result = await Navigator.push(
                    context,
                    MaterialPageRoute(
                        builder: (context) => CompetitorEdit(c, null)),
                  );
                  setState(() {
                    fieldText.clear();
                    ixFocusNode.requestFocus();
                  });
                }
              },
            )),
          ]),
          Consumer<JudokaListModel>(builder: (context, judokas, child) {
            return StreamBuilder<List<Judoka>>(
              stream: judokas.homeScreenEntries,
              builder: (context, snapshot) {
                //print(snapshot);

                if (!snapshot.hasData) {
                  return const Align(
                    alignment: Alignment.center,
                    child: CircularProgressIndicator(),
                  );
                }

                final judokas = snapshot.data!;
                print('BUILD ${judokas.length} judokas');

                return Flexible(
                    child: ListView.builder(
                  //scrollDirection: Axis.vertical,
                  //shrinkWrap: true,
                  itemCount: judokas.length,
                  itemBuilder: (context, index) {
                    return JudokaCard(judokas[index]);
                  },
                ));
              },
            );
          }),
        ],
      );
  }
}
