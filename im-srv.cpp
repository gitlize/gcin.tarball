#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <X11/Xatom.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "gcin.h"
#include "gcin-protocol.h"
#include "im-srv.h"
#include <gdk/gdk.h>

int im_sockfd, im_tcp_sockfd;
Atom get_gcin_sockpath_atom(Display *dpy);
Server_IP_port srv_ip_port;
static Window prop_win;
static Atom addr_atom;


#ifdef __cplusplus
extern "C" gint gdk_input_add	  (gint		     source,
			   GdkInputCondition condition,
			   GdkInputFunction  function,
			   gpointer	     data);
#endif
void gdk_input_remove	  (gint		     tag);


void get_gcin_im_srv_sock_path(char *outstr, int outstrN);
void process_client_req(int fd);

#if UNIX
static gboolean cb_read_gcin_client_data(GIOChannel *source, GIOCondition condition, gpointer data)
{
  int fd=GPOINTER_TO_INT(data);

  process_client_req(fd);
  return TRUE;
}
#endif

Atom get_gcin_addr_atom(Display *dpy);

static void gen_passwd_idx()
{
  srv_ip_port.passwd.seed = (rand() >> 1) % __GCIN_PASSWD_N_;

  Server_IP_port tsrv_ip_port = srv_ip_port;

  to_gcin_endian_4(&srv_ip_port.passwd.seed);
  XChangeProperty(dpy, prop_win , addr_atom, XA_STRING, 8,
     PropModeReplace, (unsigned char *)&tsrv_ip_port, sizeof(srv_ip_port));

  XSync(GDK_DISPLAY(), FALSE);
}



static gboolean cb_new_gcin_client(GIOChannel *source, GIOCondition condition, gpointer data)
{
  Connection_type type=(Connection_type) GPOINTER_TO_INT(data);
#if 0
  dbg("im-srv: cb_new_gcin_client %s\n", type==Connection_type_unix ? "unix":"tcp");
#endif
  unsigned int newsockfd;
  socklen_t clilen;

  if (type==Connection_type_unix) {
    struct sockaddr_un cli_addr;

    bzero(&cli_addr, sizeof(cli_addr));
    clilen=0;
    newsockfd = accept(im_sockfd,(struct sockaddr *) & cli_addr, &clilen);
  } else
  {
    struct sockaddr_in cli_addr;

    bzero(&cli_addr, sizeof(cli_addr));
    clilen=sizeof(cli_addr);
    newsockfd = accept(im_tcp_sockfd,(struct sockaddr *) & cli_addr, &clilen);
  }

  if (newsockfd < 0) {
    perror("accept");
    return FALSE;
  }

//  dbg("newsockfd %d\n", newsockfd);

  if (newsockfd >= gcin_clientsN) {
    gcin_clients = trealloc(gcin_clients, GCIN_ENT, newsockfd+1);
    gcin_clientsN = newsockfd;
  }

  bzero(&gcin_clients[newsockfd], sizeof(gcin_clients[0]));

  gcin_clients[newsockfd].tag = g_io_add_watch(g_io_channel_unix_new(newsockfd), G_IO_IN, cb_read_gcin_client_data,
              GINT_TO_POINTER(newsockfd));

  if (type==Connection_type_tcp) {
    gcin_clients[newsockfd].seed = srv_ip_port.passwd.seed;
    gen_passwd_idx();
  }
  gcin_clients[newsockfd].type = type;
  return TRUE;
}


static int get_ip_address(u_int *ip)
{
  char hostname[64];

  if (gethostname(hostname, sizeof(hostname)) < 0) {
    perror("cannot get hostname\n");
    return -1;
  }

#if 0
  dbg("hostname %s\n", hostname);
  struct hostent *hent;

  if (!(hent=gethostbyname(hostname))) {
    dbg("cannot call gethostbyname to get IP address");
    return -1;
  }

  memcpy(ip, hent->h_addr_list[0], hent->h_length);
#else
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  int s = getaddrinfo(NULL, hostname, &hints, &result);
  if (s != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
      exit(EXIT_FAILURE);
  }


  for (rp = result; rp != NULL; rp = rp->ai_next) {
      int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
          continue;

      if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
          close(sfd);
          break;
      }

      close(sfd);
  }

  if (rp == NULL) { /* No address succeeded */
      fprintf(stderr, "Could not bind\n");
      exit(EXIT_FAILURE);
  }

  freeaddrinfo(result);
  memcpy(ip, &rp->ai_addr, rp->ai_addrlen);
#endif
  return 0;
}



void start_pipe_svr();

void init_gcin_im_serv(Window win)
{
  dbg("init_gcin_im_serv\n");

  int servlen;
  prop_win = win;
  struct sockadd_un;
  struct sockaddr_un serv_addr;

  // unix socket
  bzero(&serv_addr,sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  char sock_path[128];
  get_gcin_im_srv_sock_path(sock_path, sizeof(sock_path));
  strcpy(serv_addr.sun_path, sock_path);

#ifdef SUN_LEN
  servlen = SUN_LEN (&serv_addr);
#else
  servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
#endif

  dbg("-- %s\n",serv_addr.sun_path);
  struct stat st;

  if (!stat(serv_addr.sun_path, &st)) {
    if (unlink(serv_addr.sun_path) < 0) {
      char tt[512];
      snprintf(tt, sizeof(tt), "unlink error %s", serv_addr.sun_path);
      perror(tt);
    }
  }


  if ((im_sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
    perror("cannot open unix socket");
    exit(-1);
  }

  if (bind(im_sockfd, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("cannot bind");
    exit(-1);
  }

  listen(im_sockfd,2);

  dbg("im_sockfd:%d\n", im_sockfd);

  g_io_add_watch(g_io_channel_unix_new(im_sockfd), G_IO_IN, cb_new_gcin_client,
                GINT_TO_POINTER(Connection_type_unix));

  Display *dpy = GDK_DISPLAY();

  Server_sock_path srv_sockpath;
  strcpy(srv_sockpath.sock_path, sock_path);
  Atom sockpath_atom = get_gcin_sockpath_atom(dpy);
  XChangeProperty(dpy, prop_win , sockpath_atom, XA_STRING, 8,
     PropModeReplace, (unsigned char *)&srv_sockpath, sizeof(srv_sockpath));

  addr_atom = get_gcin_addr_atom(dpy);
  XSetSelectionOwner(dpy, addr_atom, win, CurrentTime);

  if (!gcin_remote_client) {
    dbg("connection via TCP is disabled\n");
    return;
  }

  // tcp socket
  if (get_ip_address(&srv_ip_port.ip) < 0)
    return;

  if ((im_tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("cannot open tcp socket");
    exit(-1);
  }

//  dbg("socket succ\n");

  struct sockaddr_in serv_addr_tcp;
  u_short port;

  for(port=9999; port < 20000; port++)
  {
    // tcp socket
    bzero(&serv_addr_tcp, sizeof(serv_addr_tcp));
    serv_addr_tcp.sin_family = AF_INET;

    serv_addr_tcp.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr_tcp.sin_port = htons(port);
    if (bind(im_tcp_sockfd, (struct sockaddr *) &serv_addr_tcp, sizeof(serv_addr_tcp)) == 0)
      break;
  }

  srv_ip_port.port = serv_addr_tcp.sin_port;
  dbg("server port bind to %s:%d\n", inet_ntoa(serv_addr_tcp.sin_addr), port);
  time_t t;
  srand(time(&t));

  int i;
  for(i=0; i < __GCIN_PASSWD_N_; i++) {
    srv_ip_port.passwd.passwd[i] = (rand()>>2) & 0xff;
  }

  if (listen(im_tcp_sockfd, 5) < 0) {
    perror("cannot listen: ");
	exit(1);
  }

  dbg("after listen\n");


  gen_passwd_idx();

  g_io_add_watch(g_io_channel_unix_new(im_tcp_sockfd), G_IO_IN, cb_new_gcin_client,
                GINT_TO_POINTER(Connection_type_tcp));
}
