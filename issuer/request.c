#include "request.h"

int handleRequest(isomessage *message, int sfd)
{
	message->set_messagefunction(isomessage::REQUESTRESP);

	message->set_responsecode(14);

	ipcsendmsg(sfd, message, "switch");

	return 0;
}
