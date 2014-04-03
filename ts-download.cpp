#if UNIX
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
typedef int SOCKET;
#define closesocket(a) close(a)
#else
#include <WinSock2.h>
#include <ws2tcpip.h>
#define write(a,b,c) send(a,b,c,0)
#define read(a,b,c) recv(a,b,c,0)
#endif

#include "gcin.h"
#include "pho.h"
#include "config.h"
#if GCIN_i18n_message
#include <libintl.h>
#endif
#include "lang.h"
#include "tsin.h"
#include "gtab.h"
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(2,90,7)
#include <gdk/gdkkeysyms-compat.h>
#endif
#include "ts-share.h"
#include "gcin-conf.h"

extern int tsN;
void load_tsin_at_ts_idx(int ts_row, char *len, usecount_t *usecount, void *pho, u_char *ch);

char *get_tag();
extern gboolean b_en;
int connect_ts_share_svr();
extern char downloaded_file_src[];
void send_format(SOCKET sock);
int get_key_sz();
void write_tsin_src(FILE *fw, char len, phokey_t *pho, char *s);
FILE *en_file_head(char *fname);

void ts_download()
{
  int sock = connect_ts_share_svr();

  REQ_HEAD head;
  bzero(&head, sizeof(head));
  head.cmd = REQ_DOWNLOAD2;
  write(sock, (char *)&head, sizeof(head));

  REQ_DOWNLOAD_S req;
  bzero(&req, sizeof(req));
  strcpy(req.tag, get_tag());

static char DL_CONF[]="ts-share-download-time";
static char DL_CONF_EN[]="ts-share-download-time-en";

  char *dl_conf = b_en?DL_CONF_EN:DL_CONF;

  req.last_dl_time = get_gcin_conf_int(dl_conf, 0);
 // req.last_dl_time = 0;
  int rn, wn;
  wn = write(sock, (char*)&req, sizeof(req));

  REQ_DOWNLOAD_REPLY_S rep;
  rn = read(sock, (char*)&rep, sizeof(rep));

  send_format(sock);

  FILE *fw;

  if (b_en)
    fw = en_file_head(downloaded_file_src);
  else
  if ((fw=fopen(downloaded_file_src,"a"))==NULL)
    p_err("cannot create %s:%s", downloaded_file_src, sys_err_strA());

  int key_sz =  get_key_sz();

  int N=0;
  for(;;) {
    char len=0;
    rn = read(sock, &len, sizeof(len));
    if (len<=0)
      break;
    phokey_t pho[128];
    rn = read(sock, (char*)pho, len * key_sz);

	if (b_en) {
	  char *s = (char *)pho;
	  s[len]=0;
	  save_phrase_to_db(&en_hand, pho, NULL, len, 0);
	  write_tsin_src(fw, len, NULL, s);
	} else {
      char slen=0;
      rn = read(sock, &slen, sizeof(slen));
      char s[256];
      rn = read(sock, s, slen);
      s[slen]=0;
      save_phrase_to_db(&tsin_hand, pho, s, len, 0);
	  write_tsin_src(fw, len, pho, s);
	}

    N++;
  }

  dbg("N:%d\n", N);
  if (N)
    save_gcin_conf_int(dl_conf, (int)rep.this_dl_time);

  fclose(fw);

  closesocket(sock);
}
