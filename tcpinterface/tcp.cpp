#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include "net.h"
#include "tcp.h"

int tcpinit(void)
{
	int i, sct;
	struct sockaddr_in addr;

	sct=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(sct==-1)
	{
		printf("Error: Unable to create a socket: %s\n", strerror(errno));
		return -1;
	}

	i=1;

	if(setsockopt(sct, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)))
	{
		printf("Warning: setsockopt failed: %s\n", strerror(errno));
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=INADDR_ANY;
	addr.sin_port=htons(12345);

	if(bind(sct, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)))
	{
		printf("Error: Unable to bind a socket: %s\n", strerror(errno));
		close(sct);
		return -1;
	}

	if(listen(sct, 1)==-1)
	{
		printf("Error: listen failed: %s\n", strerror(errno));
		close(sct);
		return -1;
	}

	return sct;
}

int tcpconnect(int sct) //blocking
{
	struct pollfd sfd;

	sfd.fd=sct;
	sfd.events=POLLIN;

	if(ppoll(&sfd, 1, NULL, NULL)==-1)
		return -1;

	sfd.fd=accept(sct, NULL, NULL);
//	fcntl(sfd.fd, F_SETFL, O_NONBLOCK);
	return sfd.fd;
}

//returns:
//0: No data to receive or partial message received
//-1: Corrupted message
//-2: Connection is broken
//-3: Other error (see errno)
//>0: Valid message received
int tcprecv(int sfd, field &message)
{
	static char buf[10000];
	unsigned int maxlen=sizeof(buf);
	static unsigned int toread=1;
	static unsigned int numread=0;
	int i;

	while(true)
	{
		errno=0;
		i=read(sfd, buf+numread, toread);
		if(i==-1 && (errno==EAGAIN || errno==EWOULDBLOCK))
			return 0;
		if(i==0)
		{
			numread=0;
			toread=1;
			printf("Error: Unable to receive a net message: Connection closed by the remote host\n");
			return -2;
		}
		if(i==-1)
		{
			numread=0;
			toread=1;
			i=errno;
			printf("Error: Unable to receive a net message: %s\n", strerror(errno));
			errno=i;
			return -3;
		}

		numread+=i;
		toread-=i;

		if(toread==0)
		{
			i=parseNetMsg(message, buf, numread);
			if(i==0)
			{
				numread=0;
				toread=1;
				read(sfd, buf, maxlen); //parse error. We don't know where in the message we are. Try to consume all available data in hope to be on message boundaries again. Another approach would be to disconnect and reconnect again
				printf("Error: a corrupted message received\n");
				return -1;
			}
			if(i<0)
			{
				toread=-i;
				if(numread+toread>maxlen)
				{
					numread=0;
					toread=1;
					read(sfd, buf, maxlen); //see above
					printf("Error: a corrupted message received\n");
					return -1;
				}
				return 0;
			}

			numread=0;
			toread=1;

			return i;
		}
	}
}

//returns:
//0: Message error
//-1: Tcp error
//otherwise, message size is returned
int tcpsend(int sfd, field &message)
{
	unsigned int length;
	int numwritten=0;
	static char buf[10000];
	int i;

	length=buildNetMsg(buf, sizeof(buf), &message);

	while(length!=numwritten)
	{
		i=write(sfd, buf+numwritten, length-numwritten);

		if(i==-1 || i==0)
		{
			printf("Error: %d of %d bytes sent: Unable to send the message to network: %s\n", numwritten, length, strerror(errno));
			return -1;
		}
	}

	return numwritten;
}

int tcpclose(int sfd)
{
	return close(sfd);
}
