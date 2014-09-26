#include <stdio.h>
#include "net.h"
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include "visa.pb.h"

int debug=1;

int ipcconnect(int);

int processIncoming(isomessage *visamsg, field *fullmessage, VisaContext *context);
field* processOutgoing(isomessage *visamsg, fldformat *frm, VisaContext *context);

char stationid[7]="456789";

double pow01(unsigned char x)
{
	if(!x)
		return 1;

	return 0.1*pow01(x-1);
}

fldformat* loadNetFormat(void)
{
	return load_format((char*)"../parser/formats/fields_visa.frm");
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

int fillDefaultContext(VisaContext *context)
{
	return 0;
}

int translateNetToSwitch(isomessage *visamsg, field *fullmessage)
{
	VisaContext context;
	int i;

	if(!fullmessage)
	{
		printf("Error: No message\n");
		return 1;
	}

	if(!visamsg)
	{
		printf("Error: No visamsg\n");
		return 1;
	}

	visamsg->Clear();

	i=processIncoming(visamsg, fullmessage, &context);

	if(i!=0)
		return i;

	visamsg->set_currentinterface("visa");

	if(!context.SerializeToString(visamsg->mutable_currentcontext()))
	{
		printf("Warning: Unable to serialize the context\n");
		return 1;
	}

	return 0;
}

field* translateSwitchToNet(isomessage *visamsg, fldformat *frm)
{
	VisaContext context;

	if(!frm)
	{
		printf("Error: No frm\n");
		return NULL;
	}

	if(!visamsg)
	{
		printf("Error: No visamsg\n");
		return NULL;
	}

	if(visamsg->has_currentcontext())
		context.ParseFromString(visamsg->currentcontext());
	else
		fillDefaultContext(&context);

	return processOutgoing(visamsg, frm, &context);
}

int processIncoming(isomessage *visamsg, field *fullmessage, VisaContext *context)
{
	field *header;
	field *message;

	struct tm datetime, current;
	time_t now;

	char tmpstr[26];
	unsigned int tmpint, i;

	time(&now);
	gmtime_r(&now, &current);

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

	context->set_sourcestationid(get_field(header, 6));
	context->set_visaroundtripinf(get_field(header, 7));
	context->set_visabaseiflags(get_field(header, 8));
	context->set_visamsgstatusflags(get_field(header, 9));
	context->set_batchnumber(get_field(header, 10));
	context->set_visareserved(get_field(header, 11));
	context->set_visauserinfo(get_field(header, 12));

	if(!has_field(message, 0))
	{
		printf("Error: No message type\n");
		return 1;
	}

	switch(get_field(message, 0)[1]-'0')
	{
		case 1:
			visamsg->set_messagetype(isomessage::AUTHORIZATION);
			break;
		case 2:
			visamsg->set_messagetype(isomessage::AUTHORIZATION | isomessage::CLEARING);
			break;
		case 4:
			visamsg->set_messagetype(isomessage::AUTHORIZATION | isomessage::REVERSAL);
			break;
		default:
			printf("Error: Unknown message type: '%s'\n", get_field(message, 0));
	}

	switch(get_field(message, 0)[2]-'0')
	{
		case 0:
			break;
		case 1:
			visamsg->set_messagetype(visamsg->messagetype() | isomessage::RESPONSE);
			break;
		case 2:
			visamsg->set_messagetype(visamsg->messagetype() | isomessage::ADVICE);
			break;
		case 3:
			visamsg->set_messagetype(visamsg->messagetype() | isomessage::ADVICE | isomessage::RESPONSE);
			break;
		default:
			printf("Error: Unknown message type: '%s'\n", get_field(message, 0));
	}

	switch(get_field(message, 0)[3]-'0')
	{
		case 0:
			break;
		case 1:
			visamsg->set_messagetype(visamsg->messagetype() | isomessage::REPEAT);
			break;
		case 2:
			visamsg->set_messagetype(visamsg->messagetype() | isomessage::ISSUER);
			break;
		case 3:
			visamsg->set_messagetype(visamsg->messagetype() | isomessage::ISSUER | isomessage::REPEAT);
			break;
		default:
			printf("Error: Unknown message type: '%s'\n", get_field(message, 0));
	}

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
		visamsg->set_amounttransaction(atoll(get_field(message, 4)));

	if(has_field(message, 5))
		visamsg->set_amountsettlement(atoll(get_field(message, 5)));

	if(has_field(message, 6))
		visamsg->set_amountbilling(atoll(get_field(message, 6)));

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
		visamsg->set_stan(atoi(get_field(message, 11)));

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
		visamsg->set_mcc(atoi(get_field(message, 18)));

	if(has_field(message, 19))
		visamsg->set_acquirercountry(atoi(get_field(message, 19)));

	if(has_field(message, 20))
		visamsg->set_issuercountry(atoi(get_field(message, 20)));

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
				visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::PINCAPABLE);
				break;
			case 2:
				visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::NOTPINCAPABLE);
				break;
			case 8:
				visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::PINCAPABLE | isomessage::NOTPINCAPABLE);
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
			visamsg->set_messagetype(visamsg->messagetype() | isomessage::PREAUTHCOMPLETION);
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

	if(has_field(message, 26))
		visamsg->set_termpinmaxdigits(atoi(get_field(message, 26)));

	if(has_field(message, 28))
	{
		if(get_field(message, 28)[0]=='C')
			visamsg->set_amountacquirerfee(-atoll(get_field(message, 28)+1));
		else
			visamsg->set_amountacquirerfee(atoll(get_field(message, 28)+1));
	}

	if(has_field(message, 32))
		visamsg->set_acquirerid(atoll(get_field(message, 32)));
	
	if(has_field(message, 33))
		visamsg->set_forwardingid(atoll(get_field(message, 33)));

	if(has_field(message, 35))
		visamsg->set_track2(get_field(message, 35));

	if(has_field(message, 37))
		visamsg->set_rrn(atoll(get_field(message, 37)));

	if(has_field(message, 38))
		visamsg->set_authid(get_field(message, 38));

	if(has_field(message, 39))
		visamsg->set_responsecode(atoi(get_field(message, 39)));

	if(has_field(message, 41))
	{
		strcpy(tmpstr, get_field(message, 41));

		for(i=7; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_terminalid(tmpstr);
	}
			
	if(has_field(message, 42))
	{
		strcpy(tmpstr, get_field(message, 42));

		for(i=14; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantid(tmpstr);
	}

	if(has_field(message, 43))
	{
		strcpy(tmpstr, get_field(message, 43,1));

		for(i=24; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantname(tmpstr);

		strcpy(tmpstr, get_field(message, 43,2));

		for(i=12; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantcity(tmpstr);

		visamsg->set_merchantcountry(get_field(message, 43,3));
	}

	if(has_field(message, 44,1))
	{
		if(get_field(message, 44,1)[0]=='5')
			visamsg->set_responsesource(isomessage::RSP_ISSUER);
		else
			visamsg->set_responsesource(isomessage::RSP_NETWORK);
	}

	switch(get_field(message, 44,2)[0])
	{
		case '\0':
		case ' ':
			break;

		case 'A':
			visamsg->set_addressverification(isomessage::MATCH);
			visamsg->set_postalcodeverification(isomessage::NOMATCH);
			break;

		case 'B':
			visamsg->set_addressverification(isomessage::MATCH);
			visamsg->set_postalcodeverification(isomessage::ERROR);
			break;

		case 'C':
			visamsg->set_addressverification(isomessage::ERROR);
			visamsg->set_postalcodeverification(isomessage::ERROR);
			break;

		case 'D':
		case 'F':
		case 'M':
		case 'X':
		case 'Y':
			visamsg->set_addressverification(isomessage::MATCH);
			visamsg->set_postalcodeverification(isomessage::MATCH);
			break;

		case 'G':
		case 'I':
		case 'R':
		case 'S':
		case 'U':
			visamsg->set_addressverification(isomessage::NOTPERFORMED);
			visamsg->set_postalcodeverification(isomessage::NOTPERFORMED);
			break;

		case 'N':
			visamsg->set_addressverification(isomessage::NOMATCH);
			visamsg->set_postalcodeverification(isomessage::NOMATCH);
			break;

		case 'P':
			visamsg->set_addressverification(isomessage::ERROR);
			visamsg->set_postalcodeverification(isomessage::MATCH);
			break;

		case 'W':
		case 'Z':
			visamsg->set_addressverification(isomessage::NOMATCH);
			visamsg->set_postalcodeverification(isomessage::MATCH);
			break;

		default:
			visamsg->set_addressverification(isomessage::ERROR);
			visamsg->set_postalcodeverification(isomessage::ERROR);
	}

	switch(get_field(message, 44,5)[0])
	{
		case '\0':
		case ' ':
			break;

		case '1':
			visamsg->set_cvvverification(isomessage::MATCH);
			break;

		case '2':
			visamsg->set_cvvverification(isomessage::NOMATCH);
			break;

		default:
			visamsg->set_cvvverification(isomessage::ERROR);
			break;
	}

	switch(get_field(message, 44,8)[0])
	{
		case '\0':
		case ' ':
			break;

		case '1':
			visamsg->set_cardauthenticationresults(isomessage::NOMATCH);
			break;

		case '2':
			visamsg->set_cardauthenticationresults(isomessage::MATCH);
			break;

		default:
			visamsg->set_cardauthenticationresults(isomessage::ERROR);
	}

	switch(get_field(message, 44,10)[0])
	{
		case '\0':
		case ' ':
			break;

		case 'M':
			visamsg->set_cvv2verification(isomessage::MATCH);
			break;

		case 'N':
			visamsg->set_cvv2verification(isomessage::NOMATCH);
			break;

		case 'P':
			visamsg->set_cvv2verification(isomessage::NOTPERFORMED);
			break;

		default:
			visamsg->set_cvv2verification(isomessage::ERROR);
			break;
	}

	if(has_field(message, 44,11))
		visamsg->set_originalresponsecode(atoi(get_field(message, 44,11)));

	switch(get_field(message, 44,13)[0])
	{
		case '\0':
		case ' ':
			break;

		case '0':
			visamsg->set_cavvverification(isomessage::ERROR);
			break;

		case '1':
			visamsg->set_cavvverification(isomessage::NOMATCH);
			break;

		case '2':
			visamsg->set_cavvverification(isomessage::MATCH);
			break;

		case 'B':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::MERCHANTSUSPICIOUS);
			//no break
		case '3':
		case '8':
		case 'A':
			visamsg->set_cavvverification(isomessage::MATCH);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMNOTSECUREISSUER);
			break;

		case '4':
		case '7':
		case '9':
			visamsg->set_cavvverification(isomessage::NOMATCH);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMNOTSECUREISSUER);
			break;

		case '6':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMNOTSECUREISSUER);
			visamsg->set_cavvverification(isomessage::NOTPERFORMED);
			break;

		case 'C':
			visamsg->set_cavvverification(isomessage::ERROR);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMNOTSECUREISSUER);
			break;

		case 'D':
		default:
			visamsg->set_cavvverification(isomessage::ERROR);
	}

	if(has_field(message, 45))
		visamsg->set_track1(get_field(message, 45));

	if(has_field(message, 48))
		visamsg->set_additionaltext(get_field(message, 48));

	if(has_field(message, 49))
		visamsg->set_currencytransaction(atoi(get_field(message, 49)));

	if(has_field(message, 51))
		visamsg->set_currencybilling(atoi(get_field(message, 51)));

	if(has_field(message, 52))
		visamsg->set_pin(get_field(message, 52));

	if(has_field(message, 53))
	{
		switch (atoi(get_field(message, 53,1)))
		{
			case 02:
				visamsg->set_pinsecurityformat(isomessage::ISSUERKEY);
				break;

			case 20:
				visamsg->set_pinsecurityformat(isomessage::ZONE);
				break;
		}

		if(!strcmp(get_field(message, 53,2), "01"))
			visamsg->set_pinencryptionalgorithm(isomessage::ANSIDES);

		switch (atoi(get_field(message, 53,3)))
		{
			case 01:
				visamsg->set_pinblockformat(isomessage::ISO0);
				break;

			case 02:
				visamsg->set_pinblockformat(isomessage::DOCUTEL);
				break;

			case 03:
				visamsg->set_pinblockformat(isomessage::DIEBOLD);
				break;

			case 04:
				visamsg->set_pinblockformat(isomessage::PLUS);
				break;
		}

		visamsg->set_pinkeyindex(atoi(get_field(message, 53,4)));

		if(!strcmp(get_field(message, 53,5), "01"))
			visamsg->set_pinispassword(true);
	}

	if(has_field(message, 54))
	{
		for(i=0; i<6; i++)
			if(has_field(message, 54,i))
			{
				isomessage::AddAmnt *addamnt=visamsg->add_additionalamount();

				addamnt->set_accounttype((isomessage_AccntType)atoi(get_field(message, 54,i,1)));

				addamnt->set_amounttype((isomessage_AddAmnt_AmntType)atoi(get_field(message, 54,i,2)));

				addamnt->set_currency(atoi(get_field(message, 54,i,3)));

				if(get_field(message, 54,i,4)[0]=='C')
					addamnt->set_amount(atoi(get_field(message, 54,i,5)));
				else
					addamnt->set_amount(-atoi(get_field(message, 54,i,5)));
			}
			else
				break;
	}

	for(i=0; i < has_field(message, 55,1); i++)
	{
		sscanf(get_tag(message, 55,1,i), "%X", &tmpint);

		switch(tmpint)
		{
			case 0x71:
				visamsg->set_issuerscript1(get_field(message, 55,1,i));
				break;
			case 0x72:
				visamsg->set_issuerscript2(get_field(message, 55,1,i));
				break;
			case 0x82:
				visamsg->set_applicationinterchangeprofile(get_field(message, 55,1,i));
				break;
			case 0x91:
				visamsg->set_issuerauthenticationdata(get_field(message, 55,1,i));
				break;
			case 0x95:
				visamsg->set_terminalverificationresults(get_field(message, 55,1,i));
				break;
			case 0x9A:
				visamsg->set_terminaltransactiondate(get_field(message, 55,1,i));
				break;
			case 0x9C:
				visamsg->set_cryptogramtransactiontype(get_field(message, 55,1,i));
				break;
			case 0xC0:
				visamsg->set_secondarypinblock(get_field(message, 55,1,i));
				break;
			case 0x5F2A:
				visamsg->set_cryptogramcurrency(get_field(message, 55,1,i));
				break;
			case 0x9F02:
				visamsg->set_cryptogramtransactionamount(get_field(message, 55,1,i));
				break;
			case 0x9F03:
				visamsg->set_cryptogramcashbackamount(get_field(message, 55,1,i));
				break;
			case 0x9F09:
				visamsg->set_applicationversionnumber(get_field(message, 55,1,i));
				break;
			case 0x9F10:
				visamsg->set_issuerapplicationdata(get_field(message, 55,1,i));
				break;
			case 0x9F1A:
				visamsg->set_terminalcountry(get_field(message, 55,1,i));
				break;
			case 0x9F1E:
				visamsg->set_terminalserialnumber(get_field(message, 55,1,i));
				break;
			case 0x9F26:
				visamsg->set_cryptogram(get_field(message, 55,1,i));
				break;
			case 0x9F27:
				visamsg->set_cryptograminformationdata(get_field(message, 55,1,i));
				break;
			case 0x9F33:
				visamsg->set_terminalcapabilityprofile(get_field(message, 55,1,i));
				break;
			case 0x9F34:
				visamsg->set_cvmresults(get_field(message, 55,1,i));
				break;
			case 0x9F35:
				visamsg->set_cryptogramterminaltype(get_field(message, 55,1,i));
				break;
			case 0x9F36:
				visamsg->set_applicationtransactioncounter(get_field(message, 55,1,i));
				break;
			case 0x9F37:
				visamsg->set_unpredictablenumber(get_field(message, 55,1,i));
				break;
			case 0x9F5B:
				visamsg->set_issuerscriptresults(get_field(message, 55,1,i));
				break;
			case 0x9F6E:
				visamsg->set_formfactorindicator(get_field(message, 55,1,i));
				break;
			case 0x9F7C:
				visamsg->set_customerexclusivedata(get_field(message, 55,1,i));
				break;
			default:
				printf("No map for tag %X\n", tmpint);
		}
	}

	if(has_field(message, 59))
		visamsg->set_merchantaddress(get_field(message, 59));

	switch(get_field(message, 60,1)[0])
	{
		case '\0':
			break;

		case '1':
			visamsg->set_terminaltype(isomessage::LIMITEDAMOUNT);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::NOTAUTHORIZED | isomessage::TERMUNATTENDED);
			break;

		case '2':
			visamsg->set_terminaltype(isomessage::ATM);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::TERMUNATTENDED);
			break;

		case '3':
			visamsg->set_terminaltype(isomessage::SELFSERVICE);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::TERMUNATTENDED);
			break;

		case '4':
			visamsg->set_terminaltype(isomessage::CASHREGISTER);
			break;

		case '5':
			visamsg->set_terminaltype(isomessage::PERSONAL);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::CARDNOTPRESENT);
			break;

		case '7':
			visamsg->set_terminaltype(isomessage::PHONE);
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::CARDNOTPRESENT);
			break;

		default:
			visamsg->set_terminaltype(isomessage::TT_UNKNOWN);
	}

	switch(get_field(message, 60,2)[0])
	{
		case '\0':
		case '0':
			break;

		case '1':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::TERMNOTPRESENT);
			break;

		case '2':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::MAGSTRIPECAPABLE);
			break;

		case '3':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::BARCODECAPABLE);
			break;

		case '4':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::OCRCAPABLE);
			break;

		case '5':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::CHIPCAPABLE | isomessage::MAGSTRIPECAPABLE);
			break;

		case '8':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::CONTACTLESSCAPABLE);
			break;

		case '9':
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::NOREADCAPABLE);
			break;
	}

	if(get_field(message, 60,3)[0]=='2')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::LASTTERMCHIPREADFAILED);

	if(get_field(message, 60,4)[0]=='9')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::EXISTINGDEBT);

	if(get_field(message, 60,6)[0]=='2')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::EXPANDEDTHIRDBM);

	switch(get_field(message, 60,7)[0])
	{
		case '\0':
		case '0':
			break;

		case '1':
			visamsg->set_cardauthenticationreliability(isomessage::ACQUNRELIABLE);
			break;

		case '2':
			visamsg->set_cardauthenticationreliability(isomessage::ACQINACTIVE);
			break;

		case '3':
			visamsg->set_cardauthenticationreliability(isomessage::ISSINACTIVE);
			break;
	}

	switch(atoi(get_field(message, 60,8)))
	{
		case 0:
			break;

		case 1:
		case 4:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::PHONEORDER | isomessage::CARDNOTPRESENT | isomessage::CARDHOLDERNOTPRESENT | isomessage::TERMUNATTENDED);
			break;

		case 2:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::RECURRING | isomessage::CARDNOTPRESENT | isomessage::CARDHOLDERNOTPRESENT | isomessage::TERMUNATTENDED);
			break;

		case 3:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::EM_INSTALLMENT);
			break;

		case 5:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMMERCE | isomessage::CARDNOTPRESENT | isomessage::TERMUNATTENDED);
			break;

		case 6:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMMERCE | isomessage::CARDNOTPRESENT | isomessage::TERMUNATTENDED | isomessage::ECOMNOTAUTHENTICATED | isomessage::ECOMNOTSECUREISSUER);
			break;

		case 7:
		case 9:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMMERCE | isomessage::CARDNOTPRESENT | isomessage::TERMUNATTENDED | isomessage::ECOMNOTAUTHENTICATED);
			break;

		case 8:
			visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::ECOMMERCE | isomessage::CARDNOTPRESENT | isomessage::TERMUNATTENDED | isomessage::ECOMNOTAUTHENTICATED | isomessage::ECOMNOTSECUREACQUIRER);
			break;
	}

	if(get_field(message, 60,10)[0]=='1')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::PARTIALCAPABLE);

	if(has_field(message, 61,1))
		visamsg->set_cashbackamount(atoll(get_field(message, 61,1)));

	if(has_field(message, 61,2))
		visamsg->set_cashbackbillingamount(atoll(get_field(message, 61,2)));

	if(has_field(message, 61,3))
		visamsg->set_replacementbillingamount(atoll(get_field(message, 61,3)));








	return 0;
}

field* processOutgoing(isomessage *visamsg, fldformat *frm, VisaContext *context)
{
	field *header;
	field *message;
	field *fullmessage=(field*)calloc(1, sizeof(field));

	fullmessage->frm=frm;

	time_t datetime;

	unsigned int i;

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
		strncpy(add_field(header, 5), context->sourcestationid().c_str(), 6);
		strncpy(add_field(header, 7), context->visaroundtripinf().c_str(), 8);
		strncpy(add_field(header, 8), context->visabaseiflags().c_str(), 16);
		strncpy(add_field(header, 9), context->visamsgstatusflags().c_str(), 24);
		strncpy(add_field(header, 10), context->visamsgstatusflags().c_str(), 2);
		strncpy(add_field(header, 11), context->visareserved().c_str(), 6);
		strncpy(add_field(header, 12), context->visauserinfo().c_str(), 2);
	}
	
	strcpy(add_field(header, 6), stationid);

	add_field(message, 0)[0]='0';

	if(visamsg->messagetype() & isomessage::REVERSAL)
		message->fld[0]->data[1]='4';
	else if(visamsg->messagetype() & isomessage::CLEARING)
		message->fld[0]->data[1]='2';
	else
		message->fld[0]->data[1]='1';

	if(visamsg->messagetype() & isomessage::ADVICE)
		if(visamsg->messagetype() & isomessage::RESPONSE)
			message->fld[0]->data[2]='3';
		else
			message->fld[0]->data[2]='2';
	else
		if(visamsg->messagetype() & isomessage::RESPONSE)
			message->fld[0]->data[2]='1';
		else
			message->fld[0]->data[2]='0';

	if(visamsg->messagetype() & isomessage::ISSUER)
		if(visamsg->messagetype() & isomessage::REPEAT)
			message->fld[0]->data[3]='3';
		else
			message->fld[0]->data[3]='2';
	else
		if(visamsg->messagetype() & isomessage::REPEAT)
			message->fld[0]->data[3]='1';
		else
			message->fld[0]->data[3]='0';

	if(visamsg->has_pan())
		strcpy(add_field(message, 2), visamsg->pan().c_str());

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
		snprintf(add_field(message, 4), 13, "%012lld", visamsg->amounttransaction());

	if(visamsg->has_amountsettlement())
		snprintf(add_field(message, 5), 13, "%012lld", visamsg->amountsettlement());

	if(visamsg->has_amountbilling())
		snprintf(add_field(message, 6), 13, "%012lld", visamsg->amountbilling());

	if(visamsg->has_transactiondatetime())
	{
		datetime=visamsg->transactiondatetime();
		printf("UTC Transaction date time: %s\n", asctime(gmtime(&datetime)));
		strftime(add_field(message, 7), 11, "%m%d%H%M%S", gmtime(&datetime));
	}

	if(visamsg->has_stan())
		snprintf(add_field(message, 11), 7, "%06d", visamsg->stan());
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
		snprintf(add_field(message, 18), 5, "%04d", visamsg->mcc());

	if(visamsg->has_acquirercountry())
		snprintf(add_field(message, 19), 4, "%03d", visamsg->acquirercountry());

	if(visamsg->has_issuercountry())
		snprintf(add_field(message, 20), 4, "%03d", visamsg->issuercountry());
	
	if(isRequest(visamsg) && visamsg->has_entrymode())
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

		switch(visamsg->entrymodeflags() & (isomessage::PINCAPABLE | isomessage::NOTPINCAPABLE))
		{
			case isomessage::PINCAPABLE:
				strcpy(add_field(message, 22,2 ), "1");
				break;
			case isomessage::NOTPINCAPABLE:
				strcpy(add_field(message, 22,2 ), "2");
				break;
			case isomessage::PINCAPABLE | isomessage::NOTPINCAPABLE:
				strcpy(add_field(message, 22,2 ), "8");
				break;
			default:
				strcpy(add_field(message, 22,2 ), "0");
		}
	}

	if(isRequest(visamsg) && visamsg->has_cardsequencenumber())
		snprintf(add_field(message, 23), 4, "%03d", visamsg->cardsequencenumber());

	if(visamsg->messagetype() & isomessage::PREAUTHORIZATION)
		strcpy(add_field(message, 25 ), "00");
	else if(visamsg->messagetype() & isomessage::PREAUTHCOMPLETION)
		strcpy(add_field(message, 25 ), "06");
	else if((visamsg->entrymodeflags() & isomessage::PHONEORDER) || (visamsg->entrymodeflags() & isomessage::RECURRING))
		strcpy(add_field(message, 25 ), "08");
	else if(visamsg->entrymodeflags() & isomessage::ECOMMERCE)
		strcpy(add_field(message, 25 ), "59");
	else if(visamsg->entrymodeflags() & isomessage::NOTAUTHORIZED)
		strcpy(add_field(message, 25 ), "51");
	else if(visamsg->entrymodeflags() & isomessage::MERCHANTSUSPICIOUS)
		strcpy(add_field(message, 25 ), "03");
	else if((visamsg->entrymodeflags() & isomessage::TERMUNATTENDED) && !(visamsg->entrymodeflags() & isomessage::CARDNOTPRESENT))
		strcpy(add_field(message, 25 ), "02");
	else if(visamsg->entrymodeflags() & isomessage::CARDHOLDERNOTPRESENT)
		strcpy(add_field(message, 25 ), "01");
	else if(visamsg->entrymodeflags() & isomessage::CARDNOTPRESENT)
		strcpy(add_field(message, 25 ), "05");
	else if((visamsg->entrymode() == isomessage::MANUAL) && (visamsg->entrymodeflags() & isomessage::FALLBACK))
		strcpy(add_field(message, 25 ), "71");
	else
		strcpy(add_field(message, 25 ), "00");

	if(isRequest(visamsg) && visamsg->has_termpinmaxdigits() && visamsg->has_pin())
		snprintf(add_field(message, 26), 3, "%02d", visamsg->termpinmaxdigits());

	if(isRequest(visamsg) && visamsg->has_amountacquirerfee())
	{
		if(visamsg->amountacquirerfee()<0)
			snprintf(add_field(message, 28), 10, "C%08lld", -visamsg->amountacquirerfee());
		else
			snprintf(add_field(message, 28), 10, "D%08lld", visamsg->amountacquirerfee());
	}

	if(visamsg->has_acquirerid())
		snprintf(add_field(message, 32), 12, "%lld", visamsg->acquirerid());

	if(visamsg->has_forwardingid())
		snprintf(add_field(message, 33), 12, "%lld", visamsg->forwardingid());

	if(isRequest(visamsg) && visamsg->has_track2() && (visamsg->entrymode()==isomessage::MAGSTRIPE || visamsg->entrymode()==isomessage::EM_UNKNOWN))
		strcpy(add_field(message, 35), visamsg->track2().c_str());

	if(visamsg->has_rrn());
		snprintf(add_field(message, 37), 13, "%012lld", visamsg->rrn());

	if(visamsg->has_authid())
		strcpy(add_field(message, 38), visamsg->authid().c_str());

	if(visamsg->has_responsecode() || (visamsg->messagetype() & isomessage::RESPONSE))
		sprintf(add_field(message, 39), "%02d", visamsg->responsecode());

	if(visamsg->has_terminalid())
	{
		strcpy(add_field(message, 41), "        ");
		i=strlen(visamsg->terminalid().c_str());
		memcpy(add_field(message, 41), visamsg->terminalid().c_str(), i > 8 ? 8 : i );
	}

	if(visamsg->has_merchantid())
	{
		strcpy(add_field(message, 42), "               ");
		i=strlen(visamsg->merchantid().c_str());
		memcpy(add_field(message, 42), visamsg->merchantid().c_str(), i > 15 ? 15 : i );
	}

	if(isRequest(visamsg) && (visamsg->has_merchantname() || visamsg->has_merchantcity() || visamsg->has_merchantcountry()))
	{
		strcpy(add_field(message, 43,1), "                         ");
		i=strlen(visamsg->merchantname().c_str());
		memcpy(add_field(message, 43,1), visamsg->merchantname().c_str(), i > 25 ? 25 : i );

		strcpy(add_field(message, 43,2), "             ");
		i=strlen(visamsg->merchantcity().c_str());
		memcpy(add_field(message, 43,2), visamsg->merchantcity().c_str(), i > 13 ? 13 : i );

		strcpy(add_field(message, 43,3), visamsg->merchantcountry().c_str());
	}

	if(!isRequest(visamsg) && visamsg->has_cavvverification())
	{
		if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREISSUER)
			switch(visamsg->cavvverification())
			{
				case isomessage::MATCH:
					strcpy(add_field(message, 44,13), "3");
					break;

				case isomessage::NOMATCH:
					strcpy(add_field(message, 44,13), "4");
					break;

				default:
					strcpy(add_field(message, 44,13), "0");
					break;
			}
		else
			switch(visamsg->cavvverification())
			{
				case isomessage::MATCH:
					strcpy(add_field(message, 44,13), "2");
					break;

				case isomessage::NOMATCH:
					strcpy(add_field(message, 44,13), "1");
					break;

				default:
					strcpy(add_field(message, 44,13), "0");
					break;
			}

		strcpy(add_field(message, 44,12), " ");
		strcpy(add_field(message, 44,11), "  ");
		strcpy(add_field(message, 44,10), " ");
		strcpy(add_field(message, 44,9), " ");
		strcpy(add_field(message, 44,8), " ");
		strcpy(add_field(message, 44,7), " ");
		strcpy(add_field(message, 44,6), "  ");
		strcpy(add_field(message, 44,5), " ");
		strcpy(add_field(message, 44,4), " ");
		strcpy(add_field(message, 44,3), " ");
		strcpy(add_field(message, 44,2), " ");
		strcpy(add_field(message, 44,1), " ");
	}

	if(!isRequest(visamsg) && visamsg->has_cvv2verification())
	{
		switch(visamsg->cvv2verification())
		{
			case isomessage::MATCH:
				strcpy(add_field(message, 44,10), "M");
				break;

			case isomessage::NOMATCH:
				strcpy(add_field(message, 44,10), "N");
				break;

			default:
				strcpy(add_field(message, 44,10), "P");
				break;
		}

		strcpy(add_field(message, 44,9), " ");
		strcpy(add_field(message, 44,8), " ");
		strcpy(add_field(message, 44,7), " ");
		strcpy(add_field(message, 44,6), "  ");
		strcpy(add_field(message, 44,5), " ");
		strcpy(add_field(message, 44,4), " ");
		strcpy(add_field(message, 44,3), " ");
		strcpy(add_field(message, 44,2), " ");
		strcpy(add_field(message, 44,1), " ");
	}

	if(!isRequest(visamsg) && visamsg->has_cardauthenticationresults())
		if(visamsg->cardauthenticationresults()==isomessage::MATCH || visamsg->cardauthenticationresults()==isomessage::NOMATCH)
		{
			if(visamsg->cardauthenticationresults()==isomessage::MATCH)
				strcpy(add_field(message, 44,8), "2");
			else
				strcpy(add_field(message, 44,8), "1");

			strcpy(add_field(message, 44,7), " ");
			strcpy(add_field(message, 44,6), "  ");
			strcpy(add_field(message, 44,5), " ");
			strcpy(add_field(message, 44,4), " ");
			strcpy(add_field(message, 44,3), " ");
			strcpy(add_field(message, 44,2), " ");
			strcpy(add_field(message, 44,1), " ");
		}

	if(!isRequest(visamsg) && visamsg->has_cvvverification())
		if(visamsg->cvvverification()==isomessage::MATCH || visamsg->cvvverification()==isomessage::NOMATCH)
		{
			if(visamsg->cvvverification()==isomessage::MATCH)
				strcpy(add_field(message, 44,5), "2");
			else
				strcpy(add_field(message, 44,5), "1");

			strcpy(add_field(message, 44,4), " ");
			strcpy(add_field(message, 44,3), " ");
			strcpy(add_field(message, 44,2), " ");
			strcpy(add_field(message, 44,1), " ");
		}

	if(!isRequest(visamsg) && (visamsg->has_addressverification() || visamsg->has_postalcodeverification()))
	{
		switch(visamsg->addressverification())
		{
			case isomessage::MATCH:
				switch(visamsg->postalcodeverification())
				{
					case isomessage::MATCH:
						if(isDomestic(visamsg))
							strcpy(add_field(message, 44,2), "Y");
						else
							strcpy(add_field(message, 44,2), "M");
						break;

					case isomessage::NOMATCH:
					case isomessage::NOTPERFORMED:
						strcpy(add_field(message, 44,2), "A");
						break;

					default:
						strcpy(add_field(message, 44,2), "B");
						break;
				}
				break;

			case isomessage::NOMATCH:
				if(visamsg->postalcodeverification()==isomessage::MATCH)
					strcpy(add_field(message, 44,2), "Z");
				else
					strcpy(add_field(message, 44,2), "N");
				break;

			case isomessage::NOTPERFORMED:
				switch(visamsg->postalcodeverification())
				{
					case isomessage::MATCH:
						strcpy(add_field(message, 44,2), "Z");
						break;

					case isomessage::NOMATCH:
						strcpy(add_field(message, 44,2), "N");
						break;

					default:
						if(isDomestic(visamsg))
							strcpy(add_field(message, 44,2), "C");
						else
							strcpy(add_field(message, 44,2), "I");
						break;
				}

			default:
				switch(visamsg->postalcodeverification())
				{
					case isomessage::MATCH:
						strcpy(add_field(message, 44,2), "P");
						break;

					case isomessage::NOMATCH:
						strcpy(add_field(message, 44,2), "N");
						break;

					case isomessage::NOTPERFORMED:
						if(isDomestic(visamsg))
							strcpy(add_field(message, 44,2), "N");
						else
							strcpy(add_field(message, 44,2), "I");
						break;

					default:
						strcpy(add_field(message, 44,2), "C");
						break;
				}
		}

		strcpy(add_field(message, 44,1), " ");
	}

	if(isRequest(visamsg) && visamsg->has_track1() && (visamsg->entrymode()==isomessage::MAGSTRIPE || visamsg->entrymode()==isomessage::EM_UNKNOWN))
		strcpy(add_field(message, 45), visamsg->track1().c_str());

	if(visamsg->has_additionaltext())
		strcpy(add_field(message, 48), visamsg->additionaltext().c_str());

	if(visamsg->has_currencytransaction())
		snprintf(add_field(message, 49), 4, "%03d", visamsg->currencytransaction());

	if(!isRequest(visamsg) && visamsg->has_currencybilling())
		snprintf(add_field(message, 51), 4, "%03d", visamsg->currencybilling());

	if(isRequest(visamsg) && visamsg->has_pin())
	{
		strcpy(add_field(message, 52), visamsg->pin().c_str());

		if(visamsg->pinsecurityformat()!=isomessage::ZONE)
			printf("Warning: Unsupported PIN Security Format, must be Zone encryption.\n");
		strcpy(add_field(message, 53,1), "20");

		if(visamsg->pinencryptionalgorithm()!=isomessage::ANSIDES)
			printf("Warning: Unsupported PIN Encryption Algorithm, must be ANSI DES.\n");
		strcpy(add_field(message, 53,2), "01");

		if(visamsg->pinblockformat()!=isomessage::ISO0)
			printf("Warning: Unsupported PIN Block Format, must be ISO Format 0.\n");
		strcpy(add_field(message, 53,3), "01");

		snprintf(add_field(message, 53,4), 3, "%02d", visamsg->pinkeyindex());

		strcpy(add_field(message, 53,5), "00");

		strcpy(add_field(message, 53,6), "000000");
	}

	for(i=0; i<visamsg->additionalamount_size() && i<6; i++)
	{
		const isomessage::AddAmnt& addamnt=visamsg->additionalamount(i);

		snprintf(add_field(message, 54,i,1), 3, "%02d", addamnt.accounttype());

		snprintf(add_field(message, 54,i,2), 3, "%02d", addamnt.amounttype());

		snprintf(add_field(message, 54,i,3), 4, "%03d", addamnt.currency());

		if(addamnt.amount() < 0)
		{
			strcpy(add_field(message, 54,i,4), "D");
			snprintf(add_field(message, 54,i,5), 13, "%012d", -addamnt.amount());
		}
		else
		{
			strcpy(add_field(message, 54,i,4), "C");
			snprintf(add_field(message, 54,i,5), 13, "%012d", addamnt.amount());
		}
	}

	if(visamsg->has_issuerscript1())
		strcpy(add_tag("71", message, 55,1), visamsg->issuerscript1().c_str());

	if(visamsg->has_issuerscript2())
		strcpy(add_tag("72", message, 55,1), visamsg->issuerscript2().c_str());

	if(visamsg->has_applicationinterchangeprofile())
		strcpy(add_tag("82", message, 55,1), visamsg->applicationinterchangeprofile().c_str());

	if(visamsg->has_issuerauthenticationdata())
		strcpy(add_tag("91", message, 55,1), visamsg->issuerauthenticationdata().c_str());

	if(visamsg->has_terminalverificationresults())
		strcpy(add_tag("95", message, 55,1), visamsg->terminalverificationresults().c_str());

	if(visamsg->has_terminaltransactiondate())
		strcpy(add_tag("9A", message, 55,1), visamsg->terminaltransactiondate().c_str());

	if(visamsg->has_cryptogramtransactiontype())
		strcpy(add_tag("9C", message, 55,1), visamsg->cryptogramtransactiontype().c_str());

	if(visamsg->has_secondarypinblock())
		strcpy(add_tag("C0", message, 55,1), visamsg->secondarypinblock().c_str());

	if(visamsg->has_cryptogramcurrency())
		strcpy(add_tag("5F2A", message, 55,1), visamsg->cryptogramcurrency().c_str());

	if(visamsg->has_cryptogramtransactionamount())
		strcpy(add_tag("9F02", message, 55,1), visamsg->cryptogramtransactionamount().c_str());

	if(visamsg->has_cryptogramcashbackamount())
		strcpy(add_tag("9F03", message, 55,1), visamsg->cryptogramcashbackamount().c_str());

	if(visamsg->has_applicationversionnumber())
		strcpy(add_tag("9F09", message, 55,1), visamsg->applicationversionnumber().c_str());

	if(visamsg->has_issuerapplicationdata())
		strcpy(add_tag("9F10", message, 55,1), visamsg->issuerapplicationdata().c_str());

	if(visamsg->has_terminalcountry())
		strcpy(add_tag("9F1A", message, 55,1), visamsg->terminalcountry().c_str());

	if(visamsg->has_terminalserialnumber())
		strcpy(add_tag("9F1E", message, 55,1), visamsg->terminalserialnumber().c_str());

	if(visamsg->has_cryptogram())
		strcpy(add_tag("9F26", message, 55,1), visamsg->cryptogram().c_str());

	if(visamsg->has_cryptograminformationdata())
		strcpy(add_tag("9F27", message, 55,1), visamsg->cryptograminformationdata().c_str());

	if(visamsg->has_terminalcapabilityprofile())
		strcpy(add_tag("9F33", message, 55,1), visamsg->terminalcapabilityprofile().c_str());

	if(visamsg->has_cvmresults())
		strcpy(add_tag("9F34", message, 55,1), visamsg->cvmresults().c_str());

	if(visamsg->has_cryptogramterminaltype())
		strcpy(add_tag("9F35", message, 55,1), visamsg->cryptogramterminaltype().c_str());

	if(visamsg->has_applicationtransactioncounter())
		strcpy(add_tag("9F36", message, 55,1), visamsg->applicationtransactioncounter().c_str());

	if(visamsg->has_unpredictablenumber())
		strcpy(add_tag("9F37", message, 55,1), visamsg->unpredictablenumber().c_str());

	if(visamsg->has_issuerscriptresults())
		strcpy(add_tag("9F5B", message, 55,1), visamsg->issuerscriptresults().c_str());

	if(visamsg->has_formfactorindicator())
		strcpy(add_tag("9F6E", message, 55,1), visamsg->formfactorindicator().c_str());

	if(visamsg->has_customerexclusivedata())
		strcpy(add_tag("9F7C", message, 55,1), visamsg->customerexclusivedata().c_str());

	if(visamsg->has_merchantaddress())
		strcpy(add_field(message, 59), visamsg->merchantaddress().c_str());

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::PARTIALCAPABLE)
	{
		strcpy(add_field(message, 60,10), "1");

		strcpy(add_field(message, 60,9), "0");
		strcpy(add_field(message, 60,8), "00");
		strcpy(add_field(message, 60,7), "0");
		strcpy(add_field(message, 60,6), "0");
		strcpy(add_field(message, 60,5), "00");
		strcpy(add_field(message, 60,4), "0");
		strcpy(add_field(message, 60,3), "0");
		strcpy(add_field(message, 60,2), "0");
		strcpy(add_field(message, 60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & (isomessage::PHONEORDER | isomessage::RECURRING | isomessage::EM_INSTALLMENT | isomessage::ECOMMERCE))
	{
		if(visamsg->entrymodeflags() & isomessage::RECURRING)
			strcpy(add_field(message, 60,8), "02");
		else if(visamsg->entrymodeflags() & isomessage::EM_INSTALLMENT)
			strcpy(add_field(message, 60,8), "03");
		else if(visamsg->entrymodeflags() & isomessage::PHONEORDER)
			strcpy(add_field(message, 60,8), "01");
		else if(visamsg->entrymodeflags() & isomessage::ECOMMERCE)
		{
			if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREACQUIRER)
				strcpy(add_field(message, 60,8), "08");
			else if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREISSUER)
				strcpy(add_field(message, 60,8), "06");
			else if(visamsg->entrymodeflags() & isomessage::ECOMNOTAUTHENTICATED)
				strcpy(add_field(message, 60,8), "07");
			else
				strcpy(add_field(message, 60,8), "05");
		}
		else
			strcpy(add_field(message, 60,8), "00");

		strcpy(add_field(message, 60,7), "0");
		strcpy(add_field(message, 60,6), "0");
		strcpy(add_field(message, 60,5), "00");
		strcpy(add_field(message, 60,4), "0");
		strcpy(add_field(message, 60,3), "0");
		strcpy(add_field(message, 60,2), "0");
		strcpy(add_field(message, 60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->cardauthenticationreliability()==isomessage::ACQUNRELIABLE)
	{
		strcpy(add_field(message, 60,7), "1");

		strcpy(add_field(message, 60,6), "0");
		strcpy(add_field(message, 60,5), "00");
		strcpy(add_field(message, 60,4), "0");
		strcpy(add_field(message, 60,3), "0");
		strcpy(add_field(message, 60,2), "0");
		strcpy(add_field(message, 60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::EXISTINGDEBT)
	{
		strcpy(add_field(message, 60,4), "9");

		strcpy(add_field(message, 60,3), "0");
		strcpy(add_field(message, 60,2), "0");
		strcpy(add_field(message, 60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::FALLBACK)
	{
		if(visamsg->entrymodeflags() | isomessage::LASTTERMCHIPREADFAILED)
			strcpy(add_field(message, 60,3), "2");
		else
			strcpy(add_field(message, 60,3), "1");

		strcpy(add_field(message, 60,2), "0");
		strcpy(add_field(message, 60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & (isomessage::NOREADCAPABLE | isomessage::TERMNOTPRESENT | isomessage::MAGSTRIPECAPABLE | isomessage::CHIPCAPABLE | isomessage::CONTACTLESSCAPABLE))
	{
		if(visamsg->entrymodeflags() & isomessage::NOREADCAPABLE)
			strcpy(add_field(message, 60,2), "9");
		else if(visamsg->entrymodeflags() & isomessage::TERMNOTPRESENT)
			strcpy(add_field(message, 60,2), "1");
		else if(visamsg->entrymodeflags() & isomessage::CHIPCAPABLE)
			strcpy(add_field(message, 60,2), "5");
		else if(visamsg->entrymodeflags() & isomessage::CONTACTLESSCAPABLE)
			strcpy(add_field(message, 60,2), "8");
		else if(visamsg->entrymodeflags() & isomessage::MAGSTRIPECAPABLE)
			strcpy(add_field(message, 60,2), "2");
		else
			strcpy(add_field(message, 60,2), "0");

		strcpy(add_field(message, 60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->has_terminaltype())
		switch(visamsg->terminaltype())
		{
			case isomessage::LIMITEDAMOUNT:
				strcpy(add_field(message, 60,1), "1");
				break;

			case isomessage::ATM:
				strcpy(add_field(message, 60,1), "2");
				break;

			case isomessage::SELFSERVICE:
				strcpy(add_field(message, 60,1), "3");
				break;

			case isomessage::CASHREGISTER:
				strcpy(add_field(message, 60,1), "4");
				break;

			case isomessage::PERSONAL:
				strcpy(add_field(message, 60,1), "5");
				break;

			case isomessage::PHONE:
				strcpy(add_field(message, 60,1), "7");
				break;

			default:
				strcpy(add_field(message, 60,1), "0");
				break;
		}

	if(isRequest(visamsg) && visamsg->replacementbillingamount())
	{
		snprintf(add_field(message, 61,3), 13, "%012lld", visamsg->replacementbillingamount());
		strcpy(add_field(message, 61,2), "000000000000");
		strcpy(add_field(message, 61,1), "000000000000");
	}

	if(isRequest(visamsg) && visamsg->has_cashbackbillingamount())
	{
		snprintf(add_field(message, 61,2), 13, "%012lld", visamsg->cashbackbillingamount());
		strcpy(add_field(message, 61,1), "000000000000");
	}

	if(isRequest(visamsg) && visamsg->has_cashbackamount())
		snprintf(add_field(message, 61,1), 13, "%012lld", visamsg->cashbackamount());







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
