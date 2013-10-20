#include "../parser/parser.h"
#include "../lib/isomessage.pb.h"

field* parseNetMsg(char*, unsigned int, fldformat*);
int translateNetToSwitch(isomessage*, field*);
fldformat* loadNetFormat(void);

