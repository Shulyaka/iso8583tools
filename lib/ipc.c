#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include "ipc.h"

int ipcinit(void)
{
	struct sockaddr_un addr={AF_UNIX, "/home/denis/ipc/visa"};
	int sfd;

	sfd=socket(AF_UNIX, SOCK_DGRAM, 0);

	if(sfd==-1)
	{
		printf("Error: Unable to create a socket: %s\n", strerror(errno));
		return -1;
	}

	unlink(addr.sun_path);
	if(bind(sfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)))
	{
		printf("Error: Unable to bind a socket: %s\n", strerror(errno));
		close(sfd);
		return -1;
	}

	return sfd;
}

int ipcsend(int sfd, char *buf, int size)
{
	static struct sockaddr_un addr = {AF_UNIX, "/home/denis/ipc/switch"};

	if(sendto(sfd, buf, size, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_un))==-1)
	{
		printf("Error: Unable to send the message to switch: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

