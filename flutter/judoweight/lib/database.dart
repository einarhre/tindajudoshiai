// RUN: flutter pub run build_runner build

import 'dart:convert';
import 'dart:io';

import 'package:drift/drift.dart';
import 'package:judolib/judolib.dart';
import 'package:judoweight/comm.dart';
import 'connection.dart' as impl;
import 'svg.dart';
import 'package:dart_now_time_filename/dart_now_time_filename.dart';
part 'database.g.dart';

class Judokas extends Table {
  IntColumn get ix => integer()();

  TextColumn get last => text()();

  TextColumn get first => text()();

  IntColumn get birthyear => integer()();

  IntColumn get belt => integer()();

  TextColumn get club => text()();

  TextColumn get regcat => text()();

  IntColumn get weight => integer()();

  TextColumn get category => text()();

  TextColumn get country => text()();

  TextColumn get id => text()();

  IntColumn get seeding => integer()();

  IntColumn get clubseeding => integer()();

  IntColumn get gender => integer()();

  TextColumn get comment => text()();

  TextColumn get coachid => text()();

  IntColumn get flags => integer()();

  @override
  Set<Column> get primaryKey => {ix};
}


/***
LazyDatabase _openConnection() {
  // the LazyDatabase util lets us find the right location for the file async.
  return LazyDatabase(() async {
    // put the database file, called db.sqlite here, into the documents folder
    // for your app.
    final dbFolder = await getApplicationDocumentsDirectory();
    final file = File(p.join(dbFolder.path, 'db.sqlite'));
    print('DATABASE FILE: $file');
    return NativeDatabase(file);
  });
}
    ***/

@DriftDatabase(
    tables: [Judokas],
    queries: {'setFlagDbSaved': 'UPDATE judokas SET flags=flags|${DB_SAVED};',
    'getMaxIx': 'SELECT MAX(ix) FROM judokas;'})
class JudoDatabase extends _$JudoDatabase {
  //JudoDatabase() : super(_openConnection());
  JudoDatabase(): super(impl.openConnection());

  Future<List<Judoka>> get allJudokaEntries => select(judokas).get();

  Stream<List<Judoka>> watchEntriesInJudoka(Judoka c) {
    return (select(judokas)..where((t) => t.ix.equals(c.ix))).watch();
  }

  Stream<List<Judoka>> watchJudokaEntries() {
    //return (select(judokas)..where((t) => t.flags.equals(0))).watch();
    return (select(judokas)
          ..orderBy([
            (t) => OrderingTerm(expression: t.weight.isBiggerThanValue(0)),
            (t) => OrderingTerm(expression: t.last),
            (t) => OrderingTerm(expression: t.first)
          ]))
        .watch();
  }

  Future<Judoka?> getJudoka(int ix) {
    return (select(judokas)..where((t) => t.ix.equals(ix))).getSingleOrNull();
  }

  Future<int> addJudoka(JudokasCompanion entry) {
    //print('INSERT ${entry.ix} ${entry.last}');
    return into(judokas).insert(entry);
  }

  Future<int> createOrUpdateJudoka(JudokasCompanion entry) async {
    return into(judokas).insertOnConflictUpdate(entry);
  }

  Future<int> deleteJudoka(JudokasCompanion entry) {
    print('DELETE ${entry.ix} ${entry.last}');
    return delete(judokas).delete(entry);
  }

  Future<bool> updateJudoka(JudokasCompanion entry) {
    print('UPDATE ${entry.ix} ${entry.last}');
    return update(judokas).replace(entry);
  }

  Future<void> deleteEverything() {
    return transaction(() async {
      for (final table in allTables) {
        await delete(table).go();
      }
    });
  }

  Future<void> saveValue(Judoka c, Map<String, dynamic>? a) async {
    int flags = 0;
    flags = setGenderStr(flags, a?['gender'] ?? '?');
    flags = setControlStr(flags, a?['control'] ?? '?');
    flags = setHansokumake(flags, a?['hansokumake'] ?? false);
    flags = setNoShow(flags, a?['hide'] ?? false);
    int nextIx = c.ix;

    if (c.ix == 0) {
      List<int?> biggiest = await getMaxIx().get();
      nextIx = (biggiest[0] ?? 9000) + 1;
      if (nextIx < 9000) nextIx = 9000;
    }

    Judoka j = Judoka(
      ix: nextIx,
      last: a?['last'] ?? '',
      first: a?['first'] ?? '',
      club: a?['club'] ?? '',
      country: a?['country'] ?? '',
      regcat: a?['regcat'] ?? '',
      category: c.category,
      id: a?['id'] ?? '',
      coachid: a?['coachid'] ?? '',
      flags: flags,
      birthyear: a?['yob'] ?? 0,
      comment:
          '${a?["comment1"] ?? ""}#${a?["comment2"] ?? ""}#${a?["comment3"] ?? ""}',
      belt: gradeStr2Val(a?["grade"]),
      weight: weightStr2Val(a?['weight']),
      clubseeding: 0,
      seeding: 0,
      gender: (flags & GENDER_MALE) != 0
          ? IS_MALE
          : ((flags & GENDER_FEMALE) != 0 ? IS_FEMALE : 0),
    );
    final ret = createOrUpdateJudoka(j.toCompanion(true));
    int i = await sendJudokaData(j);
    if (i >= 10 && (i == c.ix || nextIx >= 9000)) {
      (update(judokas)..where((t) => t.ix.equals(nextIx))
      ).write(JudokasCompanion(
        flags: Value(flags | DB_SAVED),
          ix: Value(i)));
    }
  }

  Future<void> uploadToJudoshiai() async {
    List<Judoka> all = await allJudokaEntries;
    for (var j in all) {
      int ix = j.ix;
      int flags = j.flags;
      int i = await sendJudokaData(j);
      if (i >= 10 && (i == ix || ix >= 9000)) {
        (update(judokas)..where((t) => t.ix.equals(ix))
        ).write(JudokasCompanion(
            flags: Value(flags | DB_SAVED),
            ix: Value(i)));
      }
    }
  }

  Future<Map<String, dynamic>?> saveWeight(int old_ix, Map<String, dynamic>? a) async {
    print('SAVE WEIGHT old_ix=$old_ix a=$a');
    final weight = weightStr2Val(a?['weight']);
    var ix = old_ix;
    final String ixstr = a?['ix'];
    if (ixstr != null && ixstr.length >= 2) ix = int.tryParse(ixstr) ?? old_ix;

    String last = a?["last"] ?? '';
    String first = a?["first"] ?? '';
    String regcat = a?["regcat"] ?? '';
    int flags = 0;
    String club = '', country = '';

    print('WEIGHT EDIT ix=$ix weight=$weight');

    Judoka? j = await getJudoka(ix);

    if (j != null) {
      print('JUDOKA $ix FOUND');
      (update(judokas)
        ..where((t) => t.ix.equals(ix))
      ).write(JudokasCompanion(
          weight: Value(weight),
          flags: Value(j.flags & ~DB_SAVED)));

      flags = j.flags;
      if (last.length == 0) last = j.last;
      if (first.length == 0) first = j.first;
      if (regcat.length == 0) regcat = j.regcat;
      club = j.club;
      country = j.country;
    } else {
      Judoka c = getNewJudoka({'ix': ix, 'weight': weight,
      'last': last, 'first': first, 'regcat': regcat});
      createOrUpdateJudoka(c.toCompanion(true));
    }

    var json = await sendWeightData(ix, weight);
    if (json != null) {
      if (json["last"] != null) last = json["last"];
      if (json["first"] != null) first = json["first"];
      if (json["regcat"] != null) regcat = json["regcat"];
      if (json["club"] != null) club = json["club"];
      if (json["country"] != null) country = json["country"];

      (update(judokas)
        ..where((t) => t.ix.equals(ix))
      ).write(JudokasCompanion(
        flags: Value(flags | DB_SAVED),
        last: Value(last),
        first: Value(first),
        regcat: Value(regcat),
      ));

    }

    printSvg(ix, weight, last, first, club, country);

    return json;
  }

  void removeDB() {
    transaction(() async {
      await close();
    });
    impl.deleteDB();
  }

  void saveToFile() async {
    var all = await allJudokaEntries;
    var r = json.encode(all);
    saveTextFile(r, NowFilename.gen(prefix: 'DB-', ext: '.json'));
    setFlagDbSaved();
  }

  @override
  int get schemaVersion => 1;
}

Map<String, dynamic> judoka2json(Judoka c) {
  return {
    'ix': c.ix,
    'last': c.last,
    'first': c.first,
    'club': c.club,
    'country': c.country,
    'regcat': c.regcat,
    'category': c.category,
    'id': c.id,
    'coachid': c.coachid,
    'flags': c.flags,
    'birthyear': c.birthyear,
    'comment': c.comment,
    'belt': c.belt,
    'weight': c.weight,
    'clubseeding': c.clubseeding,
    'seeding': c.seeding,
    'gender': c.gender,
  };
}

Judoka getNewJudoka(Map<String, dynamic>? a) {
  var ix = a?['ix'] ?? -1;
  if (ix == -1) ix = a?['index'] ?? 0;
  return  Judoka(
    ix: ix,
    last: a?['last'] ?? '',
    first: a?['first'] ?? '',
    club: a?['club'] ?? '',
    country: a?['country'] ?? '',
    regcat: a?['regcat'] ?? '',
    category: a?['category'] ?? '',
    id: a?['id'] ?? '',
    coachid: a?['coachid'] ?? '',
    flags: a?['flags'] ?? 0,
    birthyear: a?['yob'] ?? 0,
    comment: a?["comment"] ?? "",
    belt: a?["grade"] ?? 0,
    weight: a?['weight'] ?? 0,
    clubseeding: a?['clubseeding'] ?? 0,
    seeding: a?['seeding'] ?? 0,
    gender: a?['gender'] ?? 0,
  );
}

