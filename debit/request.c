#include "request.h"

int handleRequest(isomessage *message, int sfd)
{
	message->set_messagetype(message->messagetype() | isomessage::RESPONSE);

	message->set_responsecode(14);

	ipcsendmsg(sfd, message, "switch");

	return 0;
}
