#include "../parser/parser.h"

int tcpinit(void);
int tcpconnect(int); //blocking
int tcprecv(int, field&);
int tcpsend(int, field&);
int tcpclose(int);

