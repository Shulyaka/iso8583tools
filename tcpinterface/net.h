#ifndef __NET_H__
#define __NET_H__

#include "../parser/parser.h"
#include "../lib/isomessage.pb.h"

int isRequest(isomessage*);
int isDomestic(isomessage*);

int parseNetMsg(field&, char*, unsigned int);
unsigned int buildNetMsg(char*, unsigned int, field*);
int translateNetToSwitch(isomessage*, field*);
int translateSwitchToNet(field*, isomessage*, fldformat*);
int loadNetFormat(fldformat&);

int isNetMgmt(field*);
int isNetRequest(field*);
int processNetMgmt(field*);
int declineNetMsg(field*);

#endif
