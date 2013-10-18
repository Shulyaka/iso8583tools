#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include "ipc.h"

#define IPCDIR "/home/denis/ipc"

int ipcopen(char *myname)
{
	struct sockaddr_un addr={AF_UNIX, IPCDIR};
	int sfd;

	if(!myname)
	{
		printf("Error: no local IPC address\n");
		return -1;
	}

	strcpy(addr.sun_path+strlen(IPCDIR), myname);

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

int ipcsend(int sfd, char *buf, int size, char *dest)
{
	static struct sockaddr_un addr = {AF_UNIX, IPCDIR};

	if(!dest)
	{
		printf("Error: No dest address\n");
		return -1;
	}

	strcpy(addr.sun_path+strlen(IPCDIR), dest);

	if(sendto(sfd, buf, size, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_un))==-1)
	{
		printf("Error: Unable to send the message to %s: %s\n", dest, strerror(errno));
		return -1;
	}

	return 0;
}

int ipcrecv(int sfd, char *buf, int size)
{
	int i=recv(sfd, buf, size, 0);

	if(i==-1)
		printf("Error: recv: %s\n", strerror(errno));
	
	return i;
}

int ipcclose(int sfd)
{
	if(close(sfd)==-1)
	{
		printf("Error: close: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


int ipcsendmsg(int sfd, isomessage *message, char *dest)
{
	static char buf[1000];
	int size;

	size=message->ByteSize();
	if(size>sizeof(buf))
	{
		printf("Error: Message is too big (%d bytes)\n", size);
		return 0;
	}

	if(!message->SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf))
	{
		printf("Error: Unable to serialize the message\n");
		return 0;
	}

	if(ipcsend(sfd, buf, size, dest)==-1)
	{
		printf("Error: Unable to send the message to switch\n");
		return -1;
	}

	return size;
}

int ipcrecvmsg(int sfd, isomessage *message)
{
	static char buf[1000];
	int size;

	size=ipcrecv(sfd, buf, sizeof(buf));

	if(size==-1)
		return -1;

	if(!message->ParseFromArray(buf, size))
	{
		printf("Warning: Unable to de-serialize the message\n");
		return 0;
	}

	return size;
}

