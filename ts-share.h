#define TS_SHARE_SERVER "hyperrate.com"
#define TS_SHARE_SERVER_PORT 443


enum {
  REQ_CONTRIBUTE=1,
  REQ_DOWNLOAD=2,
  REQ_CONTRIBUTE2=3,
  REQ_DOWNLOAD2=4,
};

typedef struct {
  int cmd;
  int flag;
} REQ_HEAD;

enum {
  REQ_HEAD_FLAG_HAS_FORMAT=1,
};

typedef struct {
  int flag;
  int key_sz;
  char os_str[32];
} REQ_FORMAT;

typedef int ts_time_t;

typedef struct {
  char tag[32];
} REQ_CONTRIBUTE_S;
// char len  // len<=0 end mark
// phokey_t[len]
// char str_len
// char[str_len]


typedef struct {
  int res;
} REQ_CONTRIBUTE_REPLY_S;

typedef struct {
  char tag[32];
  ts_time_t last_dl_time;
} REQ_DOWNLOAD_S;

typedef struct {
  ts_time_t this_dl_time;
} REQ_DOWNLOAD_REPLY_S;


