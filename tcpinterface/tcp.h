#include "../parser/parser.h"

int tcpinit(void);
int tcpconnect(int); //blocking
int tcpsend(int, char*, unsigned int);
int tcprecv(int, char*, unsigned int, fldformat*);
int tcpclose(int);

int tcprecvmsg(int, field**, fldformat*);
int tcpsendmsg(int, field*, fldformat*);
