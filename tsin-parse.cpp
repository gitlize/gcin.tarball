#include <string.h>
#include "gcin.h"
#include "pho.h"
#include "tsin.h"
#include "gcin-conf.h"
#include <math.h>
#include "tsin-parse.h"
#include "gtab-buf.h"
#include "gst.h"

#define DBG 0
extern gboolean tsin_is_gtab;
extern int ph_key_sz;
void add_cache(int start, int usecount, TSIN_PARSE *out, short match_phr_N, short no_match_ch_N, int tc_len);
void extract_gtab_key(gboolean is_en, int start, int len, void *out);
gboolean check_gtab_fixed_mismatch(int idx, char *mtch, int plen);
void mask_tone(phokey_t *pho, int plen, char *tone_mask);
void mask_pho_ref(phokey_t *pho, phokey_t *refpho, int plen, char *tone_mask);

static int tsin_parse_len;

void set_tsin_parse_len(int len)
{
  tsin_parse_len = len;
}

static char *c_pinyin_set;

int tsin_parse_recur(int start, TSIN_PARSE *out, short *r_match_phr_N, short *r_no_match_ch_N)
{
//  dbg("tsin_parse_recur start:%d tsin_parse_len:%d\n", start, tsin_parse_len);
  int plen;
  double bestscore = -1;
  int bestusecount = 0;
  *r_match_phr_N = 0;
  *r_no_match_ch_N = tsin_parse_len - start;
  int ini_len = 1;

#define USE_MALLOC 1
#if USE_MALLOC
	TSIN_PARSE *pbest = tmalloc(TSIN_PARSE, tsin_parse_len);
#else
    TSIN_PARSE pbest[MAX_PH_BF_EXT+1];
#endif

  if (tsin_hand.tsin_is_gtab) {
	  for(plen=0; start + plen < tsin_parse_len && plen <= MAX_PHRASE_LEN; plen++)
		if ((gbuf[start+plen].ch[0] & 0x80))
			break;
  } else {
	  for(plen=0; start + plen < tsin_parse_len && plen <= MAX_PHRASE_LEN; plen++)
		  if (	(tss.chpho[start+plen].cha[0] & 0x80) )
			break;
  }

  if (plen==0)
	  plen=1;

#if DBG
  dbg("tsin_parse_recur start:%d plen:%d\n", start, plen);
#endif
  for(; start + plen <= tsin_parse_len && plen <= MAX_PHRASE_LEN; plen++) {

#if DBG
    dbg("---- aa st:%d hh plen:%d ", start, plen);utf8_putchar(tss.chpho[start].ch); dbg("\n");
#endif
    if (plen > 1) {
      if (tsin_hand.tsin_is_gtab) {
        if (gbuf[start+plen-1].flag & (FLAG_CHPHO_PHRASE_USER_HEAD)) {
          break;
		}
      } else
		if (tss.chpho[start+plen-1].flag & (FLAG_CHPHO_PHRASE_USER_HEAD)) {
          break;
		}
    }

    bzero(pbest, sizeof(TSIN_PARSE) * tsin_parse_len);
    u_int64_t pp64[MAX_PHRASE_LEN + 1];
    u_int *pp32 = (u_int *)pp64;
    phokey_t *pp = (phokey_t *)pp64;
    int sti, edi;

#define MAXV 1000
    int maxusecount = 5-MAXV;
    int remlen;
    short match_phr_N=0, no_match_ch_N = plen;
    void *ppp = pp64;

	if ((tsin_hand.tsin_is_gtab && (!(gbuf[start].ch[0] & 0x80)	)) ||  (!tsin_hand.tsin_is_gtab && ( !(tss.chpho[start].ch[0] & 0x80)	 ))) {
		 if ((tsin_hand.tsin_is_gtab && (gbuf[start+plen-1].ch[0] & 0x80) ) ||  (!tsin_hand.tsin_is_gtab && (tss.chpho[start+plen-1].ch[0] & 0x80) )) {
//		   dbg("break\n");
		   break;
		 }
		 pbest[0].len = plen;
		 maxusecount = 0;
		 no_match_ch_N = 0;
		 match_phr_N = 1;
		 int i;
		 if (tsin_hand.tsin_is_gtab) {
			 for(i=0;i<plen;i++) {
				 utf8cpy((char *)&pbest[0].str[i], gbuf[start+i].ch);
			 }
		 } else {
			 for(i=0;i<plen;i++) {
				 utf8cpy((char *)&pbest[0].str[i], tss.chpho[start+i].cha);
			 }
		 }
		 goto next;
    }


#if 0
    if (tsin_hand.ph_key_sz==2)
      ppp=pp;
    else if (tsin_hand.ph_key_sz==4)
      ppp=pp32;
    else
      ppp=pp64;
#endif

    pbest[0].len = plen;
    pbest[0].start = start;
    int i, ofs;

    if (tsin_hand.tsin_is_gtab)
      for(ofs=i=0; i < plen; i++)
        ofs += utf8cpy((char *)pbest[0].str + ofs, gbuf[start + i].ch);
    else
      for(ofs=i=0; i < plen; i++)
        ofs += utf8cpy((char *)pbest[0].str + ofs, tss.chpho[start + i].ch);

#if DBG
    dbg("st:%d hh plen:%d ", start, plen);utf8_putchar(tss.chpho[start].ch); dbg("\n");
#endif
	int m_remlen = tsin_parse_len - start;


    if (tsin_hand.tsin_is_gtab)
      extract_gtab_key(FALSE, start, m_remlen, ppp);
    else {
      extract_pho(FALSE, start, m_remlen, (phokey_t *)ppp);
      if (c_pinyin_set)
        mask_tone(pp, m_remlen, c_pinyin_set + start);
    }

#if DBG
	if (c_pinyin_set)
    for(i=0; i < plen; i++) {
      prph(pp[i]); dbg("%d", c_pinyin_set[i+start]);
    }
    dbg("\n");
#endif
#if DBG
	dbg("check %d %d\n", start, plen);
#endif
    char *pinyin_set = c_pinyin_set ? c_pinyin_set+start:NULL;
    if (!tsin_seek(ppp, plen, &sti, &edi, pinyin_set)) {
//      dbg("tsin_seek not found...\n");
      if (plen > 1)
        break;
      goto next;
    }

    u_int64_t mtk64[MAX_PHRASE_LEN];
    phokey_t *mtk = (phokey_t *)mtk64;
    u_int *mtk32 = (u_int *)mtk64;
    void *pho = mtk64;

#if 0
    if (tsin_hand.ph_key_sz==2)
      pho=mtk;
    else if (tsin_hand.ph_key_sz==4)
      pho=mtk32;
    else
      pho=mtk64;
#endif

    for (;sti < edi; sti++) {
      char mtch[MAX_PHRASE_LEN*CH_SZ+1];
      char match_len;
      usecount_t usecount;

      load_tsin_entry(sti, &match_len, &usecount, pho, (u_char *)mtch);

      if (match_len < plen)
        continue;

      if (tsin_hand.tsin_is_gtab) {
        if (check_gtab_fixed_mismatch(start, mtch, plen))
          continue;
      } else {
		if (c_pinyin_set)
			mask_pho_ref((phokey_t*)pho, (phokey_t*)ppp, plen, c_pinyin_set + start);
		if (check_fixed_mismatch(start, mtch, plen))
			continue;
	  }

      if (usecount < 0)
        usecount = 0;

	  int max_len = tsin_parse_len - start;
	  if (max_len > match_len)
		  max_len = match_len;

      int i;
      if (tsin_hand.ph_key_sz==2) {
        if (c_pinyin_set) {
#if 0
          mask_tone(mtk, max_len, c_pinyin_set + start);
#else
		  mask_pho_ref((phokey_t*)pho, (phokey_t*)ppp, max_len, c_pinyin_set + start);
#endif
        }
        for(i=0;i < max_len;i++)
          if (mtk[i]!=pp[i])
            break;
      } else if (tsin_hand.ph_key_sz==4) {
        for(i=0;i < plen;i++)
          if (mtk32[i]!=pp32[i])
            break;
      } else {
        for(i=0;i < plen;i++)
          if (mtk64[i]!=pp64[i])
            break;
      }

      if (i < plen) {
        continue;
	  }

#if 1
//      if (match_len > plen)
	  if (i != match_len) {
        continue;
      }
#endif

      if (usecount <= maxusecount)
        continue;

      pbest[0].len = plen;
      maxusecount = usecount;
      utf8cpyN((char *)pbest[0].str, mtch, plen);
      pbest[0].flag |= FLAG_TSIN_PARSE_PHRASE;

      match_phr_N = 1;
      no_match_ch_N = 0;
#if DBG
      utf8_putcharn(mtch, plen);
      dbg(" match_len:%d plen:%d usecount:%d  ", match_len, plen, usecount);
        utf8_putcharn(mtch, plen);
      dbg("\n");
#endif
    }


next:

#if 0
    if (!match_phr_N) {
      if (tsin_is_gtab) {
        if (!(gbuf[start].ch[0] & 0x80))
          no_match_ch_N = 0;
      } else
      if (!(tss.chpho[start].ch[0] & 0x80))
        no_match_ch_N = 0;
    }
#else
//	dbg("no_match_ch_N %d\n", no_match_ch_N);
#endif

    remlen = tsin_parse_len - (start + plen);
#if DBG || 0
	dbg("remlen %d\n", remlen);
#endif
    if (remlen) {
      int next = start + plen;
      CACHE *pca;

      short smatch_phr_N, sno_match_ch_N;
      int uc;

      if (pca = cache_lookup(next)) {
        uc = pca->usecount;
        smatch_phr_N = pca->match_phr_N;
        sno_match_ch_N = pca->no_match_ch_N;
        memcpy(&pbest[1], pca->best, (tsin_parse_len - next) * sizeof(TSIN_PARSE));
      } else {
        uc = tsin_parse_recur(next, &pbest[1], &smatch_phr_N, &sno_match_ch_N);
//        dbg("   gg %d\n", smatch_phr_N);
        add_cache(next, uc, &pbest[1], smatch_phr_N, sno_match_ch_N, tsin_parse_len);
      }

      match_phr_N += smatch_phr_N;
      no_match_ch_N += sno_match_ch_N;
      maxusecount += uc;
    }


    double score = log((double)maxusecount + MAXV) / (pow((double)match_phr_N, 2)+ 1.0E-6) / (pow((double)no_match_ch_N, 4) + 1.0E-6);

#if DBG || 0
    dbg("st:%d plen:%d zz muse:%d ma:%d noma:%d  score:%.4e %.4e\n", start, plen, maxusecount, match_phr_N, no_match_ch_N, score, bestscore);
#endif

    if (score > bestscore) {
#if DBG || 0
      dbg("is best org %.4e\n", bestscore);
#endif
      bestscore = score;
      memcpy(out, pbest, sizeof(TSIN_PARSE) * (tsin_parse_len - start));

#if DBG
      dbg("    str:%d  ", start);
      int i;
      for(i=0; i<tsin_parse_len - start && out[i].len; i++) {
		dbg("%d]", i);
        utf8_putcharn((char *)out[i].str, out[i].len);
      }
      dbg("\n");
#endif

      bestusecount = maxusecount;
      *r_match_phr_N = match_phr_N;
      *r_no_match_ch_N = no_match_ch_N;
    }
  }

  if (bestusecount < 0)
    bestusecount = 0;

#if USE_MALLOC
  free(pbest);
#endif

  return bestusecount;
}

void disp_ph_sta_idx(int idx);

void free_cache(), load_tsin_db();
char *get_first_pho(int idx);

void tsin_parse()
{
  dbg("--------- tsin_parse()\n");
  TSIN_PARSE out[MAX_PH_BF_EXT+1];
  bzero(out, sizeof(out));

  int i, ofsi;

  if (tss.c_len <= 1)
    return;

  load_tsin_db();

  set_tsin_parse_len(tss.c_len);

  init_cache(tss.c_len);

  char pinyin_set[MAX_PH_BF_EXT];
  c_pinyin_set = (pin_juyin||pho_no_tone)?pinyin_set:NULL;
  get_chpho_pinyin_set(pinyin_set);

  short smatch_phr_N, sno_match_ch_N;
  tsin_parse_recur(0, out, &smatch_phr_N, &sno_match_ch_N);

  for(i=0; i < tss.c_len; i++) {
	if (BITON(tss.chpho[i].flag, FLAG_CHPHO_EN_PHRASE)) {
//      dbg("skip en\n", i);
	  continue;
	}
    tss.chpho[i].flag &= ~(FLAG_CHPHO_PHRASE_HEAD|FLAG_CHPHO_PHRASE_BODY);
  }
#if DBG
  dbg("-- result\n");
#endif
  for(ofsi=i=0; out[i].len; i++) {
#if DBG
	dbg("out %d] %d\n", i, out[i].len);
#endif
    int j, ofsj;
    int psta = ofsi;

    if (out[i].flag & FLAG_TSIN_PARSE_PHRASE)
        tss.chpho[ofsi].flag |= FLAG_CHPHO_PHRASE_HEAD;

    for(ofsj=j=0; j < out[i].len; j++) {
      ofsj += utf8cpy(tss.chpho[ofsi].cha, (char *)&out[i].str[ofsj]);
//      tss.chpho[ofsi].ch = tss.chpho[ofsi].cha;

      if (out[i].flag & FLAG_TSIN_PARSE_PHRASE) {
        tss.chpho[ofsi].psta = psta;
		tss.chpho[ofsi].flag |= FLAG_CHPHO_PHRASE_BODY;
	  }

      ofsi++;
    }
  }

  int ph_sta_idx = tss.ph_sta;
  if (tss.chpho[tss.c_len-1].psta>=0 && tss.c_len - tss.chpho[tss.c_len-1].psta > 1) {
    ph_sta_idx = tss.chpho[tss.c_len-1].psta;
  }

  int idx2 = tss.c_len - 2;
//  dbg("0000000 %x\n", tss.chpho[idx2].flag & (FLAG_CHPHO_PHRASE_BODY|FLAG_CHPHO_FIXED));

  if (idx2>=0 && !(tss.chpho[idx2].flag & (FLAG_CHPHO_PHRASE_BODY|FLAG_CHPHO_FIXED))) {
//	 dbg("111111111\n");
     char *ps = get_first_pho(idx2);
	 if (ps) {
		 utf8cpy(tss.chpho[idx2].cha, ps);
//		 dbg("parse %s\n", tss.chpho[idx2].cha);
	 }
  }

#if 0
  disp_ph_sta_idx(ph_sta_idx);
#endif

#if 0
  for(i=0;i<tss.c_len;i++)
    utf8_putchar(tss.chpho[i].ch);
  puts("");
#endif

  free_cache();
}
