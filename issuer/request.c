#include "request.h"

int handleRequest(isomessage *message, int sfd)
{
	message->set_messagetype(message->messagetype() | isomessage::RESPONSE);

	//message->set_responsecode(14);

	isomessage::Destination *destination=message->add_destinationinterface();
	destination->set_name("debit");

	ipcsendmsg(sfd, message, "switch");

	return 0;
}
