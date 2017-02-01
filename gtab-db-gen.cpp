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
#include "gtab-db.h"


static char *bf;
static int bfN_a = 0, ofs=0;
static gboolean b_pinyin;

int *phidx, *sidx, phcount;
int bfsize, phidxsize;
u_char *sf;
gboolean is_gtab, gtabkey64;
int phsz, hash_shift;
int (*key_cmp)(char *a, char *b, char len);
char **textArr;
int textArrN = 0, textArrN_a;
int *textPhyOfs;
FILE *fw;

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
  int text_idxa, text_idxb;

  lena=*(pa++); memcpy(&usecounta, pa, sizeof(usecount_t)); pa+= sizeof(usecount_t);memcpy(&text_idxa, pa, sizeof(text_idxa)); pa+=sizeof(text_idxa);  
  char *ka = pa;
//  pa += lena * phsz;
  lenb=*(pb++); memcpy(&usecountb, pb, sizeof(usecount_t)); pb+= sizeof(usecount_t);memcpy(&text_idxb, pb, sizeof(text_idxb)); pb+=sizeof(text_idxb);
  char *kb = pb;
//  pb += lenb * phsz;
  len=Min(lena,lenb);

  int d = (*key_cmp)(ka, kb, len);
  if (d)
    return d;

  if (lena > lenb)
    return 1;
  if (lena < lenb)
    return -1;

  int tlena = strlen(textArr[text_idxa]);
  int tlenb = strlen(textArr[text_idxb]);

  if (tlena > tlenb)
    return 1;
  if (tlena < tlenb)
    return -1;

  if ((d=memcmp(pa, pb, tlena)))
    return d;

  // large first, so large one will be kept after delete
  return usecountb - usecounta;
}

static int qcmp_eq(const void *a, const void *b)
{
  int idxa=*((int *)a);  char *pa = (char *)&bf[idxa];
  int idxb=*((int *)b);  char *pb = (char *)&bf[idxb];
  char lena,lenb, len;
  int text_idxa, text_idxb;

  lena=*(pa++); if (lena < 0) lena = -lena; pa+= sizeof(usecount_t);memcpy(&text_idxa, pa, sizeof(text_idxa)); pa+=sizeof(text_idxa);
  char *ka = pa;
//  pa += lena * phsz;
  lenb=*(pb++); if (lenb < 0) lenb = -lenb; pb+= sizeof(usecount_t);memcpy(&text_idxb, pb, sizeof(text_idxb)); pb+=sizeof(text_idxb);
  char *kb = pb;
//  pb += lenb * phsz;
  len=Min(lena,lenb);

  int d = (*key_cmp)(ka, kb, len);
  if (d)
    return d;

  if (lena > lenb)
    return 1;
  if (lena < lenb)
    return -1;

  int tlena = strlen(textArr[text_idxa]);
  int tlenb = strlen(textArr[text_idxb]);
  
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
  int text_idxa, text_idxb;

  lena=*(pa++); memcpy(&usecounta, pa, sizeof(usecount_t)); pa+= sizeof(usecount_t); memcpy(&text_idxa, pa, sizeof(text_idxa)); pa+=sizeof(text_idxa);
  lenb=*(pb++); memcpy(&usecountb, pb, sizeof(usecount_t)); pb+= sizeof(usecount_t); memcpy(&text_idxb, pb, sizeof(text_idxb)); pb+=sizeof(text_idxb);
  len=Min(lena,lenb);

  int d = (*key_cmp)(pa, pb, len);
  if (d)
    return d;
#if 0    
  pa += len*phsz;
  pb += len*phsz;
#endif
  if (lena > lenb)
    return 1;
  if (lena < lenb)
    return -1;

  // now lena == lenb
  int tlena = strlen(textArr[text_idxa]);
  int tlenb = strlen(textArr[text_idxb]);  

  if (tlena > tlenb)
    return 1;
  if (tlena < tlenb)
    return -1;

  return usecountb - usecounta;
}

void send_gcin_message(Display *dpy, char *s);
#if WIN32 && 1
 #pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

void init_TableDir();

static int qcmp_strcmp(const void *aa, const void *bb) {
	const char **a = (const char **)aa;
	const char **b = (const char **)bb;
	return strcmp(*a, *b);
}

static int find_text(char *s) {
	char **p = bsearch(&s, textArr, textArrN, sizeof(char *), qcmp_strcmp);
	if (!p)
		return -1;
	return p - textArr;
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
    int new_bfN = ofs + 1 + sizeof(usecount)+ sizeof(int) + phsz * clen;

    if (bfsize < new_bfN) {
      bfsize = new_bfN + 1024*1024;
      bf = (char *)realloc(bf, bfsize);
    }

//    dbg("clen:%d\n", clen);
    
	char oclen = clen;
    memcpy(&bf[ofs++], &oclen,1);
    memcpy(&bf[ofs],&usecount, sizeof(usecount_t)); ofs+=sizeof(usecount_t);
	int text_idx = find_text(chbuf);
	if (text_idx < 0)
		p_err("not found '%s'", chbuf);
	memcpy(&bf[ofs], &text_idx, sizeof(text_idx)); 
	ofs+=sizeof(text_idx);
	memcpy(&bf[ofs], cphbuf, clen * phsz);
	ofs+=clen * phsz;
#if 0
    memcpy(&bf[ofs], chbuf, chbufN);
    ofs+=chbufN;
#endif
}


static int prefix_eq(int idxa, int idxb, int preLen)
{
  char *pa = (char *)&sf[sidx[idxa]];
  char *pb = (char *)&sf[sidx[idxb]];	
  char lena,lenb, len;
  usecount_t usecounta, usecountb;
  int text_idxa, text_idxb;

  lena=*(pa++); pa+= sizeof(usecount_t)+sizeof(text_idxa);  
  char *ka = pa;
  pa += lena * phsz;
  lenb=*(pb++); pb+= sizeof(usecount_t)+sizeof(text_idxb);
  char *kb = pb;
  pb += lenb * phsz;
  len=Min(lena,lenb);
  if (len > preLen)
	len = preLen;

  return (*key_cmp)(ka, kb, len)==0;
}
  

int gen_tree(int start, int end, int prelen) {	
//	dbg("gen_tree %d %d %d\n", start, end, prelen);
	int prelen1 = prelen+1;
	if (start>=end)
		p_err("error found %d %d", start, end);
	// start is always included
	fseek(fw, 0, SEEK_END);	
	int start_ofs = ftell(fw);		
	BLOCK_HEAD bh;
	bzero(&bh, sizeof(bh));
	fwrite(&bh, sizeof(bh), 1, fw);
	GNODE gn;
	bzero(&gn, sizeof(gn));		
	for(int i=start;i<end;i++) {
		 char *p = (char *)&sf[sidx[i]];
		 int len = *p;
//		 dbg("%d[ len %d\n",i, len);		 
		 int text_idx;
		 p+= sizeof(usecount_t); memcpy(&text_idx, p, sizeof(text_idx)); p+=sizeof(text_idx);		 
		 
		 if (len < prelen)
			p_err("A gen_tree err %d %d prelen:%d %d] len:%d", start, end, prelen, i, len);

		if (len == prelen) {
			gn.key = 0;
			gn.link = textPhyOfs[i];
			fwrite(&gn, sizeof(gn), 1, fw);
			bh.N++;
		} else		 
		if (i==start || !prefix_eq(i, i-1, prelen1)) {
			gn.link = 0;
			memcpy(&gn.key, p + phsz * prelen, phsz);						
			fwrite(&gn, sizeof(gn), 1, fw);
			bh.N++;
		}
	}
	fseek(fw, start_ofs, SEEK_SET);
	// bh.N is at offset 0
//	dbg("bh.N %d\n", bh.N);
	fwrite(&bh.N, sizeof(bh.N), 1, fw);
					
	int g_idx;
	for(int i=start;i<end;) {
		 char *p = (char *)&sf[sidx[i]];
		 int len = *p;
//		 dbg("b len %d\n", len);
		 
		 if (len < prelen)
			p_err("B gen_tree err %d %d %d", start, end, prelen);

#if 1
		 if (len==prelen) {			 
			 i++;
			 g_idx++;
			 continue;			 
		 }
#endif		 
		 
		 int j;
		 for(j=i+1;j<end;j++) {
			char *pj = (char *)&sf[sidx[j]];
			int lenj = *pj;
//			dbg("%d] lenj %d\n", j, lenj);
#if 1
			if (lenj < prelen1)
			  continue;
#endif
			if (!prefix_eq(j, j-1, prelen1))
				break;
		 }
				
		if (j<=i) {
		  dbg("j<=i");
		  break;
		}
		  
		int ofs = gen_tree(i, j, prelen1);
		fseek(fw, start_ofs+sizeof(bh)+ g_idx * sizeof(GNODE), SEEK_SET);
		fwrite(&ofs, sizeof(int), 1, fw);
		
		g_idx++;
		i=j;
	}	
	
	return start_ofs;
//	char *blk = malloc(sizeof(BLOCK_HEAD)+ sizeof(GNODE) * prefixN);
}	

int main(int argc, char **argv)
{
  FILE *fp;
  char s[1024];
  u_char chbuf[MAX_PHRASE_LEN * CH_SZ];
  char phbuf8[128];
//  u_short phbuf[80];
  u_int phbuf32[80];
  u_int64_t phbuf64[80];
  int i,j,idx,len;
//  u_short kk;
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
#if 0
    if (argc < 3)
      p_err("useage %s input_file output_file", argv[0]);

    outfile = argv[2];
#else
	outfile = "t.gtt";
#endif    

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
  }

  dbg("phsz: %d\n", phsz);
  
  fofs = ftell(fp);

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

	char *p = strchr(s, ' ');
	if (!p)
		continue;
	*p = 0;
	
    if (textArrN >= textArrN_a) {
		textArrN_a += 1024;
		textArr = trealloc(textArr, char *, textArrN_a);
	}
	textArr[textArrN++]=strdup(s);
  }
  dbg("textArrN %d\n", textArrN);
  qsort(textArr, textArrN, sizeof(char *), qcmp_strcmp);
  int ntextArrN=1;
  for(int i=1;i<textArrN;i++)
	if (strcmp(textArr[i], textArr[i-1]))
		textArr[ntextArrN++]=textArr[i];
		
  dbg("textArrN %d\n", textArrN);
  textArrN = ntextArrN;
  textPhyOfs = tmalloc(int, textArrN);

  fw = fopen(outfile, "wb");
  if (fw==NULL)
	p_err("cannot create");
  DBHEAD dbhead;
  bzero(&dbhead, sizeof(dbhead));
  fwrite(&dbhead, sizeof(dbhead), 1, fw);
  int ofs;
  BLOCK_HEAD bh;
  bzero(&bh, sizeof(bh));
  bh.N = textArrN;
  Stext ste;
  bzero(&ste, sizeof(ste));
  
  for(int i=0;i<textArrN;i++) {
	  ste.len = strlen(textArr[i]);
	  textPhyOfs[i] = ftell(fw);
	  fwrite(&ste, sizeof(ste), 1, fw);
	  fwrite(textArr[i], 1, ste.len, fw);
  }

  dbg("textArrN %d\n", textArrN);

  fseek(fp, fofs, SEEK_SET);
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

//    if (!is_en_word) 
    {
      while (s[i]!=' ' && i<len) {
        int len = utf8_sz((char *)&s[i]);

        memcpy(&chbuf[chbufN], &s[i], len);

        i+=len;
        chbufN+=len;
        charN++;
      }
	}

	chbuf[chbufN]=0;

//boolean b_en_need_str = FALSE;
	
    while (i < len && (s[i]==' ' || s[i]=='\t'))
      i++;

    int phbufN=0;
    while (i<len && (phbufN < charN) && (s[i]!=' ') && s[i]!='\t') {
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
      }

      i++;
    }

    if (phbufN!=charN) {
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

	add_one_line(clen, usecount, chbufN, cphbuf, chbuf, FALSE);
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
//    printf("tlen %d phsz:%d len:%d\n", tlen, phsz, len);
    int clen= phsz*len + sizeof(int) + 1 + sizeof(usecount_t);

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

  dbg("---------------------------\n");
  // We already knows the min phrase len is 1
  int root=gen_tree(0, phcount, 0);  
  dbhead.h.gnode_root_ofs = root;
  fwrite(&dbhead, sizeof(dbhead), 1, fw);  

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
