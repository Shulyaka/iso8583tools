#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
//#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include "../lib/isomessage.pb.h"
#include "../lib/ipc.h"

#include "request.h"

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
	int size;

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	sfd[0].fd=ipcopen((char*)"issuer");

	if(sfd[0].fd==-1)
	{
		printf("Error: Unable to init IPC\n");
		return -1;
	}

	sfd[0].events=POLLIN;

	if (signal(SIGINT, catch_sigint) == SIG_ERR)
		printf("Warning: unable to set the signal handler\n");

	while(1)
	{
		printf("Waiting for a message...\n");

		if(ppoll(sfd, 1, NULL, NULL)==-1)
		{
			if(sigint_caught)
				break;

			printf("Error: poll: %s\n", strerror(errno));
			break;
		}

		printf("recieving message\n");

		size=ipcrecvmsg(sfd[0].fd, &inmsg);
		
		if(size<=0)
			continue;

		printf("Size is %d\n", size);

		printf("\nIncommingMessage:\n");
		inmsg.PrintDebugString();

		if(inmsg.messagetype() & isomessage::RESPONSE)
			printf("Error: Responses are not handled\n");
		else
			handleRequest(&inmsg, sfd[0].fd);
	}

	printf("ancelling^\n");

	ipcclose(sfd[0].fd);

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
