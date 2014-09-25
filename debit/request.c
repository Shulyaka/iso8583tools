#include "request.h"

int handleRequest(isomessage *message, int sfd)
{
	if(message->messagefunction()==isomessage::REQUEST)
		message->set_messagefunction(isomessage::REQUESTRESP);
	else if(message->messagefunction()==isomessage::ADVICE)
		message->set_messagefunction(isomessage::ADVICERESP);

	message->set_responsecode(0);

	ipcsendmsg(sfd, message, "switch");

	return 0;
}
