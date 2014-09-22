#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include "../lib/isomessage.pb.h"
#include "../lib/ipc.h"

#include "response.h"

int main(void)
{
	struct pollfd sfd[1];
	isomessage inmsg;

	char buf[1000];
	int ret;
	redisContext *rcontext=NULL;

	struct timespec timeout={1, 0};

	GOOGLE_PROTOBUF_VERIFY_VERSION;	

	sfd[0].fd=ipcopen((char*)"switch");

	if(sfd[0].fd==-1)
	{
		printf("Error: Unable to init IPC\n");
		return -1;
	}

	sfd[0].events=POLLIN;

	printf("Connecting to Redis...\n");

	while(1)
	{
		if(rcontext==NULL)
		{
			rcontext=kvsconnect(NULL, 0);
			if(!rcontext)
			{
				sleep(1);
				continue;
			}
		}

		printf("Waiting for a message...\n");

		for(ret=0; ret==0; ret=ppoll(sfd, 1, &timeout, NULL))
		{
			ret=checkExpired(sfd[0].fd, rcontext);
			if(ret==2)
				break;
		}

		if(ret==-1)
		{
			printf("Error: poll: %s\n", strerror(errno));
			break;
		}
		else if(ret==2)
		{
			printf("Disconnected from Redis. Reconnecting again...\n");
			kvsfree(rcontext);
			rcontext=NULL;
			continue;
		}

		printf("recieving message\n");

		ret=ipcrecvmsg(sfd[0].fd, &inmsg);
		
		if(ret<=0)
			continue;

		printf("Size is %d\n", ret);

		printf("\nIncommingMessage:\n");
		inmsg.PrintDebugString();

		switch(inmsg.messagefunction())
		{
			case isomessage::ADVICERESP:
				ret=handleResponse(&inmsg, sfd[0].fd, rcontext);
				break;
			default:
				ret=0;
				printf("Error: Unhandled message function %d\n", inmsg.messagefunction());
		}

		if(ret==2)
		{
			printf("Disconnected from Redis. Reconnecting again...\n");
			kvsfree(rcontext);
			rcontext=NULL;
			continue;
		}
	}

	ipcclose(sfd[0].fd);

	kvsfree(rcontext);

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
