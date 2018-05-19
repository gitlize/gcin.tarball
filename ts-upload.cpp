#define USE_HTTP 1

#if !USE_HTTP
#if UNIX
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
typedef int SOCKET;
#define closesocket(a) close(a)
#else
#include <WinSock2.h>
#include <ws2tcpip.h>
#define write(a,b,c) send(a,b,c,0);
#endif
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

extern int tsN;
void load_tsin_at_ts_idx(int ts_row, char *len, usecount_t *usecount, void *pho, u_char *ch);

#if WIN32
char *err_strA(DWORD dw);
char *sock_err_strA()
{
	return err_strA(WSAGetLastError());
}
#endif

#if !USE_HTTP
int connect_ts_share_svr()
{
    SOCKET ConnectSocket = -1;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    int iResult;

    bzero( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_s[8];
    sprintf(port_s, "%d", TS_SHARE_SERVER_PORT);

    iResult = getaddrinfo(TS_SHARE_SERVER, port_s, &hints, &result);
    if ( iResult != 0 ) {
#if UNIX
        p_err("getaddrinfo failed: %s\n", sys_err_strA());
#else
		p_err("getaddrinfo failed: %s\n", sock_err_strA());
#endif
    }

    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket < 0) {
            dbg("Error at socket(): %s\n", sys_err_strA());
            continue;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult < 0) {
            closesocket(ConnectSocket);
            ConnectSocket = -1;
            continue;
        }
        break;
    }

    if (ConnectSocket <= 0) {
      p_err("cannot connect to %s:%d", TS_SHARE_SERVER, TS_SHARE_SERVER_PORT, sys_err_strA());
    }

    freeaddrinfo(result);
    return ConnectSocket;
}
#endif

extern char contributed_file_src[];
extern gboolean b_en;
char *get_tag();
int get_key_sz();

void write_tsin_src(FILE *fw, char len, phokey_t *pho, char *s)
{
  fprintf(fw, "%s", s);
  if (pho) {
    int j;
    for(j=0;j<len; j++)
      fprintf(fw, " %s", phokey_to_str2(pho[j], TRUE));
  }

  if (b_en)
	fprintf(fw, "\t0\n");
  else
	fprintf(fw, " 0\n");
}

#if !USE_HTTP
void send_format(SOCKET sock)
{
  REQ_FORMAT format;
  bzero(&format, sizeof(format));
  format.key_sz = get_key_sz();
#if UNIX
  sprintf(format.os_str, "Linux");
#else
  OSVERSIONINFO osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);

  sprintf(format.os_str, "Win32 %d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion);
#endif
  write(sock, (char *)&format, sizeof(format));
}
#endif

FILE *en_file_head(char *fname)
{
  FILE *fp;
  gboolean new_head = TRUE;

  if ((fp=fopen(fname, "r")) != NULL) {
    fclose(fp);
    new_head = FALSE;
  }

  if ((fp=fopen(fname, "a"))==NULL) {
    p_err("cannot write to to %s", fname);
  }

  if (new_head)
   fprintf(fp, TSIN_EN_WORD_KEY"\n");

  return fp;
}

#if USE_HTTP
#include <curl/curl.h>
#endif


void ts_upload()
{
  int i;
#if USE_HTTP
#if UNIX
  char tmpf[128]="/tmp/gcin-uploadXXXXXX";
  int fd = mkstemp(tmpf);
  if (fd < 0)
	p_err("mkstemp %s", tmpf);
  FILE *fu;
  if (!(fu=fdopen(fd, "wb+")))
	p_err("create %s failed", tmpf);
#else
  DWORD dwRetVal = 0;
  UINT uRetVal   = 0;
  char tmpf[MAX_PATH];
  char lpTempPathBuffer[MAX_PATH];
  dwRetVal = GetTempPathA(MAX_PATH, lpTempPathBuffer); // buffer for path
  if (dwRetVal > MAX_PATH || (dwRetVal == 0))
	p_err("GetTempPath failed");
  uRetVal = GetTempFileNameA(lpTempPathBuffer, // directory for tmp files
                              "gcin",     // temp file name prefix
                              0,                // create unique name
                              tmpf);  // buffer for name
    if (uRetVal == 0)
		p_err("GetTempFileName failed\n");
  FILE *fu;
  if (!(fu=fopen(tmpf, "wb")))
	  p_err("cannot create %s", tmpf);
#endif
#else
  int sock = connect_ts_share_svr();

  REQ_HEAD head;
  bzero(&head, sizeof(head));
#if 0
  head.cmd = REQ_CONTRIBUTE;
#else
  head.cmd = REQ_CONTRIBUTE2;
#endif
  write(sock, (char *)&head, sizeof(head));

  REQ_CONTRIBUTE_S req;
  bzero(&req, sizeof(req));
  strcpy(req.tag, get_tag());
  write(sock, (char *)&req, sizeof(req));
  send_format(sock);
#endif

  int key_sz = get_key_sz();

  dbg("tsN:%d\n", tsN);

  FILE *fw;

  if (b_en)
    fw = en_file_head(contributed_file_src);
  else
  if ((fw=fopen(contributed_file_src, "a"))==NULL)
    p_err("cannot write %s", contributed_file_src);

  for(i=0;i<tsN;i++) {
    phokey_t pho[MAX_PHRASE_LEN];
    char s[MAX_PHRASE_LEN * CH_SZ + 1];
    char len, slen;
    usecount_t usecount;
    load_tsin_at_ts_idx(i, &len, &usecount, pho, (u_char *)s);
    slen = strlen(s);

#if USE_HTTP
	fwrite(&len, 1, sizeof(len), fu);
	fwrite((char *)pho, key_sz, len, fu);
#else
    write(sock, &len, sizeof(len));
    write(sock, (char *)pho, len * key_sz);
#endif

    if (!b_en) {
#if USE_HTTP
	  fwrite(&slen, 1, sizeof(slen), fu);
	  fwrite(s, 1, slen, fu);
#else
      write(sock, &slen, sizeof(slen));
      write(sock, s, slen);
#endif
    }

#if 1
    if (b_en) {
      char *p = (char *)pho;
      p[len]=0;
      write_tsin_src(fw, len, NULL, p);
    } else
      write_tsin_src(fw, len, pho, s);
#endif
  }
  fclose(fu);
  fclose(fw);

#if USE_HTTP
  CURL *curl;
  CURLcode res;

  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;
  struct curl_slist *headerlist=NULL;
  static const char buf[] = "Expect:";

  curl_global_init(CURL_GLOBAL_ALL);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "tag",
               CURLFORM_COPYCONTENTS, get_tag(),
               CURLFORM_END);
  char ksz[4];
  sprintf(ksz, "%d", key_sz);
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "key_sz",
               CURLFORM_COPYCONTENTS, ksz,
               CURLFORM_END);

  /* Fill in the file upload field */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "data",
               CURLFORM_FILE, tmpf,
               CURLFORM_END);


  /* Fill in the submit field too, even if this is rarely needed */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "submit",
               CURLFORM_COPYCONTENTS, "send",
               CURLFORM_END);


  curl = curl_easy_init();

  /* initialize custom header list (stating that Expect: 100-continue is not
     wanted */
  headerlist = curl_slist_append(headerlist, buf);
  if(curl) {
    /* what URL that receives this POST */
    curl_easy_setopt(curl, CURLOPT_URL, "http://hyperrate.com/ts-share/u.php");
#if 0
    if((argc == 2) && (!strcmp(argv[1], "noexpectheader")))
      /* only disable 100-continue header if explicitly requested */
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
#endif
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);

    /* then cleanup the formpost chain */
    curl_formfree(formpost);
    /* free slist */
    curl_slist_free_all(headerlist);
  }

  unlink(tmpf);
#else
  char end_mark=0;
  write(sock, &end_mark, sizeof(end_mark));
  closesocket(sock);
#endif
}
