#include "gcin.h"
#include <sys/stat.h>
#if UNIX
#include <linux/limits.h>
#endif

void update_table_file(char *name, int version)
{
#if UNIX
  char fname_user[PATH_MAX];
  char fname_version[PATH_MAX];
  char fname_sys[PATH_MAX];
  char version_name[PATH_MAX];

  strcat(strcpy(version_name, name), ".version");
  get_gcin_user_fname(version_name, fname_version);
  get_gcin_user_fname(name, fname_user);
  get_sys_table_file_name(name, fname_sys);

  FILE *fp;
  if ((fp=fopen(fname_version, "r"))) {
    int ver=0;
    fscanf(fp, "%d", &ver);
    fclose(fp);

    if (ver >= version)
      return;
  }

  char cmd[6*PATH_MAX];
  snprintf(cmd, sizeof(cmd), "mv -f %s %s.old && cp %s %s && echo %d > %s", fname_user, fname_user,
      fname_sys, fname_user, version, fname_version);
  dbg("exec %s\n", cmd);
  system(cmd);
#endif
}
