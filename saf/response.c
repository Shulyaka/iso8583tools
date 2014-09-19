#include "response.h"

int makeKey(isomessage *message, char *key, int size)
{
	snprintf(key, size, "saf%s%d", message->currentinterface().c_str(), message->rrn());

	return 0;
}
int handleResponse(isomessage *message, int sfd, redisContext *rcontext)
{
	char key[100];
	int i;

	if(message->responsecode()!=96)
		return 0;

	message->clear_responsecode();

	isomessage::Destination *destination=message->add_destinationinterface();
	destination->set_name(message->currentinterface());

	message->set_currentinterface("saf");
	message->clear_sourceinterface();

	message->clear_timeout();

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
	return 0;
}
