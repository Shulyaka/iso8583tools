#include "../lib/ipc.h"
#include "../lib/kvs.h"

int handleRequest(isomessage*, int, redisContext*);
int handleResponse(isomessage*, int, redisContext*);
