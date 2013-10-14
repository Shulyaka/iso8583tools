#include <stdio.h>
#include "net.h"

fldformat* loadNetFormat(void)
{
	return load_format((char*)"../parser/formats/fields_visa");
}

void processNetMsg(char *buf, unsigned int length, fldformat *frm)
{
	field *message;
	isomessage visamsg;
	printf("\nMessage received, length %d\n", length);

	//printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	message=parse_message(buf, length, frm);   //TODO: For Visa, parse header (frm->fld[1]) and message(frm->fld[2]) separately to handle reject header properly.

	if(!message)
	{
		printf("Error: Unable to parse the message\n");
		return;
	}

	print_message(message, frm);

	if(message->fld[2]->fld[2]->length)
		visamsg.set_pan(message->fld[2]->fld[2]->data);

	visamsg.set_isoversion(isomessage::ISO1987);

	switch(message->fld[2]->fld[0]->data[1])
	{
		case '1':
			visamsg.set_messageclass(isomessage::AUTHORIZATION);
			break;
		case '2':
			visamsg.set_messageclass(isomessage::FINANCIAL);
			break;
		case '3':
			visamsg.set_messageclass(isomessage::FILEACTIONS);
			break;
		case '4':
			visamsg.set_messageclass(isomessage::REVERSAL);
			break;
//		case '5':
//			visamsg.set_messageclass(isomessage::RECONCILIATION);
//			break;
//		case '6':
//			visamsg.set_messageclass(isomessage::ADMINISTRATIVE);
//			break;
//		case '7':
//			visamsg.set_messageclass(isomessage::FEECOLLECTION);
//			break;
		case '8':
			visamsg.set_messageclass(isomessage::NETWORKMANAGEMENT);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[2]->fld[0]->data);
			return;
	}

	switch(message->fld[2]->fld[0]->data[2])
	{
		case '0':
			visamsg.set_messagefunction(isomessage::REQUEST);
			break;
		case '1':
			visamsg.set_messagefunction(isomessage::REQUESTRESP);
			break;
		case '2':
			visamsg.set_messagefunction(isomessage::ADVICE);
			break;
		case '3':
			visamsg.set_messagefunction(isomessage::ADVICERESP);
			break;
		case '4':
			visamsg.set_messagefunction(isomessage::NOTIFICATION);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[2]->fld[0]->data);
			return;
	}

	switch(message->fld[2]->fld[0]->data[3])
	{
		case '0':
			visamsg.set_messageorigin(isomessage::ACQUIRER);
			break;
		case '1':
			visamsg.set_messageorigin(isomessage::ACQREPEAT);
			break;
		case '2':
			visamsg.set_messageorigin(isomessage::ISSUER);
			break;
		case '3':
			visamsg.set_messageorigin(isomessage::ISSREPEAT);
			break;
		default:
			printf("Error: Unknown message type [%s]\n", message->fld[2]->fld[0]->data);
			return;
		}
	
}

