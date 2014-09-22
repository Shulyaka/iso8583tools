#include <time.h>
#include "reqresp.h"

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

		if(message->messageorigin()==isomessage::ISSUER || message->messageorigin()==isomessage::ISSREPEAT) //no real logic yet. Will be changed to something more clever later.
			destination->set_name("visa");
		else
			destination->set_name("issuer");
	}

	return destination->name().c_str();
}

int makeKey(isomessage *message, char *key, int size)
{
	snprintf(key, size, "switch%s%s%d", message->currentinterface().c_str(), (message->messagefunction()==isomessage::ADVICE || message->messagefunction()==isomessage::ADVICERESP)?"A":"R", message->rrn());

	return 0;
}

int mergeResponse(isomessage *message, isomessage *newmessage)
{
	int i;
	isomessage::Destination *destination;

	message->mutable_destinationinterface()->RemoveLast();

	for(i=0; i<newmessage->destinationinterface_size(); i++)
	{
		destination=message->add_destinationinterface();
		destination->CopyFrom(newmessage->destinationinterface(i));
	}

	message->set_responsecode(newmessage->responsecode());

	return 0;
}

int saveContext(isomessage *message)
{
	isomessage::Source *source=message->add_sourceinterface();
	source->set_name(message->currentinterface());
	source->set_context(message->currentcontext());

	return 0;
}

int handleRequest(isomessage *message, int sfd, redisContext *rcontext)
{
	const char *dest;
	char key[100];
	int i;

	saveContext(message);

	if(message->timeout()<500000000)
		message->set_timeout(time(NULL)+message->timeout());

	if(!message->has_firsttransmissiontime())
		message->set_firsttransmissiontime(time(NULL));

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

int handleResponse(isomessage *message, int sfd, redisContext *rcontext)
{
	char key[100];
	int i;
	int copied=0;
	
	isomessage newmessage;
	isomessage::Destination *destination;

	if(!makeKey(message, key, sizeof(key)))
	{
		printf("Error: Unable to create unique key\n");
		return 1;
	}

	printf("Key=\"%s\"\n", key);

	newmessage.CopyFrom(*message);

	i=kvsget(rcontext, key, message);

	if(i==0)
		return 2;
	else if(i<0)
		return 1;

	mergeResponse(message, &newmessage);

	if(!(message->messageclass()==isomessage::REVERSAL && message->messagefunction()==isomessage::ADVICERESP && !strcmp(message->sourceinterface(0).name().c_str(),"saf"))) //only accept other destinations if not auto reversal advice
	{
		for(i=message->destinationinterface_size(); i>=0; i--) //send pending advices
		{
			if(!(message->destinationinterface(i).flags()&isomessage::SAFADVICE) && !(message->messagefunction()==isomessage::ADVICERESP) && !(message->responsecode()!=0)) //only break if not advice and not decline
				break;

			if(!copied)
			{
				newmessage.CopyFrom(*message);
				newmessage.set_currentinterface("saf");
				newmessage.set_messagefunction(isomessage::ADVICE);
				copied=1;
			}

			handleRequest(&newmessage, sfd, rcontext); //error handler needed

			message->mutable_destinationinterface()->RemoveLast();
		}

		if(message->responsecode()==0)
		{
			if(i>=0) //send to next remaining destination
			{
				message->set_messagefunction(isomessage::REQUEST);
				return handleRequest(message, sfd, rcontext);
			}
		}
		else
		{
			for(i=message->sourceinterface_size(); i>=1; i--) //send reversal advices to all sources except first
			{
				switch(copied)
				{
					case 0:
						newmessage.CopyFrom(*message);
						newmessage.set_currentinterface("saf");
						newmessage.set_messagefunction(isomessage::ADVICE);
						//no break
					case 1:
						newmessage.set_messageclass(isomessage::REVERSAL);
						copied=2;
				}

				newmessage.set_currentcontext(message->sourceinterface(i).context());

				handleRequest(&newmessage, sfd, rcontext); //error handler needed
			}
		}
	}

	message->set_currentcontext(message->sourceinterface(0).context());

	if(!ipcsendmsg(sfd, message, message->currentinterface().c_str()))
	{
		printf("Error: Unable to send the response\n");
		return 1;
	}

	return 0;
}
