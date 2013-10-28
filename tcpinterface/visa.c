#include <stdio.h>
#include "net.h"
#include <sys/socket.h>
#include <errno.h>
#include <time.h>

int ipcconnect(int);

char stationid[7]="456789";

double pow01(unsigned char x)
{
	if(!x)
		return 1;

	return 0.1*pow01(x-1);
}

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

	if(!message)
		printf("Error: Unable to parse\n");

	return message;
}

unsigned int buildNetMsg(char *buf, unsigned int maxlength, field *message)
{
	unsigned int length;

	if(!buf)
	{
		printf("Error: no buf\n");
		return 0;
	}

	if(!message)
	{
		printf("Error: no message\n");
		return 0;
	}

	strcpy(add_field(message, 0,4 ), "0000");

	length=get_length(message);

	if(length==0 || snprintf(message->fld[0]->fld[4]->data, 5, "%04X", length - message->frm->lengthLength) > 4)
	{
		printf("Error: Unable to calculate the message length (%d)\n", length);
//		return 0;
	}

	return build_message(buf, maxlength, message);
	
}

int translateNetToSwitch(isomessage *visamsg, field *fullmessage)
{
	field *header;
	field *message;

	struct tm datetime, current;
	time_t now;

	char tmpstr[9];
	unsigned int tmpint;

	time(&now);
	gmtime_r(&now, &current);

	if(!fullmessage)
	{
		printf("Error: No message\n");
		return 1;
	}

	if(!visamsg)
	{
		printf("Error: no visamsg\n");
		return 1;
	}

	if(!fullmessage->fld[0])
	{
		printf("Error: No message header found\n");
		return 1;
	}

	if(!fullmessage->fld[1])
	{
		printf("Error: No message body found\n");
		return 1;
	}

	header=fullmessage->fld[0];
	message=fullmessage->fld[1];

	visamsg->Clear();

	visamsg->set_sourceinterface("visa");
	visamsg->set_sourcestationid(get_field(header, 6));
	visamsg->set_visaroundtripinf(get_field(header, 7));
	visamsg->set_visabaseiflags(get_field(header, 8));
	visamsg->set_visamsgstatusflags(get_field(header, 9));
	visamsg->set_batchnumber(get_field(header, 10));
	visamsg->set_visareserved(get_field(header, 11));
	visamsg->set_visauserinfo(get_field(header, 12));

	if(!has_field(message, 0))
	{
		printf("Error: No message type\n");
		return 1;
	}

	visamsg->set_isoversion(isomessage::ISO1987);
	visamsg->set_messageclass((isomessage_MsgClass)(get_field(message, 0)[1]-'0'));
	visamsg->set_messagefunction((isomessage_MsgFunction)(get_field(message, 0)[2]-'0'));
	visamsg->set_messageorigin((isomessage_MsgOrigin)(get_field(message, 0)[3]-'0'));

	if(has_field(message, 2))
		visamsg->set_pan(get_field(message, 2));

	if(has_field(message, 3))
	{
		switch(atoi(get_field(message, 3,1)))
		{
			case 00:
				visamsg->set_transactiontype(isomessage::PURCHASE);
				break;

			case 01:
				visamsg->set_transactiontype(isomessage::CASH);
				break;

			case 03:
				visamsg->set_transactiontype(isomessage::CHECK);
				break;

			case 10:
				visamsg->set_transactiontype(isomessage::ACCNTFUNDING);
				break;

			case 26:
				visamsg->set_transactiontype(isomessage::ORIGINALCREDIT);
				break;

			default:
				printf("Error: Unknown processing code: '%s'\n", get_field(message, 3,1));
				return 1;
		}

		switch(atoi(get_field(message,3,2)))
		{
			case 00:
				visamsg->set_accounttypefrom(isomessage::DEFAULT);
				break;

			case 10:
				visamsg->set_accounttypefrom(isomessage::SAVINGS);
				break;

			case 20:
				visamsg->set_accounttypefrom(isomessage::CHECKING);
				break;

			case 30:
				visamsg->set_accounttypefrom(isomessage::CREDIT);
				break;

			case 40:
				visamsg->set_accounttypefrom(isomessage::UNIVERSAL);
				break;

			default:
				printf("Warning: Unknown From account type: '%s'\n", get_field(message, 3,2));
		}

		switch(atoi(get_field(message, 3,3)))
		{
			case 00:
				visamsg->set_accounttypeto(isomessage::DEFAULT);
				break;

			case 10:
				visamsg->set_accounttypeto(isomessage::SAVINGS);
				break;

			case 20:
				visamsg->set_accounttypeto(isomessage::CHECKING);
				break;

			case 30:
				visamsg->set_accounttypeto(isomessage::CREDIT);
				break;

			case 40:
				visamsg->set_accounttypeto(isomessage::UNIVERSAL);
				break;

			default:
				printf("Warning: Unknown To account type: '%s'\n", get_field(message, 3,3));
		}
	}

	if(has_field(message,4))
		visamsg->set_amounttransaction(atol(get_field(message, 4)));

	if(has_field(message, 5))
		visamsg->set_amountsettlement(atol(get_field(message, 5)));

	if(has_field(message, 6))
		visamsg->set_amountbilling(atol(get_field(message, 6)));

	if(has_field(message, 7))
	{
		memset(&datetime, 0, sizeof(datetime));
		sscanf(get_field(message, 7), "%2d%2d%2d%2d%2d", &datetime.tm_mon, &datetime.tm_mday, &datetime.tm_hour, &datetime.tm_min, &datetime.tm_sec);
		datetime.tm_mon--;

		datetime.tm_year=current.tm_year;

		if(datetime.tm_mon>8 && current.tm_mon<3)
			datetime.tm_year--;
		else if(datetime.tm_mon<3 && current.tm_mon>8)
			datetime.tm_year++;

		visamsg->set_transactiondatetime(timegm(&datetime));
	}

	if(has_field(message, 9))
		visamsg->set_conversionratesettlement(atof(get_field(message, 9,1))*pow01(get_field(message, 9,0)[0]-'0'));
	
	if(has_field(message, 10))
		visamsg->set_conversionratebilling(atof(get_field(message, 10,1))*pow01(get_field(message, 10,0)[0]-'0'));
	
	if(has_field(message, 11))
	{
		printf("%d\n", get_field(message, 11));
		printf("%s\n", get_field(message, 11));

		visamsg->set_stan(get_field(message, 11));
	}

	if(has_field(message, 12))
		visamsg->set_terminaltime(get_field(message, 12));

	if(has_field(message, 13))
	{
		if(get_field(message, 13)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(get_field(message, 13)[0]=='0' && get_field(message, 13)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, get_field(message, 13));

		visamsg->set_terminaldate(tmpstr);
	}

	if(has_field(message, 14))
		visamsg->set_expirydate(get_field(message, 14));

	if(has_field(message, 15))
	{
		if(get_field(message, 15)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(get_field(message, 15)[0]=='0' && get_field(message, 15)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, get_field(message, 15));

		visamsg->set_settlementdate(tmpstr);
	}

	if(has_field(message, 16))
	{
		if(get_field(message, 16)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(get_field(message, 16)[0]=='0' && get_field(message, 16)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, get_field(message, 16));

		visamsg->set_conversiondate(tmpstr);
	}

	if(has_field(message, 18))
		visamsg->set_mcc(get_field(message, 18));

	if(has_field(message, 19))
		visamsg->set_acquirercountry(get_field(message, 19));

	if(has_field(message, 20))
		visamsg->set_issuercountry(get_field(message, 20));

	if(has_field(message, 22))
	{
		tmpint=0;

		switch(atoi(get_field(message, 22,1 )))
		{
			case 01:
				visamsg->set_entrymode(isomessage::MANUAL);
				break;
			case 02:
				visamsg->set_entrymode(isomessage::MAGSTRIPE);
				visamsg->set_entrymodeflags(isomessage::CVVUNRELIABLE);
				break;
			case 03:
				visamsg->set_entrymode(isomessage::BARCODE);
				break;
			case 04:
				visamsg->set_entrymode(isomessage::OPTICAL);
				break;
			case 05:
				visamsg->set_entrymode(isomessage::CHIP);
				break;
			case 07:
				visamsg->set_entrymode(isomessage::CHIP);
				visamsg->set_entrymodeflags(isomessage::CONTACTLESS);
				break;
			case 90:
				visamsg->set_entrymode(isomessage::MAGSTRIPE);
				break;
			case 91:
				visamsg->set_entrymode(isomessage::MAGSTRIPE);
				visamsg->set_entrymodeflags(isomessage::CONTACTLESS);
				break;
			case 95:
				visamsg->set_entrymode(isomessage::CHIP);
				visamsg->set_entrymodeflags(isomessage::CVVUNRELIABLE);
				break;
			case 96:
				visamsg->set_entrymode(isomessage::STORED);
				break;
			default:
				visamsg->set_entrymode(isomessage::EM_UNKNOWN);
		}

		switch(atoi(get_field(message, 22,2 )))
		{
			case 1:
				visamsg->set_terminalpincapabilities(isomessage::CANACCEPT);
				break;
			case 2:
				visamsg->set_terminalpincapabilities(isomessage::CANNOTACCEPT);
				break;
			case 8:
				visamsg->set_terminalpincapabilities(isomessage::PINPADDOWN);
				break;
			default:
				visamsg->set_terminalpincapabilities(isomessage::PC_UNKNOWN);
		}
	}

	if(has_field(message, 23))
		visamsg->set_cardsequencenumber(atoi(get_field(message, 23)));

	switch(atoi((get_field(message, 25))))
	{
		case 1:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::CARDHOLDERNOTPRESENT | isomessage::CARDNOTPRESENT);
			break;

		case 2:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::TERMUNATTENDED);
			break;

		case 3:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::MERCHANTSUSPICIOUS);
			break;

		case 5:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::CARDNOTPRESENT);
			break;

		case 6:
			visamsg->set_messageclass(isomessage::PREAUTHCOMPLETION);
			break;

		case 8:
			if(get_field(message, 126,13)[0]=='R' || !strcmp(get_field(message, 60,8 ), "02"))
				visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::RECURRING | isomessage::CARDNOTPRESENT | isomessage::CARDHOLDERNOTPRESENT | isomessage::TERMUNATTENDED);
			else
				visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::PHONEORDER | isomessage::CARDNOTPRESENT | isomessage::CARDHOLDERNOTPRESENT | isomessage::TERMUNATTENDED);
			break;

		case 51:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::NOTAUTHORIZED);
			break;

		case 59:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMMERCE | isomessage::CARDNOTPRESENT | isomessage::TERMUNATTENDED);
			break;

		case 71:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::FALLBACK);
			visamsg->set_entrymode(isomessage::MANUAL);
	}






	return 0;
}

field* translateSwitchToNet(isomessage *visamsg, fldformat *frm)
{
	field *header;
	field *message;
	field *fullmessage=(field*)calloc(1, sizeof(field));

	fullmessage->frm=frm;

	time_t datetime;

	add_field(fullmessage, 0);
	add_field(fullmessage, 1);

	header=fullmessage->fld[0];
	message=fullmessage->fld[1];

	strcpy(add_field(header, 2), "01");

	if(message->frm->fld[62] && message->frm->fld[62]->fld[0] && message->frm->fld[62]->fld[0]->dataFormat==FRM_BITMAP)
		strcpy(add_field(header, 3), "02");
	else
		strcpy(add_field(header, 3), "01");

	if(isRequest(visamsg))
	{
		strcpy(add_field(header, 5), "000000");
		strcpy(add_field(header, 7), "00000000");
		strcpy(add_field(header, 8), "0000000000000000");
		strcpy(add_field(header, 9), "000000000000000000000000");
		strcpy(add_field(header, 10), "00");
		strcpy(add_field(header, 11), "000000");
		strcpy(add_field(header, 12), "00");
	}
	else
	{
		strncpy(add_field(header, 5), visamsg->sourcestationid().c_str(), 6);
		strncpy(add_field(header, 7), visamsg->visaroundtripinf().c_str(), 8);
		strncpy(add_field(header, 8), visamsg->visabaseiflags().c_str(), 16);
		strncpy(add_field(header, 9), visamsg->visamsgstatusflags().c_str(), 24);
		strncpy(add_field(header, 10), visamsg->visamsgstatusflags().c_str(), 2);
		strncpy(add_field(header, 11), visamsg->visareserved().c_str(), 6);
		strncpy(add_field(header, 12), visamsg->visauserinfo().c_str(), 2);
	}
	
	strcpy(add_field(header, 6), stationid);

	add_field(message, 0)[0]='0';
	switch(visamsg->messageclass())
	{
		case isomessage::PREAUTHORIZATION:
		case isomessage::PREAUTHCOMPLETION:
			message->fld[0]->data[1]='1';
			break;
		default:
			message->fld[0]->data[1]='0'+visamsg->messageclass();
	}
	message->fld[0]->data[2]='0'+visamsg->messagefunction();
	message->fld[0]->data[3]='0'+visamsg->messageorigin();
	
	if(visamsg->has_pan())
		strcpy(add_field(message, 2), visamsg->pan().c_str());

	if(visamsg->has_responsecode())
		sprintf(add_field(message, 39), "%02d", visamsg->responsecode());

	if(visamsg->has_transactiontype())
	{
		switch(visamsg->transactiontype())
		{
			case isomessage::PURCHASE:
				strcpy(add_field(message, 3,1 ), "00");
				break;

			case isomessage::CASH:
				strcpy(add_field(message, 3,1 ), "01");
				break;

			case isomessage::CHECK:
				strcpy(add_field(message, 3,1 ), "03");
				break;

			case isomessage::ACCNTFUNDING:
				strcpy(add_field(message, 3,1 ), "10");
				break;

			case isomessage::ORIGINALCREDIT:
				strcpy(add_field(message, 3,1 ), "26");
				break;

			default:
				printf("Error: Unknown transaction type: %d\n", visamsg->transactiontype());
				freeField(fullmessage);
				return NULL;
		}

		switch(visamsg->accounttypefrom())
		{
			case isomessage::DEFAULT:
				strcpy(add_field(message, 3,2 ), "00");
				break;

			case isomessage::SAVINGS:
				strcpy(add_field(message, 3,2 ), "10");
				break;

			case isomessage::CHECKING:
				strcpy(add_field(message, 3,2 ), "20");
				break;

			case isomessage::CREDIT:
				strcpy(add_field(message, 3,2 ), "30");
				break;

			case isomessage::UNIVERSAL:
				strcpy(add_field(message, 3,2 ), "40");
				break;

			default:
				printf("Warning: Unknown account type From: '%d'. Using default.\n", visamsg->accounttypefrom());
				strcpy(add_field(message, 3,2 ), "00");
				break;
		}

		switch(visamsg->accounttypeto())
		{
			case isomessage::DEFAULT:
				strcpy(add_field(message, 3,3 ), "00");
				break;

			case isomessage::SAVINGS:
				strcpy(add_field(message, 3,3 ), "10");
				break;

			case isomessage::CHECKING:
				strcpy(add_field(message, 3,3 ), "20");
				break;

			case isomessage::CREDIT:
				strcpy(add_field(message, 3,3 ), "30");
				break;

			case isomessage::UNIVERSAL:
				strcpy(add_field(message, 3,3 ), "40");
				break;

			default:
				printf("Warning: Unknown account type To: '%d'. Using default.\n", visamsg->accounttypeto());
				strcpy(add_field(message, 3,3 ), "00");
				break;
		}
	}

	if(visamsg->has_amounttransaction())
		snprintf(add_field(message, 4), 13, "%012ld", visamsg->amounttransaction());

	if(visamsg->has_amountsettlement())
		snprintf(add_field(message, 5), 13, "%012ld", visamsg->amountsettlement());

	if(visamsg->has_amountbilling())
		snprintf(add_field(message, 6), 13, "%012ld", visamsg->amountbilling());

	if(visamsg->has_transactiondatetime())
	{
		datetime=visamsg->transactiondatetime();
		printf("UTC Transaction date time: %s\n", asctime(gmtime(&datetime)));
		strftime(add_field(message, 7), 11, "%m%d%H%M%S", gmtime(&datetime));
	}

	if(visamsg->has_stan())
		strncpy(add_field(message, 11), visamsg->stan().c_str(), 6);
	else
		printf("Warning: No STAN\n");

	if(visamsg->has_terminaltime())
		strcpy(add_field(message, 12), visamsg->terminaltime().c_str());

	if(visamsg->has_terminaldate())
		strcpy(add_field(message, 13), visamsg->terminaldate().c_str()+4);

	if(visamsg->has_expirydate() && isRequest(visamsg))
		strcpy(add_field(message, 14), visamsg->expirydate().c_str());

	if(visamsg->has_settlementdate() && !isRequest(visamsg))
		strcpy(add_field(message, 15), visamsg->settlementdate().c_str()+4);

	if(visamsg->has_mcc() && isRequest(visamsg))
		strcpy(add_field(message, 18), visamsg->mcc().c_str());

	if(visamsg->has_acquirercountry())
		strcpy(add_field(message, 19), visamsg->acquirercountry().c_str());

	if(visamsg->has_issuercountry())
		strcpy(add_field(message, 20), visamsg->issuercountry().c_str());
	
	if(isRequest(visamsg) && (visamsg->has_entrymode() || visamsg->has_terminalpincapabilities()))
	{
		switch(visamsg->entrymode())
		{
			case isomessage::MANUAL:
				strcpy(add_field(message, 22,1 ), "01");
				break;
			case isomessage::BARCODE:
				strcpy(add_field(message, 22,1 ), "03");
				break;
			case isomessage::OPTICAL:
				strcpy(add_field(message, 22,1 ), "04");
				break;
			case isomessage::CHIP:
				if(visamsg->entrymodeflags() & isomessage::CONTACTLESS)
					strcpy(add_field(message, 22,1 ), "07");
				else if(visamsg->entrymodeflags() & isomessage::CVVUNRELIABLE)
					strcpy(add_field(message, 22,1 ), "95");
				else
					strcpy(add_field(message, 22,1 ), "05");
				break;
			case isomessage::MAGSTRIPE:
				if(visamsg->entrymodeflags() & isomessage::CONTACTLESS)
					strcpy(add_field(message, 22,1 ), "91");
				else if(visamsg->entrymodeflags() & isomessage::CVVUNRELIABLE)
					strcpy(add_field(message, 22,1 ), "02");
				else
					strcpy(add_field(message, 22,1 ), "91");
				break;
			case isomessage::STORED:
				strcpy(add_field(message, 22,1 ), "96");
				break;
			default:
				strcpy(add_field(message, 22,1 ), "00");
		}

		switch(visamsg->terminalpincapabilities())
		{
			case isomessage::CANACCEPT:
				strcpy(add_field(message, 22,2 ), "1");
				break;
			case isomessage::CANNOTACCEPT:
				strcpy(add_field(message, 22,2 ), "2");
				break;
			case isomessage::PINPADDOWN:
				strcpy(add_field(message, 22,2 ), "8");
				break;
			default:
				strcpy(add_field(message, 22,2 ), "0");
		}
	}

	if(isRequest(visamsg) && visamsg->has_cardsequencenumber())
		snprintf(add_field(message, 23), 4, "%03d", visamsg->cardsequencenumber());

	if(visamsg->messageclass()==isomessage::PREAUTHORIZATION)
		strcpy(add_field(message, 25 ), "00");
	else if(visamsg->messageclass()==isomessage::PREAUTHCOMPLETION)
		strcpy(add_field(message, 25 ), "06");
	else if((visamsg->entrymodeflags() & isomessage::PHONEORDER) || (visamsg->entrymodeflags() & isomessage::RECURRING))
		strcpy(add_field(message, 25 ), "08");
	else if(visamsg->entrymodeflags() & isomessage::ECOMMERCE)
		strcpy(add_field(message, 25 ), "59");
	else if(visamsg->entrymodeflags() & isomessage::NOTAUTHORIZED)
		strcpy(add_field(message, 25 ), "51");
	else if(visamsg->entrymodeflags() & isomessage::MERCHANTSUSPICIOUS)
		strcpy(add_field(message, 25 ), "03");
	else if(visamsg->entrymodeflags() & isomessage::TERMUNATTENDED)
		strcpy(add_field(message, 25 ), "02");
	else if(visamsg->entrymodeflags() & isomessage::CARDHOLDERNOTPRESENT)
		strcpy(add_field(message, 25 ), "01");
	else if(visamsg->entrymodeflags() & isomessage::CARDNOTPRESENT)
		strcpy(add_field(message, 25 ), "05");
	else if((visamsg->entrymode() == isomessage::MANUAL) && (visamsg->entrymodeflags() & isomessage::FALLBACK))
		strcpy(add_field(message, 25 ), "71");
	else
		strcpy(add_field(message, 25 ), "00");



	return fullmessage;
}

int isNetMgmt(field *message)
{
	if(!message || !message->fld || !message->fld[1] || !message->fld[1]->fld || !message->fld[1]->fld[0] || !message->fld[1]->fld[0]->data)
		return 0;

	if(message->fld[1]->fld[0]->data[1]=='8')
		return 1;

	return 0;
}

int isNetRequest(field *message)
{
	if(!message || !message->fld || !message->fld[1] || !message->fld[1]->fld || !message->fld[1]->fld[0] || !message->fld[1]->fld[0]->data)
		return 0;

	if(message->fld[1]->fld[0]->data[2]=='0' || message->fld[1]->fld[0]->data[2]=='2')
		return 1;

	return 0;
}

int processNetMgmt(field *message)
{
	field *header;
	field *mbody;

	if(!message || !message->fld || !message->fld[0] || !message->fld[1])
		return 0;

	header=message->fld[0];
	mbody=message->fld[1];

	strncpy(add_field(header, 5), get_field(header, 6), 6);
	strncpy(add_field(header, 6), stationid, 6);

	if(get_field(mbody, 0)[2]=='0')
		add_field(mbody, 0)[2]='1';

	if(get_field(mbody, 0)[2]=='2')
		add_field(mbody, 0)[2]='3';

	strcpy(add_field(mbody, 39), "00");

	return 1;
}

int declineNetMsg(field *message)
{
	field *header;
	field *mbody;

	if(!message || !message->fld || !message->fld[0] || !message->fld[1])
		return 0;

	header=message->fld[0];
	mbody=message->fld[1];

	strncpy(add_field(header, 5), get_field(header, 6), 6);
	strncpy(add_field(header, 6), stationid, 6);

	if(get_field(mbody, 0)[2]=='0')
		add_field(mbody, 0)[2]='1';

	if(get_field(mbody, 0)[2]=='2')
		add_field(mbody, 0)[2]='3';

	add_field(header,9)[16]='1';

	strcpy(add_field(mbody, 39 ), "96");
	strcpy(add_field(mbody, 44,1 ), "5");

	remove_field(mbody, 18);
	remove_field(mbody, 22);
	remove_field(mbody, 35);
	remove_field(mbody, 43);
	remove_field(mbody, 60);
	remove_field(mbody, 104);

	return 1;
}
