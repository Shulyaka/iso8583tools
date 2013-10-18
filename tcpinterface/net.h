#include "../parser/parser.h"
#include "../lib/isomessage.pb.h"

field* parseNetMsg(char*, unsigned int, fldformat*);
int convertNetMsg(isomessage*, field*);
fldformat* loadNetFormat(void);

