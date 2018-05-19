#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gcin.h"
#include "pho.h"
#include "tsin.h"

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

static inline int edit_dist(char *s1, int s1len, char *s2, int s2len) {
    unsigned int x, y;
    unsigned int matrix[128][128];
    matrix[0][0] = 0;
    for (x = 1; x <= s2len; x++)
        matrix[x][0] = matrix[x-1][0] + 1;
    for (y = 1; y <= s1len; y++)
        matrix[0][y] = matrix[0][y-1] + 1;
    for (x = 1; x <= s2len; x++)
        for (y = 1; y <= s1len; y++)
            matrix[x][y] = MIN3(matrix[x-1][y] + 1, matrix[x][y-1] + 1, matrix[x-1][y-1] + (s1[y-1] == s2[x-1] ? 0 : 1));

    return(matrix[s2len][s1len]);
}


#define MaxCand 10
void get_en_miss_cand(FILE *fp, char *s, int slen, char best[][MAX_PHRASE_LEN]) {
#define phsz 1
  dbg("get_en_miss_cand %s", s);

  fseek(fp, sizeof(TSIN_GTAB_HEAD), SEEK_SET);

#define MCOST  0x3f00000

  int bestd[MaxCand];
  int midx=0;

  int i;
  for(i=0;i<MaxCand;i++) {
    best[i][0]=0;
    bestd[i]=MCOST;
  }

  int is_lower=s[0]>='a' && s[0]<='z';
  int is_upper=s[0]>='A' && s[0]<='Z';

  while (!feof(fp)) {
    char ln[512];
    signed char clen;
    if (fread(&clen,1,1,fp) <= 0)
      break;

    if (!clen) {
	  dbg("!clen");
      break;
    }

    gboolean en_has_str = FALSE, is_deleted=FALSE;
    usecount_t usecount;

    if (clen < 0) {
      clen = - clen;
      en_has_str = TRUE;
    }

    if (fread(&usecount, sizeof(usecount_t), 1, fp) <=0) {
      dbg("usecount_t");
      break;
    }

    if (usecount < 0) {
      is_deleted = TRUE;
    }

    ln[0]=0;
    if (fread(ln, phsz, clen, fp) < 0) {
       break;
    }
     ln[clen]=0;
//        dbg("%s %d\n", phbuf8, usecount);


    char tt[128];
    tt[0]=0;
    if (en_has_str) {
      int ttlen=0;
      tt[0]=0;

      for(i=0;i<clen;i++) {
        char ch[CH_SZ];
        int n = fread(ch, 1, 1, fp);
        if (n<=0) {
		  dbg("n<=0 a\n");
          break;
        }
        int len=utf8_sz(ch);
        if (len > 1) {
		  if (fread(&ch[1], 1, len-1, fp)<=0) {
			dbg("n<=0 b\n");
			break;
		  }
        }
        memcpy(tt+ttlen, ch, len);
        ttlen+=len;
      }

      tt[ttlen]=0;
      if (!tt[0])
        continue;
    }


   if (en_has_str || is_deleted || clen >= MAX_PHRASE_LEN)
     continue;

//   dbg("- %s\n", ln);

    int len=strlen(ln);
    if (len < slen - 5)
      continue;

#define SHIFT 16

	if (is_upper) {
      if (ln[0]>='a' && ln[0]<='z') {
        ln[0]-=0x20;
      }
	}

	int cost;
    int d = edit_dist(s, slen, ln, len);
    if (d>=slen*2)
      continue;
    cost = d<<SHIFT;

    if (is_lower) {
      if (ln[0]>='A' && ln[0]<='Z') {
        ln[0]+=0x20;
        int ncost = (edit_dist(s, slen, ln, len) << SHIFT) + 0x100;
        ln[0]-=0x20;
        if (cost > ncost)
          cost = ncost;
      }
    }
    
    cost -= usecount;

    
    if (cost >= bestd[midx])
    	continue;

    strcpy(best[midx], ln);
    bestd[midx]=cost;

    midx = 0;
    for(int i =1;i<MaxCand;i++)
      if (bestd[midx]<bestd[i])
         midx = i;
  }
}
