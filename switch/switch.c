#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include "../lib/isomessage.pb.h"

int main(void)
{
	struct sockaddr_un addr={AF_UNIX, "/home/denis/ipc/switch"};
	struct pollfd sfd[1];
	isomessage inmsg;

	char buf[1000];
	int size;

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	sfd[0].fd=socket(AF_UNIX, SOCK_DGRAM, 0);

	if(sfd[0].fd==-1)
	{
		printf("Error: Unable to create a socket: %s\n", strerror(errno));
		return -1;
	}

	unlink(addr.sun_path);

	if(bind(sfd[0].fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)))
	{
		printf("Error: Unable to bind a socket: %s\n", strerror(errno));
		close(sfd[0].fd);
		return -1;
	}

	sfd[0].events=POLLIN;

	while(1)
	{
		printf("Waiting for a message...\n");

		if(poll(sfd, 1, -1)==-1)
		{
			printf("Error: poll: %s\n", strerror(errno));
			break;
		}

		printf("recieving message\n");

		size=recv(sfd[0].fd, buf, sizeof(buf), 0);
		
		if(size==-1)
		{
			printf("Error: recv: %s\n", strerror(errno));
			break;
		}

		if(size==0)
		{
			printf("Warning: Client disconnected\n");
			continue;
		}

		printf("Size is %d\n", size);

		if(!inmsg.ParseFromArray(buf, size))
		{
			printf("Warning: Unable to parse the message\n");
			continue;
		}

		printf("\nIncommingMessage:\n");
		inmsg.PrintDebugString();
		
	}

	close(sfd[0].fd);

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
