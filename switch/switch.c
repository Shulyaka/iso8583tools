#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include "../lib/isomessage.pb.h"
#include "../lib/ipc.h"

#include "reqresp.h"

volatile sig_atomic_t sigint_caught=0;

static void catch_sigint(int signo)
{
	sigint_caught=1;
}

int main(void)
{
	struct pollfd sfd[1];
	isomessage inmsg;

	char buf[1000];
	int ret;
	redisContext *rcontext=NULL;

	GOOGLE_PROTOBUF_VERIFY_VERSION;	

	sfd[0].fd=ipcopen((char*)"switch");

	if(sfd[0].fd==-1)
	{
		printf("Error: Unable to init IPC\n");
		return -1;
	}

	sfd[0].events=POLLIN;

	if (signal(SIGINT, catch_sigint) == SIG_ERR)
		printf("Warning: unable to set the signal handler\n");

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

		if(ppoll(sfd, 1, NULL, NULL)==-1)
		{
			if(sigint_caught)
				break;

			printf("Error: poll: %s\n", strerror(errno));
			sleep(1);
			continue;
		}

		printf("recieving message\n");

		ret=ipcrecvmsg(sfd[0].fd, &inmsg);
		
		if(ret<=0)
			continue;

		printf("Size is %d\n", ret);

		printf("\nIncommingMessage:\n");
		inmsg.PrintDebugString();

		if(inmsg.messagetype() & isomessage::RESPONSE)
			ret=handleResponse(&inmsg, sfd[0].fd, rcontext);
		else
		{
			ret=handleRequest(&inmsg, sfd[0].fd, rcontext);
			if(ret)
				reverseRequest(&inmsg, sfd[0].fd, rcontext);
		}

		if(ret==2)
		{
			printf("Disconnected from Redis. Reconnecting again...\n");
			kvsfree(rcontext);
			rcontext=NULL;
		}
	}

	printf("ancelling^\n");

	ipcclose(sfd[0].fd);

	kvsfree(rcontext);

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
