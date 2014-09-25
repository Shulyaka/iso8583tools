#include <time.h>
#include "reqresp.h"

int routeMessage(isomessage *message, char *dest)
{
	isomessage::Destination *destination;

	if(message->destinationinterface_size()>0)
	{
		printf("Message already routed\n");
		destination=message->mutable_destinationinterface(message->destinationinterface_size()-1);
	}
	else
	{
		destination=message->add_destinationinterface();

		if(message->messageorigin()==isomessage::ISSUER || message->messageorigin()==isomessage::ISSREPEAT) //no real logic yet. Will be changed to something more clever later.
			destination->set_name("visa");
		else
			destination->set_name("issuer");
	}

	strcpy(dest, destination->name().c_str());

	return 0;
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

	message->set_messagefunction(newmessage->messagefunction());

	if(newmessage->has_responsecode())
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

int restoreContext(isomessage *to, isomessage *from, int i)
{
	to->set_currentcontext(from->sourceinterface(i).context());
//	to->set_currentinterface(from->sourceinterface(i).name());

	return 0;
}

int handleRequest(isomessage *message, int sfd, redisContext *rcontext)
{
	char dest[50];
	char key[50];
	int i;

	saveContext(message);

	if(message->timeout()<500000000)
		message->set_timeout(time(NULL)+message->timeout());

	if(!message->has_firsttransmissiontime())
		message->set_firsttransmissiontime(time(NULL));

	if(routeMessage(message, dest) || !strcmp(dest, "switch"))
	{
		printf("Error: Unable to route the message\n");
		return 1;
	}

	printf("Destination=\"%s\"\n", dest);

	message->set_currentinterface(dest);
	message->clear_currentcontext();

	if(makeKey(message, key, sizeof(key)))
	{
		printf("Error: Unable to create unique key\n");
		return 1;
	}

	printf("Key=\"%s\"\n", key);

//	printf("Saving message:\n");
//	message->PrintDebugString();

	i=kvsset(rcontext, key, message);
	if(i==0)
		return 2;
	else if(i<0)
		return 1;

	message->clear_destinationinterface();
	message->clear_sourceinterface();

	printf("Sending request to %s\n", dest);

	if(ipcsendmsg(sfd, message, dest)<=0)
	{
		printf("Sending decline\n");

		if(message->messagefunction()==isomessage::REQUEST)
			message->set_messagefunction(isomessage::REQUESTRESP);
		else if(message->messagefunction()==isomessage::ADVICE)
			message->set_messagefunction(isomessage::ADVICERESP);

		message->set_responsecode(96);

		return handleResponse(message, sfd, rcontext);
	}

	return 0;
}

int reverseRequest(isomessage *message, int sfd, redisContext *rcontext)
{
	char src[50];

	printf("Attempting to send a decline\n");
	message->PrintDebugString();

	if(message->sourceinterface_size()==0)
	{
		printf("Error: Source unknown. Message dropped.\n");
		return 1;
	}

	if(message->messagefunction()==isomessage::REQUEST)
		message->set_messagefunction(isomessage::REQUESTRESP);
	else if(message->messagefunction()==isomessage::ADVICE)
		message->set_messagefunction(isomessage::ADVICERESP);

	message->set_responsecode(96);

	if(ipcsendmsg(sfd, message, message->sourceinterface(0).name().c_str())<=0)
	{
		printf("Error: Unable to send the decline. Message dropped.\n");
		return 1;
	}

	return 0;
}

int handleResponse(isomessage *message, int sfd, redisContext *rcontext)
{
	char dest[50];
	char key[50];
	int i;
	
	isomessage newmessage;
	isomessage::Destination *destination;

	if(makeKey(message, key, sizeof(key)))
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

//	printf("Merged message:\n");
//	message->PrintDebugString();

	if(!(message->messageclass()==isomessage::REVERSAL && message->messagefunction()==isomessage::ADVICERESP && !strcmp(message->sourceinterface(0).name().c_str(),"saf"))) //only accept other destinations if not auto reversal advice
	{
		for(i=message->destinationinterface_size()-1; i>=0; i--) //send pending advices
		{
			if(!(message->destinationinterface(i).flags()&isomessage::SAFADVICE) && !(message->messagefunction()==isomessage::ADVICERESP) && !(message->has_responsecode() && message->responsecode()!=0)) //only break if not advice and not decline
				break;

			printf("Sending saf advice %d, %d, %d\n", message->has_responsecode(), message->responsecode()!=0, message->responsecode());

			newmessage.CopyFrom(*message);
			newmessage.clear_sourceinterface();
			newmessage.set_currentinterface("saf");
			newmessage.set_messagefunction(isomessage::ADVICE);

			if(handleRequest(&newmessage, sfd, rcontext)==2)
				return 2;

			message->mutable_destinationinterface()->RemoveLast();
		}

		if(message->responsecode()==0)
		{
			if(i>=0) //send to next remaining destination
			{
				i=kvsdel(rcontext, key);

				if(i==0)
					return 2;
				else if(i<0)
					return 1;

				message->set_messagefunction(isomessage::REQUEST);
				return handleRequest(message, sfd, rcontext);
			}
		}
		else
		{
			for(i=message->sourceinterface_size()-1; i>=1; i--) //send reversal advices to all sources except first
			{
				newmessage.CopyFrom(*message);
				newmessage.clear_sourceinterface();
				newmessage.set_currentinterface("saf");
				newmessage.set_messagefunction(isomessage::ADVICE);
				newmessage.set_messageclass(isomessage::REVERSAL);

				restoreContext(&newmessage, message, i);

				if(handleRequest(&newmessage, sfd, rcontext)==2)
					return 2;
			}
		}
	}

	strcpy(dest, message->sourceinterface(0).name().c_str());
	restoreContext(message, message, 0);
	message->clear_sourceinterface();

	printf("Sending response to %s\n", dest);

	if(ipcsendmsg(sfd, message, dest)<=0)
	{
		printf("Error: Unable to send the response\n");
		isomessage::Source *source=message->add_sourceinterface();
		source->set_name(dest);
		return 1;
	}

	i=kvsdel(rcontext, key);

	if(i==0)
	{
		isomessage::Source *source=message->add_sourceinterface();
		source->set_name(dest);
		return 2;
	}
	else if(i<0)
	{
		isomessage::Source *source=message->add_sourceinterface();
		source->set_name(dest);
		return 1;
	}

	return 0;
}
