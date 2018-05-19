#include "gcin.h"
#include "pho.h"

void utf8_putchar_fp(FILE *fp, char *s);
void utf8_dbg(char *s);


void prph2_fp(FILE *fp, phokey_t kk)
{
  u_int k[4];
  phokey_t okk = kk;

  k[3]=(kk&7);
  kk>>=3;
  k[2]=(kk&15) * PHO_CHAR_LEN;
  kk>>=4;
  k[1]=(kk&3) * PHO_CHAR_LEN;
  kk>>=2;
  k[0]=(kk&31) * PHO_CHAR_LEN;


  if (k[0]==BACK_QUOTE_NO*PHO_CHAR_LEN) {
    utf8_putchar(&pho_chars[0][k[0]]);
    char c = okk & 0x7f;
    if (c > ' ')
      fprintf(fp, "%c", c);
  } else {
    int i;
    for(i=0; i < 3; i++) {
      if (!k[i])
        continue;
      utf8_putchar_fp(fp, &pho_chars[i][k[i]]);
    }

    if (k[3])
      fprintf(fp, "%d", k[3]);
  }
}


void prph2(phokey_t kk)
{
  u_int k[4];
  phokey_t okk = kk;

  k[3]=(kk&7);
  kk>>=3;
  k[2]=(kk&15) * PHO_CHAR_LEN;
  kk>>=4;
  k[1]=(kk&3) * PHO_CHAR_LEN;
  kk>>=2;
  k[0]=(kk&31) * PHO_CHAR_LEN;

  if (k[0]==BACK_QUOTE_NO*PHO_CHAR_LEN) {
    utf8_dbg(&pho_chars[0][k[0]]);
    char c = okk & 0x7f;
    if (c > ' ')
	  utf8_dbg(&c);
  } else {
    int i;
    for(i=0; i < 3; i++) {
      if (!k[i])
        continue;
	  utf8_dbg(&pho_chars[i][k[i]]);
    }

    if (k[3])
	  dbg("%d", k[3]);
  }
}

extern FILE *dbgfp;

void prph(phokey_t kk)
{
#if UNIX
	prph2(kk);
#else
#if DEBUG
#if USE_FP
	prph2(dbgfp, kk);
#else
	prph2(kk);
#endif
#else
	prph2(kk);
#endif
#endif
}


void prphs(phokey_t *ks, int ksN)
{
  int i;
  for(i=0;i<ksN;i++) {
    prph(ks[i]); dbg(" ");
  }
}

