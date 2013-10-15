#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include "ipc.h"

int ipcinit(void)
{
	struct sockaddr_un addr;
	int sfd;

	sfd=socket(AF_UNIX, SOCK_DGRAM, 0);

	if(sfd==-1)
	{
		printf("Error: Unable to create a socket: %s\n", strerror(errno));
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family=AF_UNIX;
	strncpy(addr.sun_path, "/home/denis/switch_visa", sizeof(addr.sun_path) - 1);

	unlink(addr.sun_path);
	if(bind(sfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)))
	{
		printf("Error: Unable to bind a socket: %s\n", strerror(errno));
		close(sfd);
		return -1;
	}

	if(ipcconnect(sfd)==-1)
		printf("Warning: Unable to connect to switch: %s\nWill try again later\n", strerror(errno));

	return sfd;
}

int ipcconnect(int sfd)
{
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family=AF_UNIX;
	strncpy(addr.sun_path, "/home/denis/switch", sizeof(addr.sun_path) - 1);

	return connect(sfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
}

int ipcsend(int sfd, char *buf, int size)
{
	if(send(sfd, buf, size, 0)==-1)
	{
		if(errno==ECONNREFUSED || errno==ECONNRESET || errno==ENOTCONN)
		{
			printf("Warning: %s, trying to reconnect\n", strerror(errno));
			if(ipcconnect(sfd)==-1)
			{
				printf("Error: unable to reconnect: %s\nDiscarding the message\nCheck if switch is running\n", strerror(errno));
				return 2;
			}

			printf("Successful. Resending the message\n");

			if(send(sfd, buf, size, 0)==-1)
			{
				printf("Error: Unable to send the message to switch anyway: %s\n", strerror(errno));
				return 3;
			}
		}
		else
		{
			printf("Error: Unable to send the message to switch: %s\n", strerror(errno));
			return 1;
		}
	}

	return 0;
}

