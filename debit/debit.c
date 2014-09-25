#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
//#include <unistd.h>
#include <poll.h>
#include "../lib/isomessage.pb.h"
#include "../lib/ipc.h"

#include "request.h"

int main(void)
{
	struct pollfd sfd[1];
	isomessage inmsg;

	char buf[1000];
	int size;

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	sfd[0].fd=ipcopen((char*)"debit");

	if(sfd[0].fd==-1)
	{
		printf("Error: Unable to init IPC\n");
		return -1;
	}

	sfd[0].events=POLLIN;

	while(1)
	{
		printf("Waiting for a message...\n");

		if(ppoll(sfd, 1, NULL, NULL)==-1)
		{
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

		switch(inmsg.messagefunction())
		{
			case isomessage::REQUEST:
			case isomessage::ADVICE:
				handleRequest(&inmsg, sfd[0].fd);
				break;
			default:
				printf("Error: Unhandled message function %d\n", inmsg.messagefunction());
		}		
	}

	ipcclose(sfd[0].fd);

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
