#include <stdio.h>
#include "net.h"
#include <sys/socket.h>
#include <errno.h>

int ipcconnect(int);

fldformat* loadNetFormat(void)
{
	return load_format((char*)"../parser/formats/fields_visa");
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

	message=parse_message(buf, length, frm);   //TODO: For Visa, parse header (frm->fld[0]) and message(frm->fld[1]) separately to handle reject header properly.

	return message;
}

int convertNetMsg(isomessage *visamsg, field *message)
{

	if(!message)
	{
		printf("Error: No message\n");
		return 1;
	}

	if(!visamsg)
	{
		printf("Error: no visamsg\n");
		return 1;
	}

	if(!message->fld[1])
	{
		printf("Error: No message body found\n");
		return 1;
	}

	visamsg->set_sourceinterface("visa");

	if(!message->fld[1]->fld[0])
	{
		printf("Error: No message type\n");
		return 1;
	}

	visamsg->set_isoversion(isomessage::ISO1987);

	switch(message->fld[1]->fld[0]->data[1])
	{
		case '1':
			visamsg->set_messageclass(isomessage::AUTHORIZATION);
			break;
		case '2':
			visamsg->set_messageclass(isomessage::FINANCIAL);
			break;
		case '3':
			visamsg->set_messageclass(isomessage::FILEACTIONS);
			break;
		case '4':
			visamsg->set_messageclass(isomessage::REVERSAL);
			break;
//		case '5':
//			visamsg->set_messageclass(isomessage::RECONCILIATION);
//			break;
//		case '6':
//			visamsg->set_messageclass(isomessage::ADMINISTRATIVE);
//			break;
//		case '7':
//			visamsg->set_messageclass(isomessage::FEECOLLECTION);
//			break;
		case '8':
			visamsg->set_messageclass(isomessage::NETWORKMANAGEMENT);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[1]->fld[0]->data);
			return 2;
	}

	switch(message->fld[1]->fld[0]->data[2])
	{
		case '0':
			visamsg->set_messagefunction(isomessage::REQUEST);
			break;
		case '1':
			visamsg->set_messagefunction(isomessage::REQUESTRESP);
			break;
		case '2':
			visamsg->set_messagefunction(isomessage::ADVICE);
			break;
		case '3':
			visamsg->set_messagefunction(isomessage::ADVICERESP);
			break;
		case '4':
			visamsg->set_messagefunction(isomessage::NOTIFICATION);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[1]->fld[0]->data);
			return 2;
	}

	switch(message->fld[1]->fld[0]->data[3])
	{
		case '0':
			visamsg->set_messageorigin(isomessage::ACQUIRER);
			break;
		case '1':
			visamsg->set_messageorigin(isomessage::ACQREPEAT);
			break;
		case '2':
			visamsg->set_messageorigin(isomessage::ISSUER);
			break;
		case '3':
			visamsg->set_messageorigin(isomessage::ISSREPEAT);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[1]->fld[0]->data);
			return 2;
		}

	if(message->fld[1]->fld[2])
		visamsg->set_pan(message->fld[1]->fld[2]->data);

	return 0;
}

