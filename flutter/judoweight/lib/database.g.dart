// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'database.dart';

// **************************************************************************
// MoorGenerator
// **************************************************************************

// ignore_for_file: type=lint
class Judoka extends DataClass implements Insertable<Judoka> {
  final int ix;
  final String last;
  final String first;
  final int birthyear;
  final int belt;
  final String club;
  final String regcat;
  final int weight;
  final String category;
  final String country;
  final String id;
  final int seeding;
  final int clubseeding;
  final int gender;
  final String comment;
  final String coachid;
  final int flags;
  Judoka(
      {required this.ix,
      required this.last,
      required this.first,
      required this.birthyear,
      required this.belt,
      required this.club,
      required this.regcat,
      required this.weight,
      required this.category,
      required this.country,
      required this.id,
      required this.seeding,
      required this.clubseeding,
      required this.gender,
      required this.comment,
      required this.coachid,
      required this.flags});
  factory Judoka.fromData(Map<String, dynamic> data, {String? prefix}) {
    final effectivePrefix = prefix ?? '';
    return Judoka(
      ix: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}ix'])!,
      last: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}last'])!,
      first: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}first'])!,
      birthyear: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}birthyear'])!,
      belt: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}belt'])!,
      club: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}club'])!,
      regcat: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}regcat'])!,
      weight: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}weight'])!,
      category: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}category'])!,
      country: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}country'])!,
      id: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}id'])!,
      seeding: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}seeding'])!,
      clubseeding: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}clubseeding'])!,
      gender: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}gender'])!,
      comment: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}comment'])!,
      coachid: const StringType()
          .mapFromDatabaseResponse(data['${effectivePrefix}coachid'])!,
      flags: const IntType()
          .mapFromDatabaseResponse(data['${effectivePrefix}flags'])!,
    );
  }
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['ix'] = Variable<int>(ix);
    map['last'] = Variable<String>(last);
    map['first'] = Variable<String>(first);
    map['birthyear'] = Variable<int>(birthyear);
    map['belt'] = Variable<int>(belt);
    map['club'] = Variable<String>(club);
    map['regcat'] = Variable<String>(regcat);
    map['weight'] = Variable<int>(weight);
    map['category'] = Variable<String>(category);
    map['country'] = Variable<String>(country);
    map['id'] = Variable<String>(id);
    map['seeding'] = Variable<int>(seeding);
    map['clubseeding'] = Variable<int>(clubseeding);
    map['gender'] = Variable<int>(gender);
    map['comment'] = Variable<String>(comment);
    map['coachid'] = Variable<String>(coachid);
    map['flags'] = Variable<int>(flags);
    return map;
  }

  JudokasCompanion toCompanion(bool nullToAbsent) {
    return JudokasCompanion(
      ix: Value(ix),
      last: Value(last),
      first: Value(first),
      birthyear: Value(birthyear),
      belt: Value(belt),
      club: Value(club),
      regcat: Value(regcat),
      weight: Value(weight),
      category: Value(category),
      country: Value(country),
      id: Value(id),
      seeding: Value(seeding),
      clubseeding: Value(clubseeding),
      gender: Value(gender),
      comment: Value(comment),
      coachid: Value(coachid),
      flags: Value(flags),
    );
  }

  factory Judoka.fromJson(Map<String, dynamic> json,
      {ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return Judoka(
      ix: serializer.fromJson<int>(json['ix']),
      last: serializer.fromJson<String>(json['last']),
      first: serializer.fromJson<String>(json['first']),
      birthyear: serializer.fromJson<int>(json['birthyear']),
      belt: serializer.fromJson<int>(json['belt']),
      club: serializer.fromJson<String>(json['club']),
      regcat: serializer.fromJson<String>(json['regcat']),
      weight: serializer.fromJson<int>(json['weight']),
      category: serializer.fromJson<String>(json['category']),
      country: serializer.fromJson<String>(json['country']),
      id: serializer.fromJson<String>(json['id']),
      seeding: serializer.fromJson<int>(json['seeding']),
      clubseeding: serializer.fromJson<int>(json['clubseeding']),
      gender: serializer.fromJson<int>(json['gender']),
      comment: serializer.fromJson<String>(json['comment']),
      coachid: serializer.fromJson<String>(json['coachid']),
      flags: serializer.fromJson<int>(json['flags']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'ix': serializer.toJson<int>(ix),
      'last': serializer.toJson<String>(last),
      'first': serializer.toJson<String>(first),
      'birthyear': serializer.toJson<int>(birthyear),
      'belt': serializer.toJson<int>(belt),
      'club': serializer.toJson<String>(club),
      'regcat': serializer.toJson<String>(regcat),
      'weight': serializer.toJson<int>(weight),
      'category': serializer.toJson<String>(category),
      'country': serializer.toJson<String>(country),
      'id': serializer.toJson<String>(id),
      'seeding': serializer.toJson<int>(seeding),
      'clubseeding': serializer.toJson<int>(clubseeding),
      'gender': serializer.toJson<int>(gender),
      'comment': serializer.toJson<String>(comment),
      'coachid': serializer.toJson<String>(coachid),
      'flags': serializer.toJson<int>(flags),
    };
  }

  Judoka copyWith(
          {int? ix,
          String? last,
          String? first,
          int? birthyear,
          int? belt,
          String? club,
          String? regcat,
          int? weight,
          String? category,
          String? country,
          String? id,
          int? seeding,
          int? clubseeding,
          int? gender,
          String? comment,
          String? coachid,
          int? flags}) =>
      Judoka(
        ix: ix ?? this.ix,
        last: last ?? this.last,
        first: first ?? this.first,
        birthyear: birthyear ?? this.birthyear,
        belt: belt ?? this.belt,
        club: club ?? this.club,
        regcat: regcat ?? this.regcat,
        weight: weight ?? this.weight,
        category: category ?? this.category,
        country: country ?? this.country,
        id: id ?? this.id,
        seeding: seeding ?? this.seeding,
        clubseeding: clubseeding ?? this.clubseeding,
        gender: gender ?? this.gender,
        comment: comment ?? this.comment,
        coachid: coachid ?? this.coachid,
        flags: flags ?? this.flags,
      );
  @override
  String toString() {
    return (StringBuffer('Judoka(')
          ..write('ix: $ix, ')
          ..write('last: $last, ')
          ..write('first: $first, ')
          ..write('birthyear: $birthyear, ')
          ..write('belt: $belt, ')
          ..write('club: $club, ')
          ..write('regcat: $regcat, ')
          ..write('weight: $weight, ')
          ..write('category: $category, ')
          ..write('country: $country, ')
          ..write('id: $id, ')
          ..write('seeding: $seeding, ')
          ..write('clubseeding: $clubseeding, ')
          ..write('gender: $gender, ')
          ..write('comment: $comment, ')
          ..write('coachid: $coachid, ')
          ..write('flags: $flags')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(
      ix,
      last,
      first,
      birthyear,
      belt,
      club,
      regcat,
      weight,
      category,
      country,
      id,
      seeding,
      clubseeding,
      gender,
      comment,
      coachid,
      flags);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is Judoka &&
          other.ix == this.ix &&
          other.last == this.last &&
          other.first == this.first &&
          other.birthyear == this.birthyear &&
          other.belt == this.belt &&
          other.club == this.club &&
          other.regcat == this.regcat &&
          other.weight == this.weight &&
          other.category == this.category &&
          other.country == this.country &&
          other.id == this.id &&
          other.seeding == this.seeding &&
          other.clubseeding == this.clubseeding &&
          other.gender == this.gender &&
          other.comment == this.comment &&
          other.coachid == this.coachid &&
          other.flags == this.flags);
}

class JudokasCompanion extends UpdateCompanion<Judoka> {
  final Value<int> ix;
  final Value<String> last;
  final Value<String> first;
  final Value<int> birthyear;
  final Value<int> belt;
  final Value<String> club;
  final Value<String> regcat;
  final Value<int> weight;
  final Value<String> category;
  final Value<String> country;
  final Value<String> id;
  final Value<int> seeding;
  final Value<int> clubseeding;
  final Value<int> gender;
  final Value<String> comment;
  final Value<String> coachid;
  final Value<int> flags;
  const JudokasCompanion({
    this.ix = const Value.absent(),
    this.last = const Value.absent(),
    this.first = const Value.absent(),
    this.birthyear = const Value.absent(),
    this.belt = const Value.absent(),
    this.club = const Value.absent(),
    this.regcat = const Value.absent(),
    this.weight = const Value.absent(),
    this.category = const Value.absent(),
    this.country = const Value.absent(),
    this.id = const Value.absent(),
    this.seeding = const Value.absent(),
    this.clubseeding = const Value.absent(),
    this.gender = const Value.absent(),
    this.comment = const Value.absent(),
    this.coachid = const Value.absent(),
    this.flags = const Value.absent(),
  });
  JudokasCompanion.insert({
    this.ix = const Value.absent(),
    required String last,
    required String first,
    required int birthyear,
    required int belt,
    required String club,
    required String regcat,
    required int weight,
    required String category,
    required String country,
    required String id,
    required int seeding,
    required int clubseeding,
    required int gender,
    required String comment,
    required String coachid,
    required int flags,
  })  : last = Value(last),
        first = Value(first),
        birthyear = Value(birthyear),
        belt = Value(belt),
        club = Value(club),
        regcat = Value(regcat),
        weight = Value(weight),
        category = Value(category),
        country = Value(country),
        id = Value(id),
        seeding = Value(seeding),
        clubseeding = Value(clubseeding),
        gender = Value(gender),
        comment = Value(comment),
        coachid = Value(coachid),
        flags = Value(flags);
  static Insertable<Judoka> custom({
    Expression<int>? ix,
    Expression<String>? last,
    Expression<String>? first,
    Expression<int>? birthyear,
    Expression<int>? belt,
    Expression<String>? club,
    Expression<String>? regcat,
    Expression<int>? weight,
    Expression<String>? category,
    Expression<String>? country,
    Expression<String>? id,
    Expression<int>? seeding,
    Expression<int>? clubseeding,
    Expression<int>? gender,
    Expression<String>? comment,
    Expression<String>? coachid,
    Expression<int>? flags,
  }) {
    return RawValuesInsertable({
      if (ix != null) 'ix': ix,
      if (last != null) 'last': last,
      if (first != null) 'first': first,
      if (birthyear != null) 'birthyear': birthyear,
      if (belt != null) 'belt': belt,
      if (club != null) 'club': club,
      if (regcat != null) 'regcat': regcat,
      if (weight != null) 'weight': weight,
      if (category != null) 'category': category,
      if (country != null) 'country': country,
      if (id != null) 'id': id,
      if (seeding != null) 'seeding': seeding,
      if (clubseeding != null) 'clubseeding': clubseeding,
      if (gender != null) 'gender': gender,
      if (comment != null) 'comment': comment,
      if (coachid != null) 'coachid': coachid,
      if (flags != null) 'flags': flags,
    });
  }

  JudokasCompanion copyWith(
      {Value<int>? ix,
      Value<String>? last,
      Value<String>? first,
      Value<int>? birthyear,
      Value<int>? belt,
      Value<String>? club,
      Value<String>? regcat,
      Value<int>? weight,
      Value<String>? category,
      Value<String>? country,
      Value<String>? id,
      Value<int>? seeding,
      Value<int>? clubseeding,
      Value<int>? gender,
      Value<String>? comment,
      Value<String>? coachid,
      Value<int>? flags}) {
    return JudokasCompanion(
      ix: ix ?? this.ix,
      last: last ?? this.last,
      first: first ?? this.first,
      birthyear: birthyear ?? this.birthyear,
      belt: belt ?? this.belt,
      club: club ?? this.club,
      regcat: regcat ?? this.regcat,
      weight: weight ?? this.weight,
      category: category ?? this.category,
      country: country ?? this.country,
      id: id ?? this.id,
      seeding: seeding ?? this.seeding,
      clubseeding: clubseeding ?? this.clubseeding,
      gender: gender ?? this.gender,
      comment: comment ?? this.comment,
      coachid: coachid ?? this.coachid,
      flags: flags ?? this.flags,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (ix.present) {
      map['ix'] = Variable<int>(ix.value);
    }
    if (last.present) {
      map['last'] = Variable<String>(last.value);
    }
    if (first.present) {
      map['first'] = Variable<String>(first.value);
    }
    if (birthyear.present) {
      map['birthyear'] = Variable<int>(birthyear.value);
    }
    if (belt.present) {
      map['belt'] = Variable<int>(belt.value);
    }
    if (club.present) {
      map['club'] = Variable<String>(club.value);
    }
    if (regcat.present) {
      map['regcat'] = Variable<String>(regcat.value);
    }
    if (weight.present) {
      map['weight'] = Variable<int>(weight.value);
    }
    if (category.present) {
      map['category'] = Variable<String>(category.value);
    }
    if (country.present) {
      map['country'] = Variable<String>(country.value);
    }
    if (id.present) {
      map['id'] = Variable<String>(id.value);
    }
    if (seeding.present) {
      map['seeding'] = Variable<int>(seeding.value);
    }
    if (clubseeding.present) {
      map['clubseeding'] = Variable<int>(clubseeding.value);
    }
    if (gender.present) {
      map['gender'] = Variable<int>(gender.value);
    }
    if (comment.present) {
      map['comment'] = Variable<String>(comment.value);
    }
    if (coachid.present) {
      map['coachid'] = Variable<String>(coachid.value);
    }
    if (flags.present) {
      map['flags'] = Variable<int>(flags.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('JudokasCompanion(')
          ..write('ix: $ix, ')
          ..write('last: $last, ')
          ..write('first: $first, ')
          ..write('birthyear: $birthyear, ')
          ..write('belt: $belt, ')
          ..write('club: $club, ')
          ..write('regcat: $regcat, ')
          ..write('weight: $weight, ')
          ..write('category: $category, ')
          ..write('country: $country, ')
          ..write('id: $id, ')
          ..write('seeding: $seeding, ')
          ..write('clubseeding: $clubseeding, ')
          ..write('gender: $gender, ')
          ..write('comment: $comment, ')
          ..write('coachid: $coachid, ')
          ..write('flags: $flags')
          ..write(')'))
        .toString();
  }
}

class $JudokasTable extends Judokas with TableInfo<$JudokasTable, Judoka> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $JudokasTable(this.attachedDatabase, [this._alias]);
  final VerificationMeta _ixMeta = const VerificationMeta('ix');
  @override
  late final GeneratedColumn<int?> ix = GeneratedColumn<int?>(
      'ix', aliasedName, false,
      type: const IntType(), requiredDuringInsert: false);
  final VerificationMeta _lastMeta = const VerificationMeta('last');
  @override
  late final GeneratedColumn<String?> last = GeneratedColumn<String?>(
      'last', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _firstMeta = const VerificationMeta('first');
  @override
  late final GeneratedColumn<String?> first = GeneratedColumn<String?>(
      'first', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _birthyearMeta = const VerificationMeta('birthyear');
  @override
  late final GeneratedColumn<int?> birthyear = GeneratedColumn<int?>(
      'birthyear', aliasedName, false,
      type: const IntType(), requiredDuringInsert: true);
  final VerificationMeta _beltMeta = const VerificationMeta('belt');
  @override
  late final GeneratedColumn<int?> belt = GeneratedColumn<int?>(
      'belt', aliasedName, false,
      type: const IntType(), requiredDuringInsert: true);
  final VerificationMeta _clubMeta = const VerificationMeta('club');
  @override
  late final GeneratedColumn<String?> club = GeneratedColumn<String?>(
      'club', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _regcatMeta = const VerificationMeta('regcat');
  @override
  late final GeneratedColumn<String?> regcat = GeneratedColumn<String?>(
      'regcat', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _weightMeta = const VerificationMeta('weight');
  @override
  late final GeneratedColumn<int?> weight = GeneratedColumn<int?>(
      'weight', aliasedName, false,
      type: const IntType(), requiredDuringInsert: true);
  final VerificationMeta _categoryMeta = const VerificationMeta('category');
  @override
  late final GeneratedColumn<String?> category = GeneratedColumn<String?>(
      'category', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _countryMeta = const VerificationMeta('country');
  @override
  late final GeneratedColumn<String?> country = GeneratedColumn<String?>(
      'country', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _idMeta = const VerificationMeta('id');
  @override
  late final GeneratedColumn<String?> id = GeneratedColumn<String?>(
      'id', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _seedingMeta = const VerificationMeta('seeding');
  @override
  late final GeneratedColumn<int?> seeding = GeneratedColumn<int?>(
      'seeding', aliasedName, false,
      type: const IntType(), requiredDuringInsert: true);
  final VerificationMeta _clubseedingMeta =
      const VerificationMeta('clubseeding');
  @override
  late final GeneratedColumn<int?> clubseeding = GeneratedColumn<int?>(
      'clubseeding', aliasedName, false,
      type: const IntType(), requiredDuringInsert: true);
  final VerificationMeta _genderMeta = const VerificationMeta('gender');
  @override
  late final GeneratedColumn<int?> gender = GeneratedColumn<int?>(
      'gender', aliasedName, false,
      type: const IntType(), requiredDuringInsert: true);
  final VerificationMeta _commentMeta = const VerificationMeta('comment');
  @override
  late final GeneratedColumn<String?> comment = GeneratedColumn<String?>(
      'comment', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _coachidMeta = const VerificationMeta('coachid');
  @override
  late final GeneratedColumn<String?> coachid = GeneratedColumn<String?>(
      'coachid', aliasedName, false,
      type: const StringType(), requiredDuringInsert: true);
  final VerificationMeta _flagsMeta = const VerificationMeta('flags');
  @override
  late final GeneratedColumn<int?> flags = GeneratedColumn<int?>(
      'flags', aliasedName, false,
      type: const IntType(), requiredDuringInsert: true);
  @override
  List<GeneratedColumn> get $columns => [
        ix,
        last,
        first,
        birthyear,
        belt,
        club,
        regcat,
        weight,
        category,
        country,
        id,
        seeding,
        clubseeding,
        gender,
        comment,
        coachid,
        flags
      ];
  @override
  String get aliasedName => _alias ?? 'judokas';
  @override
  String get actualTableName => 'judokas';
  @override
  VerificationContext validateIntegrity(Insertable<Judoka> instance,
      {bool isInserting = false}) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('ix')) {
      context.handle(_ixMeta, ix.isAcceptableOrUnknown(data['ix']!, _ixMeta));
    }
    if (data.containsKey('last')) {
      context.handle(
          _lastMeta, last.isAcceptableOrUnknown(data['last']!, _lastMeta));
    } else if (isInserting) {
      context.missing(_lastMeta);
    }
    if (data.containsKey('first')) {
      context.handle(
          _firstMeta, first.isAcceptableOrUnknown(data['first']!, _firstMeta));
    } else if (isInserting) {
      context.missing(_firstMeta);
    }
    if (data.containsKey('birthyear')) {
      context.handle(_birthyearMeta,
          birthyear.isAcceptableOrUnknown(data['birthyear']!, _birthyearMeta));
    } else if (isInserting) {
      context.missing(_birthyearMeta);
    }
    if (data.containsKey('belt')) {
      context.handle(
          _beltMeta, belt.isAcceptableOrUnknown(data['belt']!, _beltMeta));
    } else if (isInserting) {
      context.missing(_beltMeta);
    }
    if (data.containsKey('club')) {
      context.handle(
          _clubMeta, club.isAcceptableOrUnknown(data['club']!, _clubMeta));
    } else if (isInserting) {
      context.missing(_clubMeta);
    }
    if (data.containsKey('regcat')) {
      context.handle(_regcatMeta,
          regcat.isAcceptableOrUnknown(data['regcat']!, _regcatMeta));
    } else if (isInserting) {
      context.missing(_regcatMeta);
    }
    if (data.containsKey('weight')) {
      context.handle(_weightMeta,
          weight.isAcceptableOrUnknown(data['weight']!, _weightMeta));
    } else if (isInserting) {
      context.missing(_weightMeta);
    }
    if (data.containsKey('category')) {
      context.handle(_categoryMeta,
          category.isAcceptableOrUnknown(data['category']!, _categoryMeta));
    } else if (isInserting) {
      context.missing(_categoryMeta);
    }
    if (data.containsKey('country')) {
      context.handle(_countryMeta,
          country.isAcceptableOrUnknown(data['country']!, _countryMeta));
    } else if (isInserting) {
      context.missing(_countryMeta);
    }
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    } else if (isInserting) {
      context.missing(_idMeta);
    }
    if (data.containsKey('seeding')) {
      context.handle(_seedingMeta,
          seeding.isAcceptableOrUnknown(data['seeding']!, _seedingMeta));
    } else if (isInserting) {
      context.missing(_seedingMeta);
    }
    if (data.containsKey('clubseeding')) {
      context.handle(
          _clubseedingMeta,
          clubseeding.isAcceptableOrUnknown(
              data['clubseeding']!, _clubseedingMeta));
    } else if (isInserting) {
      context.missing(_clubseedingMeta);
    }
    if (data.containsKey('gender')) {
      context.handle(_genderMeta,
          gender.isAcceptableOrUnknown(data['gender']!, _genderMeta));
    } else if (isInserting) {
      context.missing(_genderMeta);
    }
    if (data.containsKey('comment')) {
      context.handle(_commentMeta,
          comment.isAcceptableOrUnknown(data['comment']!, _commentMeta));
    } else if (isInserting) {
      context.missing(_commentMeta);
    }
    if (data.containsKey('coachid')) {
      context.handle(_coachidMeta,
          coachid.isAcceptableOrUnknown(data['coachid']!, _coachidMeta));
    } else if (isInserting) {
      context.missing(_coachidMeta);
    }
    if (data.containsKey('flags')) {
      context.handle(
          _flagsMeta, flags.isAcceptableOrUnknown(data['flags']!, _flagsMeta));
    } else if (isInserting) {
      context.missing(_flagsMeta);
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {ix};
  @override
  Judoka map(Map<String, dynamic> data, {String? tablePrefix}) {
    return Judoka.fromData(data,
        prefix: tablePrefix != null ? '$tablePrefix.' : null);
  }

  @override
  $JudokasTable createAlias(String alias) {
    return $JudokasTable(attachedDatabase, alias);
  }
}

abstract class _$JudoDatabase extends GeneratedDatabase {
  _$JudoDatabase(QueryExecutor e) : super(SqlTypeSystem.defaultInstance, e);
  late final $JudokasTable judokas = $JudokasTable(this);
  Future<int> setFlagDbSaved() {
    return customUpdate(
      'UPDATE judokas SET flags=flags|16384;',
      variables: [],
      updates: {judokas},
      updateKind: UpdateKind.update,
    );
  }

  Selectable<int?> getMaxIx() {
    return customSelect('SELECT MAX(ix) FROM judokas;',
        variables: [],
        readsFrom: {
          judokas,
        }).map((QueryRow row) => row.read<int?>('MAX(ix)'));
  }

  @override
  Iterable<TableInfo> get allTables => allSchemaEntities.whereType<TableInfo>();
  @override
  List<DatabaseSchemaEntity> get allSchemaEntities => [judokas];
}
