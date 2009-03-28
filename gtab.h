#define MAX_CIN_PHR (100*CH_SZ + 1)



typedef enum {
  GTAB_space_auto_first_none=0,   // use the value set by .cin
  GTAB_space_auto_first_any=1,    // boshiamy, dayi
  GTAB_space_auto_first_full=2,   // simplex
  GTAB_space_auto_first_nofull=4,  // windows ar30 cj
  GTAB_space_auto_first_dayi=8    // dayi: input:2   select:1
} GTAB_space_pressed_E;

typedef struct {
  u_char key[4];   /* If I use u_long key, the struc size will be 8 */
  u_char ch[CH_SZ];
} ITEM;

typedef struct {
  u_char key[8];   /* If I use u_long key, the struc size will be 8 */
  u_char ch[CH_SZ];
} ITEM64;

typedef struct {
  u_char quick1[46][10][CH_SZ];
  u_char quick2[46][46][10][CH_SZ];
} QUICK_KEYS;


enum {
  FLAG_KEEP_KEY_CASE=1,
  FLAG_GTAB_SYM_KBM=2, // auto close, auto switch to default input method
};

struct TableHead {
  int version;
  u_int flag;
  char cname[32];         /* prompt */
  char selkey[12];        /* select keys */
  GTAB_space_pressed_E space_style;
  int KeyS;               /* number of keys needed */
  int MaxPress;           /* Max len of keystroke  ar30:4  changjei:5 */
  int M_DUP_SEL;          /* how many keys used to select */
  int DefC;               /* Defined characters */
  QUICK_KEYS qkeys;

  union {
    struct {
      char endkey[99];
      char keybits;
    };

    char dummy[128];  // for future use
  };
};


#define KeyBits (cur_inmd->keybits)
#define MAX_GTAB_KEYS (1<<KeyBits)

#define MAX_GTAB_NUM_KEY (16)
#define MAX_SELKEY 16

#define MAX_TAB_KEY_NUM (32/KeyBits)
#define MAX_TAB_KEY_NUM64 (64/KeyBits)
#define MAX_TAB_KEY_NUM64_6 (10)


typedef u_int gtab_idx1_t;

typedef struct {
  ITEM *tbl;
  ITEM64 *tbl64;
  QUICK_KEYS qkeys;
  int use_quick;
  u_int flag;
#define MAX_CNAME (4*CH_SZ+1)
  char *cname;
  u_char *keycol;
  int KeyS;               /* number of keys needed */
  int MaxPress;           /* Max len of keystrike  ar30:5  changjei:5 */
  int DefChars;           /* defined chars */
  u_char *keyname; // including ?*
  u_char *keyname_lookup; // used by boshiamy only
  gtab_idx1_t *idx1;
  u_char *keymap;
  char selkey[MAX_SELKEY];
  u_char *sel1st;
  int M_DUP_SEL;
  int phrnum;
  int *phridx;
  char *phrbuf;
  char *filename;
  time_t file_modify_time;
  gboolean key64;        // db is 64 bit-long key
  int max_keyN;
  char *endkey;       // only pinin/ar30 use it
  GTAB_space_pressed_E space_style;
  char *icon;
  u_char kmask, keybits, last_k_bitn;
  char WILD_QUES, WILD_STAR;
} INMD;

extern INMD inmd[MAX_GTAB_NUM_KEY+1];

u_int64_t CONVT2(INMD *inmd, int i);
extern INMD *cur_inmd;
void load_gtab_list();

#define LAST_K_bitN (cur_inmd->last_k_bitn)

#define KEY_MASK ((1<<cur_inmd->keybits)-1);


#define GTAB_LIST "gtab.list"

#if 1
#define NEED_SWAP (__BYTE_ORDER == __BIG_ENDIAN && 0)
#else
#define NEED_SWAP (1)
#endif
