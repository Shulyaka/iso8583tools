#include <hiredis/hiredis.h>
#include "isomessage.pb.h"

redisContext *kvsconnect(const char *hostname, int port);
int kvsset(redisContext *c, const char *key, isomessage *message, int timeout);
int kvsget(redisContext *c, const char *key, isomessage *message);
void kvsfree(redisContext *c);
