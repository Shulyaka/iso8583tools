#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <netdb.h>
#include "net.h"
#include "tcp.h"

int tcpserverinit(const std::string &port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(NULL, port.c_str(), &hints, &result);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		s=1;
		setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s));

		if(bind(sfd, rp->ai_addr, rp->ai_addrlen)!=0)
		{
			close(sfd);
			continue;
		}

		if(listen(sfd, 1)==0)
			break;

		close(sfd);
	}

	freeaddrinfo(result);

	if (rp == NULL)
	{
		fprintf(stderr, "Could not bind\n");
		return -1;
	}

	return sfd;
}

int tcpclientconnect(const std::string &host, const std::string &port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	s = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
	if (s != 0)
	{
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	    return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sfd);
	}

	freeaddrinfo(result);

	if (rp == NULL)
	{
		//fprintf(stderr, "Could not connect\n");
		return -1;
	}

	return sfd;
}

int tcpserverconnect(int sct, std::string &host, std::string &port) //blocking
{
	struct pollfd sfd;
	struct sockaddr addr;
	socklen_t addrlen=sizeof(addr);
	char c_host[100];
	char c_port[10];

	sfd.fd=sct;
	sfd.events=POLLIN;

	if(poll(&sfd, 1, -1)==-1)
		return -1;

	sfd.fd=accept(sct, &addr, &addrlen);
	//fcntl(sfd.fd, F_SETFL, O_NONBLOCK);

	getnameinfo(&addr, addrlen, c_host, sizeof(c_host), c_port, sizeof(c_port), 0);

	host.assign(c_host);
	port.assign(c_port);
	return sfd.fd;
}

//returns:
//0: No data to receive or partial message received
//-1: Corrupted message
//-2: Connection is broken
//-3: Other error (see errno)
//>0: Valid message received
long int tcprecv(int sfd, field &message)
{
	static char buf[10000];
	size_t maxlen=sizeof(buf);
	static size_t toread=1;
	static size_t numread=0;
	long int i;

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
				toread=-i-numread;
				if((size_t)-i>maxlen) //shouldn't happen, parseNetMsg() should return reasonable values
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
long int tcpsend(int sfd, field &message)
{
	size_t length, numwritten=0;
	static char buf[10000];
	int i;

	length=serializeNetMsg(buf, sizeof(buf), message);

	while(length!=numwritten)
	{
		i=write(sfd, buf+numwritten, length-numwritten);

		if(i==-1 || i==0)
		{
			printf("Error: %lu of %lu bytes sent: Unable to send the message to network: %s\n", numwritten, length, strerror(errno));
			return -1;
		}

		numwritten+=i;
	}

	return numwritten;
}

int tcpclose(int sfd)
{
	return close(sfd);
}
