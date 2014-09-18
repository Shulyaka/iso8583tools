#include "response.h"

int makeKey(isomessage *message, char *key, int size);

int removeLastDestination(isomessage *message)
{


}

int mergeResponse(isomessage *message, isomessage *newmessage)
{
	isomessage::Destination *destination;

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

	i=kvsget(rcontext, key, message);

	if(i==0)
		return 2;
	else if(i<0)
		return 1;

	mergeResponse(message, &newmessage);

	for(i=message->destinationinterface_size(); i>=0; i--) //send pending advices
	{
		destination=message->destinationinterface(i);
		if(!destination->flags()&isomessage::SAFADVICE)
			break;

		newmessage.CopyFrom(message);
		newmessage.set_messagefunction(isomessage::ADVICE);
		newmessage.set_currentinterface("saf");

		handleRequest(&newmessage, sfd, rcontext); //error handler needed

		removeLastDestination(message);
	}

	if(i>=0) //send to next remaining destination
	{
		message->set_messagefunction(isomessage::REQUEST);
		return handleRequest(message, sfd, rcontext);
	}

	if(!ipcsendmsg(sfd, message, dest))
	{
		printf("Error: Unable to send the response\n");
		return 1;
	}

	return 0;
}
