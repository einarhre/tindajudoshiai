const FALSE = false;
const TRUE = true;

const MSG_NEXT_MATCH = 1;
const MSG_RESULT     = 2;
const MSG_ACK        = 3;
const MSG_SET_COMMENT = 4;
const MSG_SET_POINTS = 5;
const MSG_HELLO      = 6;
const MSG_DUMMY      = 7;
const MSG_MATCH_INFO = 8;
const MSG_NAME_INFO  = 9;
const MSG_NAME_REQ   = 10;
const MSG_ALL_REQ    = 11;
const MSG_CANCEL_REST_TIME = 12;
const MSG_UPDATE_LABEL = 13;
const MSG_EDIT_COMPETITOR = 14;
const MSG_SCALE      = 15;
const MSG_11_MATCH_INFO = 16;
const MSG_EVENT      = 17;
const MSG_WEB        = 18;
const MSG_LANG       = 19;
const MSG_LOOKUP_COMP = 20;
const NUM_MESSAGE    = 21;

const MATCH_FLAG_BLUE_DELAYED  = 0x0001;
const MATCH_FLAG_WHITE_DELAYED = 0x0002;
const MATCH_FLAG_REST_TIME     = 0x0004;
const MATCH_FLAG_BLUE_REST     = 0x0008;
const MATCH_FLAG_WHITE_REST    = 0x0010;
const MATCH_FLAG_SEMIFINAL_A   = 0x0020;
const MATCH_FLAG_SEMIFINAL_B   = 0x0040;
const MATCH_FLAG_BRONZE_A      = 0x0080;
const MATCH_FLAG_BRONZE_B      = 0x0100;
const MATCH_FLAG_GOLD          = 0x0200;
const MATCH_FLAG_SILVER        = 0x0400;
const MATCH_FLAG_JUDOGI1_OK    = 0x0800;
const MATCH_FLAG_JUDOGI1_NOK   = 0x1000;
const MATCH_FLAG_JUDOGI2_OK    = 0x2000;
const MATCH_FLAG_JUDOGI2_NOK   = 0x4000;
const MATCH_FLAG_JUDOGI_MASK   = (MATCH_FLAG_JUDOGI1_OK | MATCH_FLAG_JUDOGI1_NOK | MATCH_FLAG_JUDOGI2_OK | MATCH_FLAG_JUDOGI2_NOK);
const MATCH_FLAG_REPECHAGE     = 0x8000;
const MATCH_FLAG_TEAM_EVENT    = 0x10000;

const ROUND_MASK            = 0x00ff;
const ROUND_TYPE_MASK       = 0x0f00;
const ROUND_UP_DOWN_MASK    = 0xf000;
const ROUND_UPPER           = 0x1000;
const ROUND_LOWER           = 0x2000;
const ROUND_ROBIN           = (1<<8);
const ROUND_REPECHAGE       = (2<<8);
const ROUND_REPECHAGE_1     = (ROUND_REPECHAGE | ROUND_UPPER);
const ROUND_REPECHAGE_2     = (ROUND_REPECHAGE | ROUND_LOWER);
const ROUND_SEMIFINAL       = (3<<8);
const ROUND_SEMIFINAL_1     = (ROUND_SEMIFINAL | ROUND_UPPER);
const ROUND_SEMIFINAL_2     = (ROUND_SEMIFINAL | ROUND_LOWER);
const ROUND_BRONZE          = (4<<8);
const ROUND_BRONZE_1        = (ROUND_BRONZE | ROUND_UPPER);
const ROUND_BRONZE_2        = (ROUND_BRONZE | ROUND_LOWER);
const ROUND_SILVER          = (5<<8);
const ROUND_FINAL           = (6<<8);
bool ROUND_IS_FRENCH(_n)   => ((_n & ROUND_TYPE_MASK) == 0 || (_n & ROUND_TYPE_MASK) == ROUND_REPECHAGE);
int ROUND_TYPE(_n)        => (_n & ROUND_TYPE_MASK);
int ROUND_NUMBER(_n)      => (_n & ROUND_MASK);
int ROUND_TYPE_NUMBER(_n) => (ROUND_TYPE(_n) | ROUND_NUMBER(_n));

const ROUND_EXTRA_MATCH     = 0x010000;
const ROUND_GOLDEN_SCORE    = 0x020000;

const COMM_VERSION          = 4;

const DELETED       = 0x01;
const HANSOKUMAKE   = 0x02;
const JUDOGI_OK     = 0x20;
const JUDOGI_NOK    = 0x40;
const GENDER_MALE   = 0x80;
const GENDER_FEMALE = 0x100;
const POOL_TIE3     = 0x200;
const DO_NOT_SHOW   = 0x400;
const DB_SAVED      = 0x4000;
// category flags
const TEAM          = 0x04;
const TEAM_EVENT    = 0x08;

const IS_MALE   = 1;
const IS_FEMALE = 2;


const DEV_TYPE_NORMAL = 0;
const DEV_TYPE_STATHMOS = 1;
const DEV_TYPE_AP1 = 2;
const DEV_TYPE_MYWEIGHT = 3;

const baudrate2int = [1200, 9600, 19200, 38400, 115200];
