#include "request.h"

int handleRequest(isomessage *message, int sfd)
{
	message->set_messagefunction(isomessage::REQUESTRESP);

	message->set_responsecode(0);

	ipcsendmsg(sfd, message, message->sourceinterface().c_str());

	return 0;
}
