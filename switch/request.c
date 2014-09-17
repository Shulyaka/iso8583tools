#include "request.h"

const char* routeMessage(isomessage *message)
{
	isomessage::Destination *destination;

	if(message->destinationinterface_size()>0)
	{
		printf("Message already routed\n");
		destination=message->mutable_destinationinterface(message->destinationinterface_size());
	}
	else
	{
		destination=message->add_destinationinterface();
		destination->set_name("issuer"); //no real logic yet. Will be changed to something more clever later.
	}

	return destination->name().c_str();
}

int makeKey(isomessage *message, char *key, int size)
{
	snprintf(key, size, "%d", message->rrn());

	return 0;
}

int handleRequest(isomessage *message, int sfd, redisContext *rcontext)
{
	const char *dest;
	char key[100];
	int i;

	if(message->sourceindex()!=1 || message->sourceinterface_size()!=1)
	{
		printf("Error: Wrong number of sources of the request\n");
		return 1;
	}

	if((dest=routeMessage(message))==NULL)
	{
		printf("Error: Unable to route the message\n");
		return 1;
	}

	printf("Destination=\"%s\"\n", dest);

	if(!makeKey(message, key, sizeof(key)))
	{
		printf("Error: Unable to create unique key\n");
		return 1;
	}

	printf("Key=\"%s\"\n", key);

	i=kvsset(rcontext, key, message, message->timeout());
	if(i==0)
		return 2;
	else if(i<0)
		return 1;

	if(!ipcsendmsg(sfd, message, dest))
	{
		printf("Error: Unable to send the message\n");
		return 1;
	}

	return 0;
}
