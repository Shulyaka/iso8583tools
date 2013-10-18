#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "../parser/parser.h"
#include "net.h"
#include "tcp.h"
#include "../lib/ipc.h"

int main(void)
{
	int sct;
	int sfd;
	int i;
	int swfd;

	const unsigned int naptime=10000; //microseconds, must be less than a second
	const unsigned int tcptimeout=500; //seconds

	unsigned int timeoutcnt;

	char buf[1000];
	int size;

	field *pmessage;

	fldformat *frm;

	isomessage smessage;

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	frm=loadNetFormat();

	if(!frm)
	{
		printf("Error: Can't load format\n");
		return 1;
	}

	printf("Message format loaded\n");

	sct=tcpinit();

	if(sct==-1)
	{
		printf("Error: Unable to start TCP connection\n");
		freeFormat(frm);
		return 1;
	}

	swfd=ipcinit();

	if(swfd==-1)
	{
		printf("Error: Unable to connect to switch\n");
		close(sct);
		freeFormat(frm);
		return 1;
	}

	while (1)
	{
		printf("Waiting for a connection...\n");
		sfd=tcpconnect(sct);
		printf("Connected.\n");

		timeoutcnt=tcptimeout*(1000000/naptime);

		while (1)
		{
			i=tcpreceive(sfd, buf, sizeof(buf), frm);

			if(i==-2)
			{
				printf("Error: Unable to receive a net message: Connection closed by the remote host\n");
				close(sfd);
				break;
			}
			else if(i==-3)
			{
				printf("Error: Unable to receive a net message: %s\nClosing connection\n", strerror(errno));
				close(sfd);
				break;
			}
			else if(i==0)
			{
				if(timeoutcnt--==0)
				{
					printf("Error: Connection is inactive, closing it\n");
					close(sfd);
					break;
				}

				usleep(10000);
				continue;
			}
			else if(i==-1)
				printf("Warning: a corrupted message received\n");
			
			pmessage=parseNetMsg(buf, i, frm);

			if(!pmessage)
			{
				printf("Error: Unable to parse the message\n");
				continue;
			}

			print_message(pmessage, frm);

			smessage.Clear();

			if(convertNetMsg(&smessage, pmessage)!=0)
			{
				printf("Error: Unable to translate the message to format-independent representation.\n");
				freeField(pmessage);
				continue;
			}

			printf("Converted message:\n");
			smessage.PrintDebugString();

			size=smessage.ByteSize();
			if(size>sizeof(buf))
			{      
				printf("Error: Message is too big (%d bytes)\n", size);
				freeField(pmessage);
				continue;
			}      

			smessage.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buf);

			if(ipcsend(swfd, buf, size)!=0)
			{
				printf("Error: Unable to send the message to Switch\n");
				freeField(pmessage);
				continue;
			}

			printf("Message sent, size is %d bytes.\n", size);

		}
		printf("Disconnected.\n");
	}

	close(sct);
	close(swfd);
	freeFormat(frm);
	google::protobuf::ShutdownProtobufLibrary();
	
	return 0;
}

