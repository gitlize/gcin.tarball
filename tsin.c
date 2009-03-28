/*
	Copyright (C) 2004	Edward Liu, Taiwan
*/
#include <string.h>

#include "gcin.h"
#include "pho.h"
#include "tsin.h"
#include "gcin-conf.h"

static gint64 key_press_time;
extern gboolean b_hsu_kbm;

extern PHO_ITEM *ch_pho;
extern char *pho_chars[];

extern int ityp3_pho;
extern u_char typ_pho[];
gboolean typ_pho_empty();
extern char inph[];

extern u_short hash_pho[];
extern PHOKBM phkbm;

extern char tsidxfname[64];
extern int hashidx[TSIN_HASH_N];

CHPHO chpho[MAX_PH_BF_EXT];
int c_idx;
extern int c_len;
int ph_sta=-1, ph_sta_last=-1;  // phrase start
static int sel_pho;
static gboolean eng_ph=TRUE;  // english(FALSE) <-> pho(juyin, TRUE)
static int save_frm, save_to;
static int current_page;
static int startf;
static gboolean full_match;
static gboolean tsin_half_full;
static int bufferEditing = 0; //0 = buffer editing mode is disabled

typedef struct {
  phokey_t phokey[MAX_PHRASE_LEN];
  int phidx;
  char str[MAX_PHRASE_LEN*CH_SZ+1];
  int len;
  usecount_t usecount;
} PRE_SEL;

static PRE_SEL pre_sel[10];
static int pre_selN;

void clrin_pho(), hide_win0();

static void clrin_pho_tsin()
{
  clrin_pho();

  if (!c_len && gcin_pop_up_win)
    hide_win0();
}

gboolean pho_has_input();

gboolean tsin_has_input()
{
  return c_len || pho_has_input() || !eng_ph;
}


gboolean save_phrase_to_db2(CHPHO *chph, int len);

void disp_char(int index, u_char *ch);

static void disp_char_chbuf(int idx)
{
  disp_char(idx, chpho[idx].ch);
}

static void init_chpho_i(int i)
{
  chpho[i].ch[0]=' ';
  chpho[i].ch[1]=0;
  chpho[i].flag=0;
  chpho[i].psta=-1;
}

void clr_tsin_cursor(int index);

static void clrcursor()
{
  clr_tsin_cursor(c_idx);
}

static int last_cursor_idx=0;
void set_cursor_tsin(int index);

void drawcursor()
{
  clr_tsin_cursor(last_cursor_idx);
  last_cursor_idx = c_idx;

  if (c_idx == c_len) {
    if (!eng_ph) {
      if (tsin_half_full) {
        disp_char(c_idx,"  ");
        set_cursor_tsin(c_idx);
      } else {
        disp_char(c_idx, " ");
        set_cursor_tsin(c_idx);
      }
    }
  }
  else {
    set_cursor_tsin(c_idx);
  }
}

static void chpho_extract(CHPHO *chph, int len, phokey_t *pho, char *ch, char *och)
{
   int i;
   int ofs=0, oofs=0;

   for(i=0; i < len; i++) {
      pho[i] = chph[i].pho;
      int u8len = u8cpy(&ch[ofs], chph[i].ch);
      ofs+=u8len;

      if (och) {
        int ou8len = u8cpy(&och[oofs], chph[i].och);
        oofs+=ou8len;
      }
   }
}


void inc_pho_count(phokey_t key, int ch_idx);
int ch_key_to_ch_pho_idx(phokey_t phkey, char *big5);
void inc_dec_tsin_use_count(phokey_t *pho, char *ch, int prlen, int N);
void lookup_gtabn(char *ch, char *);

static void putbuf(int len)
{
  u_char tt[CH_SZ * (MAX_PH_BF_EXT+1) + 1];
  int i,idx;

//  dbg("putbuf:%d\n", len);

#if 1
  // update phrase reference count
  if (len >= 2) {
    for(i=0; i < len; i++) {
//      dbg("flag %d %x\n", i, chpho[i].flag);
      if (!BITON(chpho[i].flag, FLAG_CHPHO_PHRASE_HEAD)) {
        continue;
      }

      int j;
      for(j=i+1; j < len; j++)
        if (chpho[j].psta != i)
          break;

      int phrlen = j - i;
#if 0
      dbg("phrlen %d ", phrlen);
      int k;
      for(k=i; k < j; k++)
        utf8_putchar(chpho[k].ch);
      puts("");
      for(k=i; k < j; k++)
        utf8_putchar(chpho[k].och);
      puts("");
#endif

      if (phrlen < 1)
        continue;

      phokey_t pho[MAX_PHRASE_LEN];
      char ch[MAX_PHRASE_LEN * CH_SZ];
      char och[MAX_PHRASE_LEN * CH_SZ];

      chpho_extract(&chpho[i], phrlen, pho, ch, och);

      if (chpho[i].flag & FLAG_CHPHO_PHRASE_VOID) {
#if 0
        inc_dec_tsin_use_count(pho, och, phrlen, TRUE);
#endif
      }
      else
        inc_dec_tsin_use_count(pho, ch, phrlen, FALSE);
    }
  }
#endif

  for(idx=i=0;i<len;i++) {
    int len = utf8_sz(chpho[i].ch);

    if (chpho[i].pho && len > 1) {
      int pho_idx = ch_key_to_ch_pho_idx(chpho[i].pho, chpho[i].ch);
      if (pho_idx >= 0)
        inc_pho_count(chpho[i].pho, pho_idx);
    }

    memcpy(&tt[idx], chpho[i].ch, len);
    idx += len;
  }

  tt[idx]=0;
  send_text(tt);
  lookup_gtabn(tt, NULL);
}


void hide_char(int index);

static void prbuf()
{
  int i;

  for(i=0; i < c_len; i++) {
    disp_char_chbuf(i);
  }

  drawcursor();
}


void disp_tsin_pho(int index, char *pho);

static void disp_in_area_pho_tsin()
{
  int i;

  if (pin_juyin) {
    for(i=0;i<6;i++) {
      disp_tsin_pho(i, &inph[i]);
    }
  } else {
    for(i=0;i<4;i++) {
      disp_tsin_pho(i, &pho_chars[i][typ_pho[i] * PHO_CHAR_LEN]);
    }
  }
}


void clear_chars_all();


static void clear_disp_ph_sta();
static void clear_match()
{
  ph_sta=-1;
  clear_disp_ph_sta();
//  bzero(psta, sizeof(psta));
//  dbg("clear_match\n");
}

static void clr_ch_buf()
{
  int i;
  for(i=0; i < MAX_PH_BF_EXT; i++) {
    init_chpho_i(i);
  }

  clear_match();
}


static void clear_ch_buf_sel_area()
{
  clear_chars_all();
  c_len=c_idx=0; ph_sta=-1;
  full_match = FALSE;
  clr_ch_buf();
  drawcursor();
  clear_disp_ph_sta();
}

static void close_selection_win();

static void clear_tsin_buffer()
{
  clear_ch_buf_sel_area();
  close_selection_win();
  pre_selN = 0;
}

void clr_in_area_pho_tsin();
void close_win_pho_near();
void compact_win0_x();

void tsin_reset_in_pho0()
{
//  prbuf();
  clr_in_area_pho_tsin();
  close_selection_win();
  pre_selN = 0;
  drawcursor();
  close_win_pho_near();
}

void tsin_reset_in_pho()
{
  clrin_pho_tsin();
  tsin_reset_in_pho0();
}

gboolean flush_tsin_buffer()
{
  tsin_reset_in_pho();

  if (gcin_pop_up_win)
    hide_win0();

  if (c_len) {
    putbuf(c_len);
    compact_win0_x();
    clear_ch_buf_sel_area();
    clear_tsin_buffer();
    return 1;
  }

  return 0;
}


void disp_tsin_eng_pho(int eng_pho);

void show_stat()
{
  disp_tsin_eng_pho(eng_ph);
}

void load_tsin_db();


#if 0
void nputs(u_char *s, u_char len)
{
  char tt[16];

  memcpy(tt, s, len*CH_SZ);
  tt[len*CH_SZ]=0;
  dbg("%s", tt);
}


static void dump_tsidx(int i)
{
  phokey_t pho[MAX_PHRASE_LEN];
  u_char ch[MAX_PHRASE_LEN*CH_SZ];
  usecount_t usecount;
  u_char len;

  load_tsin_entry(i, &len, &usecount, pho, ch);

  int j;
  for(j=0; j < len; j++) {
    prph(pho[j]);
    dbg(" ");
  }

  nputs(ch, len);
  dbg("\n");
}


static void dump_tsidx_all()
{
  int i;

  for(i=0; i < phcount; i++) {
    dump_tsidx(i);
  }

  dbg("************************************************\n");
  for(i=0; i < 254; i++) {
    dbg("%d]%d ", i, hashidx[i]);
    dump_tsidx(hashidx[i]);
  }
}

#endif


void load_tab_pho_file();
void show_win0();

void init_tab_pp()
{
  if (!ch_pho)
    load_tab_pho_file();

  if (phcount && gwin0) {
disp_prom:
    show_stat();
    clear_ch_buf_sel_area();
    if (!gcin_pop_up_win)
      show_win0();

    return;
  }

  load_tsin_db();

  clr_ch_buf();
  show_win0();

//  dump_tsidx_all();
  goto disp_prom;
}

gboolean save_phrase_to_db2(CHPHO *chph, int len);

static void save_phrase()
{
  int i;
  u_char len;

  if (save_frm > save_to) {
    int tt=save_frm;
    save_frm=save_to;
    save_to=tt;
  }

  if (save_to==c_len)
    save_to--;

  len= save_to- save_frm+ 1;

  if (len <= 0 || len > MAX_PHRASE_LEN)
    return;

  for(i=save_frm;i<=save_to;i++) {
    if (chpho[i].pho)
      continue;
    phokey_t tpho[32];
    tpho[0]=0;

    utf8_pho_keys(chpho[i].ch, tpho);

    if (!tpho[0])
      return;

    chpho[i].pho = tpho[0];
  }

  if (!save_phrase_to_db2(&chpho[save_frm], len)) {
    bell();
  }

  clrcursor();
  ph_sta=-1;
  c_idx=c_len;
  drawcursor();
  return;
}


static void set_fixed(int idx, int len)
{
  int i;
  for(i=idx; i < idx+len; i++) {
    chpho[i].flag |= FLAG_CHPHO_FIXED;
    chpho[i].flag &= ~FLAG_CHPHO_PHRASE_USER_HEAD;
  }
}

#define PH_SHIFT_N (tsin_buffer_size - 1)

static void shift_ins()
{
   int j;

   if (!c_idx && c_len >= PH_SHIFT_N) {
     c_len--;
   }
   else
   if (c_len >= PH_SHIFT_N) {
     int ofs;

     // set it fixed so that it will not cause partial phrase in the beginning
     int fixedlen = c_len - 10;
     if (fixedlen <= 0)
       fixedlen = 1;
     set_fixed(0, fixedlen);

     ofs = 1;
     putbuf(ofs);

     ph_sta-=ofs;
     for(j=0; j < c_len - ofs; j++) {
       chpho[j] = chpho[j+ofs];
     }
     c_idx-=ofs;
     c_len-=ofs;
     prbuf();
   }

   c_len++;
   if (c_idx < c_len-1) {
     for(j=c_len; j>=c_idx; j--) {
       chpho[j+1] = chpho[j];
     }

     init_chpho_i(c_len);
    /*    prbuf(); */
   }

   compact_win0_x();
}


static void put_b5_char(char *b5ch, phokey_t key)
{
   shift_ins();

   init_chpho_i(c_idx);
   bchcpy(chpho[c_idx].ch, b5ch);
   bchcpy(chpho[c_idx].och, b5ch);
   bchcpy(chpho[c_idx].ph1ch, b5ch);

   disp_char_chbuf(c_idx);

   chpho[c_idx].pho=key;
   c_idx++;

   if (c_idx < c_len) {
     prbuf();
   }
}

#define MAX_PHRASE_SEL_N 10

static u_char selstr[MAX_PHRASE_SEL_N][MAX_PHRASE_LEN * CH_SZ];
static u_char sellen[MAX_PHRASE_SEL_N];

static u_short phrase_count;
static u_short pho_count;

static gboolean chpho_eq_pho(int idx, phokey_t *phos, int len)
{
  int i;

  for(i=0; i < len; i++)
    if (chpho[idx+i].pho != phos[i])
       return FALSE;

  return TRUE;
}


static void get_sel_phrase()
{
  int sti,edi,j;
  u_char len, mlen;

  phrase_count = 0;

  mlen=c_len-c_idx;

  if (mlen > MAX_PHRASE_LEN)
    mlen=MAX_PHRASE_LEN;

  phokey_t pp[MAX_PHRASE_LEN + 1];
  extract_pho(c_idx, mlen, pp);

  if (!tsin_seek(pp, 2, &sti, &edi))
    return;

  while (sti < edi && phrase_count < phkbm.selkeyN) {
    phokey_t stk[MAX_PHRASE_LEN];
    usecount_t usecount;
    u_char stch[MAX_PHRASE_LEN * CH_SZ + 1];

    load_tsin_entry(sti, &len, &usecount, stk, stch);

    if (len > mlen || len==1) {
      sti++;
      continue;
    }

    if (chpho_eq_pho(c_idx, stk, len)) {
      sellen[phrase_count]=len;
      utf8cpyN(selstr[phrase_count++], stch, len);
    }

    sti++;
  }
}

static void get_sel_pho()
{
  phokey_t key;

  if (c_idx==c_len)
    key=chpho[c_idx-1].pho;
  else
    key=chpho[c_idx].pho;

  if (!key)
    return;

  int i=hash_pho[key>>9];
  phokey_t ttt;

  while (i<idxnum_pho) {
    ttt=idx_pho[i].key;
    if (ttt>=key)
      break;
    i++;
  }

  if (ttt!=key) {
    return;
  }

  startf = idx_pho[i].start;
  pho_count = idx_pho[i+1].start - startf;
//  dbg("pho_count %d\n", pho_count);
}


void clear_sele();
void set_sele_text(int i, char *text, int len);
void disp_selections(int idx);
void disp_arrow_up(), disp_arrow_down();

static void disp_current_sel_page()
{
  int i;

  clear_sele();

  for(i=0; i < phkbm.selkeyN; i++) {
    int idx = current_page + i;

    if (idx < phrase_count) {
      int tlen = utf8_tlen(selstr[i], sellen[i]);
      set_sele_text(i, selstr[i], tlen);
    } else
    if (idx < phrase_count + pho_count) {
      int v = idx - phrase_count + startf;
      set_sele_text(i, ch_pho[v].ch, utf8_sz(ch_pho[v].ch));
    } else
      break;
  }

  if (current_page + phkbm.selkeyN < phrase_count + pho_count) {
    disp_arrow_down();
  }

  if (current_page > 0)
    disp_arrow_up();

  disp_selections(c_idx==c_len?c_idx-1:c_idx);
}

static int fetch_user_selection(int val, char **seltext)
{
  int idx = current_page + val;
  int len = 0;

  if (idx < phrase_count) {
    len = sellen[idx];

    *seltext = selstr[idx];
  } else
  if (idx < phrase_count + pho_count) {
    int v = idx - phrase_count + startf;

    len = 1;
    *seltext = ch_pho[v].ch;
  }

  return len;
}


int phokey_t_seq(phokey_t *a, phokey_t *b, int len);

void extract_pho(int chpho_idx, int plen, phokey_t *pho)
{
  int i;

  for(i=0; i < plen; i++) {
    pho[i] = chpho[chpho_idx + i].pho;
  }
}


void mask_key_typ_pho(phokey_t *key);

static int qcmp_pre_sel_usecount(const void *aa, const void *bb)
{
  PRE_SEL *a = (PRE_SEL *) aa;
  PRE_SEL *b = (PRE_SEL *) bb;

  return b->usecount - a->usecount;
}

gboolean check_fixed_mismatch(int chpho_idx, char *mtch, int plen)
{
  int j;
  char *p = mtch;

  for(j=0; j < plen; j++) {
    int u8sz = utf8_sz(p);
    if (!(chpho[chpho_idx+j].flag & FLAG_CHPHO_FIXED))
      continue;

    if (memcmp(chpho[chpho_idx+j].ch, p, u8sz))
      break;

    p+= u8sz;
  }

  if (j < plen)
    return TRUE;

  return FALSE;
}

static u_char scanphr(int chpho_idx, int plen, gboolean pho_incr)
{
  if (plen >= MAX_PHRASE_LEN)
    return 0;

  phokey_t tailpho;

  if (pho_incr) {
    tailpho = pho2key(typ_pho);
    if (!tailpho)
      pho_incr = FALSE;
  }

  phokey_t pp[MAX_PHRASE_LEN + 1];
  extract_pho(chpho_idx, plen, pp);
  int sti, edi;

#if 0
  dbg("scanphr %d\n", plen);

  int t;
  for(t=0; t < plen; t++)
    prph(pp[t]);
  puts("");
#endif

  if (!tsin_seek(pp, plen, &sti, &edi))
    return 0;


  pre_selN = 0;
  int maxlen=0;

#define selNMax 30
  PRE_SEL sel[selNMax];
  int selN = 0;


  while (sti < edi && selN < selNMax) {
    phokey_t mtk[MAX_PHRASE_LEN];
    u_char mtch[MAX_PHRASE_LEN*CH_SZ+1];
    char match_len;
    usecount_t usecount;

    load_tsin_entry(sti, &match_len, &usecount, mtk, mtch);

    sti++;
    if (plen > match_len || (pho_incr && plen==match_len)) {
      continue;
    }



    int i;
    for(i=0; i < plen; i++) {
      if (mtk[i]!=pp[i])
        break;
    }

    if (i < plen)
      continue;

    if (pho_incr) {
      phokey_t last_m = mtk[plen];
      mask_key_typ_pho(&last_m);
      if (last_m != tailpho)
        continue;
    }


#define VOID_PHRASE_N -1

#if 0
    if (/* match_len == 2 && */ usecount < VOID_PHRASE_N)
      continue;
#else
    if (match_len == plen && usecount < VOID_PHRASE_N)
      continue;
#endif

#if 0
    dbg("nnn ");
    nputs(mtch, match_len);
    dbg("\n");
#endif

#if 1
    if (check_fixed_mismatch(chpho_idx, mtch, plen))
      continue;
#endif

    if (maxlen < match_len)
      maxlen = match_len;

    sel[selN].len = match_len;
    sel[selN].phidx = sti - 1;
    sel[selN].usecount = usecount;
    utf8cpyN(sel[selN].str, mtch, match_len);
    memcpy(sel[selN].phokey, mtk, match_len*sizeof(phokey_t));
    selN++;
  }

  qsort(sel, selN, sizeof(PRE_SEL), qcmp_pre_sel_usecount);

  pre_selN = Min(selN, phkbm.selkeyN);

  memcpy(pre_sel, sel, sizeof(PRE_SEL) * pre_selN);

  return maxlen;
}

void hide_selections_win();

static void disp_pre_sel_page()
{

  int i;

  if (!tsin_phrase_pre_select) {
    return;
  }

  if (pre_selN==0 || (pre_selN==1 && pre_sel[0].len<=2) || ph_sta < 0) {
    hide_selections_win();
    return;
  }

  clear_sele();

  for(i=0; i < Min(phkbm.selkeyN, pre_selN); i++) {
    int tlen = utf8_tlen(pre_sel[i].str, pre_sel[i].len);

    set_sele_text(i, pre_sel[i].str, tlen);
  }
#if 0
  dbg("ph_sta:%d\n", ph_sta);
#endif
  disp_selections(ph_sta);
}

static void close_selection_win()
{
  hide_selections_win();
  current_page=sel_pho=0;
  pre_selN = 0;
}

void show_button_pho(gboolean bshow);

void tsin_set_eng_ch(int nmod)
{
  eng_ph = nmod;
  show_stat();
  drawcursor();

  show_button_pho(eng_ph);

  show_win0();
}

void tsin_toggle_eng_ch()
{
  compact_win0_x();
  tsin_set_eng_ch(!eng_ph);
}

void tsin_toggle_half_full()
{
    tsin_half_full^=1;
    key_press_time = 0;
    drawcursor();
}


#if 0
static char ochars[]="<,>.?/:;\"'{[}]_-+=|\\~`";
#else
static char ochars[]="<,>.?/:;\"'{[}]_-+=|\\";
#endif

static void hide_pre_sel()
{
  pre_selN = 0;
  hide_selections_win();
}


void clear_tsin_line();

static void clear_disp_ph_sta()
{
  clear_tsin_line();
}

void draw_underline(int index);

static void draw_ul(start, stop)
{
  int i;
  for(i=start; i < stop; i++)
    draw_underline(i);
}

void disp_ph_sta_idx(int idx)
{
//  dbg("ph_sta:%d\n", ph_sta);
  clear_disp_ph_sta();

  if (ph_sta < 0)
    return;

  draw_ul(idx, c_idx);
}

void disp_ph_sta()
{
  disp_ph_sta_idx(ph_sta);
}

void ch_pho_cpy(CHPHO *pchpho, char *utf8, phokey_t *phos, int len)
{
  int i;

  for(i=0; i < len; i++) {
    int len = u8cpy(pchpho[i].ch, utf8);
    utf8+=len;
    pchpho[i].pho = phos[i];
  }
}


void set_chpho_ch(CHPHO *pchpho, char *utf8, int len)
{
  int i;

  for(i=0; i < len; i++) {
    int u8len = u8cpy(pchpho[i].ch, utf8);
    utf8+=u8len;
  }
}


void set_chpho_ch2(CHPHO *pchpho, char *utf8, int len)
{
  int i;

  for(i=0; i < len; i++) {
    int u8len = u8cpy(pchpho[i].ch, utf8);
    u8cpy(pchpho[i].och, utf8);
    utf8+=u8len;
  }
}


gboolean add_to_tsin_buf(char *str, phokey_t *pho, int len)
{
    int i;

    if (c_idx < 0 || c_len + len >= MAX_PH_BF_EXT)
      return 0;

    if (c_idx < c_len) {
      for(i=c_len-1; i >= c_idx; i--) {
        chpho[i+len] = chpho[i];
      }
    }

    ch_pho_cpy(&chpho[c_idx], str, pho, len);

    if (c_idx == c_len)
      c_idx +=len;

    c_len+=len;

    clrin_pho_tsin();
    disp_in_area_pho_tsin();

    prbuf();

    set_fixed(c_idx, len);
#if 1
    for(i=1;i < len; i++) {
      chpho[c_idx+i].psta= c_idx;
    }
#endif
#if 0
    if (len > 0)
      chpho[c_idx].flag |= FLAG_CHPHO_PHRASE_HEAD;
#endif
    drawcursor();
    disp_ph_sta();
    hide_pre_sel();
    ph_sta=-1;

    if (gcin_pop_up_win)
      show_win0();

    return TRUE;
}

#if 1
static void set_phrase_link(int idx, int len)
{
    int j;

    if (len < 1)
      return;

    for(j=1;j < len; j++) {
      chpho[idx+j].psta=idx;
    }

    chpho[idx].flag |= FLAG_CHPHO_PHRASE_HEAD;
}
#endif


// should be used only if it is a real phrase
gboolean add_to_tsin_buf_phsta(char *str, phokey_t *pho, int len)
{
    int idx = ph_sta < 0 && c_idx==c_len ? ph_sta_last : ph_sta;
#if 0
    dbg("idx:%d  ph_sta:%d ph_sta_last:%d c_idx:%d  c_len:%d\n",
       idx, ph_sta, ph_sta_last, c_idx, c_len);
#endif
    if (idx < 0)
      return 0;

    if (idx + len >= MAX_PH_BF_EXT)
      flush_tsin_buffer();

    if (c_idx < c_len) {
      int avlen = c_idx - ph_sta;

      dbg("avlen:%d %d\n", avlen, len);
      if (avlen < len) {
        int d = len - avlen;

        memmove(&chpho[c_idx + d], &chpho[c_idx], sizeof(CHPHO) * (c_len - c_idx));
        c_len += d;
      }
    } else
      c_len = idx + len;

    ch_pho_cpy(&chpho[idx], str, pho, len);
    set_chpho_ch2(&chpho[idx], str, len);
    set_fixed(idx, len);

    c_idx=idx + len;

    clrin_pho_tsin();
    disp_in_area_pho_tsin();

    prbuf();
#if 1
    set_phrase_link(idx, len);
#endif
    drawcursor();
    disp_ph_sta();
    hide_pre_sel();
    ph_sta=-1;
    return 1;
}


void add_to_tsin_buf_str(char *str)
{
  char *pp = str;
  char *endp = pp+strlen(pp);
  int N = 0;


  while (*pp) {
    int u8sz = utf8_sz(pp);
    N++;
    pp += u8sz;

    if (pp >= endp) // bad utf8 string
      break;
  }


  phokey_t pho[MAX_PHRASE_LEN];
  bzero(pho, sizeof(pho));
  add_to_tsin_buf(str, pho, N);
}


static gboolean pre_sel_handler(KeySym xkey)
{
  static char shift_sele[]="!@#$%^&*()asdfghjkl:";
  static char noshi_sele[]="1234567890asdfghjkl;";
  char *p;

  if (!pre_selN || !tsin_phrase_pre_select)
    return 0;

  if (isupper(xkey))
    xkey = xkey - 'A' + 'a';

//  dbg("pre_sel_handler aa\n");

  if (!(p=strchr(shift_sele, xkey)))
    return 0;

  int c = p - shift_sele;
  char noshi = noshi_sele[c];

  if (!(p=strchr(phkbm.selkey, noshi)))
    return 0;

  c = p - phkbm.selkey;

  int len = pre_sel[c].len;

  if (c >= pre_selN)
    return 0;

#if 0
    dbg("eqlenN:%d %d\n", eqlenN, current_ph_idx);
#endif

  full_match = FALSE;
  gboolean b_added = add_to_tsin_buf_phsta(pre_sel[c].str, pre_sel[c].phokey, len);

  return b_added;
}



static gboolean pre_punctuation(KeySym xkey)
{
  char *p;
  static char shift_punc[]="<>?:\"{}!";
  static char *chars[]={"，","。","？","：","；","『","』","！"};

  if ((p=strchr(shift_punc, xkey))) {
    int c = p - shift_punc;
    phokey_t key=0;

    return add_to_tsin_buf(chars[c], &key, 1);
  }

  return 0;
}


static char hsu_punc[]=",./;";
static gboolean pre_punctuation_hsu(KeySym xkey)
{
  static char *chars[]={"，","。","？","；"};
  char *p;

  if ((p=strchr(hsu_punc, xkey))) {
    int c = p - hsu_punc;
    phokey_t key=0;

    return add_to_tsin_buf(chars[c], &key, 1);
  }

  return 0;
}


gboolean inph_typ_pho(char newkey);


gint64 current_time();

static void call_tsin_parse()
{
  TSIN_PARSE parse[MAX_PH_BF_EXT+1];
  bzero(parse, sizeof(parse));
  tsin_parse(parse);
  prbuf();
}

static KeySym keypad_proc(KeySym xkey)
{
  if (xkey <= XK_KP_9 && xkey >= XK_KP_0)
    xkey=xkey-XK_KP_0+'0';
  else {
    switch (xkey) {
      case XK_KP_Add:
        xkey = '+';
        break;
      case XK_KP_Subtract:
        xkey = '-';
        break;
      case XK_KP_Multiply:
        xkey = '*';
        break;
      case XK_KP_Divide:
        xkey = '/';
        break;
      case XK_KP_Decimal:
        xkey = '.';
        break;
      default:
        return 0;
    }
  }

  return xkey;
}

static int cursor_left()
{
  close_selection_win();
  if (c_idx) {
    clrcursor();
    c_idx--;
    drawcursor();
    return 1;
  }
  // Thanks to PCMan.bbs@bbs.sayya.org for the suggestion
  return c_len;
}
static int cursor_right()
{
  close_selection_win();
  if (c_idx < c_len) {
    clrcursor();
    c_idx++;
    drawcursor();
    return 1;
  }

  // Thanks to PCMan.bbs@bbs.sayya.org for the suggestion
  return c_len;
}

static int cursor_delete()
{
  if (c_idx == c_len)
    return 0;
  close_selection_win();
  clear_disp_ph_sta();
  ityp3_pho=0;
  pre_selN = 0;

  int j;
  for(j=3;j>=0;j--)
    if (typ_pho[j]) {
      typ_pho[j]=0;
      disp_in_area_pho_tsin();
      return 1;
    }

  clrcursor();
  int k;
  int pst=k=chpho[c_idx].psta;

  for(k=c_idx;k<c_len;k++) {
    chpho[k] = chpho[k+1];
    chpho[k].psta=chpho[k+1].psta-1;
  }

  c_len--;
  hide_char(c_len);
  init_chpho_i(c_len);

  prbuf();

  compact_win0_x();

  if (!c_idx)
    clear_match();
  else {
    k=c_idx-1;
    pst=chpho[k].psta;

    while (k>0 && chpho[k].psta==pst)
      k--;

    if (chpho[k].psta!=pst)
      k++;

    int match_len= c_idx - k;
    if (!(match_len=scanphr(k, match_len, FALSE)))
      ph_sta=-1;
    else
      ph_sta=k;

    if (ph_sta < 0 || c_idx - ph_sta < 2)
      pre_selN = 0;
  }

  if (!c_len && gcin_pop_up_win)
    hide_win0();

  disp_ph_sta();
  return 1;
}

void case_inverse();
void pho_play(phokey_t key);

int feedkey_pp(KeySym xkey, int kbstate)
{
  char ctyp=0;
  static u_int ii;
  static u_short key;
  int i,k,pst;
  u_char match_len;
  int shift_m=kbstate&ShiftMask;
  int j,jj,kk, idx;
  char kno;

  key_press_time = 0;

   if (kbstate & (Mod1Mask|Mod1Mask)) {
     return 0;
   }

   int o_sel_pho = sel_pho;
   close_win_pho_near();

   switch (xkey) {
     case XK_Escape:
       tsin_reset_in_pho0();
       if (typ_pho_empty()) {
         if (!c_len)
           return 0;
         if (!o_sel_pho && tsin_tab_phrase_end)
           goto tab_phrase_end;
       }
       tsin_reset_in_pho();
       return 1;
     case XK_Return:
     case XK_KP_Enter:
        if (shift_m) {
          if (!c_len)
            return 0;
          save_frm=c_idx;
          save_to=c_len-1;
          draw_ul(c_idx, c_len);
          save_phrase();
          return 1;
        } else {
          if (c_len)
            flush_tsin_buffer();
          else
          if (typ_pho_empty())
            return 0;
          return 1;
        }
     case XK_Home:
        close_selection_win();
        if (!c_len)
          return 0;
        clrcursor();
        c_idx=0;
        drawcursor();
        return 1;
     case XK_End:
        close_selection_win();
        if (!c_len)
          return 0;
        clrcursor();
        c_idx=c_len;
        drawcursor();
        return 1;
     case XK_Left:
        return cursor_left();
     case XK_Right:
        return cursor_right();
     case XK_Caps_Lock:
        if (tsin_chinese_english_toggle_key == TSIN_CHINESE_ENGLISH_TOGGLE_KEY_CapsLock) {
          close_selection_win();
          tsin_toggle_eng_ch();
          return 1;
        } else
          return 0;
     case XK_Tab:
        close_selection_win();
        if (tsin_chinese_english_toggle_key == TSIN_CHINESE_ENGLISH_TOGGLE_KEY_Tab) {
          tsin_toggle_eng_ch();
          return 1;
        }

        if (tsin_tab_phrase_end && c_len > 1) {
tab_phrase_end:
          if (c_idx==c_len)
            chpho[c_idx-1].flag |= FLAG_CHPHO_PHRASE_USER_HEAD;
          else
            chpho[c_idx].flag |= FLAG_CHPHO_PHRASE_USER_HEAD;
           call_tsin_parse();
          return 1;
        } else {
          if (c_len) {
            flush_tsin_buffer();
            return 1;
          }
        }
        return 0;
     case XK_Shift_L:
     case XK_Shift_R:
        key_press_time = current_time();
        return 1;
     case XK_Delete:
        return cursor_delete();
     case XK_BackSpace:
        close_selection_win();
        clear_disp_ph_sta();
        ityp3_pho=0;
        pre_selN = 0;

        for(j=3;j>=0;j--)
          if (typ_pho[j]) {
            typ_pho[j]=0;
            inph[j]=0;
            disp_in_area_pho_tsin();

            if (pre_selN > 1 && scanphr(ph_sta, c_idx - ph_sta, TRUE)) {
              disp_pre_sel_page();
            }

            if (!c_len && gcin_pop_up_win && !j)
              hide_win0();
            return 1;
          }

        if (!c_idx)
          return 0;

        clrcursor();
        c_idx--;
        pst=k=chpho[c_idx].psta;

        for(k=c_idx;k<c_len;k++) {
          chpho[k]=chpho[k+1];
          chpho[k].psta=chpho[k+1].psta-1;
        }

        c_len--;
        hide_char(c_len);
        init_chpho_i(c_len);
        prbuf();
        compact_win0_x();

        if (!c_idx) {
          clear_match();
        } else {
          k=c_idx-1;
          pst=chpho[k].psta;

          while (k>0 && chpho[k].psta==pst)
            k--;

          if (chpho[k].psta!=pst)
            k++;

          match_len= c_idx - k;
          if (!(match_len=scanphr(k, match_len, FALSE)))
            ph_sta=-1;
          else
            ph_sta=k;

//          if (ph_sta < 0 || c_idx - ph_sta < 2)
            pre_selN = 0;
        }

        disp_ph_sta();

        if (!c_len && gcin_pop_up_win)
          hide_win0();

        return 1;
     case XK_Up:
       if (!sel_pho) {
         if (c_len && c_idx == c_len) {
           int idx = c_len-1;
           phokey_t pk = chpho[idx].pho;

           if (pk) {
             void create_win_pho_near(phokey_t pho);
             create_win_pho_near(pk);
           }

           return 1;
         }
         return 0;
       }

       current_page = current_page - phkbm.selkeyN;
       if (current_page < 0)
         current_page = 0;

       disp_current_sel_page();
       return 1;
     case XK_space:
       if (!c_len && !ityp3_pho && !typ_pho[0] && !typ_pho[1] && !typ_pho[2]
           && tsin_half_full) {
         send_text("　");
         return 1;
       }

       if (tsin_space_opt == TSIN_SPACE_OPT_INPUT && !typ_pho[0] && !typ_pho[1] && !typ_pho[2] && !ityp3_pho && !sel_pho) {
         close_selection_win();
         goto asc_char;
       }
     case XK_Down:
       if (!eng_ph && xkey == XK_space)
           goto asc_char;

       if (!ityp3_pho && (typ_pho[0]||typ_pho[1]||typ_pho[2]) && xkey==XK_space) {
         ctyp=3;
         kno=0;

         inph_typ_pho(xkey);
         goto llll1;
       }

change_char:
       if (!c_len)
         return 0;

       idx = c_idx==c_len ? c_idx - 1 : c_idx;
       if (!chpho[idx].pho)
         return 1;

       if (!sel_pho) {
         get_sel_phrase();
         get_sel_pho();
         sel_pho=1;
         current_page = 0;
       } else {
         current_page = current_page + phkbm.selkeyN;
         if (current_page >= phrase_count + pho_count)
           current_page = 0;
       }

       disp_current_sel_page();
       return 1;
     case '\'':  // single quote
       if (phkbm.phokbm[xkey][0].num)
         goto other_keys;
       else {
         phokey_t key = 0;
         return add_to_tsin_buf("、", &key, 1);
       }
     case XK_q:
     case XK_Q:
       if (b_hsu_kbm && eng_ph)
         goto change_char;
     default:
other_keys:
       if ((kbstate & ControlMask)) {
         if (xkey==XK_u) {
           clear_tsin_buffer();
           if (gcin_pop_up_win)
             hide_win0();
           return 1;
         } else if (xkey == XK_e) {
           //trigger
           bufferEditing ^= 1;
           return 1;
         } else if (xkey>=XK_1 && xkey<=XK_3) {
           return 1;
         } else
           return 0;
       }

       char xkey_lcase = xkey;
       if ('A' <= xkey && xkey <= 'Z')
          xkey_lcase = tolower(xkey);

       if (bufferEditing) {
         if (xkey_lcase=='h')
           return cursor_left();
         else
         if (xkey_lcase=='l')
           return cursor_right();
         else
         if (xkey_lcase=='x')
           return cursor_delete();
       }

       if (tsin_space_opt == TSIN_SPACE_OPT_INPUT)
         xkey_lcase = keypad_proc(xkey);

       char *pp;
       if ((pp=strchr(phkbm.selkey,xkey_lcase)) && sel_pho) {
         int c=pp-phkbm.selkey;
         char *sel_text;
         int len = fetch_user_selection(c, &sel_text);

         if (len > 1) {
           int cpsta = chpho[c_idx].psta;
           if (cpsta >= 0) {
#if 0
             chpho[cpsta].flag |= FLAG_CHPHO_PHRASE_VOID;
#endif
             set_chpho_ch(&chpho[c_idx], sel_text, len);
           } else
             set_chpho_ch2(&chpho[c_idx], sel_text, len);

           chpho[c_idx].flag &= ~FLAG_CHPHO_PHRASE_VOID;
#if 0
           set_phrase_link(c_idx, len);
#endif
           set_fixed(c_idx, len);

           call_tsin_parse();

           if (c_idx + len == c_len) {
             ph_sta = -1;
             draw_ul(i, c_len);
           }
         } else
         if (len == 1) { // single chinese char
           i= c_idx==c_len?c_idx-1:c_idx;
           key=chpho[i].pho;
           set_chpho_ch(&chpho[i], sel_text, 1);

           if (i && chpho[i].psta == i-1 && !(chpho[i-1].flag & FLAG_CHPHO_FIXED)) {
             set_chpho_ch(&chpho[i-1], chpho[i-1].ph1ch, 1);
#if 0
             chpho[i-1].flag |= FLAG_CHPHO_PHRASE_VOID;
#endif
           }

           set_fixed(i, 1);
           call_tsin_parse();
         }

         if (len) {
           prbuf();
           current_page=sel_pho=ityp3_pho=0;
           if (len == 1) {
             hide_selections_win();
             ph_sta = -1;
             goto restart;
           }
           else
             ph_sta=-1;

           hide_selections_win();
         }
         return 1;
       }

       sel_pho=current_page=0;
   }

   KeySym key_pad = keypad_proc(xkey);
   if (!eng_ph || shift_m || key_pad) {
       if (key_pad)
         xkey = key_pad;
asc_char:
        if (shift_m) {
          if (pre_sel_handler(xkey)) {
            call_tsin_parse();
            return 1;
          }

          if (eng_ph && pre_punctuation(xkey))
            return 1;
        }


//        dbg("xkey: %c\n", xkey);

        if (shift_m && eng_ph)  {
          char *ppp=strchr(ochars,xkey);

          if (!(kbstate&LockMask) && ppp && !((ppp-ochars) & 1))
            xkey=*(ppp+1);
          if (kbstate&LockMask && islower(xkey))
            xkey-=0x20;
          else
            if (!(kbstate&LockMask) && isupper(xkey))
              xkey+=0x20;
        } else {
          if (!eng_ph && tsin_chinese_english_toggle_key == TSIN_CHINESE_ENGLISH_TOGGLE_KEY_CapsLock
              && gcin_capslock_lower) {
            case_inverse(&xkey, shift_m);
          }
        }

        if (xkey > 127)
          return 0;

        u_char tt=xkey;
        shift_ins();

        if (tsin_half_full) {
          bchcpy(chpho[c_idx].ch, half_char_to_full_char(xkey));
        } else {
//          dbg("%c\n", tt);
          chpho[c_idx].ch[0]=tt;
        }

        set_fixed(c_idx, 1);
        phokey_t tphokeys[32];
        tphokeys[0]=0;
        utf8_pho_keys(chpho[c_idx].ch, tphokeys);

        disp_char_chbuf(c_idx);
        chpho[c_idx].pho=tphokeys[0];
        c_idx++;
        if (c_idx < c_len)
          prbuf();

        if (gcin_pop_up_win)
          show_win0();

        drawcursor();
        return 1;
   } else { /* pho */
     if (xkey > 127) {
       return 0;
     }

     // for hsu & et26
     if (strchr(hsu_punc, xkey) && !phkbm.phokbm[xkey][0].num)
       return pre_punctuation_hsu(xkey);

     if (xkey >= 'A' && xkey <='Z')
       xkey+=0x20;

     inph_typ_pho(xkey);

     if (gcin_pop_up_win)
         show_win0();

     if (typ_pho[3])
       ctyp = 3;

llll1:
     jj=0;
     kk=1;
llll2:
     if (ctyp==3) {
       ityp3_pho=1;  /* last key is entered */

       if (!tsin_tone_char_input && !typ_pho[0] && !typ_pho[1] && !typ_pho[2]) {
         clrin_pho_tsin();
         return TRUE;
       }
     }

     disp_in_area_pho_tsin();

     if (pre_selN)
       scanphr(ph_sta, c_idx - ph_sta, TRUE);

     disp_pre_sel_page();

     key = pho2key(typ_pho);

     pho_play(key);

     int vv=hash_pho[typ_pho[0]];
     phokey_t ttt=0xffff;
     while (vv<idxnum_pho) {
       ttt=idx_pho[vv].key;
       if (!typ_pho[0]) ttt &= ~(31<<9);
       if (!typ_pho[1]) ttt &= ~(3<<7);
       if (!typ_pho[2]) ttt &= ~(15<<3);
       if (!typ_pho[3]) ttt &= ~(7);
       if (ttt>=key) break;
       else
       vv++;
     }

     if (ttt > key || (ityp3_pho && idx_pho[vv].key!=key) ) {
       while (jj<4) {
         while(kk<3)
         if (phkbm.phokbm[(int)inph[jj]][kk].num ) {
           if (kk) {
             ctyp=phkbm.phokbm[(int)inph[jj]][kk-1].typ;
             typ_pho[(int)ctyp]=0;
           }
           kno=phkbm.phokbm[(int)inph[jj]][kk].num;
           ctyp=phkbm.phokbm[(int)inph[jj]][kk].typ;
           typ_pho[(int)ctyp]=kno;
           kk++;
           goto llll2;
         } else kk++;
         jj++;
         kk=1;
       }

       bell(); ityp3_pho=typ_pho[3]=0;
       disp_in_area_pho_tsin();
       return 1;
     }

     if (key==0 || !ityp3_pho)
       return 1;

     ii=idx_pho[vv].start;
     start_idx=ii;
   } /* pho */

   put_b5_char(ch_pho[start_idx].ch, key);

   call_tsin_parse();

   disp_ph_sta();
   clrin_pho_tsin();
   clr_in_area_pho_tsin();
   drawcursor();
   hide_pre_sel();


   if (ph_sta < 0) {
restart:
     if ((match_len=scanphr(c_idx-1,1, FALSE)))
       ph_sta=c_idx-1;

//     dbg("scanphr c_idx:%d match_len:%d\n", c_idx, match_len);
//     chpho[c_idx-1].psta = c_idx-1;

#define MAX_SINGLE_SEL 9
     disp_ph_sta();

     if (pre_selN > MAX_SINGLE_SEL)
       pre_selN=0;
     else {
       disp_pre_sel_page();
     }

     return 1;
   } else {
     int mdist = c_idx - ph_sta;
     int max_match_phrase_len;

//     dbg("match_len:%d mdist %d = c_idx:%d - ph_sta:%d\n", match_len,  mdist, c_idx, ph_sta);

     while (ph_sta < c_idx) {
//       dbg("ph_sta:%d\n", ph_sta);
       if ((max_match_phrase_len = scanphr(ph_sta, c_idx - ph_sta, FALSE))) {
//         dbg("max_match_phrase_len: %d\n", max_match_phrase_len);
         break;
       } else
       if (full_match) {  // tstr: 選擇視窗
//         dbg("last full_match\n");
         full_match = FALSE;
         ph_sta = -1;
         goto restart;
       }

       ph_sta++;
     }

     mdist = c_idx - ph_sta;

     if (ph_sta==c_idx) {
//       dbg("uuu no match ....\n");
       clear_match();
       return 1;
     }

     if (!pre_selN) {
//       dbg("no match found ..\n");
       clear_match();
       goto restart;
     }

//     dbg("iiiiiiii ph_sta:%d %d  max_match_phrase_len:%d\n", ph_sta, c_idx, max_match_phrase_len);
     disp_ph_sta();

     if (mdist == 1) {
       chpho[c_idx-1].psta = c_idx-1;

       if (pre_selN > MAX_SINGLE_SEL)
         pre_selN=0;
       else
         disp_pre_sel_page();

       return 1;
     }

     disp_pre_sel_page();

     full_match = FALSE;

     for(i=0; i < pre_selN; i++) {
       if (pre_sel[i].len != mdist)
         continue;
       int ofs=0;

       for(j=0; j < mdist; j++) {
          int clensel = utf8_sz(&pre_sel[i].str[ofs]);
          int clen = utf8_sz(chpho[ph_sta+j].ch);

          if (clensel != clen)
            continue;

          if ((chpho[ph_sta+j].flag & FLAG_CHPHO_FIXED) &&
             memcmp(chpho[ph_sta+j].ch, &pre_sel[i].str[ofs], clen))
             break;
          ofs+=clen;
       }

       if (j < mdist)
         continue;

       ch_pho_cpy(&chpho[ph_sta], pre_sel[i].str, pre_sel[i].phokey, mdist);
       if (chpho[ph_sta].psta < 0)
         set_chpho_ch2(&chpho[ph_sta], pre_sel[i].str, mdist);


       int j;
       for(j=0;j < mdist; j++) {
         if (j)
           chpho[ph_sta+j].psta = ph_sta;
         disp_char(ph_sta+j, chpho[ph_sta+j].ch);
       }

       full_match = TRUE;
#if 0
       chpho[ph_sta].flag |= FLAG_CHPHO_PHRASE_HEAD;
#endif
       if (mdist==max_match_phrase_len) { // tstr: 選擇視窗
//         dbg("full match .......... %d\n", ph_sta);
         ph_sta_last = ph_sta;
         ph_sta = -1;
         if (pre_selN == 1)
           pre_selN = 0;
       }

       return 1;
     }
   }

   return 1;
}


int feedkey_pp_release(KeySym xkey, int kbstate)
{
  switch (xkey) {
     case XK_Shift_L:
     case XK_Shift_R:
        if (
(  (tsin_chinese_english_toggle_key == TSIN_CHINESE_ENGLISH_TOGGLE_KEY_Shift) ||
   (tsin_chinese_english_toggle_key == TSIN_CHINESE_ENGLISH_TOGGLE_KEY_ShiftL
     && xkey == XK_Shift_L) ||
   (tsin_chinese_english_toggle_key == TSIN_CHINESE_ENGLISH_TOGGLE_KEY_ShiftR
     && xkey == XK_Shift_R))
          &&  current_time() - key_press_time < 300000) {
          key_press_time = 0;
          close_selection_win();
          tsin_toggle_eng_ch();
          return 1;
        } else
          return 0;
     default:
        return 0;
  }
}


void tsin_remove_last()
{
  if (!c_len)
    return;
  c_len--;
  c_idx--;
}


gboolean save_phrase_to_db2(CHPHO *chph, int len)
{
   phokey_t pho[MAX_PHRASE_LEN];
   char ch[MAX_PHRASE_LEN * CH_SZ];

   chpho_extract(chph, len, pho, ch, NULL);

   return save_phrase_to_db(pho, ch, len, 1);
}
