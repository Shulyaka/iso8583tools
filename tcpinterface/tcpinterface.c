#include <stdio.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include "../parser/parser.h"
#include "net.h"
#include "tcp.h"
#include "../lib/ipc.h"

int declineMsg(isomessage*);

int main(void)
{
	struct pollfd sfd[3];
	const long timeout=500; //connection timeout in seconds

	struct timespec ts_timeout;

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

			ts_timeout.tv_sec=timeout;
			ts_timeout.tv_nsec=0;
			errno=0;

			size=ppoll(sfd, 2, &ts_timeout, NULL);

			printf("poll: %d: %hd, %hd: %s\n", size, sfd[0].revents, sfd[1].revents, strerror(errno));

			if(size==-1)
			{
				printf("Error: poll (%hd, %hd): %s\n", sfd[0].revents, sfd[1].revents, strerror(errno));
				if(sfd[1].revents)
					break;
				else
				{
					usleep(100000);
					continue;
				}
			}
			else if(size==0)
			{
				printf("Error: Connection is inactive, closing it %ld, %ld\n", ts_timeout.tv_sec, ts_timeout. tv_nsec);
				break;
			}

			if(sfd[1].revents & POLLIN)
			{
				printf("Receiving message from net\n");

				size=tcprecvmsg(sfd[1].fd, &pmessage, frm);

				if(size==-1)
				{
					printf("Closing connection\n");
					break;
				}
				else if(size==0)
					continue;

				print_message(pmessage);

				if(isNetMgmt(pmessage))
				{
					if(isNetRequest(pmessage))
					{
						if(!processNetMgmt(pmessage))
						{
							printf("Error: Unable to process Network Management request. Message dropped.\n");
							freeField(pmessage);
							continue;
						}

						print_message(pmessage);

						size=tcpsendmsg(sfd[1].fd, pmessage);

						if(size==-1)
						{
							printf("Closing connection\n");
							freeField(pmessage);
							break;
						}
						else if(size==0)
						{
							freeField(pmessage);
							continue;
						}

						printf("Network Management Message sent (%d bytes)\n", size);
					}

					freeField(pmessage);
					continue;
				}

				if(translateNetToSwitch(&smessage, pmessage)!=0)
				{
					printf("Error: Unable to translate the message to format-independent representation.\n");

					if(isNetRequest(pmessage))
					{
						if(!declineNetMsg(pmessage))
						{
							printf("Error: Unable to decline the request. Message dropped.\n");
							freeField(pmessage);
							continue;
						}

						print_message(pmessage);

						size=tcpsendmsg(sfd[1].fd, pmessage);

						if(size==-1)
						{
							printf("Closing connection\n");
							freeField(pmessage);
							break;
						}
						else if(size==0)
						{
							freeField(pmessage);
							continue;
						}

						printf("Decline message sent (%d bytes)\n", size);
					}

					freeField(pmessage);
					continue;
				}

				printf("Converted message:\n");
				smessage.PrintDebugString();

				size=ipcsendmsg(sfd[0].fd, &smessage, (char *)"switch");

				if(size<=0)
				{
					printf("Error: Unable to send the message to switch\n");

					if(isNetRequest(pmessage))
					{
						if(!declineNetMsg(pmessage))
						{
							printf("Error: Unable to decline the request. Message dropped.\n");
							freeField(pmessage);
							continue;
						}

						print_message(pmessage);

						size=tcpsendmsg(sfd[1].fd, pmessage);

						if(size==-1)
						{
							printf("Closing connection\n");
							freeField(pmessage);
							break;
						}
						else if(size==0)
						{
							freeField(pmessage);
							continue;
						}

						printf("Decline message sent (%d bytes)\n", size);
					}

					freeField(pmessage);
					continue;
				}

				freeField(pmessage);

				printf("Message sent, size is %d bytes.\n", size);
			}

			if(sfd[0].revents & POLLIN)
			{
				printf("Receiving message from switch\n");

				if(ipcrecvmsg(sfd[0].fd, &smessage)<0)
					continue;

				printf("\nOutgoingMessage:\n");
		                smessage.PrintDebugString();

				pmessage=translateSwitchToNet(&smessage, frm);

				if(!pmessage)
				{
					printf("Error: Unable to translate the message from format-independent representation.\n");

					if(isRequest(&smessage))
					{
						if(!declineMsg(&smessage))
						{
							printf("Error: Unable to decline the request. Message dropped.\n");
							continue;
						}

						smessage.PrintDebugString();

						size=ipcsendmsg(sfd[0].fd, &smessage, (char *)"switch");

						if(size<=0)
						{
							printf("Error: Unable to return the declined message to switch. Message dropped.\n");
							continue;
						}

						printf("Decline message sent (%d bytes)\n", size);

					}
					continue;
				}

				print_message(pmessage);

				size=tcpsendmsg(sfd[1].fd, pmessage);

				if(size==-1)
				{
					printf("Closing connection\n");

					if(isRequest(&smessage))
					{
						if(!declineMsg(&smessage))
						{
							printf("Error: Unable to decline the request. Message dropped.\n");
							freeField(pmessage);
							continue;
						}

						smessage.PrintDebugString();

						size=ipcsendmsg(sfd[0].fd, &smessage, (char *)"switch");

						if(size<=0)
						{
							printf("Error: Unable to return the declined message to switch. Message dropped.\n");
							freeField(pmessage);
							continue;
						}

						printf("Decline message sent (%d bytes)\n", size);

					}
					freeField(pmessage);
					break;
				}
				else if(size==0)
				{
					if(isRequest(&smessage))
					{
						if(!declineMsg(&smessage))
						{
							printf("Error: Unable to decline the request. Message dropped.\n");
							freeField(pmessage);
							continue;
						}

						smessage.PrintDebugString();

						size=ipcsendmsg(sfd[0].fd, &smessage, (char *)"switch");

						if(size<=0)
						{
							printf("Error: Unable to return the declined message to switch. Message dropped.\n");
							freeField(pmessage);
							continue;
						}

						printf("Decline message sent (%d bytes)\n", size);
					}
					freeField(pmessage);
					continue;
				}

				freeField(pmessage);

				printf("Message sent (%d bytes)\n", size);
			}

		}

		tcpclose(sfd[1].fd);

		printf("Disconnected.\n");
	}

	tcpclose(sfd[2].fd);
	ipcclose(sfd[0].fd);
	freeFormat(frm);
	google::protobuf::ShutdownProtobufLibrary();
	
	return 0;
}

int isRequest(isomessage *message)
{
	switch(message->messagefunction())
	{
		case isomessage::REQUEST:
			return 1;
		case isomessage::ADVICE:
			return 1;
		default:
			return 0;
	}
}

int declineMsg(isomessage *message)
{
	switch(message->messagefunction())
	{
		case isomessage::REQUEST:
			message->set_messagefunction(isomessage::REQUESTRESP);
			break;
		case isomessage::ADVICE:
			message->set_messagefunction(isomessage::ADVICERESP);
			break;
		default:
			return 1;
	}

	message->set_responsecode(96);

	return 0;
}
