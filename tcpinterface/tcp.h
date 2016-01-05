#include "../parser/parser.h"

int tcpserverinit(const std::string&);
int tcpclientconnect(const std::string&, const std::string&);
int tcpserverconnect(int, std::string&, std::string&); //blocking
long int tcprecv(int, field&);
long int tcpsend(int, field&);
int tcpclose(int);

