#include "isomessage.pb.h"

int ipcopen(char*);
int ipcsend(int, char*, int, char *);
int ipcrecv(int, char*, int);
int ipcclose(int);

int ipcsendmsg(int, isomessage*, char*);
int ipcrecvmsg(int, isomessage*);

