#include <hiredis/hiredis.h>
#include "isomessage.pb.h"

redisContext *kvsconnect(const char *hostname, int port);
int kvsset(redisContext *c, const char *key, isomessage *message);
int kvsget(redisContext *c, const char *key, isomessage *message);
int kvsdel(redisContext *c, const char *key);
void kvsfree(redisContext *c);

int kvslistexpired(redisContext *c, char ***keys);
void kvsfreelist(char **keys, int n);
