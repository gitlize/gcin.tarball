/*
	Copyright (C) 1995-2008	Edward Der-Hua Liu, Hsin-Chu, Taiwan
*/

#include "gcin.h"
#include "pho.h"
#include "tsin.h"
#include "gtab.h"
#include "gst.h"
#include "lang.h"
#include <sys/stat.h>

TSIN_HANDLE tsin_hand, en_hand;

//int ph_key_sz; // bytes

#define PHIDX_SKIP  (sizeof(tsin_hand.phcount) + sizeof(tsin_hand.hashidx))

#if 0
char *current_tsin_fname;
time_t current_modify_time;
#endif

void get_gcin_user_or_sys_fname(char *name, char fname[]);


static void get_modify_time(TSIN_HANDLE *ptsin_hand)
{
  struct stat st;
  if (!fstat(fileno(ptsin_hand->fph), &st)) {
    ptsin_hand->modify_time = st.st_mtime;
  }
}

void free_tsin_ex(TSIN_HANDLE *th)
{
  free(th->tsin_fname); th->tsin_fname=NULL;

  if (th->fph) {
    fclose(th->fph); th->fph = NULL;
  }

  if (th->fp_phidx) {
    fclose(th->fp_phidx); th->fp_phidx=NULL;
  }
}

gboolean load_tsin_db_ex(TSIN_HANDLE *ptsin_hand, char *infname, gboolean read_only, gboolean use_idx)
{
  char tsidxfname[512];
  char *fmod = read_only?"rb":"rb+";
//  dbg("cur %s %s\n", infname, current_tsin_fname);

  if (ptsin_hand->fph) {
    if (!strcmp(ptsin_hand->tsin_fname, infname))
      return TRUE;
    free_tsin_ex(ptsin_hand);
  }

  if (ptsin_hand->tsin_fname)
    free(ptsin_hand->tsin_fname);

  ptsin_hand->tsin_fname = strdup(infname);

  strcpy(tsidxfname, infname);
  strcat(tsidxfname, ".idx");

//  dbg("tsidxfname %s\n", tsidxfname);
#define BF_SZ (16 * 1024)

  FILE *fp_phidx = ptsin_hand->fp_phidx, *fph = ptsin_hand->fph;

  if (use_idx) {
    if ((fp_phidx=fopen(tsidxfname, fmod))==NULL) {
       dbg("load_tsin_db_ex A Cannot open '%s'\n", tsidxfname);
       return FALSE;
    }
    ptsin_hand->fp_phidx = fp_phidx;
#if 0
	setvbuf (fp_phidx, NULL , _IOFBF, BF_SZ);
#endif
    int rn;
    rn=fread(&ptsin_hand->phcount,4,1, fp_phidx);
    rn=fread(&ptsin_hand->hashidx,1,sizeof(ptsin_hand->hashidx), fp_phidx);
#if     0
    dbg("phcount:%d\n",phcount);
#endif
    ptsin_hand->a_phcount=ptsin_hand->phcount+256;
  }


  if (fph)
    fclose(fph);

//  dbg("tsfname: %s\n", infname);

  if ((fph=fopen(infname, fmod))==NULL)
    p_err("load_tsin_db0 B Cannot open '%s'", infname);
#if 0
  setvbuf (fph, NULL , _IOFBF, BF_SZ);
#endif
  ptsin_hand->fph = fph;

//  free(current_tsin_fname);
//  current_tsin_fname = strdup(infname);


  get_modify_time(ptsin_hand);

  gboolean is_gtab_i = FALSE;

  TSIN_GTAB_HEAD head;
  int rn;
  rn = fread(&head, sizeof(head), 1, fph);

  if (!strcmp(head.signature, TSIN_GTAB_KEY)) {
		is_gtab_i = TRUE;
		if (head.keybits*head.maxkey > 32) {
		  ptsin_hand->ph_key_sz = 8;
	//      tsin_hash_shift = TSIN_HASH_SHIFT_64;
		}
		else {
		  ptsin_hand->ph_key_sz = 4;
	//      tsin_hash_shift = TSIN_HASH_SHIFT_32;
		}
  } else
  if (!strcmp(head.signature, TSIN_EN_WORD_KEY)) {
	ptsin_hand->ph_key_sz = 1;
  } else {
    ptsin_hand->ph_key_sz = 2;
//    tsin_hash_shift = TSIN_HASH_SHIFT;
  }

  ptsin_hand->tsin_is_gtab = is_gtab_i;
  return TRUE;
}


void load_tsin_db0(char *infname, gboolean is_gtab_i)
{
  load_tsin_db_ex(&tsin_hand, infname, FALSE, TRUE);
}

void load_en_db0(char *infname)
{
  load_tsin_db_ex(&en_hand, infname, FALSE, TRUE);
}


void free_tsin()
{
  free_tsin_ex(&tsin_hand);
}

void free_en()
{
  free_tsin_ex(&en_hand);
}


extern gboolean is_chs;
void load_tsin_db()
{
  char tsfname[512];
  char *fname = tsin32_f;

  get_gcin_user_fname(fname, tsfname);
  load_tsin_db0(tsfname, FALSE);
}

void load_en_db()
{
  char tsfname[512];

  get_gcin_user_fname(TSIN_EN_FILE, tsfname);
  load_en_db0(tsfname);
}


static void seek_fp_phidx(TSIN_HANDLE *ptsin_hand, int i)
{
  fseek(ptsin_hand->fp_phidx, PHIDX_SKIP + i*sizeof(int), SEEK_SET);
}

void reload_tsin_db_ex(TSIN_HANDLE *th)
{
  char tt[512];

  if (!th->tsin_fname)
    return;

  strcpy(tt, th->tsin_fname);
  free_tsin_ex(th);

//  free(current_tsin_fname); current_tsin_fname = NULL;
//  load_tsin_db0(tt, th->tsin_is_gtab);
  load_tsin_db_ex(th, tt, FALSE, TRUE);
}


void reload_tsin_db()
{
  reload_tsin_db_ex(&tsin_hand);
}

void reload_en_db()
{
  reload_tsin_db_ex(&en_hand);
}


inline static int get_phidx(TSIN_HANDLE *ptsin_hand, int i)
{
  seek_fp_phidx(ptsin_hand, i);
  int t, rn;
  rn = fread(&t, sizeof(int), 1, ptsin_hand->fp_phidx);

  if (ptsin_hand->tsin_is_gtab || ptsin_hand->ph_key_sz ==1)
    t += sizeof(TSIN_GTAB_HEAD);

  return t;
}

inline static int phokey_t_seq8(u_char *a, u_char *b, int len)
{
  return memcmp(a, b, len);
}


inline static int phokey_t_seq16(phokey_t *a, phokey_t *b, int len)
{
  int i;

  for (i=0;i<len;i++) {
    if (a[i] > b[i]) return 1;
    else
    if (a[i] < b[i]) return -1;
  }

  return 0;
}


inline static int phokey_t_seq32(u_int *a, u_int *b, int len)
{
  int i;

  for (i=0;i<len;i++) {
    if (a[i] > b[i]) return 1;
    else
    if (a[i] < b[i]) return -1;
  }

  return 0;
}


inline static int phokey_t_seq64(u_int64_t *a, u_int64_t *b, int len)
{
  int i;

  for (i=0;i<len;i++) {
    if (a[i] > b[i]) return 1;
    else
    if (a[i] < b[i]) return -1;
  }

  return 0;
}


static int phokey_t_seq(TSIN_HANDLE *th, void *a, void *b, int len)
{
  if (th->ph_key_sz==1)
    return phokey_t_seq8((u_char *)a, (u_char *)b, len);
  else if (th->ph_key_sz==2)
    return phokey_t_seq16((phokey_t *)a, (phokey_t *)b, len);
  else if (th->ph_key_sz==4)
    return phokey_t_seq32((u_int *)a, (u_int *)b, len);
  else if (th->ph_key_sz==8)
    return phokey_t_seq64((u_int64_t*)a, (u_int64_t*)b, len);
  return 0;
}


static int phseq(TSIN_HANDLE *th, u_char *a, u_char *b)
{
  u_char lena, lenb, mlen;

  lena=*(a++); lenb=*(b++);
  a+=sizeof(usecount_t); b+=sizeof(usecount_t);   // skip usecount

  mlen=Min(lena,lenb);
  u_int64_t ka[MAX_PHRASE_LEN], kb[MAX_PHRASE_LEN];

  memcpy(ka, a, th->ph_key_sz * mlen);
  memcpy(kb, b, th->ph_key_sz * mlen);

  int d = phokey_t_seq(th, ka, kb, mlen);
  if (d)
    return d;

  if (lena > lenb) return 1;
  if (lena < lenb) return -1;
  return 0;
}

gboolean inc_tsin_use_count(TSIN_HANDLE *th, void *pho, char *ch, int N);

static gboolean saved_phrase;


static void reload_if_modified(TSIN_HANDLE *th)
{
  struct stat st;
  if (fstat(fileno(th->fph), &st) || th->modify_time != st.st_mtime) {
    reload_tsin_db_ex(th);
  }
}


gboolean save_phrase_to_db(TSIN_HANDLE *th, void *phkeys, char *utf8str, int len, usecount_t usecount)
{
  reload_if_modified(th);

  int mid, ord = 0, ph_ofs, hashno;
  u_char tbuf[MAX_PHRASE_LEN*(sizeof(u_int64_t)+CH_SZ) + 1 + sizeof(usecount_t)],
         sbuf[MAX_PHRASE_LEN*(sizeof(u_int64_t)+CH_SZ) + 1 + sizeof(usecount_t)];

  saved_phrase = TRUE;

  tbuf[0]=len;
  memcpy(&tbuf[1], &usecount, sizeof(usecount));  // usecount
  int tlen = (utf8str && th->ph_key_sz != 1)?utf8_tlen(utf8str, len):0;
#if 0
  dbg("tlen %d  '", tlen);
  for(i=0; i < tlen; i++)
    putchar(utf8str[i]);
  dbg("'\n");
#endif

  dbg("save_phrase_to_db '%s'  tlen:%d  ph_key_sz:%d\n", utf8str, tlen, th->ph_key_sz);

  memcpy(&tbuf[1 + sizeof(usecount_t)], phkeys, th->ph_key_sz * len);
  if (th->ph_key_sz > 1)
    memcpy(&tbuf[th->ph_key_sz*len + 1 + sizeof(usecount_t)], utf8str, tlen);

  if (th->ph_key_sz==1)
    hashno= *((u_char *)phkeys);
  else if (th->ph_key_sz==2)
    hashno= *((phokey_t *)phkeys) >> TSIN_HASH_SHIFT;
  else if (th->ph_key_sz==4)
    hashno= *((u_int *)phkeys) >> TSIN_HASH_SHIFT_32;
  else
    hashno= *((u_int64_t *)phkeys) >> TSIN_HASH_SHIFT_64;

//  dbg("hashno %d\n", hashno);

  if (hashno >= TSIN_HASH_N)
    return FALSE;

  for(mid=th->hashidx[hashno]; mid<th->hashidx[hashno+1]; mid++) {
    ph_ofs=get_phidx(th, mid);

    fseek(th->fph, ph_ofs, SEEK_SET);
    int rn;
    rn = fread(sbuf,1,1, th->fph);
    rn = fread(&sbuf[1], sizeof(usecount_t), 1, th->fph); // use count
	rn = fread(&sbuf[1+sizeof(usecount_t)], 1, th->ph_key_sz * sbuf[0] + tlen, th->fph);

    if ((ord=phseq(th, sbuf,tbuf)) > 0)
        break;

    if (!ord && (th->ph_key_sz==1 || !memcmp(&sbuf[sbuf[0]*th->ph_key_sz+1+sizeof(usecount_t)], utf8str, tlen))) {
//    bell();
      dbg("Phrase already exists\n");
      inc_tsin_use_count(th, phkeys, utf8str, len);
      return FALSE;
    }
  }

  int wN = th->phcount - mid;

//  dbg("wN %d  phcount:%d mid:%d\n", wN, phcount, mid);

  if (wN > 0) {
    int *phidx = tmalloc(int, wN);
    seek_fp_phidx(th, mid);
    int rn;
    rn = fread(phidx, sizeof(int), wN, th->fp_phidx);
    seek_fp_phidx(th, mid+1);
    fwrite(phidx, sizeof(int), wN, th->fp_phidx);
    free(phidx);
  }

  fseek(th->fph,0,SEEK_END);

  ph_ofs=ftell(th->fph);
  if (th->ph_key_sz !=2)
    ph_ofs -= sizeof(TSIN_GTAB_HEAD);

//  dbg("ph_ofs %d  ph_key_sz:%d\n", ph_ofs, ph_key_sz);
  seek_fp_phidx(th, mid);
  fwrite(&ph_ofs, sizeof(int), 1, th->fp_phidx);
  th->phcount++;

  fwrite(tbuf, 1, th->ph_key_sz*len + tlen + 1+ sizeof(usecount_t), th->fph);
  fflush(th->fph);

  if (th->hashidx[hashno]>mid)
    th->hashidx[hashno]=mid;

  for(hashno++; hashno<TSIN_HASH_N; hashno++)
    th->hashidx[hashno]++;

  rewind(th->fp_phidx);
  fwrite(&th->phcount, sizeof(th->phcount), 1, th->fp_phidx);
  fwrite(th->hashidx,sizeof(th->hashidx),1, th->fp_phidx);
  fflush(th->fp_phidx);

  get_modify_time(th);
//  dbg("ofs %d\n", get_phidx(mid));
  return TRUE;
}


#include <sys/stat.h>


void load_tsin_entry0_ex(TSIN_HANDLE *th, char *len, usecount_t *usecount, void *pho, u_char *ch)
{
  *usecount = 0;
  *len = 0;
  int rn;
  rn = fread(len, 1, 1, th->fph);

//  dbg("rn %d\n", rn);

  if (*len > MAX_PHRASE_LEN /* || *len <= 0 */) {
    dbg("err: tsin db changed reload len:%d\n", *len);
    reload_tsin_db_ex(th); // probably db changed, reload;
    *len = 0;
    return;
  }

  gboolean en_has_str = FALSE;
  if (ch)
    ch[0]=0;

  if (*len < 0) {
    *len = - (*len);
    en_has_str = TRUE;
  }

  rn = fread(usecount, sizeof(usecount_t), 1, th->fph); // use count
  rn = fread(pho, th->ph_key_sz, (int)(*len), th->fph);
  if (ch && (th->ph_key_sz!=1 || en_has_str)) {
    rn = fread(ch, CH_SZ, (int)(*len), th->fph);
    int tlen = utf8_tlen((char *)ch, *len);
    ch[tlen]=0;
  }
}

void load_tsin_entry_ex(TSIN_HANDLE *th, int idx, char *len, usecount_t *usecount, void *pho, u_char *ch)
{
  *usecount = 0;

//  dbg("load_tsin_entry_ex idx:%d phcount:%d\n", idx, ptsin_hand->phcount);

  if (idx >= th->phcount) {
    reload_tsin_db(); // probably db changed, reload;
    *len = 0;
    return;
  }

  int ph_ofs=get_phidx(th, idx);
//  dbg("ph_ofs:%d\n", ph_ofs);

  fseek(th->fph, ph_ofs , SEEK_SET);
  load_tsin_entry0_ex(th, len, usecount, pho, ch);
}


void load_tsin_entry(int idx, char *len, usecount_t *usecount, void *pho, u_char *ch)
{
  load_tsin_entry_ex(&tsin_hand, idx, len, usecount, pho, ch);
}

// tone_mask : 1 -> pho has tone
void mask_tone(phokey_t *pho, int plen, char *tone_mask)
{
  int i;
//  dbg("mask_tone\n");
  if (!tone_mask)
    return;

  for(i=0; i < plen; i++) {
   if (!tone_mask[i])
    pho[i] &= (~7);
  }
}


// tone_mask : 1 -> pho has tone, pho[i] longer than refpho[i] is clipped.
void mask_pho_ref(phokey_t *pho, phokey_t *refpho, int plen, char *tone_mask)
{
  int i;
//  dbg("mask_tone\n");
  if (!tone_mask)
    return;

  for(i=0; i < plen; i++) {
   if (!tone_mask[i]) {
	 pho[i] &= (~7);
	 phokey_t r = refpho[i];
#define K1 (0xf<<3) // ㄚㄛㄜ
	 if ((r&K1)==0) {
		pho[i]&=~K1;

#define K2 (0x3<<7) // ㄧㄨㄩ
		if ((r&K2)==0)
			pho[i]&=~K2;
	 }
   }
  }
}


// ***  r_sti<=  range  < r_edi
gboolean tsin_seek_ex(TSIN_HANDLE *th, void *pho, int plen, int *r_sti, int *r_edi, char *tone_mask)
{
  int mid, cmp;
  u_int64_t ss[MAX_PHRASE_LEN], stk[MAX_PHRASE_LEN];
  char len;
  usecount_t usecount;
  int hashi;

  if (tone_mask) {
	int i;
	for(i=0;i<plen;i++)
	  if (!tone_mask[i])
		  break;
	if (i==plen)
		tone_mask=NULL;
  }

#if 0
  dbg("tsin_seek %d\n", plen);
  dbg("> ");
  prphs((phokey_t *)pho, plen);
  dbg("\n");
#endif

#if 0
  if (tone_mask)
    mask_tone((phokey_t *)pho, plen, tone_mask);
#endif

  if (th->ph_key_sz==1)
    hashi= *((u_char *)pho);
  else if (th->ph_key_sz==2)
    hashi= *((phokey_t *)pho) >> TSIN_HASH_SHIFT;
  else if (th->ph_key_sz==4)
    hashi= *((u_int *)pho) >> TSIN_HASH_SHIFT_32;
  else
    hashi= *((u_int64_t *)pho) >> TSIN_HASH_SHIFT_64;

  if (hashi >= TSIN_HASH_N) {
    dbg("hashi >= TSIN_HASH_N\n");
    return FALSE;
  }

  int top=th->hashidx[hashi];
  int bot=th->hashidx[hashi+1];

//  dbg("hashi:%d top:%d bot:%d\n", hashi, top, bot);

  if (top>=th->phcount) {
//    dbg("top>=phcount\n");
    return FALSE;
  }

  while (top <= bot) {
    mid=(top+bot)/ 2;
    load_tsin_entry_ex(th, mid, &len, &usecount, ss, NULL);

    u_char mlen;
    if (len > plen)
      mlen=plen;
    else
      mlen=len;

    if (tone_mask)
	  mask_pho_ref((phokey_t *)ss, (phokey_t *)pho, mlen, tone_mask);

//    prphs(ss, mlen);
//    mask_tone((phokey_t *)ss, mlen, tone_mask);

#if DBG || 0
    int j;
    dbg("> ");
    prphs(ss, len);
    dbg("\n");
#endif

    cmp=phokey_t_seq(th, ss, pho, mlen);

#if DEBUG && 0
      if (th->ph_key_sz==1) {
		dbg("mid %d  ", mid);
	    utf8_putcharn((char*)ss, len);
	    dbg(" %d\n", cmp);
	  }
#endif

    if (!cmp && len < plen) {
//	  dbg("-2\n");
      cmp=-2;
    }

    if (cmp>0)
      bot=mid-1;
    else
    if (cmp<0)
      top=mid+1;
    else
      break;
  }

  if (cmp && !tone_mask) {
//    dbg("no match %d\n", cmp);
    return FALSE;
  }

//  dbg("<--\n");
  // seek to the first match because binary search is used
//  gboolean found=FALSE;

  int sti;
  for(sti = mid; sti>=0; sti--) {
    load_tsin_entry_ex(th, sti, &len, &usecount, stk, NULL);

#if 0
    int j;
    dbg("%d] %d*> ", sti, len);
    prphs((phokey_t *)stk, len);
    dbg("\n");
#endif

    u_char mlen;
    if (len > plen)
      mlen=plen;
    else
      mlen=len;
#if 0
    prphs(stk, len);
#endif
#if 0
    mask_tone((phokey_t *)stk, mlen, tone_mask);
#else
    if (tone_mask) {
//	  mask_pho_ref((phokey_t *)stk, (phokey_t *)pho, mlen, tone_mask);
//	  for(int i=0; i < plen; i++) 
	  for(int i=0; i < mlen; i++) 
	  {
		phokey_t r=((phokey_t*)stk)[i];
		if ((r&K1)==0) {
		  r|=K1;
		  if ((r&K2)==0)
			  r|=K2;
		}
		r|=7;
		((phokey_t*)stk)[i]=r;
	  }
	}
#endif

    int v = phokey_t_seq(th, stk, pho, mlen);
//    if (!v)
//      found = TRUE;

    if (!tone_mask) {
      if (!v && len>=plen)
        continue;
    } else {
      if (v>0 || !v && len >= plen)
		continue;
	}

    break;
  }
  sti++;

  // seek to the tail
#if 0
  char tt[CH_SZ*MAX_PHRASE_LEN];
#define TTCH (u_char *)tt
#else
#define TTCH NULL
#endif

  if (tone_mask) {
    int top=th->hashidx[hashi];
    int bot=th->hashidx[hashi+1];

    if (top>=th->phcount) {
//      dbg("top>=phcount\n");
      return FALSE;
    }

    phokey_t tpho[MAX_PHRASE_LEN];

    for(int i=0; i < plen; i++) {
	  phokey_t r = ((phokey_t*)pho)[i];
	  if ((r&K1)==0) {
		r|=K1;
		if ((r&K2)==0)
		  r|=K2;
	  }
	  r|=7;
//      tpho[i]=((phokey_t*)pho)[i] | 7;
	  tpho[i]=r;
	}

    while (top <= bot) {
      mid=(top+bot)/ 2;
      load_tsin_entry_ex(th, mid, &len, &usecount, ss, NULL);

      u_char mlen;
      if (len > plen)
        mlen=plen;
      else
        mlen=len;

  //    prphs(ss, mlen);

#if DBG || 0
      int j;
      dbg("> ");
      prphs(ss, len);
      dbg("\n");
#endif

//	  mask_pho_ref((phokey_t *)ss, (phokey_t *)pho, mlen, tone_mask);

      cmp=phokey_t_seq(th, ss, tpho, mlen);

      if (!cmp && len < plen)
        cmp=-2;

      if (cmp>0)
        bot=mid-1;
      else
      if (cmp<0)
        top=mid+1;
      else
        break;
    }

    int edi;
	for(edi = mid; edi < th->phcount; edi++) {
	  load_tsin_entry_ex(th, edi, &len, &usecount, stk, TTCH);

	  u_char mlen;
	  if (len > plen)
		mlen=plen;
	  else
		mlen=len;
#if 0
		prphs(stk, len);
#endif
#if 0
		mask_tone((phokey_t *)stk, mlen, tone_mask);
#else
		mask_pho_ref((phokey_t *)stk, (phokey_t *)tpho, mlen, tone_mask);
#endif

		int v = phokey_t_seq(th, stk, tpho, mlen);

#if 0
		dbg("^ %s edi%d -> ", tt, edi);
		prphs((phokey_t *)stk, len);
		dbg(" v:%d\n", v);
#endif
		if (v<=0)
		  continue;
		break;
	  }

#if 0
	  dbg("sti%d edi:%d\n", sti, edi);
#endif

	  *r_sti = sti;
	  *r_edi = edi;
	  return edi > sti;
  }

  int edi;
  for(edi = mid; edi < th->phcount; edi++) {
    load_tsin_entry_ex(th, edi, &len, &usecount, stk, TTCH);

    u_char mlen;
    if (len > plen)
      mlen=plen;
    else
      mlen=len;
#if 0
    prphs((phokey_t *)stk, len); dbg("%s\n", tt);
#endif

    int v = phokey_t_seq(th, stk, pho, mlen);
//    if (!v)
//      found = TRUE;
#if 0
    dbg("edi%d -> ", edi);
    prphs((phokey_t *)stk, len);
    dbg(" v:%d\n", v);
#endif

    if (v <= 0)
      continue;
    break;
  }

#if 0
  dbg("@@ sti%d edi:%d\n", sti, edi);
#endif

  *r_sti = sti;
  *r_edi = edi;

  return edi > sti;
}

static gboolean tsin_seek_en_1(u_char *pho, int plen, int *ridx)
{
  TSIN_HANDLE *th = &en_hand;
  int mid = -1, cmp;
  u_char ss[MAX_PHRASE_STR_LEN];
  char len;
  usecount_t usecount;
  int hashi;

#if 1
  dbg("tsin_seek_en %d\n", plen);
#endif

  hashi= *((u_char *)pho);

  if (hashi >= TSIN_HASH_N) {
    dbg("hashi >= TSIN_HASH_N\n");
	*ridx = th->phcount;
    return FALSE;
  }

  int top=th->hashidx[hashi];
  int bot=th->hashidx[hashi+1];

  dbg("hashi:%d top:%d bot:%d\n", hashi, top, bot);

  if (top>=th->phcount) {
//    dbg("top>=phcount\n");
	*ridx = th->phcount;
    return FALSE;
  }

  while (top <= bot) {
    mid=(top+bot)/ 2;
    load_tsin_entry_ex(th, mid, &len, &usecount, ss, NULL);

    u_char mlen;
    if (len > plen)
      mlen=plen;
    else
      mlen=len;

    cmp=phokey_t_seq8(ss, pho, mlen);

    if (!cmp && len < plen) {
	  dbg("-2\n");
      cmp=-2;
    }

    if (cmp>0)
      bot=mid-1;
    else
    if (cmp<0)
      top=mid+1;
    else
      break;
  }

  if (mid < 0)
    mid = 0;

  if (cmp) {
    dbg("no match %d\n", cmp);
	*ridx = mid;
    return FALSE;
  }

  *ridx = mid;
  return TRUE;
}



// ***  r_sti<=  range  < r_edi
gboolean tsin_seek_en(u_char *pho, int plen, int *r_sti, int *r_edi)
{
  TSIN_HANDLE *th = &en_hand;
//  int cmp;
  u_char ss[MAX_PHRASE_STR_LEN], stk[MAX_PHRASE_STR_LEN];
  char len;
  usecount_t usecount;

#if 0
  dbg("tsin_seek_en %d\n", plen);
#endif

  int eq_idx;
  if (!tsin_seek_en_1(pho, plen, &eq_idx))
    return FALSE;

  int u_idx = -1; // upper bound
  memcpy(ss, pho, plen);
//  ss[plen-1]++;
  ss[plen]=0; // fake upperbound string
  tsin_seek_en_1(ss, plen+1, &u_idx);

//  dbg("u_idx %d\n", u_idx);

//  dbg("<--\n");
  gboolean found=FALSE;
  int sti;
  for(sti = u_idx; sti>=0; sti--) {
    load_tsin_entry_ex(th, sti, &len, &usecount, stk, NULL);

    u_char mlen=0;
    if (len > plen)
      mlen=plen;
    else
      mlen=len;
#if 0
    prphs(stk, len);
#endif

    int v = phokey_t_seq8(stk, pho, plen);
	if (v > 0)
		continue;
//    if (!v)
//      found = TRUE;

#if 0
    int j;
    dbg("%d] %d*> ", sti, mlen);
    prphs(stk, len);
    dbg(" v:%d\n", v);
#endif

    if (!v && len >= plen)
      continue;
    break;
  }
  sti++;

  // seek to the tail

  int l_idx = -1;
  memcpy(ss, pho, plen);
//  ss[plen-1]--;
  ss[plen]=127;
  tsin_seek_en_1(ss, plen+1, &l_idx);

//  dbg("l_idx:%d\n", l_idx);

  int edi;
  for(edi = l_idx; edi < th->phcount; edi++) {
    load_tsin_entry_ex(th, edi, &len, &usecount, stk, NULL);

    u_char mlen=0;
    if (len > plen)
      mlen=plen;
    else
      mlen=len;

    int v = phokey_t_seq8(stk, pho, mlen);
	if (v < 0)
	  continue;

//    if (!v)
//      found = TRUE;
#if 0
    dbg("edi%d -> ", edi);
    dbg(" v:%d\n", v);
#endif

    if (!v && len >= plen)
      continue;
    break;
  }

#if 0
  dbg("sti%d edi:%d found:%d\n", sti, edi, found);
#endif

  *r_sti = sti;
  *r_edi = edi;

  return edi > sti;
}


gboolean tsin_seek(void *pho, int plen, int *r_sti, int *r_edi, char *tone_mask)
{
  return tsin_seek_ex(&tsin_hand, pho, plen, r_sti, r_edi, tone_mask);
}

gboolean inc_tsin_use_count(TSIN_HANDLE *th, void *pho, char *ch, int N)
{
  int sti, edi;

  reload_if_modified(th);

  if (ch)
	dbg("CH inc_dec_tsin_use_count '%s'\n", ch);
  else {
    dbg("EN inc_dec_tsin_use_count %d '", N);
    utf8_putcharn((char *)pho, N);
    dbg("'\n");
  }

  if (!tsin_seek_ex(th, pho, N, &sti, &edi, NULL)) {
	dbg("inc_dec_tsin_use_count not found\n");
    return FALSE;
  }

  int idx;

#if 0
  int tlen = ch?strlen(ch):0;
  dbg("otlen %d  ", tlen);
  int i;
  for(i=0; i < tlen; i++)
    putchar(ch[i]);
  puts("");
#endif

  for(idx=sti; idx < edi; idx++) {
    char len;
    usecount_t usecount, n_usecount;
    u_int64_t phi[MAX_PHRASE_LEN];
    char stch[MAX_PHRASE_LEN * CH_SZ * 2];

    load_tsin_entry_ex(th, idx, &len, &usecount, phi, (u_char *)stch);
    n_usecount = usecount;

    if (len!=N || phokey_t_seq(th, phi, pho, N))
      break;
#if 0
    for(i=0; i < tlen; i++)
      putchar(stch[i]);
    dbg(" ppp\n");
#endif

//	dbg("stch %s\n", stch);
    if (th->ph_key_sz!=1 && strcmp(stch, ch))
      continue;
#if 1
    dbg("found match %d\n", usecount);
#endif
    int ph_ofs=get_phidx(th, idx);

    fseek(th->fph, ph_ofs + 1, SEEK_SET);

    if (usecount < 0x3fffffff)
      n_usecount++;

    if (n_usecount != usecount) {
      fwrite(&n_usecount, sizeof(usecount_t), 1, th->fph); // use count
      fflush(th->fph);
    }
  }

  get_modify_time(th);
  return TRUE;
}

void strtolower(char *u8, int len)
{
  int j;
  for(j=0;j<len;j++)
    u8[j] = tolower(u8[j]);
}

gboolean inc_tsin_use_count_en(char *s, int len)
{
  if (inc_tsin_use_count(&en_hand, s, NULL, len))
    return TRUE;

  s[0]=tolower(s[0]);
  if (inc_tsin_use_count(&en_hand, s, NULL, len))
    return TRUE;

  strtolower((char *)s,len);
  return inc_tsin_use_count(&en_hand, s, NULL, len);
}
