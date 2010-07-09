#include "gcin.h"
#include "gtab.h"
int gcin_switch_keys_lookup(int key);

INMD inmd[MAX_GTAB_NUM_KEY+1];

char gtab_list[]=GTAB_LIST;

struct {
  char *id;
  char method_type;
} method_codes[] = {
 {"!PHO", method_type_PHO},
 {"!TSIN", method_type_TSIN},
 {"!INT_CODE", method_type_INT_CODE},
 {"!ANTHY", method_type_ANTHY},
 {NULL}
};

void load_gtab_list()
{
  char ttt[128];
  FILE *fp;

  inmd[3].method_type = method_type_PHO;
  inmd[6].method_type = method_type_TSIN;
  inmd[10].method_type = method_type_INT_CODE;
  inmd[12].method_type = method_type_ANTHY;

  get_gcin_user_fname(gtab_list, ttt);

  if ((fp=fopen(ttt, "rb"))==NULL) {
    get_sys_table_file_name(gtab_list, ttt);
    if ((fp=fopen(ttt, "rb"))==NULL)
      p_err("cannot open %s", ttt);
  }

  dbg("load_gtab_list %s\n", ttt);

  skip_utf8_sigature(fp);

  while (!feof(fp)) {
    char line[256];
    char name[32];
    char key[32];
    char file[32];
    char icon[128];

    name[0]=0;
    key[0]=0;
    file[0]=0;
    icon[0]=0;

    line[0]=0;
    myfgets(line, sizeof(line), fp);
    if (strlen(line) < 2)
      continue;

    if (line[0]=='#')
      continue;

    sscanf(line, "%s %s %s %s", name, key, file, icon);
    if (strlen(name) < 1)
      break;

    int keyidx = gcin_switch_keys_lookup(key[0]);
    if (keyidx < 0)
      p_err("bad key value %s in %s\n", key, ttt);

    free(inmd[keyidx].filename);
    inmd[keyidx].filename = strdup(file);
    int i;
    for(i=0; method_codes[i].id; i++)
      if (!strcmp(file, method_codes[i].id))
        break;
    if (method_codes[i].id)
      inmd[keyidx].method_type = method_codes[i].method_type;

    free(inmd[keyidx].cname);
    inmd[keyidx].cname = strdup(name);

    free(inmd[keyidx].icon);
    if (strlen(icon))
      inmd[keyidx].icon = strdup(icon);
  }
  fclose(fp);

}
