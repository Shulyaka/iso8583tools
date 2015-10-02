#ifndef __NET_H__
#define __NET_H__

#include "../parser/parser.h"
#include "../lib/isomessage.pb.h"

int isRequest(isomessage*);
int isDomestic(isomessage*);

field* parseNetMsg(char*, unsigned int, fldformat*);
unsigned int buildNetMsg(char*, unsigned int, field*);
int translateNetToSwitch(isomessage*, field*);
field* translateSwitchToNet(isomessage*, fldformat*);
int loadNetFormat(fldformat&);

int isNetMgmt(field*);
int isNetRequest(field*);
int processNetMgmt(field*);
int declineNetMsg(field*);

#endif
