#include "response.h"

int makeKey(isomessage *message, char *key, int size);

int removeLastDestination(isomessage *message)
{


}

int mergeResponse(isomessage *message, isomessage *newmessage)
{
	isomessage::Destination *destination;
	isomessage::Source *source;

	source=message->add_sourceinterface();
	source->set_name(newmessage->currentinterface());
	source->set_context(newmessage->currentcontext());

	removeLastDestination(message);

	for(i=0; i<newmessage->destinationinterface_size())
	{
		destination=message->add_destinationinterface();
		destination->CopyFrom(newmessage->destinationinterface(i));
	}

	message->set_responsecode(newmessage->responsecode());

	return 0;
}

int handleResponse(isomessage *message, int sfd, redisContext *rcontext)
{
	char key[100];
	int i;
	
	isomessage newmessage;
	isomessage::Destination *destination;

	if(!makeKey(message, key, sizeof(key)))
	{
		printf("Error: Unable to create unique key\n");
		return 1;
	}

	printf("Key=\"%s\"\n", key);

	newmessage.CopyFrom(message);

	i=kvsget(rcontext, key, message, message->timeout());

	if(i==0)
		return 2;
	else if(i<0)
		return 1;

	mergeResponse(message, &newmessage);

	for(i=message->destinationinterface_size(); i>=0; i--)
	{
		destination=message->destinationinterface(i);
		if(!destination->flags()&isomessage::SAFADVICE)
			break;

		//process advice

		removeLastDestination(message);
	}

	if(i>=0)
	{
		//process normal request
	}
	else
	{
		//process final response
	}

	return 0;
}
