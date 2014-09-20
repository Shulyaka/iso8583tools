#include <stdio.h>
#include <time.h>
#include "kvs.h"

redisContext *kvsconnect(const char *hostname, int port)
{
	redisContext *c;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	redisReply *reply;

	if(!hostname)
		hostname="127.0.0.1";

	if(port<=0)
		port=6379;

	c = redisConnectWithTimeout(hostname, port, timeout);
	if (c == NULL || c->err)
	{
		if (c)
		{
			printf("Connection error: %s\n", c->errstr);
			redisFree(c);
		}
		else
		{
			printf("Connection error: can't allocate redis context\n");
		}
		return NULL;
	}

	reply = (redisReply*)redisCommand(c,"PING");

	if(!reply)
	{
		printf("Error: Connection check failed: %s\n", c->errstr);
		redisFree(c);
		return NULL;
	}

	if(reply->type==REDIS_REPLY_ERROR || strcmp(reply->str, "PONG"))
	{
		printf("Error: Connection check failed: %s\n", reply->str);
		freeReplyObject(reply);
		redisFree(c);
		return NULL;
	}

	freeReplyObject(reply);

	printf("Info: Successfully connected to Redis at %s:%d\n", hostname, port);

	return c;
}

// return values:
// 0: connection failure. Must call kvsfree and reconnect.
// <0: other error without connection failure
// >0: Success
int kvsset(redisContext *c, const char *key, isomessage *message)
{
	static char buf[1000];
	size_t size;
	redisReply *reply;
	time_t timeout;

	size=message->ByteSize();
	if(size>sizeof(buf))
	{
		printf("Error: Message is too big (%d bytes)\n", size);
		return -1;
	}

	if(!message->SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf))
	{
		printf("Error: Unable to serialize the message\n");
		return -1;
	}

	reply = (redisReply*)redisCommand(c,"SET %b %b", key, (size_t)strlen(key), buf, (size_t)size);

	if(!reply)
	{
		printf("Error: Unable to save the message to kvs: %s\n", c->errstr);
		return 0;
	}

	freeReplyObject(reply);

	if(message->timeout()<500000000)
		timeout=time(NULL)+message->timeout();
	else
		timeout=message->timeout();

	reply = (redisReply*)redisCommand(c,"EXPIREAT %b %ld", key, (size_t)strlen(key), timeout+2592000);

	if(!reply)
	{
		printf("Error: Unable to set timeout: %s\n", c->errstr);
		return 0;
	}

	if(reply->type==REDIS_REPLY_ERROR)
		printf("Warning: %s\n", reply->str);

	freeReplyObject(reply);

	reply = (redisReply*)redisCommand(c,"ZADD kvskeys %ld %b", timeout, key, (size_t)strlen(key));

	if(!reply)
	{
		printf("Error: Unable to add key to set: %s\n", c->errstr);
		return 0;
	}

	if(reply->type==REDIS_REPLY_ERROR)
		printf("Warning: %s\n", reply->str);

	freeReplyObject(reply);

	return 1;
}

// return values:
// 0: connection failure. Must call kvsfree and reconnect.
// <0: other error without connection failure
// >0: Success
int kvsget(redisContext *c, const char *key, isomessage *message)
{
	redisReply *reply;

	reply = (redisReply*)redisCommand(c,"GET %b", key, (size_t)strlen(key));

	if(!reply)
	{
		printf("Error: Unable to receive the message from kvs: %s\n", c->errstr);
		return 0;
	}

	if(reply->type==REDIS_REPLY_NIL)
	{
		printf("Warning: Key %s not found\n", key);
		freeReplyObject(reply);
		return -1;
	}
	
	if(reply->type==REDIS_REPLY_ERROR)
	{
		printf("Error: Unable to receive the message from kvs: %s\n", reply->str);
		freeReplyObject(reply);
		return -1;
	}

	if(reply->type!=REDIS_REPLY_STRING)
	{
		printf("Error: Wrong reply type (%d)\n", reply->type);
		freeReplyObject(reply);
		return -1;
	}

	if(!message->ParseFromArray(reply->str, reply->len))
	{
		printf("Warning: Unable to de-serialize the message\n");
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);

	reply = (redisReply*)redisCommand(c,"ZREM kvskeys %b", key, (size_t)strlen(key));

	if(!reply)
	{
		printf("Error: Unable to remove key from set: %s\n", c->errstr);
		return 0;
	}

	if(reply->type==REDIS_REPLY_ERROR)
		printf("Warning: %s\n", reply->str);

	freeReplyObject(reply);

	reply = (redisReply*)redisCommand(c,"DEL %b", key, (size_t)strlen(key));

	if(!reply)
	{
		printf("Error: Unable to delete the message from kvs: %s\n", c->errstr);
		return 0;
	}

	if(reply->type==REDIS_REPLY_ERROR)
	{
		printf("Error: Unable to delete the message from kvs: %s\n", reply->str);
		freeReplyObject(reply);
		return -1;
	}

	freeReplyObject(reply);

	return 1;
}

void kvsfree(redisContext *c)
{
	redisFree(c);
}

