#include "gcin.h"
#include "gtab.h"
#include "gtab-list.h"
int gcin_switch_keys_lookup(int key);

INMD *inmd;
int inmdN;

char gtab_list[]=GTAB_LIST;

GTAB_LIST_S method_codes[] = {
 {"!PHO", method_type_PHO},
 {"!TSIN", method_type_TSIN},
 {"!SYMBOL_TABLE", method_type_SYMBOL_TABLE},
 {"!EN", method_type_EN},
 {NULL}
};

extern char *default_input_method_str;

void load_gtab_list(gboolean skip_disabled)
{
  char ttt[128];
  FILE *fp;

  get_gcin_user_fname(gtab_list, ttt);

  if ((fp=fopen(ttt, "rb"))==NULL) {
    get_sys_table_file_name(gtab_list, ttt);
    if ((fp=fopen(ttt, "rb"))==NULL)
      p_err("cannot open %s", ttt);
  }

  dbg("load_gtab_list %s\n", ttt);

  skip_utf8_sigature(fp);

  int i;
  for (i=0; i < inmdN; i++) {
    INMD *pinmd = &inmd[i];
    free(pinmd->filename); pinmd->filename=NULL;
    free(pinmd->cname); pinmd->cname=NULL;
    free(pinmd->icon); pinmd->icon=NULL;
  }

  inmdN = 0;


  char *def_file = strrchr(default_input_method_str, ' ');
  if (def_file)
    def_file++;

  while (!feof(fp)) {
    char line[256];
    char name_ar[32], *name=name_ar;
    char key[32];
    char file[32];
    char icon[128];

    inmd = trealloc(inmd, INMD, inmdN);

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

    if (skip_disabled && line[0]=='!')
      continue;


    sscanf(line, "%s %s %s %s", name, key, file, icon);
//    dbg("%s %c\n", line, key[0]);

    if (strlen(name) < 1)
      break;

    int inmd_idx;
    INMD *pinmd = &inmd[inmd_idx = inmdN++];
    bzero(pinmd, sizeof(INMD));
    pinmd->key_ch = key[0];

    pinmd->in_cycle = strchr(gcin_str_im_cycle, key[0]) != NULL;
//    dbg("%d %d '%c'\n",inmdN, pinmd->in_cycle, pinmd->key_ch);


    if (!strcmp(file, "!ANTHY")) {
#if UNIX
       strcpy(file, "anthy-module.so");
#else
       strcpy(file, "anthy-module.dll");
#endif
    }

    if (!strcmp(file, "!INT_CODE")) {
#if UNIX
       strcpy(file, "intcode-module.so");
#else
       strcpy(file, "intcode-module.dll");
#endif
    }

    pinmd->filename = strdup(file);

    if (strstr(file, ".so") || strstr(file, ".dll")) {
      pinmd->method_type = method_type_MODULE;
      dbg("%s is module file\n", file);
    } else {
      int i;
      for(i=0; method_codes[i].id; i++)
        if (!strcmp(file, method_codes[i].id))
          break;
      if (method_codes[i].id)
        pinmd->method_type = method_codes[i].method_type;
    }

    if (name[0]=='!') {
      name++;
      pinmd->disabled = TRUE;
    }

    if (default_input_method_str[0]==key[0] && !pinmd->disabled && (!def_file || !strcmp(file, def_file))) {
      default_input_method = inmd_idx;
      dbg("default_input_method %s %s %s %d\n", name,
         default_input_method_str, key, default_input_method);
    }

    pinmd->cname = strdup(name);

    if (strlen(icon))
      pinmd->icon = strdup(icon);
  }
  fclose(fp);

}


int gcin_switch_keys_lookup(int key)
{
  int i;

  for(i=0;i<inmdN;i++)
    if (inmd[i].key_ch==key)
      return i;

  return -1;
}
