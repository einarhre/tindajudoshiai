import 'package:flutter_gen/gen_l10n/app_localizations.dart';

import 'const.dart';


const List<String> genderOptions = ['?', 'Male', 'Female'];
const List<String> gradeOptions = ['?', '6.kyu', '5.kyu', '4.kyu', '3.kyu',
  '2.kyu', '1.kyu', '1.dan', '2.dan', '3.dan', '4.dan',
  '5.dan', '6.dan', '7.dan', '8.dan', '9.dan'];
const List<String> seedingOptions = ['No seeding', '1', '2', '3', '4', '5', '6', '7', '8'];
const List<String> controlOptions = ['?', 'OK', 'NOK'];

List<String> getGenderOptions() {
  return genderOptions;
}

String genderVal2Str(int val) {
  return genderOptions[val];
}

int genderStr2Val(String? str) {
  if (str != null)
    return genderOptions.indexOf(str);
  return 0;
}

List<String> getGradeOptions() {
  return gradeOptions;
}

String gradeVal2Str(int val) {
  return gradeOptions[val];
}

int gradeStr2Val(String? str) {
  if (str != null)
    return gradeOptions.indexOf(str);
  return 0;
}

List<String> getSeedingOptions() {
  return seedingOptions;
}

String seedingVal2Str(int val) {
  return seedingOptions[val];
}

int seedingStr2Val(String? str) {
  if (str != null)
    return seedingOptions.indexOf(str);
  return 0;
}

List<String> getControlOptions() {
  return controlOptions;
}

String controlVal2Str(int val) {
  return controlOptions[val];
}

int controlStr2Val(String? str) {
  if (str != null)
    return controlOptions.indexOf(str);
  return 0;
}


String getGenderStr(int flags) {
  if ((flags & 0x80) != 0) return 'Male';
  if ((flags & 0x100) != 0) return 'Female';
  return '?';
}

int setGenderStr(int flags, String g) {
  flags = flags & ~0x180;
  if (g == 'Male') flags |= 0x80;
  else if (g == 'Female') flags |= 0x100;
  return flags;
}

String getControlStr(int flags) {
  if ((flags & 0x20) != 0) return 'OK';
  if ((flags & 0x40) != 0) return 'NOK';
  return '?';
}

int setControlStr(int flags, String g) {
  flags = flags & ~0x60;
  if (g == 'OK') flags |= 0x20;
  else if (g == 'NOK') flags |= 0x40;
  return flags;
}

bool getHansokumake(int flags) {
  return (flags & 2) != 0;
}

int setHansokumake(int flags, bool a) {
  flags = flags & ~2;
  if (a) flags |= 2;
  return flags;
}

bool getNoShow(int flags) {
  return (flags & 0x400) != 0;
}

int setNoShow(int flags, bool a) {
  flags = flags & ~0x400;
  if (a) flags |= 0x400;
  return flags;
}

String weightVal2Str(int weight) {
  return (weight/1000).toString();
}

int weightStr2Val(String? w) {
  if (w != null) {
    var kg = double.parse(w);
    return (kg * 1000).toInt();
  }
  return 0;
}

String round2Str(AppLocalizations? t, int round) {
  if (round == 0)
    return "";

  switch (round & ROUND_TYPE_MASK) {
    case 0:
      return '${t?.roundb7f4 ?? ''} ${round & ROUND_MASK}';
    case ROUND_ROBIN:
      return t?.roundRobinaac1 ?? '';
    case ROUND_REPECHAGE:
      return t?.repechageb9e4 ?? '';
    case ROUND_SEMIFINAL:
      if ((round & ROUND_UP_DOWN_MASK) == ROUND_UPPER)
        return t?.semifinalA6f6d ?? '';
      else if ((round & ROUND_UP_DOWN_MASK) == ROUND_LOWER)
        return t?.semifinalBb972 ?? '';
      else
        return t?.semifinaldf72 ?? '';
    case ROUND_BRONZE:
      if ((round & ROUND_UP_DOWN_MASK) == ROUND_UPPER)
        return t?.bronzeMatchA87cd ?? '';
      else if ((round & ROUND_UP_DOWN_MASK) == ROUND_LOWER)
        return t?.bronzeMatchB966f ?? '';
      else
        return t?.bronzea0df ?? '';
    case ROUND_SILVER:
      return t?.silverMedalMatch57ec ?? '';
    case ROUND_FINAL:
      return t?.finalbeae ?? '';
  }
  return "";
}

