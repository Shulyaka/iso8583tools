#include "../parser/parser.h"
#include "../switch/isomessage.pb.h"

field* parseNetMsg(char*, unsigned int, fldformat*);
int convertNetMsg(isomessage*, field*);
fldformat* loadNetFormat(void);

