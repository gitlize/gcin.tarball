/*
	Copyright (C) 1995-2008	Edward Der-Hua Liu, Hsin-Chu, Taiwan
*/

#include "gcin.h"
#include "pho.h"
#include "tsin.h"
#include "gtab.h"
#include "gst.h"

static int qcmp_pre_sel_usecount(const void *aa, const void *bb)
{
  PRE_SEL *a = (PRE_SEL *) aa;
  PRE_SEL *b = (PRE_SEL *) bb;

  int d = b->usecount - a->usecount;

  if (d != 0)
    return d;

  return a->len - b->len; // shorter first
}

static int qcmp_pre_sel_str(const void *aa, const void *bb)
{
  PRE_SEL *a = (PRE_SEL *) aa;
  PRE_SEL *b = (PRE_SEL *) bb;

  return strcmp(a->str, b->str);
}

void extract_gtab_key(gboolean is_en, int start, int len, void *out);
gboolean tsin_seek_ex(TSIN_HANDLE *th, void *pho, int plen, int *r_sti, int *r_edi, char *tone_mask);
void load_tsin_entry_ex(TSIN_HANDLE *ptsin_hand, int idx, char *len, usecount_t *usecount, void *pho, u_char *ch);
gboolean check_gtab_fixed_mismatch(int idx, char *mtch, int plen);
void mask_tone(phokey_t *pho, int plen, char *tone_off);
void init_pre_sel();
void mask_key_typ_pho(phokey_t *key);
extern u_int64_t vmaskci;

int ph_key_length(TSIN_HANDLE *th, u_int64_t k)
{
  int klen=0;
  if (th->ph_key_sz==2) {
    int k1,k2,k3,k4;
    phokey_t kk = (phokey_t) k;
    k4=(kk&7);
    kk>>=3;
    k3=(kk&15);
    kk>>=4;
    k2=(kk&3);
    kk>>=2;
    k1=(kk&31);
    if (k1)
      klen++;
    if (k2)
      klen++;
    if (k3)
      klen++;
    if (k4)
      klen++;
  } else {
    klen = 3; // temporary fix
  }

//  dbg("ph_key_length %d\n", klen);
  return klen;
}

extern char typ_pho_len[];

u_char scanphr_e(TSIN_HANDLE *th, gboolean is_gtab, int chpho_idx, int plen, gboolean pho_incr, int *rselN)
{
  gboolean is_pho = th->ph_key_sz==2;
  if (plen >= MAX_PHRASE_LEN)
    goto empty;
  if (chpho_idx < 0)
    goto empty;

  phokey_t tailpho, tailpho0head=0;

  if (pho_incr) {
    if (is_pho) {
      tailpho = pho2key(poo.typ_pho);
      if (!tailpho)
        pho_incr = FALSE;

	  // If user input ㄣ, then ㄅㄧㄣ should not be selected
	  int i;
	  for(i=0;i<4;i++)
        if (poo.typ_pho[i])
          break;
	  if (i>0 && i < 4) {
		tailpho0head |= (1 << typ_pho_len[0]) - 1;
	    int j;
	    for(j=1;j<4;j++) {
		  tailpho0head <<= typ_pho_len[j];
		  if (j < i)
		    tailpho0head |= ((1 << typ_pho_len[j]) - 1);
	    }
	  }

	  dbg("i:%d tailpho0head %x\n",i, tailpho0head);
    } else {
      if (!ggg.kval)
        pho_incr = FALSE;
    }
  }

  u_int64_t pp64[MAX_PHRASE_LEN + 1];
  phokey_t *pp = (phokey_t*)pp64;
  char *pp8 = (char *)pp64;
  int *pp32 = (int *)pp64;

  if (!is_gtab) {
	extract_pho(FALSE, chpho_idx, plen, pp);
  } else {
    extract_gtab_key(FALSE, chpho_idx, plen, pp64);
  }


#if 0
  dbg("scanphr %d\n", plen);

  int t;
  for(t=0; t < plen; t++)
    prph(pp[t]);
  puts("");
#endif

  char pinyin_set[MAX_PH_BF_EXT];
  char *t_pinyin_set = NULL;
  gboolean is_pin_juyin = is_pho && pin_juyin;
#define selNMax 50
  PRE_SEL sel[selNMax];
  int selN = 0;
  tss.pre_selN = 0;
  int maxlen=0;

  int used_first;
  for(used_first=1; used_first>=0; used_first--) {

  if (is_pin_juyin) {
    get_chpho_pinyin_set(pinyin_set);
    t_pinyin_set = pinyin_set + chpho_idx;
    mask_tone(pp, plen, t_pinyin_set);
  }

  int sti, edi;
  if (!tsin_seek_ex(th, pp, plen, &sti, &edi, t_pinyin_set)) {
empty:
    if (rselN)
      *rselN = 0;
    return 0;
  }


  // dbg("plen:%d sti:%d edi:%d\n", plen, sti, edi);

  u_int64_t mtk64[MAX_PHRASE_LEN+1];
  phokey_t *mtk = (phokey_t*) mtk64;
  u_int *mtk32 = (u_int *)mtk64;
  u_char *mtk8 = (u_char *)mtk64;

  while (sti < edi && selN < selNMax) {
    u_char mtch[MAX_PHRASE_LEN*CH_SZ+1];
    char match_len;
    usecount_t usecount;

	mtk[plen] = 0;
    load_tsin_entry_ex(th, sti, &match_len, &usecount, mtk, mtch);

    sti++;
    if (plen > match_len || (pho_incr && plen==match_len)) {
      continue;
    }

	if (used_first && !usecount)
      continue;

    if (is_pho)
      mask_tone(mtk, plen, t_pinyin_set);

    int i;

    if (is_pho) {
      for(i=0; i < plen; i++) {
        if (mtk[i]!=pp[i])
          break;
      }
    } else {
	  if (th->ph_key_sz==4) {
        for(i=0; i < plen; i++) {
          if (mtk32[i]!=pp32[i])
            break;
        }
	  } else {
        for(i=0; i < plen; i++) {
          if (mtk64[i]!=pp64[i])
            break;
        }
      }
    }

    if (i < plen) {
      continue;
    }

    if (pho_incr) {
	  // en doesn't need this
      if (is_pho) {
        phokey_t last_m = mtk[plen];
#if 1
		if (last_m & tailpho0head) {
//		  dbg("%x ", last_m); prph(last_m); dbg("\n");
          continue;
		}
#endif
        mask_key_typ_pho(&last_m);
        if (last_m != tailpho)
          continue;
      } else {
        u_int64_t v = th->ph_key_sz==4?mtk32[plen]:mtk64[plen];
        if (ggg.kval != (v&vmaskci))
          continue;
      }
    }


#if 0
    dbg("nnn ");
    nputs(mtch, match_len);
    dbg("\n");
#endif

    if (!is_gtab) {
      if (check_fixed_mismatch(chpho_idx, (char *)mtch, plen))
        continue;
    } else {
      if (check_gtab_fixed_mismatch(chpho_idx, (char *)mtch, plen))
        continue;
    }

    if (maxlen < match_len)
      maxlen = match_len;

    sel[selN].len = match_len;
//    sel[selN].phidx = sti - 1;
    sel[selN].usecount = usecount;
    utf8cpyN(sel[selN].str, (char *)mtch, match_len);

    bzero(sel[selN].phkey, sizeof(sel[selN].phkey));
    memcpy(sel[selN].phkey, mtk, match_len*th->ph_key_sz);
    selN++;
  }
  }

  dbg("SelN:%d  maxlen:%d\n", selN, maxlen);

  if (selN > 1) {
    qsort(sel, selN, sizeof(PRE_SEL), qcmp_pre_sel_str);
    int nselN = 0;
    int i;
    for(i=0;i<selN;i++)
      if (sel[i].len>1 && (!i || strcmp(sel[i].str, sel[i-1].str)))
        sel[nselN++]=sel[i];
    selN = nselN;
  }

#if 1
  if (selN==1) {
//    if (sel[0].len==1 || (sel[0].len==2 && ph_key_length(th, sel[0].phkey[1])<2))
    if (sel[0].len==1)
      goto empty;
  }
#endif

  qsort(sel, selN, sizeof(PRE_SEL), qcmp_pre_sel_usecount);

  dbg("selN:%d\n", selN);
  if (!is_gtab)
    tss.pre_selN = Min(selN, phkbm.selkeyN);
  else
    tss.pre_selN = Min(selN, (int)strlen(cur_inmd->selkey));

  dbg("tss.pre_selN %d\n", tss.pre_selN);
  memcpy(tss.pre_sel, sel, sizeof(PRE_SEL) * tss.pre_selN);

  if (rselN)
    *rselN = selN;

  return maxlen;
}



void hide_pre_sel();
void chpho_get_str(int idx, int len, char *ch);
void disp_pre_sel_page();
gboolean tsin_seek_ex(TSIN_HANDLE *th, void *pho, int plen, int *r_sti, int *r_edi, char *tone_mask);
int tsin_sele_by_idx(int c);

void tsin_scan_pre_select(gboolean b_incr)
{
  if (!tsin_phrase_pre_select)
    return;
//  dbg("gtab_scan_pre_select %d\n", tss.c_len);

  tss.pre_selN = 0;

  hide_pre_sel();

  if (!tss.c_idx || !tss.c_len)
    return;

  init_pre_sel();

  int Maxlen = tss.c_len;
  if (Maxlen > MAX_PHRASE_LEN)
    Maxlen = MAX_PHRASE_LEN;

  int len, selN, max_len=-1, max_selN=-1;
  for(len=1; len <= Maxlen; len++) {
    int idx = tss.c_len - len;
    if (tss.chpho[idx].flag & FLAG_CHPHO_PHRASE_TAIL) {
//      dbg("phrase tail %d\n", idx);
      break;
    }
    int mlen = scanphr_e(&tsin_hand, FALSE, tss.c_len - len, len, b_incr, &selN);
//	dbg("mlen %d len:%d\n", mlen, len);

    if (mlen) {
      max_len = len;
      max_selN = selN;
    }
  }

//  dbg("max_len:%d  max_selN:%d\n", max_len, max_selN);

  if (max_len < 0 /* || max_selN >= strlen(pho_selkey) * 2 */ ) {
    tss.pre_selN=0;
    return;
  }

  scanphr_e(&tsin_hand, FALSE, tss.c_len - max_len, max_len, b_incr, &selN);

//  dbg("selN:%d %d\n", selN, tss.pre_selN);

//  dbg("selN %d %d\n",selN, tss.pre_selN);
  tss.ph_sta = tss.c_len - max_len;
  
  if (selN==1 && tss.pre_sel[0].len==max_len) {
    char out[MAX_PHRASE_LEN * CH_SZ + 1];
    chpho_get_str(tss.c_len - max_len, max_len, out);
    if (!strcmp(out, tss.pre_sel[0].str)) {
//	  tsin_sele_by_idx(0);
      return;
    }
  }
    
  disp_pre_sel_page();
}

char *ch_mode_selkey(gboolean is_gtab);

char *en_sel_keys(gboolean is_gtab)
{
  char *s = ch_mode_selkey(is_gtab);
  static char *sel = "1234567890";

  if (!s)
    return sel;
  int slen = strlen(s);
  int i;
  for(i=0; i < slen; i++)
    if (s[i] < '0' || s[i] > '9')
      break;
  if (i < slen)
    return sel;

  return s;
}

int en_sel_keys_len(gboolean is_gtab)
{
  return strlen(en_sel_keys(is_gtab));
}

extern int wselkeyN;
phokey_t utf8_pho_key(char *s);
gboolean tsin_seek_en(u_char *pho, int plen, int *r_sti, int *r_edi);

u_char scanphr_en(TSIN_HANDLE *th, gboolean is_gtab, int chpho_idx, int plen,  int *rselN)
{
  if (rselN)
    *rselN = 0;

  if (plen >= MAX_PHRASE_LEN)
    return 0;
  if (chpho_idx < 0)
    return 0;

  u_int64_t pp64[MAX_PHRASE_LEN + 1];
  phokey_t *pp = (phokey_t*)pp64;
  char *pp8 = (char *)pp64;

  tss.pre_selN = 0;
  int maxlen=0;
//#define selNMax 20
  PRE_SEL sel[selNMax];
  int selN = 0;

  if (!is_gtab) {
	extract_pho(TRUE, chpho_idx, plen, pp);
  } else {
    extract_gtab_key(TRUE, chpho_idx, plen, pp64);
  }

  gboolean isup = isupper(pp8[0]);
  gboolean isup1 = isup && plen > 1 && isupper(pp8[1]);
  int loopN = isup?4:5;

  int loop;
  int used_first;
  char pp8_o[MAX_PHRASE_LEN * 8];
  memcpy(pp8_o, pp8, plen);
  pp8[plen]=0;
  dbg("pp8 '%s'\n", pp8);

  for(used_first=1;used_first>=0; used_first--) {
  for(loop=0; loop<loopN;loop++) {
  memcpy(pp8, pp8_o, plen);
  if (loop==1) {
	if (isup1) {
	  int i;
	  for(i=0;i<plen;i++)
	    pp8[i] = tolower(pp8[i]);
	} else
	if (isup){
	  pp8[0] = tolower(pp8[0]);
	} else
	  pp8[0] = toupper(pp8[0]);
  } else if (loop==2) {
	  // loop==2 SHOUT
	  int i;
	  for(i=0;i<plen;i++)
	    pp8[i] = toupper(pp8[i]);
  } else if (loop==3) {  // iPhone, iPad
    pp8[1]=toupper(pp8[1]);
  } else if (loop==4) { // loop==4 PlayStation
	int i;
	for(i=0;i<plen;i++)
	  pp8[i] = tolower(pp8[i]);
  }

  dbg("loop:%d %s\n", loop, pp8);

  int sti, edi;
  if (!tsin_seek_en((u_char *)pp8, plen, &sti, &edi)) {
	  continue;
  }

  dbg("plen:%d sti:%d edi:%d\n", loop, plen, sti, edi);

  u_int64_t mtk64[MAX_PHRASE_LEN+1];
  u_char *mtk8 = (u_char *)mtk64;

  while (sti < edi && selN < selNMax) {
    u_char mtch[MAX_PHRASE_LEN*CH_SZ+1];
    char match_len;
    usecount_t usecount;

    load_tsin_entry_ex(th, sti, &match_len, &usecount, (phokey_t*)mtk64, mtch);
	if (!mtch[0]) {
		memcpy(mtch, mtk8, match_len);
		mtch[match_len] = 0;
	}

#if DEBUG && 0
	   dbg("load %d", sti);
	   utf8_putcharn((char*)mtk8, match_len);
	   dbg("\n");
#endif

    sti++;
    if (plen > match_len) {
      continue;
    }

	if (used_first && !usecount)
      continue;

    int i;
    if (loop == 4) {
      for(i=0; i < plen; i++) {
         if (tolower(mtch[i])!=pp8[i])
	       break;
	  }
   } else {
     for(i=0; i < plen; i++) {
       if (mtk8[i]!=pp8[i])
         break;
     }
   }

    if (i < plen) {
      continue;
    }


#if 0
    dbg("nnn ");
    nputs(mtch, match_len);
    dbg("\n");
#endif

    if (maxlen < match_len)
      maxlen = match_len;

    sel[selN].len = match_len;
//    sel[selN].phidx = sti - 1;
    sel[selN].usecount = usecount;

    if (loop==1) {
	  if (isup1) {
		int i;
		for(i=0;i<match_len;i++)
		  mtch[i]=toupper(mtch[i]);
      } else
        mtch[0]=toupper(mtch[0]);
    }

    memcpy(sel[selN].str, mtch, match_len);
    sel[selN].str[match_len]=0;

	for(i=0;i<selN;i++)
      if (!strcmp(sel[i].str, sel[selN].str))
		break;
	if (i < selN)
	  continue;

    bzero(sel[selN].phkey, sizeof(sel[selN].phkey));

#if 0
	for(i=0;i<match_len;) {
	  char t[CH_SZ + 1];
	  i+=utf8cpy(t, (char *)&mtk8[i]);
//	  sel[selN].phkey[i] = utf8_pho_key(t);
	}
#endif
 //   memcpy(sel[selN].phkey, mtk8, match_len*th->ph_key_sz);
    selN++;
  }

  }
  }

  dbg("SelN:%d  maxlen:%d\n", selN, maxlen);

  qsort(sel, selN, sizeof(PRE_SEL), qcmp_pre_sel_usecount);

  dbg("selN:%d\n", selN);

  tss.pre_selN = Min(selN, wselkeyN);

  dbg("tss.pre_selN %d\n", tss.pre_selN);
  memcpy(tss.pre_sel, sel, sizeof(PRE_SEL) * tss.pre_selN);

  if (rselN)
    *rselN = selN;

  return maxlen;
}

void load_en_db();

gboolean misalpha(char c)
{
  return c>='A' && c<='Z' || c>='a' && c<='z';
}

void tsin_en_scan_pre_select()
{
  dbg("tsin_en_scan_pre_select\n");

  if (!en_hand.fph)
    load_en_db();

  dbg("tsin_en_scan_pre_select %d th->ph_key_sz:%d\n", tss.c_len, en_hand.ph_key_sz);

  tss.pre_selN = 0;

  hide_pre_sel();

  if (!tss.c_idx || !tss.c_len)
    return;

  init_pre_sel();

  int Maxlen = tss.c_len;
  if (Maxlen > MAX_PHRASE_LEN)
    Maxlen = MAX_PHRASE_LEN;

  int len, selN, max_len=-1, max_selN=-1;
  for(len=2; len <= Maxlen; len++) {
    int idx = tss.c_len - len;
    if (tss.chpho[idx].flag & FLAG_CHPHO_PHRASE_TAIL) {
//      dbg("phrase tail %d\n", idx);
      break;
    }

    char c = tss.chpho[idx].cha[0];
    char c1 = idx==0?0:tss.chpho[idx-1].cha[0];
    char c2 = idx<=1?0:tss.chpho[idx-2].cha[0];

    if (c & 0x80)
      break;

	if (misalpha(c1))
	  continue;
	if (misalpha(c) && c1=='\'' && misalpha(c2))
	  continue;

    int mlen = scanphr_en(&en_hand, FALSE, tss.c_len - len, len, &selN);
	dbg("mlen %d len:%d\n", mlen, len);

    if (mlen) {
      max_len = len;
      max_selN = selN;
    }
  }

  dbg("max_len:%d  max_selN:%d\n", max_len, max_selN);

  if (max_len < 0 /* || max_selN > 40 */) { // will ctrl press/release + keys as select keys
    dbg("too many or no %d\n", max_len);
    tss.pre_selN=0;
    return;
  }

  scanphr_en(&en_hand, FALSE, tss.c_len - max_len, max_len, &selN);

  dbg("selN %d %d\n",selN, tss.pre_selN);
  tss.ph_sta = tss.c_len - max_len;
  
  if (selN==1 && tss.pre_sel[0].len==max_len) {
    char out[MAX_PHRASE_LEN * CH_SZ + 1];
    chpho_get_str(tss.c_len - max_len, max_len, out);
        
    dbg("rrr %s %s\n", out, tss.pre_sel[0].str);
    if (!strcmp(out, tss.pre_sel[0].str)) {
	  dbg("ssss\n");
	  tsin_sele_by_idx(0);
      return;
    }
  }  
    
  disp_pre_sel_page();
}
