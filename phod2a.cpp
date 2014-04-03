/*
	Copyright (C) 1994-2005	Edward Der-Hua Liu, Hsin-Chu, Taiwan
*/


#include "gcin.h"
#include "pho.h"
#include "gcin-conf.h"

gboolean is_chs;
extern char *pho_phrase_area;

int main(int argc, char **argv)
{
  int i;

  gtk_init(&argc, &argv);

  load_setttings();

  if (argc > 1) {
    p_err("Currently only support ~/.gcin/pho.tab2");
  }

  pho_load();

  for(i=0; i < idxnum_pho; i++) {
    phokey_t key = idx_pho[i].key;
    int frm = idx_pho[i].start;
    int to = idx_pho[i+1].start;

    int j;
    for(j=frm; j < to; j++) {
      prph(key);
      char *str = pho_idx_str(j), tt[512];
      if (str[0]==PHO_PHRASE_ESCAPE) {
		  int ofs = str[1] | (str[1]<<8) | (tt[1] << 16);
		  int len = pho_phrase_area[ofs];
		  memcpy(tt, pho_phrase_area+ofs+1, len);
		  tt[len]=0;
      }
      
      printf(" %s %d\n", str, ch_pho[j].count);
    }
  }

  return 0;
}
