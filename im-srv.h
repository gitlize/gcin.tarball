typedef enum {
  Connection_type_unix = 1,
  Connection_type_tcp = 2
} Connection_type;

typedef struct {
  ClientState *cs;
  int tag;
  u_int seed;
  Connection_type type;
#if	UNIX
  int fd;
#else
#if SHARED_MEMORY
  GCIN_SHM fd;
  DWORD pid;
#else
  HANDLE fd;
#endif
#endif
} GCIN_ENT;

extern GCIN_ENT *gcin_clients;
extern int gcin_clientsN;
extern Server_IP_port srv_ip_port;
