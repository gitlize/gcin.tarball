#include "gcin.h"
#include "gtab.h"
#include "gcin-conf.h"
#include "gcin-endian.h"
#include "pho.h"
#include "tsin.h"
#include "tsin-parse.h"
#include "win-save-phrase.h"
#include "gtab-buf.h"
#include "gst.h"
#if WIN32
#include <io.h>
#endif

void disp_gbuf(), ClrIn(), clear_after_put();
gboolean gtab_phrase_on();
int page_len();
void show_win_gtab();
void disp_selection0(gboolean phrase_selected, gboolean force_disp);
void disp_gtab_sel(char *s);
void add_cache(int start, int usecount, TSIN_PARSE *out, short match_phr_N, short no_match_ch_N, int tc_len);
int ch_pos_find(char *ch, int pos);
void inc_gtab_usecount(char *str), ClrSelArea();
void lookup_gtabn(char *ch, char *out);
char *htmlspecialchars(char *s, char out[]);

extern gboolean test_mode;

GEDIT *gbuf;
extern char **seltab;
extern int ph_key_sz;

void extract_gtab_key(int start, int len, void *out)
{
  int i;

  char *p=(char *)out;
  if (ph_key_sz==4) {
    for(i=0; i < len; i++) {
      u_int k = gbuf[i+start].keys[0];
      memcpy(p, &k, sizeof(k));
      p+=sizeof(k);
    }
  } else {
    for(i=0; i < len; i++) {
      memcpy(p, &gbuf[i+start].keys[0], sizeof(u_int64_t));
      p+=sizeof(u_int64_t);
    }
  }
}

gboolean gtab_cursor_end()
{
  return ggg.gbuf_cursor==ggg.gbufN;
}

void dump_gbuf()
{
  int i;

  for(i=0; i<ggg.gbufN; i++) {
    int j;
    for(j=0;j < gbuf[i].selN; j++)
      printf("%d:%s ", j, gbuf[i].sel[j]);
    puts("");
  }
}

static unich_t latin_chars[]=
_L("ÀÁÂÃÄÅÆÆÇÈÉÊËÌÍÎÏÐÐÑÒÓÔÕÖØÙÚÛÜÝÞÞßàáâãäåææçèéêëìíîïððñòóôõöøùúûüýþþÿ")
_L("ĀāĂăĄąĆćĈĉĊċČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğĠġĢģĤĥĦħĨĩĪīĬĭĮįİıĲĲĳĳĴĵĶķĸĹĺĻļĽľĿŀŁł")
_L("ŃńŅņŇňŉŊŋŌōŎŏŐőŒŒœœŔŕŖŗŘřŚśŜŝŞşŠšŢţŤťŦŧŨũŪūŬŭŮůŰűŲųŴŵŶŷŸŹźŻżŽž");

int en_word_len(char *bf)
{
  int i;
  char *s;

  for(s=bf;*s;) {
    int sz = utf8_sz(s);
    if (sz==1) {
      if (!(*s >= 'A' && *s<='Z' || *s >= 'a' && *s<='z' || strchr("-_'", *s)))
        break;
    } else
    if (sz==2) {
      char *p;
#if WIN32
	  p = _(latin_chars);
	  for (; *p; p+=2)
#else
      for (p=latin_chars; *p; p+=2)
#endif
        if (!memcmp(p, s, 2))
          break;
      if (!(*p))
        break;
    } else
    if (sz>=3)
      break;
    s+=sz;
  }

  if (*s)
    return 0;
  return strlen(bf);
}

static char *gen_buf_str(int start, gboolean add_spc)
{
  int i;
  char *out = tmalloc(char, 1);
  int outN=0;

  gboolean last_en_word = FALSE;
  for(i=start;i<ggg.gbufN;i++) {
    char *t = gbuf[i].ch;
    int len = strlen(t);

    if (add_spc && en_word_len(t) && !(gbuf[i].flag & FLAG_CHPHO_GTAB_BUF_EN_NO_SPC)) {
      if (last_en_word) {
        out = trealloc(out, char, outN+1);
        out[outN++]=' ';
      }
      last_en_word = TRUE;
    } else
      last_en_word = FALSE;

    out = trealloc(out, char, outN+len+1);
    memcpy(out + outN, t, len);
    outN+=len;
  }

  out[outN] = 0;
  return out;
}

extern gboolean last_cursor_off;

static char *gen_buf_str_disp()
{
  if (!ggg.gbufN) {
    return strdup("");
  }

  int i;
  char *out = tmalloc(char, 1);
  int outN=0;

  out[0]=0;
  gbuf[ggg.gbufN].ch = " ";

  gboolean last_is_en_word = FALSE;

  int N = last_cursor_off ? ggg.gbufN-1:ggg.gbufN;
  for(i=0;i<=N;i++) {
    char addspc[MAX_CIN_PHR * 2 + 2];
    char spec[MAX_CIN_PHR * 2 + 2];
    int len = en_word_len(gbuf[i].ch);
//    dbg("i %d N:%d bufN:%d\n",i,N,ggg.gbufN);
    if (gbuf[i].flag & FLAG_CHPHO_GTAB_BUF_EN_NO_SPC)
      len = 0;
//    dbg("%d %d is_en:%d\n",i, len, last_is_en_word);

    if (len) {
      if (last_is_en_word) {
        strcpy(addspc, " ");
        strcat(addspc, gbuf[i].ch);
      } else
        strcpy(addspc, gbuf[i].ch);
      last_is_en_word = TRUE;
    } else {
      last_is_en_word = FALSE;
      strcpy(addspc, gbuf[i].ch);
    }

    htmlspecialchars(addspc, spec);
//    dbg("addspc '%s'  spec:%s out:%s\n", addspc, spec, out);

    char www[MAX_CIN_PHR * 2 + 2];
    char *t = spec;

    if (i==ggg.gbuf_cursor) {
      sprintf(www, "<span background=\"%s\">%s</span>", "red", spec);
      t = www;
    }

    len = strlen(t);
    out = trealloc(out, char, outN+len+1);
    memcpy(out + outN, t, len);
    outN+=len;
    out[outN] = 0;
  }

  return out;
}


void disp_label_edit(char *str);

static void free_pgbuf(GEDIT *p)
{
  int i;
  for(i=0; i < p->selN; i++)
    free(p->sel[i]);
  free(p->sel);
  p->ch = NULL;
  p->sel=NULL;
  p->flag = 0;
}


static void free_gbuf(int idx)
{
  free_pgbuf(&gbuf[idx]);
}


static void clear_gtab_buf_all()
{
  int i;
  for(i=0;i<ggg.gbufN;i++)
    free_gbuf(i);
  ggg.gbuf_cursor = ggg.gbufN=0;
  ggg.gtab_buf_select = 0;
  disp_gbuf();
}


void minimize_win_gtab();
void disp_gbuf()
{
#if WIN32
  if (test_mode)
    return;
#endif
  char *bf=gen_buf_str_disp();
  disp_label_edit(bf);

  if (ggg.gbufN && gtab_disp_key_codes)
    lookup_gtabn(gbuf[ggg.gbufN-1].ch, NULL);

  free(bf);

  minimize_win_gtab();
}

void clear_gbuf_sel()
{
  ggg.gtab_buf_select = 0;
  ggg.total_matchN = 0;
  ClrSelArea();
}

int gbuf_cursor_left()
{
  if (!ggg.gbuf_cursor)
    return ggg.gbufN;

  if (test_mode)
    return 1;

  if (ggg.gtab_buf_select)
    clear_gbuf_sel();
  ClrIn();
  ggg.gbuf_cursor--;
  disp_gbuf();
  return 1;
}


int gbuf_cursor_right()
{
  if (ggg.gbuf_cursor==ggg.gbufN)
    return ggg.gbufN;
#if WIN32
  if (test_mode)
    return 1;
#endif
  if (ggg.gtab_buf_select)
    clear_gbuf_sel();
  ggg.gbuf_cursor++;
  disp_gbuf();
  return 1;
}

int gbuf_cursor_home()
{
  if (!ggg.gbufN)
    return 0;
#if WIN32
  if (test_mode)
    return 1;
#endif
  if (ggg.gtab_buf_select)
    clear_gbuf_sel();

  ggg.gbuf_cursor = 0;
  disp_gbuf();
  return 1;
}


int gbuf_cursor_end()
{
  if (!ggg.gbufN)
    return 0;
#if WIN32
  if (test_mode)
    return 1;
#endif
  if (ggg.gtab_buf_select)
    clear_gbuf_sel();

  ggg.gbuf_cursor = ggg.gbufN;
  disp_gbuf();
  return 1;
}

void inc_gtab_use_count(char *s);
void inc_dec_tsin_use_count(void *pho, char *ch, int N);

gboolean output_gbuf()
{
  if (!ggg.gbufN)
    return FALSE;
#if WIN32
  if (test_mode)
    return TRUE;
#endif
  char *bf=gen_buf_str(0, TRUE);
#if 0
  printf("out %s\n", bf);
#endif

#if 0
  // single character
  char *p;
  for(p=bf; *p; p+=utf8_sz(p))
    inc_gtab_use_count(p);
#endif

  send_text(bf);
  free(bf);

  int i;
  for(i=0; i < ggg.gbufN;) {
    char t[MAX_CIN_PHR+1];
    t[0]=0;
    inc_gtab_use_count(gbuf[i].ch);

    int j;
    for(j=i; j < i+gbuf[i].plen; j++)
      strcat(t, gbuf[j].ch);

    if (!gbuf[i].plen)
      i++;
#if USE_TSIN
    else {
      u_int64_t kk[MAX_PHRASE_LEN];
	  extract_gtab_key(i, gbuf[i].plen, kk);
	  inc_dec_tsin_use_count(kk, t, gbuf[i].plen);
      i+=gbuf[i].plen;
    }
#endif
  }


  clear_gtab_buf_all();
  ClrIn();
  return TRUE;
}


gboolean check_gtab_fixed_mismatch(int idx, char *mtch, int plen)
{
  int j;
  char *p = mtch;

  for(j=0; j < plen; j++) {
    int u8sz = utf8_sz(p);
    if (!(gbuf[idx+j].flag & FLAG_CHPHO_FIXED))
      continue;

    if (memcmp(gbuf[idx+j].ch, p, u8sz))
      break;

    p+= u8sz;
  }

  if (j < plen)
    return TRUE;

  return FALSE;
}

void set_gtab_user_head()
{
  gbuf[ggg.gbuf_cursor].flag |= FLAG_CHPHO_PHRASE_USER_HEAD;
}


CACHE *cache_lookup(int start);

#define DBG 0

void init_cache();
void free_cache();
void init_tsin_table();
void set_tsin_parse_len(int);

void gtab_parse()
{
  int i;
  TSIN_PARSE out[MAX_PH_BF_EXT+1];
  bzero(out, sizeof(out));

  if (test_mode)
    return;

  if (ggg.gbufN <= 1)
    return;

  init_tsin_table();

  init_cache(ggg.gbufN);

  set_tsin_parse_len(ggg.gbufN);

  short smatch_phr_N, sno_match_ch_N;
  tsin_parse_recur(0, out, &smatch_phr_N, &sno_match_ch_N);
#if 0
  puts("vvvvvvvvvvvvvvvv");
  for(i=0;  i < out[i].len; i++) {
    printf("%x %d:", out[i].str, out[i].len);
    utf8_putcharn(out[i].str, out[i].len);
  }
  dbg("\n");
#endif

  for(i=0; i < ggg.gbufN; i++)
    gbuf[i].flag &= ~(FLAG_CHPHO_PHRASE_HEAD|FLAG_CHPHO_PHRASE_BODY);

  int ofsi;
  for(ofsi=i=0; out[i].len; i++) {
    int j, ofsj;

    if (out[i].flag & FLAG_TSIN_PARSE_PHRASE) {
      gbuf[ofsi].flag |= FLAG_CHPHO_PHRASE_HEAD;
      gbuf[ofsi].plen = out[i].len;
    }

    for(ofsj=j=0; j < out[i].len; j++) {
      char *w = (char *)&out[i].str[ofsj];
      int wsz = utf8_sz(w);
      ofsj += wsz;

      int k;
      for(k=0;k<gbuf[ofsi].selN; k++) {
        int sz = utf8_sz(gbuf[ofsi].sel[k]);
        if (wsz == sz && !memcmp(gbuf[ofsi].sel[k], w, sz))
          break;
      }
      if (k==gbuf[ofsi].selN) {
#if 0
        dbg("qq ");
        utf8_putchar(w);
        p_err(" err 1 selN:%d ofsi:%d", gbuf[ofsi].selN, ofsi);
#endif
        k=0;
      }

      if (!(gbuf[ofsi].flag & FLAG_CHPHO_FIXED)) {
        gbuf[ofsi].ch = gbuf[ofsi].sel[k];
        gbuf[ofsi].c_sel = k;
      }
      gbuf[ofsi].flag |= FLAG_CHPHO_PHRASE_BODY;

      ofsi++;
    }
  }

#if 0
  puts("-----------------------------");
  for(i=0;i<ggg.gbufN;i++)
    puts(gbuf[i].ch);
#endif
  free_cache();
}

static GEDIT *cursor_gbuf()
{
  return ggg.gbuf_cursor == ggg.gbufN ? &gbuf[ggg.gbuf_cursor-1] : &gbuf[ggg.gbuf_cursor];
}

typedef struct {
  char *s;
  int usecount;
  int org_seq;
} GITEM;

int get_gtab_use_count(char *s);

int qcmp_gitem(const void *aa, const void *bb)
{
  int d = ((GITEM *)bb)->usecount - ((GITEM *)aa)->usecount;
  if (d)
    return d;

  return ((GITEM *)aa)->org_seq - ((GITEM *)bb)->org_seq;
}

void hide_row2_if_necessary();

unich_t auto_end_punch[]=_L(", . ? : ; ! [ ] 「 」 ， 。 ？ ； ： 、 ～ ！ （ ）");
GEDIT *insert_gbuf_cursor(char **sel, int selN, u_int64_t key, gboolean b_gtab_en_no_spc)
{
  hide_row2_if_necessary();

  if (!sel || !selN)
    return NULL;
//  dbg("insert_gbuf_cursor %x\n", key);

  gbuf=trealloc(gbuf, GEDIT, ggg.gbufN+2);

  GEDIT *pbuf = &gbuf[ggg.gbuf_cursor];

  if (ggg.gbuf_cursor < ggg.gbufN)
    memmove(&gbuf[ggg.gbuf_cursor+1], &gbuf[ggg.gbuf_cursor], sizeof(GEDIT) * (ggg.gbufN - ggg.gbuf_cursor));

  ggg.gbuf_cursor++;
  ggg.gbufN++;

  bzero(pbuf, sizeof(GEDIT));
  bzero(gbuf+ggg.gbufN, sizeof(GEDIT));

  GITEM *items = tmalloc(GITEM, selN);

  int i;
  for(i=0; i < selN; i++) {
    items[i].s = sel[i];
    items[i].org_seq = i;
    items[i].usecount = get_gtab_use_count(sel[i]);
  }
  qsort(items, selN, sizeof(GITEM), qcmp_gitem);

  for(i=0; i < selN; i++)
    sel[i] = items[i].s;

  pbuf->ch = sel[0];
  pbuf->sel = sel;
  pbuf->selN = selN;
  pbuf->c_sel = 0;
  pbuf->keys[0] = key;
  pbuf->keysN=1;
  pbuf->flag = b_gtab_en_no_spc ? FLAG_CHPHO_GTAB_BUF_EN_NO_SPC:0;

  if (ggg.gbufN==ggg.gbuf_cursor && selN==1 && strstr(_(auto_end_punch), sel[0])) {
    char_play(pbuf->ch);
    output_gbuf();
  } else {
    gtab_parse();
    disp_gbuf();
    char_play(pbuf->ch);
  }

  free(items);
  return pbuf;
}


void set_gbuf_c_sel(int v)
{
  GEDIT *pbuf = cursor_gbuf();

  pbuf->c_sel = v + ggg.pg_idx;
  pbuf->ch = pbuf->sel[pbuf->c_sel];
//  dbg("zzzsel v:%d %d %s\n",v, pbuf->c_sel,pbuf->ch);
  pbuf->flag |= FLAG_CHPHO_FIXED;
  ggg.gtab_buf_select = 0;
  disp_gtab_sel("");
  gtab_parse();
  disp_gbuf();
//  dbg("zzzsel v:%d\n", pbuf->c_sel);
}

GEDIT *insert_gbuf_cursor1(char *s, u_int64_t key, gboolean b_gtab_en_no_spc)
{
   if (!gtab_phrase_on())
     return NULL;

//   dbg("insert_gbuf_cursor1 %s %x\n", s, key);
   char **sel = tmalloc(char *, 1);
   sel[0] = strdup(s);
   GEDIT *e = insert_gbuf_cursor(sel, 1, key, b_gtab_en_no_spc);
   clear_after_put();
   return e;
}

static int key_N(u_int64_t k)
{
  int n=0;
  int mask = (1 << KeyBits) - 1;

  while (k) {
    k>>=mask;
    n++;
  }

  return n;
}

static int qcmp_key_N(const void *aa, const void *bb)
{
  u_int64_t a = *((u_int64_t *)aa);
  u_int64_t b = *((u_int64_t *)bb);

  return key_N(a) - key_N(b);
}


void insert_gbuf_nokey(char *s)
{
#if WIN32 || 1
   if (test_mode)
     return;
#endif
   if (!gtab_phrase_on())
     return;

//   dbg("insert_gbuf_nokey\n");

   int i;
   u_int64_t keys[32];
   int keysN=0;
   int sz = utf8_sz(s);

   keys[0]=0;
   if (cur_inmd->tbl64) {
     for(i=0; i < cur_inmd->DefChars; i++) {
       if (!memcmp(cur_inmd->tbl64[i].ch, s, sz)) {
         u_int64_t t;
         memcpy(&t, cur_inmd->tbl64[i].key, sizeof(u_int64_t));
         keys[keysN++] = t;
       }
     }
   } else
   if (cur_inmd->tbl) {
     for(i=0; i < cur_inmd->DefChars; i++) {
       if (!memcmp(cur_inmd->tbl[i].ch, s, sz)) {
         u_int t;
         memcpy(&t, cur_inmd->tbl[i].key, sizeof(u_int));
         keys[keysN++] = t;
       }
     }
   }

   qsort(keys, keysN, sizeof(u_int64_t), qcmp_key_N);

   GEDIT *e = insert_gbuf_cursor1(s, keys[0], TRUE);
   if (keysN > 8)
     keysN = 8;

   memcpy(e->keys, keys, sizeof(u_int64_t) * keysN);
   e->keysN = keysN;
}

void insert_gbuf_cursor1_cond(char *s, u_int64_t key, gboolean valid_key)
{
  if (test_mode)
    return;

  if (valid_key)
    insert_gbuf_cursor1(s, key, FALSE);
  else
    insert_gbuf_nokey(s);
}

void insert_gbuf_cursor_char(char ch)
{
  if (test_mode)
    return;

  char t[2];
  t[0]=ch;
  t[1]=0;
  insert_gbuf_cursor1(t, 0, TRUE);
}

gboolean gtab_has_input();
void hide_win_gtab();

int gtab_buf_delete()
{
  if (ggg.gbuf_cursor==ggg.gbufN)
    return 0;

  if (test_mode)
    return 1;

  if (ggg.gtab_buf_select)
    clear_gbuf_sel();

  free_gbuf(ggg.gbuf_cursor);
  memmove(&gbuf[ggg.gbuf_cursor], &gbuf[ggg.gbuf_cursor+1], sizeof(GEDIT) * (ggg.gbufN - ggg.gbuf_cursor -1));
  ggg.gbufN--;
  disp_gbuf();

  if (gcin_pop_up_win && !gtab_has_input())
    hide_win_gtab();

  return 1;
}

gboolean gtab_has_input();
void hide_win_gtab();

int gtab_buf_backspace()
{
  if (!ggg.gbuf_cursor) {
    return ggg.gbufN>0;
  }

  if (test_mode)
    return 1;

  ggg.gbuf_cursor--;
  gtab_buf_delete();

  if (gcin_pop_up_win && !gtab_has_input())
    hide_win_gtab();

  return 1;
}

extern int more_pg;

void gtab_disp_sel()
{
  int idx = ggg.gbuf_cursor==ggg.gbufN ? ggg.gbuf_cursor-1:ggg.gbuf_cursor;
  GEDIT *pbuf=&gbuf[idx];

  int i;
  for(i=0; i < cur_inmd->M_DUP_SEL; i++) {
    int v = i + ggg.pg_idx;
    if (v >= pbuf->selN)
      seltab[i][0]=0;
    else
      strcpy(seltab[i], pbuf->sel[v]);
  }

  if (pbuf->selN > page_len())
    ggg.more_pg = 1;
#if WIN32
  show_win_gtab();
  disp_selection0(FALSE, TRUE);
#else
  disp_selection0(FALSE, TRUE);
  show_win_gtab();
#endif
}


int show_buf_select()
{
  if (!ggg.gbufN)
    return 0;

  int idx = ggg.gbuf_cursor==ggg.gbufN ? ggg.gbuf_cursor-1:ggg.gbuf_cursor;
  GEDIT *pbuf=&gbuf[idx];
  ggg.gtab_buf_select = 1;
  ggg.total_matchN = pbuf->selN;
  ggg.pg_idx = 0;

  gtab_disp_sel();

  return 1;
}

void gbuf_prev_pg()
{
  ggg.pg_idx -= page_len();
  if (ggg.pg_idx < 0)
    ggg.pg_idx = 0;

  gtab_disp_sel();
}

void gbuf_next_pg()
{
  ggg.pg_idx += page_len();
  if (ggg.pg_idx >= ggg.total_matchN)
    ggg.pg_idx = 0;

  gtab_disp_sel();
}

#include "im-client/gcin-im-client-attr.h"

int get_DispInArea_str(char *out);

int gtab_get_preedit(char *str, GCIN_PREEDIT_ATTR attr[], int *pcursor, int *sub_comp_len)
{
  int i=0;
  int strN=0;
  int attrN=0;
  int ch_N=0;

//  dbg("gtab_get_preedit\n");
  str[0]=0;
  *pcursor=0;

#if WIN32
  *sub_comp_len = ggg.ci > 0;
#endif
  gboolean ap_only = gcin_edit_display_ap_only();

  if (gtab_phrase_on()) {
    attr[0].flag=GCIN_PREEDIT_ATTR_FLAG_UNDERLINE;
    attr[0].ofs0=0;

    if (ggg.gbufN)
      attrN=1;

    gboolean last_is_en_word = FALSE;
    for(i=0; i < ggg.gbufN; i++) {
      char *s = gbuf[i].ch;
      char tt[MAX_CIN_PHR+2];

      if (en_word_len(s) && !(gbuf[i].flag & FLAG_CHPHO_GTAB_BUF_EN_NO_SPC)) {
        if (last_is_en_word) {
          strcpy(tt, " ");
          strcat(tt, s);
          s = tt;
        }
        last_is_en_word = TRUE;
      } else {
        last_is_en_word = FALSE;
      }

      int len = strlen(s);
      int N = utf8_str_N(s);
      ch_N+=N;
      if (i < ggg.gbuf_cursor)
        *pcursor+=N;
      if (i==ggg.gbuf_cursor) {
        attr[1].ofs0=*pcursor;
        attr[1].ofs1=*pcursor+N;
        attr[1].flag=GCIN_PREEDIT_ATTR_FLAG_REVERSE;
        attrN++;
      }

      if (gcin_on_the_spot_key && i==ggg.gbuf_cursor)
        strN += get_DispInArea_str(str+strN);

      memcpy(str+strN, s, len);
      strN+=len;
    }
  }


  if (ap_only && gcin_on_the_spot_key && i==ggg.gbuf_cursor)
    strN += get_DispInArea_str(str+strN);

  str[strN]=0;

  attr[0].ofs1 = ch_N;
  return attrN;
}

extern GtkWidget *gwin_gtab;
void gtab_reset()
{
#if UNIX
  if (!gwin_gtab)
    return;
#endif
  int v = ggg.gbufN > 0;
  clear_gtab_buf_all();
  clear_gbuf_sel();
  ClrIn();
  return;
}


void save_gtab_buf_phrase(KeySym key)
{
  int len = key - '0';
  int idx0 = ggg.gbuf_cursor - len;
  int idx1 = ggg.gbuf_cursor - 1;

  if (idx0 < 0 || idx0 > idx1)
    return;

  WSP_S wsp[MAX_PHRASE_LEN];

  bzero(wsp, sizeof(wsp));
  int i;
  for(i=0; i < len; i++) {
    u8cpy(wsp[i].ch, gbuf[idx0 + i].ch);
    wsp[i].key = gbuf[idx0 + i].keys[0];
  }

  create_win_save_phrase(wsp, len);
}

gboolean save_gtab_buf_shift_enter()
{
	if (ggg.gbuf_cursor== ggg.gbufN)
		return 0;

	int keys[512];
	int N = ggg.gbufN - ggg.gbuf_cursor;
	extract_gtab_key(ggg.gbuf_cursor, N, keys);
	char *str = gen_buf_str(ggg.gbuf_cursor, FALSE);
	save_phrase_to_db(keys, str, N, 1);

	free(str);
	gbuf_cursor_end();
	return 1;
}


void load_tsin_db0(char *infname, gboolean is_gtab_i);
void get_gcin_user_or_sys_fname(char *name, char fname[]);

void init_tsin_table()
{
#if USE_TSIN
  if (!current_CS)
    return;

  INMD *p= &inmd[current_CS->in_method];

  char fname[256], fname_idx[256], gtab_phrase_src[256], gtabfname[256];
  if (p->filename_append) {
//    dbg("filename_append %s\n",p->filename_append);
    strcpy(fname, p->filename_append);
    strcpy(gtabfname, fname);
  } else
  if (p->filename) {
    get_gcin_user_fname(p->filename, fname);
    get_gcin_user_or_sys_fname(p->filename, gtabfname);
  } else {
    dbg("no file name\n");
    return;
  }

  strcat(fname, ".tsin-db");
  strcat(strcpy(fname_idx, fname), ".idx");
  strcat(strcpy(gtab_phrase_src, fname), ".src");
//  dbg("init_tsin_table %s\n", fname);

#if UNIX
  putenv("GCIN_NO_RELOAD=");
#else
  _putenv("GCIN_NO_RELOAD=Y");
#endif

#if UNIX
  if (access(fname, W_OK) < 0 || access(fname_idx, W_OK) < 0)
#else
  if (_access(fname, 02) < 0 || _access(fname, 02) < 0)
#endif
  {
#if UNIX
    unix_exec(GCIN_BIN_DIR"/tsin2gtab-phrase %s %s", gtabfname, gtab_phrase_src);
    unix_exec(GCIN_BIN_DIR"/tsa2d32 %s %s", gtab_phrase_src, fname);
#else
    win32exec_va("tsin2gtab-phrase", gtabfname, gtab_phrase_src, NULL);
    win32exec_va("tsa2d32", gtab_phrase_src, fname, NULL);
#endif
  }

  load_tsin_db0(fname, TRUE);
#endif
}
