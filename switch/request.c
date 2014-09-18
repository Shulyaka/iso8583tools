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

		if(message->messageorigin==isomessage::ISSUER || message->messageorigin==isomessage::ISSREPEAT) //no real logic yet. Will be changed to something more clever later.
			destination->set_name("visa");
		else
			destination->set_name("issuer");
	}

	return destination->name().c_str();
}

int makeKey(isomessage *message, char *key, int size)
{
	snprintf(key, size, "switch%s%s%d", message->currentinterface(), (message->messagefunction()==isomessage::ADVICE || message->messagefunction()==isomessage::ADVICERESP)?"A":"R", message->rrn());

	return 0;
}

int handleRequest(isomessage *message, int sfd, redisContext *rcontext)
{
	const char *dest;
	char key[100];
	int i;

	isomessage::Source *source=message->add_sourceinterface();
	source->set_name(message->currentinterface());
	source->set_context(message->currentcontext());

	if((dest=routeMessage(message))==NULL)
	{
		printf("Error: Unable to route the message\n");
		return 1;
	}

	printf("Destination=\"%s\"\n", dest);

	message->set_currentinterface(dest);

	if(!makeKey(message, key, sizeof(key)))
	{
		printf("Error: Unable to create unique key\n");
		return 1;
	}

	printf("Key=\"%s\"\n", key);

	i=kvsset(rcontext, key, message);
	if(i==0)
		return 2;
	else if(i<0)
		return 1;

	message->clear_destinationinterface();
	message->clear_sourceinterface();

	if(!ipcsendmsg(sfd, message, dest))
	{
		printf("Error: Unable to send the message\n");
		return 1;
	}

	return 0;
}
