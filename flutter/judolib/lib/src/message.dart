import 'package:judolib/judolib.dart';

int ntoh32(List<dynamic> m, int pos) {
  int a = (m[pos] as int) << 24;
  a += (m[pos+1] as int) << 16;
  a += (m[pos+2] as int) << 8;
  a += m[pos+3] as int;
  return a;
}

class Message {
  List<dynamic> m;
  Message(this.m);

  int get version => m[0];
  int get type => m[1];
  int get sender => m[3];

  List<dynamic> get message => m.sublist(4);
}

class MsgNextMatch {
  List<dynamic> m;
  MsgNextMatch(this.m);

  int get tatami => m[0];
  int get category => m[1];
  int get match => m[2];
  int get minutes => m[3];
  int get match_time => m[4];
  int get gs_time => m[5];
  int get rep_time => m[6];
  int get rest_time => m[7];
  int get pin_time_ippon => m[8];
  int get pin_time_wazaari => m[9];
  int get pin_time_yuko => m[10];
  int get pin_time_koka => m[11];
  String get cat_1 => m[12];
  String get blue_1 => m[13];
  String get white_1 => m[14];
  String get cat_2 => m[15];
  String get blue_2 => m[16];
  String get white_2 => m[17];
  int get flags => m[18];
  int get round => m[19];
  String get layout  => m[20];
}

class MsgResult {
  List<dynamic> m;
  MsgResult(this.m);

  int get tatami => m[0];
  int get category => m[1];
  int get match => m[2];
  int get minutes => m[3];
  int get blue_score => m[4];
  int get white_score => m[5];
  int get blue_vote => m[6];
  int get white_vote => m[7];
  int get blue_hansokumake => m[8];
  int get white_hansokumake => m[9];
  int get legend => m[10];
}

class MsgUpdateLabel {
  List<dynamic> m;
  MsgUpdateLabel(this.m);

  int get label_num => m[0];

  StartCompetitors get start_competitors => StartCompetitors(m.sublist(1));
  SetTimerRunColor get set_timer_run_color => SetTimerRunColor(m.sublist(1));
  SetTimerOsaekomiColor get set_timer_osaekomi_color => SetTimerOsaekomiColor(m.sublist(1));
  SetTimerValue get set_timer_value => SetTimerValue(m.sublist(1));
  SetOsaekomiValue get set_osaekomi_value => SetOsaekomiValue(m.sublist(1));
  SetPoints get set_points => SetPoints(m.sublist(1));
  SetScore get set_score => SetScore(m.sublist(1));
  ShowMsg get show_msg => ShowMsg(m.sublist(1));
  SavedLastNames get saved_last_names => SavedLastNames(m.sublist(1));
  StartBig get start_big => StartBig(m.sublist(1));
  StopCompetitors get stop_competitors => StopCompetitors(m.sublist(1));
  StartWinner get start_winner => StartWinner(m.sublist(1));
}

class StartCompetitors {
  List<dynamic> m;
  StartCompetitors(this.m);

  String get layout => m[0];
  String get blue_1 => m[1];
  String get white_1 => m[2];
  String get cat_1 => m[3];
  int get round => m[4];
}

class SetTimerRunColor {
  List<dynamic> m;
  SetTimerRunColor(this.m);

  bool get running => m[0] != 0;
  bool get resttime => m[1] != 0;
}

class SetTimerOsaekomiColor {
  List<dynamic> m;
  SetTimerOsaekomiColor(this.m);

  int get osaekomi_state => m[0];
  int get pts => m[1];
  bool get orun => m[2] != 0;
}

class SetTimerValue {
  List<dynamic> m;
  SetTimerValue(this.m);

  int get min => m[0];
  int get tsec => m[1];
  int get sec => m[2];
  bool get isrest => m[3] != 0;
  int get flags => m[4];
}

class SetOsaekomiValue {
  List<dynamic> m;
  SetOsaekomiValue(this.m);

  int get tsec => m[0];
  int get sec => m[1];
}

class SetPoints {
  List<dynamic> m;
  SetPoints(this.m);

  List<int> get pts1 => m.sublist(0, 5).cast<int>();
  List<int> get pts2 => m.sublist(5, 10).cast<int>();
}

class SetScore {
  List<dynamic> m;
  SetScore(this.m);

  int get score => m[0];
}

class ShowMsg {
  List<dynamic> m;
  ShowMsg(this.m);

  String get cat_1 => m[0];
  String get blue_1 => m[1];
  String get white_1 => m[2];
  String get cat_2 => m[3];
  String get blue_2 => m[4];
  String get white_2 => m[5];
  int get flags => m[6];
  int get round => m[7];
}

class SavedLastNames {
  List<dynamic> m;
  SavedLastNames(this.m);

  String get last1 => m[0];
  String get last2 => m[1];
  String get cat => m[2];
}

class StartBig {
  List<dynamic> m;
  StartBig(this.m);

  String get big_text => m[0];
}

class StopCompetitors {
  List<dynamic> m;
  StopCompetitors(this.m);

  int get page => m[0];
}

class StartWinner {
  List<dynamic> m;
  List<String> s = ['', ''];

  StartWinner(this.m) {
    s = (m[0] as String).split('\t');
  }

  String get last => s[0];
  String get first => s[1];
  String get name => m[0];
  String get cat => m[1];
  int get winner => m[2];
}

class MsgMatchInfo {
  List<dynamic> m;

  MsgMatchInfo(this.m);

  int get tatami => m[0];
  int get position => m[1];
  int get category => m[2];
  int get number => m[3];
  int get comp1 => m[4];
  int get comp2 => m[5];
  int get flags => m[6];
  int get restTime => m[7];
  int get round => m[8];
}

class MsgMatchInfo11 {
  List<dynamic> m;

  MsgMatchInfo11(this.m);

  MsgMatchInfo getMatchInfo(int n) => MsgMatchInfo(m.sublist(9*n, 9*n + 9));
}

class MsgNameInfo {
  List<dynamic> m;

  MsgNameInfo(this.m);

  int get ix => m[0];
  String get last => m[1];
  String get first => m[2];
  String get club => m[3];
}
