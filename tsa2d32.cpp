/*
	Copyright (C) 1995-2008	Edward Der-Hua Liu, Hsin-Chu, Taiwan
*/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "gcin.h"
#include "pho.h"
#include "tsin.h"
#include "gtab.h"
#include "gst.h"

void load_pin_juyin();
phokey_t pinyin2phokey(char *s);

static char *bf;
static int bfN_a = 0, ofs=0;
static gboolean b_pinyin;

int *phidx, *sidx, phcount;
int bfsize, phidxsize;
u_char *sf;
gboolean is_gtab, is_en_word, gtabkey64;
int phsz, hash_shift;
int (*key_cmp)(char *a, char *b, char len);


int key_cmp8(char *a, char *b, char len)
{
#if 0
  dbg("len %d\n", len);
  utf8_putcharn(a, len);  dbg(" ");
  utf8_putcharn(b, len);
  dbg("\n");
#endif
  return memcmp(a, b, len);
}


int key_cmp16(char *a, char *b, char len)
{
  u_char i;
  for(i=0; i < len; i++) {
    phokey_t ka,kb;
    memcpy(&ka, a, 2);
    memcpy(&kb, b, 2);
    if (ka > kb) return 1;
    if (kb > ka) return -1;
    a+=2;
    b+=2;
  }

  return 0;
}

int key_cmp32(char *a, char *b, char len)
{
  u_char i;
  for(i=0; i < len; i++) {
    u_int ka,kb;
    memcpy(&ka, a, 4);
    memcpy(&kb, b, 4);
    if (ka > kb) return 1;
    if (kb > ka) return -1;
    a+=4;
    b+=4;
  }
  return 0;
}

int key_cmp64(char *a, char *b, char len)
{
  u_char i;
  for(i=0; i < len; i++) {
    u_int64_t ka,kb;
    memcpy(&ka, a, 8);
    memcpy(&kb, b, 8);
    if (ka > kb) return 1;
    if (kb > ka) return -1;
    a+=8;
    b+=8;
  }
  return 0;
}

static int qcmp(const void *a, const void *b)
{
  int idxa=*((int *)a);  char *pa = (char *)&bf[idxa];
  int idxb=*((int *)b);  char *pb = (char *)&bf[idxb];
  char lena,lenb, len;
  usecount_t usecounta, usecountb;

  lena=*(pa++); if (lena < 0) lena = -lena; memcpy(&usecounta, pa, sizeof(usecount_t)); pa+= sizeof(usecount_t);
  char *ka = pa;
  pa += lena * phsz;
  lenb=*(pb++); if (lenb < 0) lenb = -lenb; memcpy(&usecountb, pb, sizeof(usecount_t)); pb+= sizeof(usecount_t);
  char *kb = pb;
  pb += lenb * phsz;
  len=Min(lena,lenb);

  int d = (*key_cmp)(ka, kb, len);
  if (d)
    return d;

  if (lena > lenb)
    return 1;
  if (lena < lenb)
    return -1;

  if (!is_en_word) {
  int tlena = utf8_tlen(pa, lena);
  int tlenb = utf8_tlen(pb, lenb);

  if (tlena > tlenb)
    return 1;
  if (tlena < tlenb)
    return -1;

  if ((d=memcmp(pa, pb, tlena)))
    return d;
  }

  // large first, so large one will be kept after delete
  return usecountb - usecounta;
}

static int qcmp_eq(const void *a, const void *b)
{
  int idxa=*((int *)a);  char *pa = (char *)&bf[idxa];
  int idxb=*((int *)b);  char *pb = (char *)&bf[idxb];
  char lena,lenb, len;

  lena=*(pa++); if (lena < 0) lena = -lena; pa+= sizeof(usecount_t);
  char *ka = pa;
  pa += lena * phsz;
  lenb=*(pb++); if (lenb < 0) lenb = -lenb; pb+= sizeof(usecount_t);
  char *kb = pb;
  pb += lenb * phsz;
  len=Min(lena,lenb);

  int d = (*key_cmp)(ka, kb, len);
  if (d)
    return d;

  if (lena > lenb)
    return 1;
  if (lena < lenb)
    return -1;

  if (is_en_word)
	return 0;

  int tlena = utf8_tlen(pa, lena);
  int tlenb = utf8_tlen(pb, lenb);

  if (tlena > tlenb)
    return 1;
  if (tlena < tlenb)
    return -1;

  return memcmp(pa, pb, tlena);
}

static int qcmp_usecount(const void *a, const void *b)
{
  int idxa=*((int *)a);  char *pa = (char *)&sf[idxa];
  int idxb=*((int *)b);  char *pb = (char *)&sf[idxb];
  char lena,lenb, len;
  usecount_t usecounta, usecountb;

  lena=*(pa++); if (lena < 0) lena = -lena; memcpy(&usecounta, pa, sizeof(usecount_t)); pa+= sizeof(usecount_t);
  lenb=*(pb++); if (lenb < 0) lenb = -lenb; memcpy(&usecountb, pb, sizeof(usecount_t)); pb+= sizeof(usecount_t);
  len=Min(lena,lenb);

  int d = (*key_cmp)(pa, pb, len);
  if (d)
    return d;
  pa += len*phsz;
  pb += len*phsz;

  if (lena > lenb)
    return 1;
  if (lena < lenb)
    return -1;

  if (!is_en_word) {
  // now lena == lenb
  int tlena = utf8_tlen(pa, lena);
  int tlenb = utf8_tlen(pb, lenb);

  if (tlena > tlenb)
    return 1;
  if (tlena < tlenb)
    return -1;
  }

  return usecountb - usecounta;
}

void send_gcin_message(Display *dpy, char *s);
#if WIN32 && 1
 #pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

void init_TableDir();

gboolean en_need_str(u_char *s)
{
  int len = strlen((char *)s);
  // 0,1 will be varied
  if (len < 3)
    return FALSE;

  gboolean has_l = FALSE, has_u = FALSE;
  int i;
  for(i=2;i<len;i++) {
	if (s[i] >= 127)
	  continue;

    if (islower(s[i]))
      has_l = TRUE;
    else if (isupper(s[i]))
      has_u = TRUE;
  }

  return has_l && has_u;
}

void add_one_line(char clen, usecount_t usecount, int chbufN, char *cphbuf, u_char *chbuf, gboolean b_en_need_str)
{
    if (phcount >= phidxsize) {
      phidxsize+=1024;
      if (!(phidx=(int *)realloc(phidx, phidxsize*sizeof(phidx[0])))) {
        puts("realloc err");
        exit(1);
      }
    }

    phidx[phcount++]=ofs;

//	dbg("phcount:%d  clen:%d\n", phcount, clen);

    int new_bfN = ofs + 1 + sizeof(usecount_t)+ phsz * clen + chbufN;

    if (bfsize < new_bfN) {
      bfsize = new_bfN + 1024*1024;
      bf = (char *)realloc(bf, bfsize);
    }

//    dbg("clen:%d\n", clen);

	char oclen = b_en_need_str ? -clen:clen;
    memcpy(&bf[ofs++], &oclen,1);

    memcpy(&bf[ofs],&usecount, sizeof(usecount_t)); ofs+=sizeof(usecount_t);

	memcpy(&bf[ofs], cphbuf, clen * phsz);
	ofs+=clen * phsz;

    memcpy(&bf[ofs], chbuf, chbufN);
    ofs+=chbufN;
}


int main(int argc, char **argv)
{
  FILE *fp,*fw;
  char s[1024];
  u_char chbuf[MAX_PHRASE_LEN * CH_SZ];
  char phbuf8[128];
  u_short phbuf[80];
  u_int phbuf32[80];
  u_int64_t phbuf64[80];
  int i,j,idx,len;
  u_short kk;
  u_int64_t kk64;
  int hashidx[TSIN_HASH_N];
  char clen;
  int lineCnt=0;
  int max_len = 0;
  gboolean reload = getenv("GCIN_NO_RELOAD")==NULL;

  if (reload) {
    dbg("need reload\n");
  } else {
    dbg("NO_GTK_INIT\n");
  }

  if (getenv("NO_GTK_INIT")==NULL)
    gtk_init(&argc, &argv);

  dbg("enter %s\n", argv[0]);

  if (argc < 2)
    p_err("must specify input file");


  init_TableDir();

  if ((fp=fopen(argv[1], "rb"))==NULL) {
     p_err("Cannot open %s\n", argv[1]);
  }

  skip_utf8_sigature(fp);
  char *outfile;
  int fofs = ftell(fp);
  myfgets(s, sizeof(s), fp);
  if (strstr(s, "!!pinyin")) {
    b_pinyin = TRUE;
    printf("is pinyin\n");
    load_pin_juyin();
  } else
    fseek(fp, fofs, SEEK_SET);

  fofs = ftell(fp);
  int keybits=0, maxkey=0;
  char keymap[128];
  char kno[128];
  bzero(kno, sizeof(kno));
  myfgets(s, sizeof(s), fp);
  puts(s);
  if (strstr(s, TSIN_GTAB_KEY)) {
    is_gtab = TRUE;
    lineCnt++;

    if (argc < 3)
      p_err("useage %s input_file output_file", argv[0]);

    outfile = argv[2];

    len=strlen((char *)s);
    if (s[len-1]=='\n')
      s[--len]=0;
    char aa[128];
    keymap[0]=' ';
    sscanf(s, "%s %d %d %s", aa, &keybits, &maxkey, keymap+1);
    for(i=0; keymap[i]; i++)
      kno[keymap[i]]=i;

    if (maxkey * keybits > 32)
      gtabkey64 = TRUE;
  } else
  if (strstr(s, TSIN_EN_WORD_KEY)) {
    if (argc==3)
      outfile = argv[2];
    else
      outfile = "en-american";

    is_en_word = TRUE;
	dbg("is_en_word = TRUE\n");
    lineCnt++;
  } else  {
    if (argc==3)
      outfile = argv[2];
    else
      outfile = "tsin32";

    fseek(fp, fofs, SEEK_SET);
  }

  INMD inmd, *cur_inmd = &inmd;

  char *cphbuf;
  if (is_gtab) {
    cur_inmd->keybits = keybits;
    if (gtabkey64) {
      cphbuf = (char *)phbuf64;
      phsz = 8;
      key_cmp = key_cmp64;
      hash_shift = TSIN_HASH_SHIFT_64;
      cur_inmd->key64 = TRUE;
    } else {
      cphbuf = (char *)phbuf32;
      phsz = 4;
      hash_shift = TSIN_HASH_SHIFT_32;
      key_cmp = key_cmp32;
      cur_inmd->key64 = FALSE;
    }
    cur_inmd->last_k_bitn = (((cur_inmd->key64 ? 64:32) / cur_inmd->keybits) - 1) * cur_inmd->keybits;
    dbg("cur_inmd->last_k_bitn %d\n", cur_inmd->last_k_bitn);
  } else if (is_en_word) {
	cphbuf = (char *)phbuf8;
	phsz = 1;
    key_cmp = key_cmp8;
  } else {
      cphbuf = (char *)phbuf;
      phsz = 2;
      key_cmp = key_cmp16;
      hash_shift = TSIN_HASH_SHIFT;
  }

  dbg("phsz: %d\n", phsz);

  phcount=ofs=0;
  while (!feof(fp)) {
    usecount_t usecount=0;

    lineCnt++;

    myfgets((char *)s,sizeof(s),fp);
    len=strlen((char *)s);
    if (s[0]=='#')
      continue;

    if (strstr(s, TSIN_GTAB_KEY) || strstr(s, TSIN_EN_WORD_KEY))
      continue;

    if (s[len-1]=='\n')
      s[--len]=0;

    if (len==0) {
      dbg("len==0\n");
      continue;
    }

    i=0;
    int chbufN=0;
    int charN = 0;

    if (!is_en_word) {
      while (s[i]!=' ' && i<len) {
        int len = utf8_sz((char *)&s[i]);

        memcpy(&chbuf[chbufN], &s[i], len);

        i+=len;
        chbufN+=len;
        charN++;
      }
	}

	gboolean b_en_need_str = FALSE;
	if (is_en_word) {
      b_en_need_str =  en_need_str((u_char *)s);
	}

    while (i < len && (s[i]==' ' || s[i]=='\t'))
      i++;

    int phbufN=0;
    while (i<len && (phbufN < charN || is_en_word) && (s[i]!=' ' || is_en_word) && s[i]!='\t') {
      if (is_gtab) {
        kk64=0;
        int idx=0;
        while (s[i]!=' ' && i<len) {
          int k = kno[s[i]];
          kk64|=(u_int64_t)k << ( LAST_K_bitN - idx*keybits);
          i++;
          idx++;
        }

        if (phsz==8)
          phbuf64[phbufN++]=kk64;
        else
          phbuf32[phbufN++]=(u_int)kk64;
      } else
	  if (is_en_word) {
		phbuf8[phbufN++]=s[i];
	  } else {
        kk=0;
        if (b_pinyin) {
          kk = pinyin2phokey(s+i);
          while (s[i]!=' ' && i<len)
            i++;
        } else {
          while (s[i]!=' ' && i<len) {
            if (kk==(BACK_QUOTE_NO << 9))
              kk|=s[i];
            else
              kk |= lookup((u_char *)&s[i]);

            i+=utf8_sz((char *)&s[i]);
          }
        }

        phbuf[phbufN++]=kk;
      }

      i++;
    }

    if (!is_en_word && phbufN!=charN) {
      dbg("%s   Line %d problem in phbufN!=chbufN %d != %d\n", s, lineCnt, phbufN, chbufN);
      continue;
    }

    clen=phbufN;

    while (i<len && (s[i]==' ' || s[i]=='\t'))
      i++;

    if (i==len)
      usecount = 0;
    else
      usecount = atoi((char *)&s[i]);

    /*      printf("len:%d\n", clen); */

#if 0
    if (phcount >= phidxsize) {
      phidxsize+=1024;
      if (!(phidx=(int *)realloc(phidx,phidxsize*4))) {
        puts("realloc err");
        exit(1);
      }
    }

    phidx[phcount++]=ofs;

//	dbg("phcount:%d  clen:%d\n", phcount, clen);

    int new_bfN = ofs + 1 + sizeof(usecount_t)+ phsz * clen + chbufN;

    if (bfsize < new_bfN) {
      bfsize = new_bfN + 1024*1024;
      bf = (char *)realloc(bf, bfsize);
    }

//    dbg("clen:%d\n", clen);

    memcpy(&bf[ofs++],&clen,1);
    memcpy(&bf[ofs],&usecount, sizeof(usecount_t)); ofs+=sizeof(usecount_t);

	memcpy(&bf[ofs], cphbuf, clen * phsz);
	ofs+=clen * phsz;

    memcpy(&bf[ofs], chbuf, chbufN);
    ofs+=chbufN;
#else
	add_one_line(clen, usecount, chbufN, cphbuf, chbuf, FALSE);
#if 1
	if (b_en_need_str) {
	  memcpy(chbuf, cphbuf, clen);
	  int i;
	  for(i=0;i<clen;i++)
		cphbuf[i]=tolower(cphbuf[i]);
	  add_one_line(clen, usecount, clen, cphbuf, chbuf, TRUE);
	}
#endif
#endif
  }
  fclose(fp);

  /* dumpbf(bf,phidx); */

  puts("Sorting ....");

  qsort(phidx,phcount, sizeof(phidx[0]), qcmp);

  if (!(sf=(u_char *)malloc(bfsize))) {
    puts("malloc err");
    exit(1);
  }

  if (!(sidx=(int *)malloc(phcount*sizeof(sidx[0])))) {
    puts("malloc err");
    exit(1);
  }

  dbg("phcount %d\n", phcount);
  printf("before delete duplicate N:%d\n", phcount);

  // delete duplicate
  ofs=0;
  j=0;
  for(i=0;i<phcount;i++) {
    idx = phidx[i];
    sidx[j]=ofs;
    len=bf[idx];
    gboolean en_has_str = FALSE;
    if (len < 0) {
      len = -len;
      en_has_str = TRUE;
    }
    int tlen = (is_en_word && !en_has_str)?0:utf8_tlen(&bf[idx + 1 + sizeof(usecount_t) + phsz*len], len);
//    printf("tlen %d phsz:%d len:%d\n", tlen, phsz, len);
    int clen= phsz*len + tlen + 1 + sizeof(usecount_t);

//    printf("clen %d\n", clen);

    if (i && !qcmp_eq(&phidx[i-1], &phidx[i])) {
      continue;
    }

    if (max_len < len)
      max_len = len;

    memcpy(&sf[ofs], &bf[idx], clen);
    j++;
    ofs+=clen;
  }

  phcount=j;
  dbg("after delete duplicate N:%d  max_len:%d\n", phcount, max_len);
  printf("after delete duplicate N:%d  max_len:%d\n", phcount, max_len);

#if 1
  puts("Sorting by usecount ....");
  qsort(sidx, phcount, sizeof(sidx[0]), qcmp_usecount);
  dbg("after qcmp_usecount\n");
#endif

  for(i=0;i<TSIN_HASH_N;i++)
    hashidx[i]=-1;

  for(i=0;i<phcount;i++) {
    idx=sidx[i];
    idx+= 1 + sizeof(usecount_t);
    int v;

	if (phsz==1) {
	  v = 0;
	  memcpy(&v, &sf[idx], phsz);
	} else if (phsz==2) {
      phokey_t kk;
      memcpy(&kk, &sf[idx], phsz);
      v = kk >> TSIN_HASH_SHIFT;
    } else if (phsz==4) {
      u_int kk32;
      memcpy(&kk32, &sf[idx], phsz);
      v = kk32 >> TSIN_HASH_SHIFT_32;
    }
    else if (phsz==8) {
      u_int64_t kk64;
      memcpy(&kk64, &sf[idx], phsz);
      v = kk64 >> TSIN_HASH_SHIFT_64;
    }

    if (v >= TSIN_HASH_N)
      p_err("error found %d", v);

    if (hashidx[v] < 0) {
      hashidx[v]=i;
    }
  }
  dbg("-------------------\n");


  if (hashidx[0]==-1)
    hashidx[0]=0;

  hashidx[TSIN_HASH_N-1]=phcount;
  for(i=TSIN_HASH_N-2;i>=0;i--) {
    if (hashidx[i]==-1)
      hashidx[i]=hashidx[i+1];
  }

  for(i=1; i< TSIN_HASH_N; i++) {
    if (hashidx[i]==-1)
      hashidx[i]=hashidx[i-1];
  }

  printf("Writing data %s %d\n", outfile, ofs);
  if ((fw=fopen(outfile,"wb"))==NULL) {
    p_err("create err %s", outfile);
  }


  TSIN_GTAB_HEAD head;
  bzero(&head, sizeof(head));
  if (is_gtab) {
    strcpy(head.signature, TSIN_GTAB_KEY);
    head.keybits = keybits;
    head.maxkey = maxkey;
    strcpy(head.keymap, keymap);
    fwrite(&head, sizeof(head), 1, fw);
  } else
  if (is_en_word) {
    strcpy(head.signature, TSIN_EN_WORD_KEY);
    fwrite(&head, sizeof(head), 1, fw);
  }

  fwrite(sf,1,ofs,fw);
  fclose(fw);

  char outfileidx[512];
  strcat(strcpy(outfileidx, outfile), ".idx");

  dbg("Writing data %s\n", outfileidx);
  if ((fw=fopen(outfileidx,"wb"))==NULL) {
    p_err("cannot create %s", outfileidx);
  }

  fwrite(&phcount,4,1,fw);
  fwrite(hashidx,1,sizeof(hashidx),fw);
  fwrite(sidx,4,phcount,fw);
  printf("%d phrases\n",phcount);

  fclose(fw);
  free(sf);
  free(bf);


  if (reload) {
    printf("reload....\n");
    send_gcin_message(
#if UNIX
    GDK_DISPLAY(),
#endif
    RELOAD_TSIN_DB);
  }

  exit(0);
}
