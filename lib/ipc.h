#include "isomessage.pb.h"

int ipcopen(char*);
int ipcsend(int, char*, int, const char *);
int ipcrecv(int, char*, int);
int ipcclose(int);

int ipcsendmsg(int, isomessage*, const char*);
int ipcrecvmsg(int, isomessage*);

