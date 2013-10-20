#include "../parser/parser.h"

int tcpinit(void);
int tcpconnect(int); //blocking
int tcpsend(int, char*, unsigned int, fldformat*);
int tcprecv(int, char*, unsigned int, fldformat*);
int tcpclose(int);

field *tcprecvmsg(int, fldformat*);
