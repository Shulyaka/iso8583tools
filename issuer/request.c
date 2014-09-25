#include "request.h"

int handleRequest(isomessage *message, int sfd)
{
	if(message->messagefunction()==isomessage::REQUEST)
		message->set_messagefunction(isomessage::REQUESTRESP);
	else if(message->messagefunction()==isomessage::ADVICE)
		message->set_messagefunction(isomessage::ADVICERESP);

	//message->set_responsecode(14);

	isomessage::Destination *destination=message->add_destinationinterface();
	destination->set_name("debit");

	ipcsendmsg(sfd, message, "switch");

	return 0;
}
