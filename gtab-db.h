/*
dbhead
stext block
gnode/stext blocks
*/

typedef struct {
  int N;
  int extend_link; // The soring order is only valid within the block. If extend_link > 0, binary search one by one
  // notes follows
} BLOCK_HEAD;


// nodes

/*
  ac texta
  acb textb
  acb textc -> dup gnode
  
  a
  aab
  aac
  b
  bbc
  
*/


typedef struct {
  int link; // text/chidren link
  int key;  // key==0->text link
} GNODE;

typedef struct {
  int time;
  usecount_t usecount;
  short len;     // byte len
  char flags;
  // text follows
} Stext;

enum {
 STEXT_FLAG_TRASH=1,
};

typedef union {
  struct {
    int flags;
    // int text_start_ofs;  sizeof(DBHEAD)
    int gnode_root_ofs;
  } h;
  char dummy[128];
} DBHEAD;
