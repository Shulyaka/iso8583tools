#include <stdio.h>
#include "net.h"
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include "visa.pb.h"

int debug=1;

int ipcconnect(int);

int processIncoming(isomessage *visamsg, field *fullmessage, VisaContext *context);
int processOutgoing(field *fullmessage, isomessage *visamsg, fldformat *frm, VisaContext *context);

char stationid[7]="456789";

double pow01(unsigned char x)
{
	if(!x)
		return 1;

	return 0.1*pow01(x-1);
}

int loadNetFormat(fldformat &frm)
{
	return frm.load_format((char*)"../parser/formats/fields_visa.frm");
}

int parseNetMsg(field &message, char *buf, unsigned int length)
{
	int i;

	if(!buf)
	{
		printf("Error: no buf\n");
		return 0;
	}

	i=message.parse_message(buf, length);   //TODO: For Visa, parse header (frm->fld[0]) and message(frm->fld[2]) separately to handle reject header properly.
	if(i<0)
		return i;

	printf("\nMessage received, length %d\n", length);

	if(i==0)
		printf("Error: Unable to parse\n");

	return i;
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

	strcpy(message->add_field(1,4 ), "0000");

	length=message->estimate_length();

	if(length==0 || snprintf(message->add_field(1,4), 5, "%04X", length - message->get_lengthLength()) > 4)
	{
		printf("Error: Unable to calculate the message length (%d)\n", length);
//		return 0;
	}

	return message->build_message(buf, maxlength);
	
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

int translateSwitchToNet(field *message, isomessage *visamsg, fldformat *frm)
{
	VisaContext context;

	if(!frm)
	{
		printf("Error: No frm\n");
		return 0;
	}

	if(!visamsg)
	{
		printf("Error: No visamsg\n");
		return 0;
	}

	if(visamsg->has_currentcontext())
		context.ParseFromString(visamsg->currentcontext());
	else
		fillDefaultContext(&context);

	return processOutgoing(message, visamsg, frm, &context);
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

	if(!fullmessage->has_field(1))
	{
		printf("Error: No message header found\n");
		return 1;
	}

	if(!fullmessage->has_field(2))
	{
		printf("Error: No message body found\n");
		return 1;
	}

	header=&fullmessage->sf(1);
	message=&fullmessage->sf(2);

	context->set_sourcestationid(header->get_field(6));
	context->set_visaroundtripinf(header->get_field(7));
	context->set_visabaseiflags(header->get_field(8));
	context->set_visamsgstatusflags(header->get_field(9));
	context->set_batchnumber(header->get_field(10));
	context->set_visareserved(header->get_field(11));
	context->set_visauserinfo(header->get_field(12));

	if(!message->has_field(0))
	{
		printf("Error: No message type\n");
		return 1;
	}

	switch(message->get_field(0)[1]-'0')
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
			printf("Error: Unknown message type: '%s'\n", message->get_field(0));
	}

	switch(message->get_field(0)[2]-'0')
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
			printf("Error: Unknown message type: '%s'\n", message->get_field(0));
	}

	switch(message->get_field(0)[3]-'0')
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
			printf("Error: Unknown message type: '%s'\n", message->get_field(0));
	}

	if(message->has_field(2))
		visamsg->set_pan(message->get_field(2));

	if(message->has_field(3))
	{
		switch(atoi(message->get_field(3,1)))
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
				printf("Error: Unknown processing code: '%s'\n", message->get_field(3,1));
				return 1;
		}

		switch(atoi(message->get_field(3,2)))
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
				printf("Warning: Unknown From account type: '%s'\n", message->get_field(3,2));
		}

		switch(atoi(message->get_field(3,3)))
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
				printf("Warning: Unknown To account type: '%s'\n", message->get_field(3,3));
		}
	}

	if(message->has_field(4))
		visamsg->set_amounttransaction(atoll(message->get_field(4)));

	if(message->has_field(5))
		visamsg->set_amountsettlement(atoll(message->get_field(5)));

	if(message->has_field(6))
		visamsg->set_amountbilling(atoll(message->get_field(6)));

	if(message->has_field(7))
	{
		memset(&datetime, 0, sizeof(datetime));
		sscanf(message->get_field(7), "%2d%2d%2d%2d%2d", &datetime.tm_mon, &datetime.tm_mday, &datetime.tm_hour, &datetime.tm_min, &datetime.tm_sec);
		datetime.tm_mon--;

		datetime.tm_year=current.tm_year;

		if(datetime.tm_mon>8 && current.tm_mon<3)
			datetime.tm_year--;
		else if(datetime.tm_mon<3 && current.tm_mon>8)
			datetime.tm_year++;

		visamsg->set_transactiondatetime(timegm(&datetime));
	}

	if(message->has_field(9))
		visamsg->set_conversionratesettlement(atof(message->get_field(9,1))*pow01(message->get_field(9,0)[0]-'0'));
	
	if(message->has_field(10))
		visamsg->set_conversionratebilling(atof(message->get_field(10,1))*pow01(message->get_field(10,0)[0]-'0'));
	
	if(message->has_field(11))
		visamsg->set_stan(atoi(message->get_field(11)));

	if(message->has_field(12))
		visamsg->set_terminaltime(message->get_field(12));

	if(message->has_field(13))
	{
		if(message->get_field(13)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(message->get_field(13)[0]=='0' && message->get_field(13)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, message->get_field(13));

		visamsg->set_terminaldate(tmpstr);
	}

	if(message->has_field(14))
		visamsg->set_expirydate(message->get_field(14));

	if(message->has_field(15))
	{
		if(message->get_field(15)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(message->get_field(15)[0]=='0' && message->get_field(15)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, message->get_field(15));

		visamsg->set_settlementdate(tmpstr);
	}

	if(message->has_field(16))
	{
		if(message->get_field(16)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(message->get_field(16)[0]=='0' && message->get_field(16)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, message->get_field(16));

		visamsg->set_conversiondate(tmpstr);
	}

	if(message->has_field(18))
		visamsg->set_mcc(atoi(message->get_field(18)));

	if(message->has_field(19))
		visamsg->set_acquirercountry(atoi(message->get_field(19)));

	if(message->has_field(20))
		visamsg->set_issuercountry(atoi(message->get_field(20)));

	if(message->has_field(22))
	{
		tmpint=0;

		switch(atoi(message->get_field(22,1 )))
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

		switch(atoi(message->get_field(22,2 )))
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

	if(message->has_field(23))
		visamsg->set_cardsequencenumber(atoi(message->get_field(23)));

	switch(atoi((message->get_field(25))))
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
			if(message->get_field(126,13)[0]=='R' || !strcmp(message->get_field(60,8 ), "02"))
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

	if(message->has_field(26))
		visamsg->set_termpinmaxdigits(atoi(message->get_field(26)));

	if(message->has_field(28))
	{
		if(message->get_field(28)[0]=='C')
			visamsg->set_amountacquirerfee(-atoll(message->get_field(28)+1));
		else
			visamsg->set_amountacquirerfee(atoll(message->get_field(28)+1));
	}

	if(message->has_field(32))
		visamsg->set_acquirerid(atoll(message->get_field(32)));
	
	if(message->has_field(33))
		visamsg->set_forwardingid(atoll(message->get_field(33)));

	if(message->has_field(35))
		visamsg->set_track2(message->get_field(35));

	if(message->has_field(37))
		visamsg->set_rrn(atoll(message->get_field(37)));

	if(message->has_field(38))
		visamsg->set_authid(message->get_field(38));

	if(message->has_field(39))
		visamsg->set_responsecode(atoi(message->get_field(39)));

	if(message->has_field(41))
	{
		strcpy(tmpstr, message->get_field(41));

		for(i=7; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_terminalid(tmpstr);
	}
			
	if(message->has_field(42))
	{
		strcpy(tmpstr, message->get_field(42));

		for(i=14; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantid(tmpstr);
	}

	if(message->has_field(43))
	{
		strcpy(tmpstr, message->get_field(43,1));

		for(i=24; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantname(tmpstr);

		strcpy(tmpstr, message->get_field(43,2));

		for(i=12; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantcity(tmpstr);

		visamsg->set_merchantcountry(message->get_field(43,3));
	}

	if(message->has_field(44,1))
	{
		if(message->get_field(44,1)[0]=='5')
			visamsg->set_responsesource(isomessage::RSP_ISSUER);
		else
			visamsg->set_responsesource(isomessage::RSP_NETWORK);
	}

	switch(message->get_field(44,2)[0])
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

	switch(message->get_field(44,5)[0])
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

	switch(message->get_field(44,8)[0])
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

	switch(message->get_field(44,10)[0])
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

	if(message->has_field(44,11))
		visamsg->set_originalresponsecode(atoi(message->get_field(44,11)));

	switch(message->get_field(44,13)[0])
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

	if(message->has_field(45))
		visamsg->set_track1(message->get_field(45));

	if(message->has_field(48))
		visamsg->set_additionaltext(message->get_field(48));

	if(message->has_field(49))
		visamsg->set_currencytransaction(atoi(message->get_field(49)));

	if(message->has_field(51))
		visamsg->set_currencybilling(atoi(message->get_field(51)));

	if(message->has_field(52))
		visamsg->set_pin(message->get_field(52));

	if(message->has_field(53))
	{
		switch (atoi(message->get_field(53,1)))
		{
			case 02:
				visamsg->set_pinsecurityformat(isomessage::ISSUERKEY);
				break;

			case 20:
				visamsg->set_pinsecurityformat(isomessage::ZONE);
				break;
		}

		if(!strcmp(message->get_field(53,2), "01"))
			visamsg->set_pinencryptionalgorithm(isomessage::ANSIDES);

		switch (atoi(message->get_field(53,3)))
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

		visamsg->set_pinkeyindex(atoi(message->get_field(53,4)));

		if(!strcmp(message->get_field(53,5), "01"))
			visamsg->set_pinispassword(true);
	}

	if(message->has_field(54))
	{
		for(i=0; i<6; i++)
			if(message->has_field(54,i))
			{
				isomessage::AddAmnt *addamnt=visamsg->add_additionalamount();

				addamnt->set_accounttype((isomessage_AccntType)atoi(message->get_field(54,i,1)));

				addamnt->set_amounttype((isomessage_AddAmnt_AmntType)atoi(message->get_field(54,i,2)));

				addamnt->set_currency(atoi(message->get_field(54,i,3)));

				if(message->get_field(54,i,4)[0]=='C')
					addamnt->set_amount(atoi(message->get_field(54,i,5)));
				else
					addamnt->set_amount(-atoi(message->get_field(54,i,5)));
			}
			else
				break;
	}

	for(i=0; i < message->has_field(55,1); i++)
	{
		sscanf(message->get_tag(55,1,i), "%X", &tmpint);

		switch(tmpint)
		{
			case 0x71:
				visamsg->set_issuerscript1(message->get_field(55,1,i));
				break;
			case 0x72:
				visamsg->set_issuerscript2(message->get_field(55,1,i));
				break;
			case 0x82:
				visamsg->set_applicationinterchangeprofile(message->get_field(55,1,i));
				break;
			case 0x91:
				visamsg->set_issuerauthenticationdata(message->get_field(55,1,i));
				break;
			case 0x95:
				visamsg->set_terminalverificationresults(message->get_field(55,1,i));
				break;
			case 0x9A:
				visamsg->set_terminaltransactiondate(message->get_field(55,1,i));
				break;
			case 0x9C:
				visamsg->set_cryptogramtransactiontype(message->get_field(55,1,i));
				break;
			case 0xC0:
				visamsg->set_secondarypinblock(message->get_field(55,1,i));
				break;
			case 0x5F2A:
				visamsg->set_cryptogramcurrency(message->get_field(55,1,i));
				break;
			case 0x9F02:
				visamsg->set_cryptogramtransactionamount(message->get_field(55,1,i));
				break;
			case 0x9F03:
				visamsg->set_cryptogramcashbackamount(message->get_field(55,1,i));
				break;
			case 0x9F09:
				visamsg->set_applicationversionnumber(message->get_field(55,1,i));
				break;
			case 0x9F10:
				visamsg->set_issuerapplicationdata(message->get_field(55,1,i));
				break;
			case 0x9F1A:
				visamsg->set_terminalcountry(message->get_field(55,1,i));
				break;
			case 0x9F1E:
				visamsg->set_terminalserialnumber(message->get_field(55,1,i));
				break;
			case 0x9F26:
				visamsg->set_cryptogram(message->get_field(55,1,i));
				break;
			case 0x9F27:
				visamsg->set_cryptograminformationdata(message->get_field(55,1,i));
				break;
			case 0x9F33:
				visamsg->set_terminalcapabilityprofile(message->get_field(55,1,i));
				break;
			case 0x9F34:
				visamsg->set_cvmresults(message->get_field(55,1,i));
				break;
			case 0x9F35:
				visamsg->set_cryptogramterminaltype(message->get_field(55,1,i));
				break;
			case 0x9F36:
				visamsg->set_applicationtransactioncounter(message->get_field(55,1,i));
				break;
			case 0x9F37:
				visamsg->set_unpredictablenumber(message->get_field(55,1,i));
				break;
			case 0x9F5B:
				visamsg->set_issuerscriptresults(message->get_field(55,1,i));
				break;
			case 0x9F6E:
				visamsg->set_formfactorindicator(message->get_field(55,1,i));
				break;
			case 0x9F7C:
				visamsg->set_customerexclusivedata(message->get_field(55,1,i));
				break;
			default:
				printf("No map for tag %X\n", tmpint);
		}
	}

	if(message->has_field(59))
		visamsg->set_merchantaddress(message->get_field(59));

	switch(message->get_field(60,1)[0])
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

	switch(message->get_field(60,2)[0])
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

	if(message->get_field(60,3)[0]=='2')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::LASTTERMCHIPREADFAILED);

	if(message->get_field(60,4)[0]=='9')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::EXISTINGDEBT);

	if(message->get_field(60,6)[0]=='2')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::EXPANDEDTHIRDBM);

	switch(message->get_field(60,7)[0])
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

	switch(atoi(message->get_field(60,8)))
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

	if(message->get_field(60,10)[0]=='1')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::PARTIALCAPABLE);

	if(message->has_field(61,1))
		visamsg->set_cashbackamount(atoll(message->get_field(61,1)));

	if(message->has_field(61,2))
		visamsg->set_cashbackbillingamount(atoll(message->get_field(61,2)));

	if(message->has_field(61,3))
		visamsg->set_replacementbillingamount(atoll(message->get_field(61,3)));








	return 0;
}

int processOutgoing(field *fullmessage, isomessage *visamsg, fldformat *frm, VisaContext *context)
{
	field *header;
	field *message;

	fullmessage->change_format(frm);

	time_t datetime;

	unsigned int i;

	strcpy(fullmessage->add_field(0), "0000");
	fullmessage->add_field(1);
	fullmessage->add_field(2);

	header=&fullmessage->sf(1);
	message=&fullmessage->sf(2);

	strcpy(header->add_field(2), "01");

//	if(message->frm->fld[62] && message->frm->fld[62]->fld[0] && message->frm->fld[62]->fld[0]->dataFormat==FRM_BITMAP)
		strcpy(header->add_field(3), "02");
//	else
//		strcpy(header->add_field(3), "01");

	if(isRequest(visamsg))
	{
		strcpy(header->add_field(5), "000000");
		strcpy(header->add_field(7), "00000000");
		strcpy(header->add_field(8), "0000000000000000");
		strcpy(header->add_field(9), "000000000000000000000000");
		strcpy(header->add_field(10), "00");
		strcpy(header->add_field(11), "000000");
		strcpy(header->add_field(12), "00");
	}
	else
	{
		strncpy(header->add_field(5), context->sourcestationid().c_str(), 6);
		strncpy(header->add_field(7), context->visaroundtripinf().c_str(), 8);
		strncpy(header->add_field(8), context->visabaseiflags().c_str(), 16);
		strncpy(header->add_field(9), context->visamsgstatusflags().c_str(), 24);
		strncpy(header->add_field(10), context->visamsgstatusflags().c_str(), 2);
		strncpy(header->add_field(11), context->visareserved().c_str(), 6);
		strncpy(header->add_field(12), context->visauserinfo().c_str(), 2);
	}
	
	strcpy(header->add_field(6), stationid);

	message->add_field(0)[0]='0';

	if(visamsg->messagetype() & isomessage::REVERSAL)
		message->add_field(0)[1]='4';
	else if(visamsg->messagetype() & isomessage::CLEARING)
		message->add_field(0)[1]='2';
	else
		message->add_field(0)[1]='1';

	if(visamsg->messagetype() & isomessage::ADVICE)
		if(visamsg->messagetype() & isomessage::RESPONSE)
			message->add_field(0)[2]='3';
		else
			message->add_field(0)[2]='2';
	else
		if(visamsg->messagetype() & isomessage::RESPONSE)
			message->add_field(0)[2]='1';
		else
			message->add_field(0)[2]='0';

	if(visamsg->messagetype() & isomessage::ISSUER)
		if(visamsg->messagetype() & isomessage::REPEAT)
			message->add_field(0)[3]='3';
		else
			message->add_field(0)[3]='2';
	else
		if(visamsg->messagetype() & isomessage::REPEAT)
			message->add_field(0)[3]='1';
		else
			message->add_field(0)[3]='0';

	if(visamsg->has_pan())
		strcpy(message->add_field(2), visamsg->pan().c_str());

	if(visamsg->has_transactiontype())
	{
		switch(visamsg->transactiontype())
		{
			case isomessage::PURCHASE:
				strcpy(message->add_field(3,1 ), "00");
				break;

			case isomessage::CASH:
				strcpy(message->add_field(3,1 ), "01");
				break;

			case isomessage::CHECK:
				strcpy(message->add_field(3,1 ), "03");
				break;

			case isomessage::ACCNTFUNDING:
				strcpy(message->add_field(3,1 ), "10");
				break;

			case isomessage::ORIGINALCREDIT:
				strcpy(message->add_field(3,1 ), "26");
				break;

			default:
				printf("Error: Unknown transaction type: %d\n", visamsg->transactiontype());
				return 0;
		}

		switch(visamsg->accounttypefrom())
		{
			case isomessage::DEFAULT:
				strcpy(message->add_field(3,2), "00");
				break;

			case isomessage::SAVINGS:
				strcpy(message->add_field(3,2), "10");
				break;

			case isomessage::CHECKING:
				strcpy(message->add_field(3,2), "20");
				break;

			case isomessage::CREDIT:
				strcpy(message->add_field(3,2), "30");
				break;

			case isomessage::UNIVERSAL:
				strcpy(message->add_field(3,2), "40");
				break;

			default:
				printf("Warning: Unknown account type From: '%d'. Using default.\n", visamsg->accounttypefrom());
				strcpy(message->add_field(3,2), "00");
				break;
		}

		switch(visamsg->accounttypeto())
		{
			case isomessage::DEFAULT:
				strcpy(message->add_field(3,3), "00");
				break;

			case isomessage::SAVINGS:
				strcpy(message->add_field(3,3), "10");
				break;

			case isomessage::CHECKING:
				strcpy(message->add_field(3,3), "20");
				break;

			case isomessage::CREDIT:
				strcpy(message->add_field(3,3), "30");
				break;

			case isomessage::UNIVERSAL:
				strcpy(message->add_field(3,3), "40");
				break;

			default:
				printf("Warning: Unknown account type To: '%d'. Using default.\n", visamsg->accounttypeto());
				strcpy(message->add_field(3,3), "00");
				break;
		}
	}

	if(visamsg->has_amounttransaction())
		snprintf(message->add_field(4), 13, "%012lld", visamsg->amounttransaction());

	if(visamsg->has_amountsettlement())
		snprintf(message->add_field(5), 13, "%012lld", visamsg->amountsettlement());

	if(visamsg->has_amountbilling())
		snprintf(message->add_field(6), 13, "%012lld", visamsg->amountbilling());

	if(visamsg->has_transactiondatetime())
	{
		datetime=visamsg->transactiondatetime();
		printf("UTC Transaction date time: %s\n", asctime(gmtime(&datetime)));
		strftime(message->add_field(7), 11, "%m%d%H%M%S", gmtime(&datetime));
	}

	if(visamsg->has_stan())
		snprintf(message->add_field(11), 7, "%06d", visamsg->stan());
	else
		printf("Warning: No STAN\n");

	if(visamsg->has_terminaltime())
		strcpy(message->add_field(12), visamsg->terminaltime().c_str());

	if(visamsg->has_terminaldate())
		strcpy(message->add_field(13), visamsg->terminaldate().c_str()+4);

	if(visamsg->has_expirydate() && isRequest(visamsg))
		strcpy(message->add_field(14), visamsg->expirydate().c_str());

	if(visamsg->has_settlementdate() && !isRequest(visamsg))
		strcpy(message->add_field(15), visamsg->settlementdate().c_str()+4);

	if(visamsg->has_mcc() && isRequest(visamsg))
		snprintf(message->add_field(18), 5, "%04d", visamsg->mcc());

	if(visamsg->has_acquirercountry())
		snprintf(message->add_field(19), 4, "%03d", visamsg->acquirercountry());

	if(visamsg->has_issuercountry())
		snprintf(message->add_field(20), 4, "%03d", visamsg->issuercountry());
	
	if(isRequest(visamsg) && visamsg->has_entrymode())
	{
		switch(visamsg->entrymode())
		{
			case isomessage::MANUAL:
				strcpy(message->add_field(22,1 ), "01");
				break;
			case isomessage::BARCODE:
				strcpy(message->add_field(22,1 ), "03");
				break;
			case isomessage::OPTICAL:
				strcpy(message->add_field(22,1 ), "04");
				break;
			case isomessage::CHIP:
				if(visamsg->entrymodeflags() & isomessage::CONTACTLESS)
					strcpy(message->add_field(22,1 ), "07");
				else if(visamsg->entrymodeflags() & isomessage::CVVUNRELIABLE)
					strcpy(message->add_field(22,1 ), "95");
				else
					strcpy(message->add_field(22,1 ), "05");
				break;
			case isomessage::MAGSTRIPE:
				if(visamsg->entrymodeflags() & isomessage::CONTACTLESS)
					strcpy(message->add_field(22,1 ), "91");
				else if(visamsg->entrymodeflags() & isomessage::CVVUNRELIABLE)
					strcpy(message->add_field(22,1 ), "02");
				else
					strcpy(message->add_field(22,1 ), "91");
				break;
			case isomessage::STORED:
				strcpy(message->add_field(22,1 ), "96");
				break;
			default:
				strcpy(message->add_field(22,1 ), "00");
		}

		switch(visamsg->entrymodeflags() & (isomessage::PINCAPABLE | isomessage::NOTPINCAPABLE))
		{
			case isomessage::PINCAPABLE:
				strcpy(message->add_field(22,2 ), "1");
				break;
			case isomessage::NOTPINCAPABLE:
				strcpy(message->add_field(22,2 ), "2");
				break;
			case isomessage::PINCAPABLE | isomessage::NOTPINCAPABLE:
				strcpy(message->add_field(22,2 ), "8");
				break;
			default:
				strcpy(message->add_field(22,2 ), "0");
		}
	}

	if(isRequest(visamsg) && visamsg->has_cardsequencenumber())
		snprintf(message->add_field(23), 4, "%03d", visamsg->cardsequencenumber());

	if(visamsg->messagetype() & isomessage::PREAUTHORIZATION)
		strcpy(message->add_field(25 ), "00");
	else if(visamsg->messagetype() & isomessage::PREAUTHCOMPLETION)
		strcpy(message->add_field(25 ), "06");
	else if((visamsg->entrymodeflags() & isomessage::PHONEORDER) || (visamsg->entrymodeflags() & isomessage::RECURRING))
		strcpy(message->add_field(25 ), "08");
	else if(visamsg->entrymodeflags() & isomessage::ECOMMERCE)
		strcpy(message->add_field(25 ), "59");
	else if(visamsg->entrymodeflags() & isomessage::NOTAUTHORIZED)
		strcpy(message->add_field(25 ), "51");
	else if(visamsg->entrymodeflags() & isomessage::MERCHANTSUSPICIOUS)
		strcpy(message->add_field(25 ), "03");
	else if((visamsg->entrymodeflags() & isomessage::TERMUNATTENDED) && !(visamsg->entrymodeflags() & isomessage::CARDNOTPRESENT))
		strcpy(message->add_field(25 ), "02");
	else if(visamsg->entrymodeflags() & isomessage::CARDHOLDERNOTPRESENT)
		strcpy(message->add_field(25 ), "01");
	else if(visamsg->entrymodeflags() & isomessage::CARDNOTPRESENT)
		strcpy(message->add_field(25 ), "05");
	else if((visamsg->entrymode() == isomessage::MANUAL) && (visamsg->entrymodeflags() & isomessage::FALLBACK))
		strcpy(message->add_field(25 ), "71");
	else
		strcpy(message->add_field(25 ), "00");

	if(isRequest(visamsg) && visamsg->has_termpinmaxdigits() && visamsg->has_pin())
		snprintf(message->add_field(26), 3, "%02d", visamsg->termpinmaxdigits());

	if(isRequest(visamsg) && visamsg->has_amountacquirerfee())
	{
		if(visamsg->amountacquirerfee()<0)
			snprintf(message->add_field(28), 10, "C%08lld", -visamsg->amountacquirerfee());
		else
			snprintf(message->add_field(28), 10, "D%08lld", visamsg->amountacquirerfee());
	}

	if(visamsg->has_acquirerid())
		snprintf(message->add_field(32), 12, "%lld", visamsg->acquirerid());

	if(visamsg->has_forwardingid())
		snprintf(message->add_field(33), 12, "%lld", visamsg->forwardingid());

	if(isRequest(visamsg) && visamsg->has_track2() && (visamsg->entrymode()==isomessage::MAGSTRIPE || visamsg->entrymode()==isomessage::EM_UNKNOWN))
		strcpy(message->add_field(35), visamsg->track2().c_str());

	if(visamsg->has_rrn());
		snprintf(message->add_field(37), 13, "%012lld", visamsg->rrn());

	if(visamsg->has_authid())
		strcpy(message->add_field(38), visamsg->authid().c_str());

	if(visamsg->has_responsecode() || (visamsg->messagetype() & isomessage::RESPONSE))
		sprintf(message->add_field(39), "%02d", visamsg->responsecode());

	if(visamsg->has_terminalid())
	{
		strcpy(message->add_field(41), "        ");
		i=strlen(visamsg->terminalid().c_str());
		memcpy(message->add_field(41), visamsg->terminalid().c_str(), i > 8 ? 8 : i );
	}

	if(visamsg->has_merchantid())
	{
		strcpy(message->add_field(42), "               ");
		i=strlen(visamsg->merchantid().c_str());
		memcpy(message->add_field(42), visamsg->merchantid().c_str(), i > 15 ? 15 : i );
	}

	if(isRequest(visamsg) && (visamsg->has_merchantname() || visamsg->has_merchantcity() || visamsg->has_merchantcountry()))
	{
		strcpy(message->add_field(43,1), "                         ");
		i=strlen(visamsg->merchantname().c_str());
		memcpy(message->add_field(43,1), visamsg->merchantname().c_str(), i > 25 ? 25 : i );

		strcpy(message->add_field(43,2), "             ");
		i=strlen(visamsg->merchantcity().c_str());
		memcpy(message->add_field(43,2), visamsg->merchantcity().c_str(), i > 13 ? 13 : i );

		strcpy(message->add_field(43,3), visamsg->merchantcountry().c_str());
	}

	if(!isRequest(visamsg) && visamsg->has_cavvverification())
	{
		if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREISSUER)
			switch(visamsg->cavvverification())
			{
				case isomessage::MATCH:
					strcpy(message->add_field(44,13), "3");
					break;

				case isomessage::NOMATCH:
					strcpy(message->add_field(44,13), "4");
					break;

				default:
					strcpy(message->add_field(44,13), "0");
					break;
			}
		else
			switch(visamsg->cavvverification())
			{
				case isomessage::MATCH:
					strcpy(message->add_field(44,13), "2");
					break;

				case isomessage::NOMATCH:
					strcpy(message->add_field(44,13), "1");
					break;

				default:
					strcpy(message->add_field(44,13), "0");
					break;
			}

		strcpy(message->add_field(44,12), " ");
		strcpy(message->add_field(44,11), "  ");
		strcpy(message->add_field(44,10), " ");
		strcpy(message->add_field(44,9), " ");
		strcpy(message->add_field(44,8), " ");
		strcpy(message->add_field(44,7), " ");
		strcpy(message->add_field(44,6), "  ");
		strcpy(message->add_field(44,5), " ");
		strcpy(message->add_field(44,4), " ");
		strcpy(message->add_field(44,3), " ");
		strcpy(message->add_field(44,2), " ");
		strcpy(message->add_field(44,1), " ");
	}

	if(!isRequest(visamsg) && visamsg->has_cvv2verification())
	{
		switch(visamsg->cvv2verification())
		{
			case isomessage::MATCH:
				strcpy(message->add_field(44,10), "M");
				break;

			case isomessage::NOMATCH:
				strcpy(message->add_field(44,10), "N");
				break;

			default:
				strcpy(message->add_field(44,10), "P");
				break;
		}

		strcpy(message->add_field(44,9), " ");
		strcpy(message->add_field(44,8), " ");
		strcpy(message->add_field(44,7), " ");
		strcpy(message->add_field(44,6), "  ");
		strcpy(message->add_field(44,5), " ");
		strcpy(message->add_field(44,4), " ");
		strcpy(message->add_field(44,3), " ");
		strcpy(message->add_field(44,2), " ");
		strcpy(message->add_field(44,1), " ");
	}

	if(!isRequest(visamsg) && visamsg->has_cardauthenticationresults())
		if(visamsg->cardauthenticationresults()==isomessage::MATCH || visamsg->cardauthenticationresults()==isomessage::NOMATCH)
		{
			if(visamsg->cardauthenticationresults()==isomessage::MATCH)
				strcpy(message->add_field(44,8), "2");
			else
				strcpy(message->add_field(44,8), "1");

			strcpy(message->add_field(44,7), " ");
			strcpy(message->add_field(44,6), "  ");
			strcpy(message->add_field(44,5), " ");
			strcpy(message->add_field(44,4), " ");
			strcpy(message->add_field(44,3), " ");
			strcpy(message->add_field(44,2), " ");
			strcpy(message->add_field(44,1), " ");
		}

	if(!isRequest(visamsg) && visamsg->has_cvvverification())
		if(visamsg->cvvverification()==isomessage::MATCH || visamsg->cvvverification()==isomessage::NOMATCH)
		{
			if(visamsg->cvvverification()==isomessage::MATCH)
				strcpy(message->add_field(44,5), "2");
			else
				strcpy(message->add_field(44,5), "1");

			strcpy(message->add_field(44,4), " ");
			strcpy(message->add_field(44,3), " ");
			strcpy(message->add_field(44,2), " ");
			strcpy(message->add_field(44,1), " ");
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
							strcpy(message->add_field(44,2), "Y");
						else
							strcpy(message->add_field(44,2), "M");
						break;

					case isomessage::NOMATCH:
					case isomessage::NOTPERFORMED:
						strcpy(message->add_field(44,2), "A");
						break;

					default:
						strcpy(message->add_field(44,2), "B");
						break;
				}
				break;

			case isomessage::NOMATCH:
				if(visamsg->postalcodeverification()==isomessage::MATCH)
					strcpy(message->add_field(44,2), "Z");
				else
					strcpy(message->add_field(44,2), "N");
				break;

			case isomessage::NOTPERFORMED:
				switch(visamsg->postalcodeverification())
				{
					case isomessage::MATCH:
						strcpy(message->add_field(44,2), "Z");
						break;

					case isomessage::NOMATCH:
						strcpy(message->add_field(44,2), "N");
						break;

					default:
						if(isDomestic(visamsg))
							strcpy(message->add_field(44,2), "C");
						else
							strcpy(message->add_field(44,2), "I");
						break;
				}

			default:
				switch(visamsg->postalcodeverification())
				{
					case isomessage::MATCH:
						strcpy(message->add_field(44,2), "P");
						break;

					case isomessage::NOMATCH:
						strcpy(message->add_field(44,2), "N");
						break;

					case isomessage::NOTPERFORMED:
						if(isDomestic(visamsg))
							strcpy(message->add_field(44,2), "N");
						else
							strcpy(message->add_field(44,2), "I");
						break;

					default:
						strcpy(message->add_field(44,2), "C");
						break;
				}
		}

		strcpy(message->add_field(44,1), " ");
	}

	if(isRequest(visamsg) && visamsg->has_track1() && (visamsg->entrymode()==isomessage::MAGSTRIPE || visamsg->entrymode()==isomessage::EM_UNKNOWN))
		strcpy(message->add_field(45), visamsg->track1().c_str());

	if(visamsg->has_additionaltext())
		strcpy(message->add_field(48), visamsg->additionaltext().c_str());

	if(visamsg->has_currencytransaction())
		snprintf(message->add_field(49), 4, "%03d", visamsg->currencytransaction());

	if(!isRequest(visamsg) && visamsg->has_currencybilling())
		snprintf(message->add_field(51), 4, "%03d", visamsg->currencybilling());

	if(isRequest(visamsg) && visamsg->has_pin())
	{
		strcpy(message->add_field(52), visamsg->pin().c_str());

		if(visamsg->pinsecurityformat()!=isomessage::ZONE)
			printf("Warning: Unsupported PIN Security Format, must be Zone encryption.\n");
		strcpy(message->add_field(53,1), "20");

		if(visamsg->pinencryptionalgorithm()!=isomessage::ANSIDES)
			printf("Warning: Unsupported PIN Encryption Algorithm, must be ANSI DES.\n");
		strcpy(message->add_field(53,2), "01");

		if(visamsg->pinblockformat()!=isomessage::ISO0)
			printf("Warning: Unsupported PIN Block Format, must be ISO Format 0.\n");
		strcpy(message->add_field(53,3), "01");

		snprintf(message->add_field(53,4), 3, "%02d", visamsg->pinkeyindex());

		strcpy(message->add_field(53,5), "00");

		strcpy(message->add_field(53,6), "000000");
	}

	for(i=0; i<visamsg->additionalamount_size() && i<6; i++)
	{
		const isomessage::AddAmnt& addamnt=visamsg->additionalamount(i);

		snprintf(message->add_field(54,i,1), 3, "%02d", addamnt.accounttype());

		snprintf(message->add_field(54,i,2), 3, "%02d", addamnt.amounttype());

		snprintf(message->add_field(54,i,3), 4, "%03d", addamnt.currency());

		if(addamnt.amount() < 0)
		{
			strcpy(message->add_field(54,i,4), "D");
			snprintf(message->add_field(54,i,5), 13, "%012d", -addamnt.amount());
		}
		else
		{
			strcpy(message->add_field(54,i,4), "C");
			snprintf(message->add_field(54,i,5), 13, "%012d", addamnt.amount());
		}
	}

	if(visamsg->has_issuerscript1())
		strcpy(message->add_tag("71", 55,1), visamsg->issuerscript1().c_str());

	if(visamsg->has_issuerscript2())
		strcpy(message->add_tag("72", 55,1), visamsg->issuerscript2().c_str());

	if(visamsg->has_applicationinterchangeprofile())
		strcpy(message->add_tag("82", 55,1), visamsg->applicationinterchangeprofile().c_str());

	if(visamsg->has_issuerauthenticationdata())
		strcpy(message->add_tag("91", 55,1), visamsg->issuerauthenticationdata().c_str());

	if(visamsg->has_terminalverificationresults())
		strcpy(message->add_tag("95", 55,1), visamsg->terminalverificationresults().c_str());

	if(visamsg->has_terminaltransactiondate())
		strcpy(message->add_tag("9A", 55,1), visamsg->terminaltransactiondate().c_str());

	if(visamsg->has_cryptogramtransactiontype())
		strcpy(message->add_tag("9C", 55,1), visamsg->cryptogramtransactiontype().c_str());

	if(visamsg->has_secondarypinblock())
		strcpy(message->add_tag("C0", 55,1), visamsg->secondarypinblock().c_str());

	if(visamsg->has_cryptogramcurrency())
		strcpy(message->add_tag("5F2A", 55,1), visamsg->cryptogramcurrency().c_str());

	if(visamsg->has_cryptogramtransactionamount())
		strcpy(message->add_tag("9F02", 55,1), visamsg->cryptogramtransactionamount().c_str());

	if(visamsg->has_cryptogramcashbackamount())
		strcpy(message->add_tag("9F03", 55,1), visamsg->cryptogramcashbackamount().c_str());

	if(visamsg->has_applicationversionnumber())
		strcpy(message->add_tag("9F09", 55,1), visamsg->applicationversionnumber().c_str());

	if(visamsg->has_issuerapplicationdata())
		strcpy(message->add_tag("9F10", 55,1), visamsg->issuerapplicationdata().c_str());

	if(visamsg->has_terminalcountry())
		strcpy(message->add_tag("9F1A", 55,1), visamsg->terminalcountry().c_str());

	if(visamsg->has_terminalserialnumber())
		strcpy(message->add_tag("9F1E", 55,1), visamsg->terminalserialnumber().c_str());

	if(visamsg->has_cryptogram())
		strcpy(message->add_tag("9F26", 55,1), visamsg->cryptogram().c_str());

	if(visamsg->has_cryptograminformationdata())
		strcpy(message->add_tag("9F27", 55,1), visamsg->cryptograminformationdata().c_str());

	if(visamsg->has_terminalcapabilityprofile())
		strcpy(message->add_tag("9F33", 55,1), visamsg->terminalcapabilityprofile().c_str());

	if(visamsg->has_cvmresults())
		strcpy(message->add_tag("9F34", 55,1), visamsg->cvmresults().c_str());

	if(visamsg->has_cryptogramterminaltype())
		strcpy(message->add_tag("9F35", 55,1), visamsg->cryptogramterminaltype().c_str());

	if(visamsg->has_applicationtransactioncounter())
		strcpy(message->add_tag("9F36", 55,1), visamsg->applicationtransactioncounter().c_str());

	if(visamsg->has_unpredictablenumber())
		strcpy(message->add_tag("9F37", 55,1), visamsg->unpredictablenumber().c_str());

	if(visamsg->has_issuerscriptresults())
		strcpy(message->add_tag("9F5B", 55,1), visamsg->issuerscriptresults().c_str());

	if(visamsg->has_formfactorindicator())
		strcpy(message->add_tag("9F6E", 55,1), visamsg->formfactorindicator().c_str());

	if(visamsg->has_customerexclusivedata())
		strcpy(message->add_tag("9F7C", 55,1), visamsg->customerexclusivedata().c_str());

	if(visamsg->has_merchantaddress())
		strcpy(message->add_field(59), visamsg->merchantaddress().c_str());

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::PARTIALCAPABLE)
	{
		strcpy(message->add_field(60,10), "1");

		strcpy(message->add_field(60,9), "0");
		strcpy(message->add_field(60,8), "00");
		strcpy(message->add_field(60,7), "0");
		strcpy(message->add_field(60,6), "0");
		strcpy(message->add_field(60,5), "00");
		strcpy(message->add_field(60,4), "0");
		strcpy(message->add_field(60,3), "0");
		strcpy(message->add_field(60,2), "0");
		strcpy(message->add_field(60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & (isomessage::PHONEORDER | isomessage::RECURRING | isomessage::EM_INSTALLMENT | isomessage::ECOMMERCE))
	{
		if(visamsg->entrymodeflags() & isomessage::RECURRING)
			strcpy(message->add_field(60,8), "02");
		else if(visamsg->entrymodeflags() & isomessage::EM_INSTALLMENT)
			strcpy(message->add_field(60,8), "03");
		else if(visamsg->entrymodeflags() & isomessage::PHONEORDER)
			strcpy(message->add_field(60,8), "01");
		else if(visamsg->entrymodeflags() & isomessage::ECOMMERCE)
		{
			if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREACQUIRER)
				strcpy(message->add_field(60,8), "08");
			else if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREISSUER)
				strcpy(message->add_field(60,8), "06");
			else if(visamsg->entrymodeflags() & isomessage::ECOMNOTAUTHENTICATED)
				strcpy(message->add_field(60,8), "07");
			else
				strcpy(message->add_field(60,8), "05");
		}
		else
			strcpy(message->add_field(60,8), "00");

		strcpy(message->add_field(60,7), "0");
		strcpy(message->add_field(60,6), "0");
		strcpy(message->add_field(60,5), "00");
		strcpy(message->add_field(60,4), "0");
		strcpy(message->add_field(60,3), "0");
		strcpy(message->add_field(60,2), "0");
		strcpy(message->add_field(60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->cardauthenticationreliability()==isomessage::ACQUNRELIABLE)
	{
		strcpy(message->add_field(60,7), "1");

		strcpy(message->add_field(60,6), "0");
		strcpy(message->add_field(60,5), "00");
		strcpy(message->add_field(60,4), "0");
		strcpy(message->add_field(60,3), "0");
		strcpy(message->add_field(60,2), "0");
		strcpy(message->add_field(60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::EXISTINGDEBT)
	{
		strcpy(message->add_field(60,4), "9");

		strcpy(message->add_field(60,3), "0");
		strcpy(message->add_field(60,2), "0");
		strcpy(message->add_field(60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::FALLBACK)
	{
		if(visamsg->entrymodeflags() | isomessage::LASTTERMCHIPREADFAILED)
			strcpy(message->add_field(60,3), "2");
		else
			strcpy(message->add_field(60,3), "1");

		strcpy(message->add_field(60,2), "0");
		strcpy(message->add_field(60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & (isomessage::NOREADCAPABLE | isomessage::TERMNOTPRESENT | isomessage::MAGSTRIPECAPABLE | isomessage::CHIPCAPABLE | isomessage::CONTACTLESSCAPABLE))
	{
		if(visamsg->entrymodeflags() & isomessage::NOREADCAPABLE)
			strcpy(message->add_field(60,2), "9");
		else if(visamsg->entrymodeflags() & isomessage::TERMNOTPRESENT)
			strcpy(message->add_field(60,2), "1");
		else if(visamsg->entrymodeflags() & isomessage::CHIPCAPABLE)
			strcpy(message->add_field(60,2), "5");
		else if(visamsg->entrymodeflags() & isomessage::CONTACTLESSCAPABLE)
			strcpy(message->add_field(60,2), "8");
		else if(visamsg->entrymodeflags() & isomessage::MAGSTRIPECAPABLE)
			strcpy(message->add_field(60,2), "2");
		else
			strcpy(message->add_field(60,2), "0");

		strcpy(message->add_field(60,1), "0");
	}

	if(isRequest(visamsg) && visamsg->has_terminaltype())
		switch(visamsg->terminaltype())
		{
			case isomessage::LIMITEDAMOUNT:
				strcpy(message->add_field(60,1), "1");
				break;

			case isomessage::ATM:
				strcpy(message->add_field(60,1), "2");
				break;

			case isomessage::SELFSERVICE:
				strcpy(message->add_field(60,1), "3");
				break;

			case isomessage::CASHREGISTER:
				strcpy(message->add_field(60,1), "4");
				break;

			case isomessage::PERSONAL:
				strcpy(message->add_field(60,1), "5");
				break;

			case isomessage::PHONE:
				strcpy(message->add_field(60,1), "7");
				break;

			default:
				strcpy(message->add_field(60,1), "0");
				break;
		}

	if(isRequest(visamsg) && visamsg->replacementbillingamount())
	{
		snprintf(message->add_field(61,3), 13, "%012lld", visamsg->replacementbillingamount());
		strcpy(message->add_field(61,2), "000000000000");
		strcpy(message->add_field(61,1), "000000000000");
	}

	if(isRequest(visamsg) && visamsg->has_cashbackbillingamount())
	{
		snprintf(message->add_field(61,2), 13, "%012lld", visamsg->cashbackbillingamount());
		strcpy(message->add_field(61,1), "000000000000");
	}

	if(isRequest(visamsg) && visamsg->has_cashbackamount())
		snprintf(message->add_field(61,1), 13, "%012lld", visamsg->cashbackamount());







	return 1;
}

int isNetMgmt(field *message)
{
	if(message->get_field(2,0)[1]=='8')
		return 1;
	else
		return 0;
}

int isNetRequest(field *message)
{
	if(message->get_field(2,0)[2]=='0' || message->get_field(2,0)[2]=='2')
		return 1;
	else
		return 0;
}

int processNetMgmt(field *message)
{
	field *header;
	field *mbody;

	if(!message->has_field(1) || !message->has_field(2))
		return 0;

	header=&message->sf(1);
	mbody=&message->sf(2);

	strncpy(header->add_field(5), header->get_field(6), 6);
	strncpy(header->add_field(6), stationid, 6);

	if(mbody->get_field(0)[2]=='0')
		mbody->add_field(0)[2]='1';

	if(mbody->get_field(0)[2]=='2')
		mbody->add_field(0)[2]='3';

	strcpy(mbody->add_field(39), "00");

	return 1;
}

int declineNetMsg(field *message)
{
	field *header;
	field *mbody;

	if(!message->has_field(1) || !message->has_field(2))
		return 0;

	header=&message->sf(1);
	mbody=&message->sf(2);

	strncpy(header->add_field(5), header->get_field(6), 6);
	strncpy(header->add_field(6), stationid, 6);

	if(mbody->get_field(0)[2]=='0')
		mbody->add_field(0)[2]='1';

	if(mbody->get_field(0)[2]=='2')
		mbody->add_field(0)[2]='3';

	header->add_field(9)[16]='1';

	strcpy(mbody->add_field(39 ), "96");
	strcpy(mbody->add_field(44,1 ), "5");

	mbody->remove_field(18);
	mbody->remove_field(22);
	mbody->remove_field(35);
	mbody->remove_field(43);
	mbody->remove_field(60);
	mbody->remove_field(104);

	return 1;
}
