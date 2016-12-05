#ifndef __NET_H__
#define __NET_H__

#include <string>
#include "../parser/parser.h"
#include "../lib/isomessage.pb.h"

int isRequest(isomessage*);
int isDomestic(isomessage*);

long int parseNetMsg(field&, char*, size_t);
size_t serializeNetMsg(char*, size_t, field&);
int translateNetToSwitch(isomessage*, field&);
int translateSwitchToNet(field&, isomessage*, fldformat*);
int loadNetFormat(fldformat&);

int isNetMgmt(field&);
int isNetRequest(field&);
int processNetMgmt(field&);
int declineNetMsg(field&);

#endif
