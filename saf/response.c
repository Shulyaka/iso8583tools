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
	{
		printf("Received a normal response. Ignoring.\n");
		return 0;
	}

	if(!strncmp(message->currentinterface().c_str(), "saf", 3))
	{
		printf("Error: Received a message from myself! It shouldn't happen!\n");
		return 0;
	}

	message->clear_responsecode();

	isomessage::Destination *destination=message->add_destinationinterface();
	destination->set_name(message->currentinterface());

	message->set_messagefunction(isomessage::ADVICE);

	message->set_timeout(message->timeout()-message->firsttransmissiontime());

	if(makeKey(message, key, sizeof(key)))
	{
		printf("Error: Unable to create unique key\n");
		return 1;
	}

	printf("Key=\"%s\"\n", key);

	message->set_currentinterface("saf");
	message->clear_sourceinterface();

	i=kvsset(rcontext, key, message);
	if(i==0)
		return 2;
	else if(i<0)
		return 1;
	return 0;
}

int handleExpired(char *key, int sfd, redisContext *rcontext)
{
	int i;

	isomessage message;

	i=kvsget(rcontext, key, &message);
	if(i==0)
		return 2;
	else if(i<0)
		return 1;

	printf("\nFound expired message with key %s:\n", key);
	message.PrintDebugString();

	message.set_timeout(message.timeout()-message.firsttransmissiontime()<10?10:message.timeout()-message.firsttransmissiontime());

	if(strcmp(message.currentinterface().c_str(), "saf")) //send the response on behalf of a failed interface
	{
		message.clear_destinationinterface();
		if(message.messagefunction()==isomessage::REQUEST)
			message.set_messagefunction(isomessage::REQUESTRESP);
		else if(message.messagefunction()==isomessage::ADVICE)
			message.set_messagefunction(isomessage::ADVICERESP);
		message.set_responsecode(96);

		if(ipcsendmsg(sfd, &message, "switch")<=0)
		{
			printf("Error: Unable to send the message to switch.\n");
			return 1;
		}

		if(message.messagefunction()==isomessage::ADVICERESP) //don't send reversal for advices - they would be resent by saf instead
		{
			if(!memcmp(key, "saf", 3))
			{
				i=kvsdel(rcontext, key);
				if(i==0)
					return 2;
				else if(i<0)
					return 1;
			}

			return 0;
		}

		message.set_messageclass(isomessage::REVERSAL);
		message.set_messagefunction(isomessage::ADVICE);
		isomessage::Destination *destination=message.add_destinationinterface();
		destination->set_name(message.currentinterface());
		message.set_currentinterface("saf");
	}

	message.clear_sourceinterface();

	if(ipcsendmsg(sfd, &message, "switch")<=0)
	{
		printf("Error: Unable to send the message to switch.\n");
		return 1;
	}

	if(!memcmp(key, "saf", 3))
	{
		i=kvsdel(rcontext, key);
		if(i==0)
			return 2;
		else if(i<0)
			return 1;
	}
	return 0;
}

int checkExpired(int sfd, redisContext *rcontext)
{
	int ret, i;
	char **keys;
	int n=kvslistexpired(rcontext, &keys);

	if(n==0)
		return 0;

	for(i=0; i<n; i++)
	{
		ret=handleExpired(keys[i], sfd, rcontext);
		if(ret)
			return ret;
	}

	kvsfreelist(keys, n);

	return 0;
}

