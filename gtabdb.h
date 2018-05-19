
struct BLOCK_HEAD {
  int N;
  int extend_link;
};
// nodes

typedef struct {
  int link; // node/extent/text/trash link
  int key;  // only support 32-bit key
  char flag;
} GNODE;

enum {
  GNODE_FLAG_CHILD=1,  // link point to children
  GNODE_FLAG_TEXT=2,
  GNODE_FLAG_TRASH=4,  // key = 0
};

struct Stext {
  int time;
  int usecount; //
  int link;
  char len;     // byte len
  // text follows
};

union DBHEAD {
  struct {
    int flags;
    int trash_link;
    int trash_text_link;
    GNODE top;
    int text_link; // use this to seq search
  } h;
  char dummy[128];
};
