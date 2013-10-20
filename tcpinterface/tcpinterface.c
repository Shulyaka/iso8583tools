#include <stdio.h>
#include <errno.h>
#include <poll.h>
//#include <unistd.h>
#include "../parser/parser.h"
#include "net.h"
#include "tcp.h"
#include "../lib/ipc.h"

int main(void)
{
	int i;

	struct pollfd sfd[3];
	const struct timespec timeout={500,0}; //connection timeout in seconds

	char buf[10000];

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

	sfd[2].fd=tcpinit();

	if(sfd[2].fd==-1)
	{
		printf("Error: Unable to start TCP connection\n");
		freeFormat(frm);
		return 1;
	}

	sfd[0].fd=ipcopen((char *)"visa");

	if(sfd[0].fd==-1)
	{
		printf("Error: Unable to connect to switch\n");
		close(sfd[2].fd);
		freeFormat(frm);
		return 1;
	}

	sfd[0].events=POLLIN;
	sfd[1].events=POLLIN;

	while (1)
	{
		printf("Waiting for a connection...\n");

		sfd[1].fd=tcpconnect(sfd[2].fd);
		printf("Connected.\n");

		while (1)
		{
			printf("Waiting for a message...\n");

			i=ppoll(sfd, 2, &timeout, NULL);

			if(i==-1)
			{
				printf("Error: poll (%hd, %hd): %s\n", sfd[0].revents, sfd[1].revents, strerror(errno));
				if(sfd[1].revents)
				{
					tcpclose(sfd[1].fd);
					break;
				}
				else
					continue;
			}

			if(i==0)
			{
				printf("Error: Connection is inactive, closing it\n");
				close(sfd[1].fd);
				break;
			}

			if(sfd[1].revents & POLLIN)
			{
				printf("Receiving message from net\n");

				pmessage=tcprecvmsg(sfd[1].fd, frm);

				if(!pmessage)
				{
					if(errno)
					{
						printf("Closing connection\n");
						close(sfd[1].fd);
						break;
					}
					else
						continue;
				}

				print_message(pmessage, frm);

				if(translateNetToSwitch(&smessage, pmessage)!=0)
				{
					printf("Error: Unable to translate the message to format-independent representation.\n");
					freeField(pmessage);
					continue;
				}

				freeField(pmessage);

				printf("Converted message:\n");
				smessage.PrintDebugString();

				size=ipcsendmsg(sfd[0].fd, &smessage, (char *)"switch");

				if(size<=0)
				{
					printf("Error: Unable to send the message to switch\n");
					continue;
				}

				printf("Message sent, size is %d bytes.\n", size);
			}

			if(sfd[0].revents & POLLIN)
			{
				printf("Receiving message from switch\n");

				if(ipcrecvmsg(sfd[0].fd, &smessage)<0)
					continue;

				printf("\nOutgoingMessage:\n");
		                smessage.PrintDebugString();

//				if(translateSwitchToNet(&smessage, pmessage)!=0)
//				{
//					printf("Error: Unable to translate the message from format-independent representation.\n");
//					continue;
//				}


			}

		}
		printf("Disconnected.\n");
	}

	tcpclose(sfd[2].fd);
	ipcclose(sfd[0].fd);
	freeFormat(frm);
	google::protobuf::ShutdownProtobufLibrary();
	
	return 0;
}

