#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "../parser/parser.h"
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
	int sfd;
	sfd=accept(sct, NULL, NULL);
	fcntl(sfd, F_SETFL, O_NONBLOCK);
	return sfd;
}

//returns:
//0: No data to receive
//-1: Corrupted message
//-2: Connection is broken
//-3: Other error (see errno)
//>0: Valid message received
int tcpreceive(int sfd, char *buf, unsigned int maxlen, fldformat *frm)
{
	int i;
	static unsigned int numread=0;
	static unsigned int length;
	static unsigned int timeoutcnt;

	if(numread==0)
	{
		length=0;
		timeoutcnt=10;
	}
	else if(timeoutcnt--==0)
	{
		numread=0;
		return -1;
	}

	if(length==0)
	{
		if(numread>=frm->lengthLength || frm->lengthLength > maxlen)
		{
			numread=0;
			return -1;
		}

		errno=0;

		i=read(sfd, buf + numread, frm->lengthLength - numread);

		if(errno==EAGAIN || errno==EWOULDBLOCK)
			return 0;
		if(i==0)
		{
			numread=0;
			return -2;
		}
		else if(i==-1)
		{
			numread=0;
			return -3;
		}

		numread+=i;

		if(numread < frm->lengthLength)
			return 0;

		length=get_field_length(buf, numread, frm);

		if(length==0)
		{
			numread=0;
			return -1;
		}

		if(length>frm->maxLength)
		{
			numread=0;
			return -1;
		}

		//printf("Message length is %d\n", length);
	}

	errno=0;

	if(numread>=frm->lengthLength + length || frm->lengthLength + length > maxlen)
	{
		numread=0;
		return -1;
	}

	errno=0;

	i=read(sfd, buf + numread, frm->lengthLength + length - numread);

	if(errno==EAGAIN || errno==EWOULDBLOCK)
		return 0;
	if(i==0)
	{
		numread=0;
		return -2;
	}
	else if(i==-1)
	{
		numread=0;
		return -3;
	}

	numread+=i;

	if(numread < frm->lengthLength + length)
		return 0;

	numread=0;
	return frm->lengthLength + length;
}

int tcpclose(int sfd)
{
	return close(sfd);
}
