#include <stdio.h>
#include "net.h"
#include <sys/socket.h>
#include <errno.h>

int ipcconnect(int);

fldformat* loadNetFormat(void)
{
	return load_format((char*)"../parser/formats/fields_mast");
}

field* parseNetMsg(char *buf, unsigned int length, fldformat *frm)
{
	field *message;

	if(!buf)
	{
		printf("Error: no buf\n");
		return NULL;
	}

	if(!frm)
	{
		printf("Error: no frm\n");
		return NULL;
	}
	
	printf("\nMessage received, length %d\n", length);

	message=parse_message(buf, length, frm);

	return message;
}

int translateNetToSwitch(isomessage *mastmsg, field *message)
{

	if(!message)
	{
		printf("Error: No message\n");
		return 1;
	}

	if(!mastmsg)
	{
		printf("Error: no mastmsg\n");
		return 1;
	}

	mastmsg->Clear();

	mastmsg->set_isoversion(isomessage::ISO1987);

	switch(message->fld[2]->fld[0]->data[1])
	{
		case '1':
			mastmsg->set_messageclass(isomessage::AUTHORIZATION);
			break;
		case '2':
			mastmsg->set_messageclass(isomessage::FINANCIAL);
			break;
		case '3':
			mastmsg->set_messageclass(isomessage::FILEACTIONS);
			break;
		case '4':
			mastmsg->set_messageclass(isomessage::REVERSAL);
			break;
//		case '5':
//			mastmsg->set_messageclass(isomessage::RECONCILIATION);
//			break;
//		case '6':
//			mastmsg->set_messageclass(isomessage::ADMINISTRATIVE);
//			break;
//		case '7':
//			mastmsg->set_messageclass(isomessage::FEECOLLECTION);
//			break;
		case '8':
			mastmsg->set_messageclass(isomessage::NETWORKMANAGEMENT);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[2]->fld[0]->data);
			return 2;
	}

	switch(message->fld[2]->fld[0]->data[2])
	{
		case '0':
			mastmsg->set_messagefunction(isomessage::REQUEST);
			break;
		case '1':
			mastmsg->set_messagefunction(isomessage::REQUESTRESP);
			break;
		case '2':
			mastmsg->set_messagefunction(isomessage::ADVICE);
			break;
		case '3':
			mastmsg->set_messagefunction(isomessage::ADVICERESP);
			break;
		case '4':
			mastmsg->set_messagefunction(isomessage::NOTIFICATION);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[2]->fld[0]->data);
			return 2;
	}

	switch(message->fld[2]->fld[0]->data[3])
	{
		case '0':
			mastmsg->set_messageorigin(isomessage::ACQUIRER);
			break;
		case '1':
			mastmsg->set_messageorigin(isomessage::ACQREPEAT);
			break;
		case '2':
			mastmsg->set_messageorigin(isomessage::ISSUER);
			break;
		case '3':
			mastmsg->set_messageorigin(isomessage::ISSREPEAT);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[2]->fld[0]->data);
			return 2;
		}

	if(message->fld[2]->fld[2]->length)
		mastmsg->set_pan(message->fld[2]->fld[2]->data);

	return 0;
}

