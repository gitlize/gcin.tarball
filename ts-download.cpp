#define USE_HTTP 1
#if USE_HTTP
#include <errno.h>
#include <curl/curl.h>
#else
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
#if !USE_HTTP
void send_format(SOCKET sock);
#endif
int get_key_sz();
void write_tsin_src(FILE *fw, char len, phokey_t *pho, char *s);
FILE *en_file_head(char *fname);

#if USE_HTTP
enum fcurl_type_e {
  CFTYPE_NONE=0,
  CFTYPE_FILE=1,
  CFTYPE_CURL=2
};

struct fcurl_data
{
  enum fcurl_type_e type;     /* type of handle */
  union {
    CURL *curl;
    FILE *file;
  } handle;                   /* handle */

  char *buffer;               /* buffer to store cached data*/
  size_t buffer_len;          /* currently allocated buffers length */
  size_t buffer_pos;          /* end of data in buffer*/
  int still_running;          /* Is background url fetch still in progress */
};

typedef struct fcurl_data URL_FILE;

/* exported functions */
URL_FILE *url_fopen(const char *url, const char *operation);
int url_fclose(URL_FILE *file);
int url_feof(URL_FILE *file);
size_t url_fread(void *ptr, size_t size, size_t nmemb, URL_FILE *file);
char *url_fgets(char *ptr, size_t size, URL_FILE *file);
void url_rewind(URL_FILE *file);

/* we use a global one for convenience */
CURLM *multi_handle;

/* curl calls this routine to get more data */
static size_t write_callback(char *buffer,
                             size_t size,
                             size_t nitems,
                             void *userp)
{
  char *newbuff;
  size_t rembuff;

  URL_FILE *url = (URL_FILE *)userp;
  size *= nitems;

  rembuff=url->buffer_len - url->buffer_pos; /* remaining space in buffer */

  if(size > rembuff) {
    /* not enough space in buffer */
    newbuff=(char *)realloc(url->buffer, url->buffer_len + (size - rembuff));
    if(newbuff==NULL) {
      fprintf(stderr, "callback buffer grow failed\n");
      size=rembuff;
    }
    else {
      /* realloc succeeded increase buffer size*/
      url->buffer_len+=size - rembuff;
      url->buffer=newbuff;
    }
  }

  memcpy(&url->buffer[url->buffer_pos], buffer, size);
  url->buffer_pos += size;

  return size;
}

/* use to attempt to fill the read buffer up to requested number of bytes */
static int fill_buffer(URL_FILE *file, size_t want)
{
  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;
  struct timeval timeout;
  int rc;
  CURLMcode mc; /* curl_multi_fdset() return code */

  /* only attempt to fill buffer if transactions still running and buffer
   * doesn't exceed required size already
   */
  if((!file->still_running) || (file->buffer_pos > want))
    return 0;

  /* attempt to fill buffer */
  do {
    int maxfd = -1;
    long curl_timeo = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    /* set a suitable timeout to fail on */
    timeout.tv_sec = 60; /* 1 minute */
    timeout.tv_usec = 0;

    curl_multi_timeout(multi_handle, &curl_timeo);
    if(curl_timeo >= 0) {
      timeout.tv_sec = curl_timeo / 1000;
      if(timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (curl_timeo % 1000) * 1000;
    }

    /* get file descriptors from the transfers */
    mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

    if(mc != CURLM_OK) {
      fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", mc);
      break;
    }

    /* On success the value of maxfd is guaranteed to be >= -1. We call
       select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
       no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
       to sleep 100ms, which is the minimum suggested value in the
       curl_multi_fdset() doc. */

    if(maxfd == -1) {
#ifdef _WIN32
      Sleep(100);
      rc = 0;
#else
      /* Portable sleep for platforms other than Windows. */
      struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
      rc = select(0, NULL, NULL, NULL, &wait);
#endif
    }
    else {
      /* Note that on some platforms 'timeout' may be modified by select().
         If you need access to the original value save a copy beforehand. */
      rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
    }

    switch(rc) {
    case -1:
      /* select error */
      break;

    case 0:
    default:
      /* timeout or readable/writable sockets */
      curl_multi_perform(multi_handle, &file->still_running);
      break;
    }
  } while(file->still_running && (file->buffer_pos < want));
  return 1;
}

/* use to remove want bytes from the front of a files buffer */
static int use_buffer(URL_FILE *file, size_t want)
{
  /* sort out buffer */
  if((file->buffer_pos - want) <=0) {
    /* ditch buffer - write will recreate */
    free(file->buffer);
    file->buffer=NULL;
    file->buffer_pos=0;
    file->buffer_len=0;
  }
  else {
    /* move rest down make it available for later */
    memmove(file->buffer,
            &file->buffer[want],
            (file->buffer_pos - want));

    file->buffer_pos -= want;
  }
  return 0;
}

URL_FILE *url_fopen(const char *url, const char *operation)
{
  /* this code could check for URLs or types in the 'url' and
     basically use the real fopen() for standard files */

  URL_FILE *file;
  (void)operation;

  file = (URL_FILE *)malloc(sizeof(URL_FILE));
  if(!file)
    return NULL;

  memset(file, 0, sizeof(URL_FILE));

  if((file->handle.file=fopen(url, operation)))
    file->type = CFTYPE_FILE; /* marked as URL */

  else {
    file->type = CFTYPE_CURL; /* marked as URL */
    file->handle.curl = curl_easy_init();

    curl_easy_setopt(file->handle.curl, CURLOPT_URL, url);
    curl_easy_setopt(file->handle.curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(file->handle.curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(file->handle.curl, CURLOPT_WRITEFUNCTION, write_callback);

    if(!multi_handle)
      multi_handle = curl_multi_init();

    curl_multi_add_handle(multi_handle, file->handle.curl);

    /* lets start the fetch */
    curl_multi_perform(multi_handle, &file->still_running);

    if((file->buffer_pos == 0) && (!file->still_running)) {
      /* if still_running is 0 now, we should return NULL */

      /* make sure the easy handle is not in the multi handle anymore */
      curl_multi_remove_handle(multi_handle, file->handle.curl);

      /* cleanup */
      curl_easy_cleanup(file->handle.curl);

      free(file);

      file = NULL;
    }
  }
  return file;
}

int url_fclose(URL_FILE *file)
{
  int ret=0;/* default is good return */

  switch(file->type) {
  case CFTYPE_FILE:
    ret=fclose(file->handle.file); /* passthrough */
    break;

  case CFTYPE_CURL:
    /* make sure the easy handle is not in the multi handle anymore */
    curl_multi_remove_handle(multi_handle, file->handle.curl);

    /* cleanup */
    curl_easy_cleanup(file->handle.curl);
    break;

  default: /* unknown or supported type - oh dear */
    ret=EOF;
    errno=EBADF;
    break;
  }

  free(file->buffer);/* free any allocated buffer space */
  free(file);

  return ret;
}

int url_feof(URL_FILE *file)
{
  int ret=0;

  switch(file->type) {
  case CFTYPE_FILE:
    ret=feof(file->handle.file);
    break;

  case CFTYPE_CURL:
    if((file->buffer_pos == 0) && (!file->still_running))
      ret = 1;
    break;

  default: /* unknown or supported type - oh dear */
    ret=-1;
    errno=EBADF;
    break;
  }
  return ret;
}

size_t url_fread(void *ptr, size_t size, size_t nmemb, URL_FILE *file)
{
  size_t want;

  switch(file->type) {
  case CFTYPE_FILE:
    want=fread(ptr, size, nmemb, file->handle.file);
    break;

  case CFTYPE_CURL:
    want = nmemb * size;

    fill_buffer(file, want);

    /* check if there's data in the buffer - if not fill_buffer()
     * either errored or EOF */
    if(!file->buffer_pos)
      return 0;

    /* ensure only available data is considered */
    if(file->buffer_pos < want)
      want = file->buffer_pos;

    /* xfer data to caller */
    memcpy(ptr, file->buffer, want);

    use_buffer(file, want);

    want = want / size;     /* number of items */
    break;

  default: /* unknown or supported type - oh dear */
    want=0;
    errno=EBADF;
    break;

  }
  return want;
}

#endif


void ts_download()
{
#if !USE_HTTP
  int sock = connect_ts_share_svr();

  REQ_HEAD head;
  bzero(&head, sizeof(head));
  head.cmd = REQ_DOWNLOAD2;
  write(sock, (char *)&head, sizeof(head));

  REQ_DOWNLOAD_S req;
  bzero(&req, sizeof(req));
  strcpy(req.tag, get_tag());
#endif

static char DL_CONF[]="ts-share-download-time";
static char DL_CONF_EN[]="ts-share-download-time-en";
  int rn, wn;
  char *dl_conf = b_en?DL_CONF_EN:DL_CONF;
#if USE_HTTP
  int last_dl_time = get_gcin_conf_int(dl_conf, 0);
//  last_dl_time = 1480473779;
//  last_dl_time = 1375473779;
//  last_dl_time = 1489366066;
  dbg("last_dl_time %d\n", last_dl_time);
#else
  req.last_dl_time = get_gcin_conf_int(dl_conf, 0);
 // req.last_dl_time = 0;
  wn = write(sock, (char*)&req, sizeof(req));
  REQ_DOWNLOAD_REPLY_S rep;
  rn = read(sock, (char*)&rep, sizeof(rep));
  send_format(sock);
#endif

  int key_sz =  get_key_sz();

#if USE_HTTP
  char url[128];
  sprintf(url, "http://hyperrate.com/ts-share/d.php?ti=%d&tag=%s&key_sz=%d",last_dl_time, get_tag(), key_sz);
  URL_FILE *handle = url_fopen(url, "r");
  int this_dl_time;
  rn = url_fread(&this_dl_time, 1, sizeof(this_dl_time), handle);
  dbg("this_dl_time %d\n", this_dl_time);
#endif

  FILE *fw;
  if (b_en)
    fw = en_file_head(downloaded_file_src);
  else
  if ((fw=fopen(downloaded_file_src,"a"))==NULL)
    p_err("cannot create %s:%s", downloaded_file_src, sys_err_strA());

  int N=0;
  for(;;) {
    char len=0;
#if USE_HTTP
    rn = url_fread(&len, 1, sizeof(len), handle);
    dbg("len %d\n", len);
#else
    rn = read(sock, &len, sizeof(len));
#endif
    if (len<=0)
      break;
    phokey_t pho[128];
#if USE_HTTP
	rn = url_fread((char*)pho, 1, len * key_sz, handle);
#else
    rn = read(sock, (char*)pho, len * key_sz);
#endif

	if (b_en) {
	  char *s = (char *)pho;
	  s[len]=0;
	  dbg("dl %s\n", s);
#if 1
	  save_phrase_to_db(&en_hand, pho, NULL, len, 0);
	  write_tsin_src(fw, len, NULL, s);
#endif
	} else {
      char slen=0;
      char s[256];
#if USE_HTTP
	  rn = url_fread(&slen, 1, sizeof(slen), handle);
	  dbg("slen %d\n", slen);
	  if (slen < 0)
	    break;
	  rn = url_fread(s, 1, slen, handle);
#else
      rn = read(sock, &slen, sizeof(slen));
      rn = read(sock, s, slen);
#endif
      s[slen]=0;
	dbg("dl %s\n", s);
#if 1
      save_phrase_to_db(&tsin_hand, pho, s, len, 0);
	  write_tsin_src(fw, len, pho, s);
#endif
	}

    N++;
  }

  dbg("N:%d\n", N);
  if (N)
#if USE_HTTP
	save_gcin_conf_int(dl_conf, (int)this_dl_time);
#else
    save_gcin_conf_int(dl_conf, (int)rep.this_dl_time);
#endif

  fclose(fw);
#if USE_HTTP
  url_fclose(handle);
#else
  closesocket(sock);
#endif
}
