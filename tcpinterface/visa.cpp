#include <stdio.h>
#include "net.h"
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include "visa.pb.h"

int debug=1;

using namespace std;

int ipcconnect(int);

int processIncoming(isomessage *visamsg, field &fullmessage, VisaContext *context);
int processOutgoing(field &fullmessage, isomessage *visamsg, fldformat *frm, VisaContext *context);

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

int parseNetMsg(field &message, char *buf, size_t length)
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

unsigned int buildNetMsg(char *buf, size_t maxlength, field &message)
{
	unsigned int length;

	if(!buf)
	{
		printf("Error: no buf\n");
		return 0;
	}

	message(1,4)="0000";

	length=message.get_blength();

	if(length==0 || message(1,4).snprintf(5, "%04X", length - message.get_lengthLength()) > 4)
	{
		printf("Error: Unable to calculate the message length (%d)\n", length);
		return 0;
	}

	return message.build_message(buf, maxlength);
	
}

int fillDefaultContext(VisaContext *context)
{
	return 0;
}

int translateNetToSwitch(isomessage *visamsg, field &fullmessage)
{
	VisaContext context;
	int i;

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

int translateSwitchToNet(field &message, isomessage *visamsg, fldformat *frm)
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

int processIncoming(isomessage *visamsg, field &fullmessage, VisaContext *context)
{
	struct tm datetime, current;
	time_t now;

	char tmpstr[26];
	unsigned int tmpint, i;

	time(&now);
	gmtime_r(&now, &current);

	if(!fullmessage.sfexist(1))
	{
		printf("Error: No message header found\n");
		return 1;
	}

	if(!fullmessage.sfexist(2))
	{
		printf("Error: No message body found\n");
		return 1;
	}

	field &header=fullmessage(1);
	field &message=fullmessage(2);

	context->set_sourcestationid(header(6));
	context->set_visaroundtripinf(header(7));
	context->set_visabaseiflags(header(8));
	context->set_visamsgstatusflags(header(9));
	context->set_batchnumber(header(10));
	context->set_visareserved(header(11));
	context->set_visauserinfo(header(12));

	if(!message.sfexist(0))
	{
		printf("Error: No message type\n");
		return 1;
	}

	switch(message(0)[1]-'0')
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
			printf("Error: Unknown message type: '%s'\n", message(0));
	}

	switch(message(0)[2]-'0')
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
			printf("Error: Unknown message type: '%s'\n", message(0));
	}

	switch(message(0)[3]-'0')
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
			printf("Error: Unknown message type: '%s'\n", message(0));
	}

	if(message.sfexist(2))
		visamsg->set_pan(message(2));

	if(message.sfexist(3))
	{
		switch(atoi(message(3,1).c_str()))
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
				printf("Error: Unknown processing code: '%s'\n", message(3,1));
				return 1;
		}

		switch(atoi(message(3,2).c_str()))
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
				printf("Warning: Unknown From account type: '%s'\n", message(3,2));
		}

		switch(atoi(message(3,3).c_str()))
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
				printf("Warning: Unknown To account type: '%s'\n", message(3,3));
		}
	}

	if(message.sfexist(4))
		visamsg->set_amounttransaction(atoll(message(4).c_str()));

	if(message.sfexist(5))
		visamsg->set_amountsettlement(atoll(message(5).c_str()));

	if(message.sfexist(6))
		visamsg->set_amountbilling(atoll(message(6).c_str()));

	if(message.sfexist(7))
	{
		memset(&datetime, 0, sizeof(datetime));
		sscanf(message(7).c_str(), "%2d%2d%2d%2d%2d", &datetime.tm_mon, &datetime.tm_mday, &datetime.tm_hour, &datetime.tm_min, &datetime.tm_sec);
		datetime.tm_mon--;

		datetime.tm_year=current.tm_year;

		if(datetime.tm_mon>8 && current.tm_mon<3)
			datetime.tm_year--;
		else if(datetime.tm_mon<3 && current.tm_mon>8)
			datetime.tm_year++;

		visamsg->set_transactiondatetime(timegm(&datetime));
	}

	if(message.sfexist(9))
		visamsg->set_conversionratesettlement(atof(message(9,1).c_str())*pow01(message(9,0)[0]-'0'));
	
	if(message.sfexist(10))
		visamsg->set_conversionratebilling(atof(message(10,1).c_str())*pow01(message(10,0)[0]-'0'));
	
	if(message.sfexist(11))
		visamsg->set_stan(atoi(message(11).c_str()));

	if(message.sfexist(12))
		visamsg->set_terminaltime(message(12));

	if(message.sfexist(13))
	{
		if(message(13)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(message(13)[0]=='0' && message(13)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, message(13).c_str());

		visamsg->set_terminaldate(tmpstr);
	}

	if(message.sfexist(14))
		visamsg->set_expirydate(message(14));

	if(message.sfexist(15))
	{
		if(message(15)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(message(15)[0]=='0' && message(15)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, message(15).c_str());

		visamsg->set_settlementdate(tmpstr);
	}

	if(message.sfexist(16))
	{
		if(message(16)[0]=='1' && current.tm_mon<3)
			sprintf(tmpstr, "%04d", current.tm_year+1900-1);
		else if(message(16)[0]=='0' && message(16)[1]-'0'<4 && current.tm_mon>8)
			sprintf(tmpstr, "%04d", current.tm_year+1900+1);
		else
			sprintf(tmpstr, "%04d", current.tm_year+1900);

		strcpy(tmpstr+4, message(16).c_str());

		visamsg->set_conversiondate(tmpstr);
	}

	if(message.sfexist(18))
		visamsg->set_mcc(atoi(message(18).c_str()));

	if(message.sfexist(19))
		visamsg->set_acquirercountry(atoi(message(19).c_str()));

	if(message.sfexist(20))
		visamsg->set_issuercountry(atoi(message(20).c_str()));

	if(message.sfexist(22))
	{
		tmpint=0;

		switch(atoi(message(22,1 ).c_str()))
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

		switch(atoi(message(22,2 ).c_str()))
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

	if(message.sfexist(23))
		visamsg->set_cardsequencenumber(atoi(message(23).c_str()));

	switch(atoi(message(25).c_str()))
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
			if(message(126,13)[0]=='R' || message(60,8)=="02")
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

	if(message.sfexist(26))
		visamsg->set_termpinmaxdigits(atoi(message(26).c_str()));

	if(message.sfexist(28))
	{
		if(message(28)[0]=='C')
			visamsg->set_amountacquirerfee(-atoll(message(28).c_str()+1));
		else
			visamsg->set_amountacquirerfee(atoll(message(28).c_str()+1));
	}

	if(message.sfexist(32))
		visamsg->set_acquirerid(atoll(message(32).c_str()));
	
	if(message.sfexist(33))
		visamsg->set_forwardingid(atoll(message(33).c_str()));

	if(message.sfexist(35))
		visamsg->set_track2(message(35));

	if(message.sfexist(37))
		visamsg->set_rrn(atoll(message(37).c_str()));

	if(message.sfexist(38))
		visamsg->set_authid(message(38));

	if(message.sfexist(39))
		visamsg->set_responsecode(atoi(message(39).c_str()));

	if(message.sfexist(41))
	{
		strcpy(tmpstr, message(41).c_str());

		for(i=7; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_terminalid(tmpstr);
	}
			
	if(message.sfexist(42))
	{
		strcpy(tmpstr, message(42).c_str());

		for(i=14; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantid(tmpstr);
	}

	if(message.sfexist(43))
	{
		strcpy(tmpstr, message(43,1).c_str());

		for(i=24; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantname(tmpstr);

		strcpy(tmpstr, message(43,2).c_str());

		for(i=12; i!=0; i--)
			if(tmpstr[i]!=' ')
				break;

		tmpstr[i+1]='\0';

		visamsg->set_merchantcity(tmpstr);

		visamsg->set_merchantcountry(message(43,3));
	}

	if(message.sfexist(44,1))
	{
		if(message(44,1)[0]=='5')
			visamsg->set_responsesource(isomessage::RSP_ISSUER);
		else
			visamsg->set_responsesource(isomessage::RSP_NETWORK);
	}

	switch(message(44,2)[0])
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

	switch(message(44,5)[0])
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

	switch(message(44,8)[0])
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

	switch(message(44,10)[0])
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

	if(message.sfexist(44,11))
		visamsg->set_originalresponsecode(atoi(message(44,11).c_str()));

	switch(message(44,13)[0])
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

	if(message.sfexist(45))
		visamsg->set_track1(message(45));

	if(message.sfexist(48))
		visamsg->set_additionaltext(message(48));

	if(message.sfexist(49))
		visamsg->set_currencytransaction(atoi(message(49).c_str()));

	if(message.sfexist(51))
		visamsg->set_currencybilling(atoi(message(51).c_str()));

	if(message.sfexist(52))
		visamsg->set_pin(message(52));

	if(message.sfexist(53))
	{
		switch (atoi(message(53,1).c_str()))
		{
			case 02:
				visamsg->set_pinsecurityformat(isomessage::ISSUERKEY);
				break;

			case 20:
				visamsg->set_pinsecurityformat(isomessage::ZONE);
				break;
		}

		if(message(53,2)=="01")
			visamsg->set_pinencryptionalgorithm(isomessage::ANSIDES);

		switch (atoi(message(53,3).c_str()))
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

		visamsg->set_pinkeyindex(atoi(message(53,4).c_str()));

		if(message(53,5)=="01")
			visamsg->set_pinispassword(true);
	}

	if(message.sfexist(54))
	{
		for(i=0; i<6; i++)
			if(message.sfexist(54,i))
			{
				isomessage::AddAmnt *addamnt=visamsg->add_additionalamount();

				addamnt->set_accounttype((isomessage_AccntType)atoi(message(54,i,1).c_str()));

				addamnt->set_amounttype((isomessage_AddAmnt_AmntType)atoi(message(54,i,2).c_str()));

				addamnt->set_currency(atoi(message(54,i,3).c_str()));

				if(message(54,i,4)[0]=='C')
					addamnt->set_amount(atoi(message(54,i,5).c_str()));
				else
					addamnt->set_amount(-atoi(message(54,i,5).c_str()));
			}
			else
				break;
	}

	if(message.sfexist(55,1))
		for(field::const_iterator i=message(55,1).begin(); i!=message(55,1).end(); ++i)
			switch(i->first)
			{
				case 0x71:
					visamsg->set_issuerscript1(i->second);
					break;
				case 0x72:
					visamsg->set_issuerscript2(i->second);
					break;
				case 0x82:
					visamsg->set_applicationinterchangeprofile(i->second);
					break;
				case 0x91:
					visamsg->set_issuerauthenticationdata(i->second);
					break;
				case 0x95:
					visamsg->set_terminalverificationresults(i->second);
					break;
				case 0x9A:
					visamsg->set_terminaltransactiondate(i->second);
					break;
				case 0x9C:
					visamsg->set_cryptogramtransactiontype(i->second);
					break;
				case 0xC0:
					visamsg->set_secondarypinblock(i->second);
					break;
				case 0x5F2A:
					visamsg->set_cryptogramcurrency(i->second);
					break;
				case 0x9F02:
					visamsg->set_cryptogramtransactionamount(i->second);
					break;
				case 0x9F03:
					visamsg->set_cryptogramcashbackamount(i->second);
					break;
				case 0x9F09:
					visamsg->set_applicationversionnumber(i->second);
					break;
				case 0x9F10:
					visamsg->set_issuerapplicationdata(i->second);
					break;
				case 0x9F1A:
					visamsg->set_terminalcountry(i->second);
					break;
				case 0x9F1E:
					visamsg->set_terminalserialnumber(i->second);
					break;
				case 0x9F26:
					visamsg->set_cryptogram(i->second);
					break;
				case 0x9F27:
					visamsg->set_cryptograminformationdata(i->second);
					break;
				case 0x9F33:
					visamsg->set_terminalcapabilityprofile(i->second);
					break;
				case 0x9F34:
					visamsg->set_cvmresults(i->second);
					break;
				case 0x9F35:
					visamsg->set_cryptogramterminaltype(i->second);
					break;
				case 0x9F36:
					visamsg->set_applicationtransactioncounter(i->second);
					break;
				case 0x9F37:
					visamsg->set_unpredictablenumber(i->second);
					break;
				case 0x9F5B:
					visamsg->set_issuerscriptresults(i->second);
					break;
				case 0x9F6E:
					visamsg->set_formfactorindicator(i->second);
					break;
				case 0x9F7C:
					visamsg->set_customerexclusivedata(i->second);
					break;
				default:
					printf("No map for tag %X\n", tmpint);
			}

	if(message.sfexist(59))
		visamsg->set_merchantaddress(message(59));

	switch(message(60,1)[0])
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

	switch(message(60,2)[0])
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

	if(message(60,3)[0]=='2')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::LASTTERMCHIPREADFAILED);

	if(message(60,4)[0]=='9')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::EXISTINGDEBT);

	if(message(60,6)[0]=='2')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::EXPANDEDTHIRDBM);

	switch(message(60,7)[0])
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

	switch(atoi(message(60,8).c_str()))
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

	if(message(60,10)[0]=='1')
		visamsg->set_entrymodeflags(visamsg->entrymodeflags() | isomessage::PARTIALCAPABLE);

	if(message.sfexist(61,1))
		visamsg->set_cashbackamount(atoll(message(61,1).c_str()));

	if(message.sfexist(61,2))
		visamsg->set_cashbackbillingamount(atoll(message(61,2).c_str()));

	if(message.sfexist(61,3))
		visamsg->set_replacementbillingamount(atoll(message(61,3).c_str()));








	return 0;
}

int processOutgoing(field &fullmessage, isomessage *visamsg, fldformat *frm, VisaContext *context)
{
	fullmessage.set_frm(frm);

	time_t datetime;

	unsigned int i;

	fullmessage(0)="0000";
	field &header=fullmessage(1);
	field &message=fullmessage(2);

	header(2)="01";

//	if(message.has_sf(62,0) && message(62,0)->frm->dataFormat==FRM_BITMAP) //TODO: uncomment
		header(3)="02";
//	else
//		header(3)="01";

	if(isRequest(visamsg))
	{
		header(5)="000000";
		header(7)="00000000";
		header(8)="0000000000000000";
		header(9)="000000000000000000000000";
		header(10)="00";
		header(11)="000000";
		header(12)="00";
	}
	else
	{
		header(5)=context->sourcestationid().c_str();
		header(7)=context->visaroundtripinf().c_str();
		header(8)=context->visabaseiflags().c_str();
		header(9)=context->visamsgstatusflags().c_str();
		header(10)=context->visamsgstatusflags().c_str();
		header(11)=context->visareserved().c_str();
		header(12)=context->visauserinfo().c_str();
	}
	
	header(6)=stationid;

	message(0)[0]='0';

	if(visamsg->messagetype() & isomessage::REVERSAL)
		message(0)[1]='4';
	else if(visamsg->messagetype() & isomessage::CLEARING)
		message(0)[1]='2';
	else
		message(0)[1]='1';

	if(visamsg->messagetype() & isomessage::ADVICE)
		if(visamsg->messagetype() & isomessage::RESPONSE)
			message(0)[2]='3';
		else
			message(0)[2]='2';
	else
		if(visamsg->messagetype() & isomessage::RESPONSE)
			message(0)[2]='1';
		else
			message(0)[2]='0';

	if(visamsg->messagetype() & isomessage::ISSUER)
		if(visamsg->messagetype() & isomessage::REPEAT)
			message(0)[3]='3';
		else
			message(0)[3]='2';
	else
		if(visamsg->messagetype() & isomessage::REPEAT)
			message(0)[3]='1';
		else
			message(0)[3]='0';

	if(visamsg->has_pan())
		message(2)=visamsg->pan();

	if(visamsg->has_transactiontype())
	{
		switch(visamsg->transactiontype())
		{
			case isomessage::PURCHASE:
				message(3,1 )="00";
				break;

			case isomessage::CASH:
				message(3,1 )="01";
				break;

			case isomessage::CHECK:
				message(3,1 )="03";
				break;

			case isomessage::ACCNTFUNDING:
				message(3,1 )="10";
				break;

			case isomessage::ORIGINALCREDIT:
				message(3,1 )="26";
				break;

			default:
				printf("Error: Unknown transaction type: %d\n", visamsg->transactiontype());
				return 0;
		}

		switch(visamsg->accounttypefrom())
		{
			case isomessage::DEFAULT:
				message(3,2)="00";
				break;

			case isomessage::SAVINGS:
				message(3,2)="10";
				break;

			case isomessage::CHECKING:
				message(3,2)="20";
				break;

			case isomessage::CREDIT:
				message(3,2)="30";
				break;

			case isomessage::UNIVERSAL:
				message(3,2)="40";
				break;

			default:
				printf("Warning: Unknown account type From: '%d'. Using default.\n", visamsg->accounttypefrom());
				message(3,2)="00";
				break;
		}

		switch(visamsg->accounttypeto())
		{
			case isomessage::DEFAULT:
				message(3,3)="00";
				break;

			case isomessage::SAVINGS:
				message(3,3)="10";
				break;

			case isomessage::CHECKING:
				message(3,3)="20";
				break;

			case isomessage::CREDIT:
				message(3,3)="30";
				break;

			case isomessage::UNIVERSAL:
				message(3,3)="40";
				break;

			default:
				printf("Warning: Unknown account type To: '%d'. Using default.\n", visamsg->accounttypeto());
				message(3,3)="00";
				break;
		}
	}

	if(visamsg->has_amounttransaction())
		message(4).snprintf(13, "%012lld", visamsg->amounttransaction());

	if(visamsg->has_amountsettlement())
		message(5).snprintf(13, "%012lld", visamsg->amountsettlement());

	if(visamsg->has_amountbilling())
		message(6).snprintf(13, "%012lld", visamsg->amountbilling());

	if(visamsg->has_transactiondatetime())
	{
		datetime=visamsg->transactiondatetime();
		printf("UTC Transaction date time: %s\n", asctime(gmtime(&datetime)));
		message(7).strftime(11, "%m%d%H%M%S", gmtime(&datetime));
	}

	if(visamsg->has_stan())
		message(11).snprintf(7, "%06d", visamsg->stan());
	else
		printf("Warning: No STAN\n");

	if(visamsg->has_terminaltime())
		message(12)=visamsg->terminaltime();

	if(visamsg->has_terminaldate())
		message(13).assign(visamsg->terminaldate(), 4, string::npos);

	if(visamsg->has_expirydate() && isRequest(visamsg))
		message(14)=visamsg->expirydate();

	if(visamsg->has_settlementdate() && !isRequest(visamsg))
		message(15).assign(visamsg->settlementdate(), 4, string::npos);

	if(visamsg->has_mcc() && isRequest(visamsg))
		message(18).snprintf(5, "%04d", visamsg->mcc());

	if(visamsg->has_acquirercountry())
		message(19).snprintf(4, "%03d", visamsg->acquirercountry());

	if(visamsg->has_issuercountry())
		message(20).snprintf(4, "%03d", visamsg->issuercountry());
	
	if(isRequest(visamsg) && visamsg->has_entrymode())
	{
		switch(visamsg->entrymode())
		{
			case isomessage::MANUAL:
				message(22,1 )="01";
				break;
			case isomessage::BARCODE:
				message(22,1 )="03";
				break;
			case isomessage::OPTICAL:
				message(22,1 )="04";
				break;
			case isomessage::CHIP:
				if(visamsg->entrymodeflags() & isomessage::CONTACTLESS)
					message(22,1 )="07";
				else if(visamsg->entrymodeflags() & isomessage::CVVUNRELIABLE)
					message(22,1 )="95";
				else
					message(22,1 )="05";
				break;
			case isomessage::MAGSTRIPE:
				if(visamsg->entrymodeflags() & isomessage::CONTACTLESS)
					message(22,1 )="91";
				else if(visamsg->entrymodeflags() & isomessage::CVVUNRELIABLE)
					message(22,1 )="02";
				else
					message(22,1 )="91";
				break;
			case isomessage::STORED:
				message(22,1 )="96";
				break;
			default:
				message(22,1 )="00";
		}

		switch(visamsg->entrymodeflags() & (isomessage::PINCAPABLE | isomessage::NOTPINCAPABLE))
		{
			case isomessage::PINCAPABLE:
				message(22,2 )="1";
				break;
			case isomessage::NOTPINCAPABLE:
				message(22,2 )="2";
				break;
			case isomessage::PINCAPABLE | isomessage::NOTPINCAPABLE:
				message(22,2 )="8";
				break;
			default:
				message(22,2 )="0";
		}
	}

	if(isRequest(visamsg) && visamsg->has_cardsequencenumber())
		message(23).snprintf(4, "%03d", visamsg->cardsequencenumber());

	if(visamsg->messagetype() & isomessage::PREAUTHORIZATION)
		message(25 )="00";
	else if(visamsg->messagetype() & isomessage::PREAUTHCOMPLETION)
		message(25 )="06";
	else if((visamsg->entrymodeflags() & isomessage::PHONEORDER) || (visamsg->entrymodeflags() & isomessage::RECURRING))
		message(25 )="08";
	else if(visamsg->entrymodeflags() & isomessage::ECOMMERCE)
		message(25 )="59";
	else if(visamsg->entrymodeflags() & isomessage::NOTAUTHORIZED)
		message(25 )="51";
	else if(visamsg->entrymodeflags() & isomessage::MERCHANTSUSPICIOUS)
		message(25 )="03";
	else if((visamsg->entrymodeflags() & isomessage::TERMUNATTENDED) && !(visamsg->entrymodeflags() & isomessage::CARDNOTPRESENT))
		message(25 )="02";
	else if(visamsg->entrymodeflags() & isomessage::CARDHOLDERNOTPRESENT)
		message(25 )="01";
	else if(visamsg->entrymodeflags() & isomessage::CARDNOTPRESENT)
		message(25 )="05";
	else if((visamsg->entrymode() == isomessage::MANUAL) && (visamsg->entrymodeflags() & isomessage::FALLBACK))
		message(25 )="71";
	else
		message(25 )="00";

	if(isRequest(visamsg) && visamsg->has_termpinmaxdigits() && visamsg->has_pin())
		message(26).snprintf(3, "%02d", visamsg->termpinmaxdigits());

	if(isRequest(visamsg) && visamsg->has_amountacquirerfee())
	{
		if(visamsg->amountacquirerfee()<0)
			message(28).snprintf(10, "C%08lld", -visamsg->amountacquirerfee());
		else
			message(28).snprintf(10, "D%08lld", visamsg->amountacquirerfee());
	}

	if(visamsg->has_acquirerid())
		message(32).snprintf(12, "%lld", visamsg->acquirerid());

	if(visamsg->has_forwardingid())
		message(33).snprintf(12, "%lld", visamsg->forwardingid());

	if(isRequest(visamsg) && visamsg->has_track2() && (visamsg->entrymode()==isomessage::MAGSTRIPE || visamsg->entrymode()==isomessage::EM_UNKNOWN))
		message(35)=visamsg->track2();

	if(visamsg->has_rrn());
		message(37).snprintf(13, "%012lld", visamsg->rrn());

	if(visamsg->has_authid())
		message(38)=visamsg->authid();

	if(visamsg->has_responsecode() || (visamsg->messagetype() & isomessage::RESPONSE))
		message(39).sprintf("%02d", visamsg->responsecode());

	if(visamsg->has_terminalid())
		message(41).snprintf(8, "%s        ", visamsg->terminalid().c_str());

	if(visamsg->has_merchantid())
		message(42).snprintf(15, "%s               ", visamsg->merchantid().c_str());

	if(isRequest(visamsg) && (visamsg->has_merchantname() || visamsg->has_merchantcity() || visamsg->has_merchantcountry()))
	{
		message(43,1).snprintf(25, "%s                         ", visamsg->merchantname().c_str());
		message(43,2).snprintf(13, "%s             ", visamsg->merchantcity().c_str());
		message(43,3)=visamsg->merchantcountry();
	}

	if(!isRequest(visamsg) && visamsg->has_cavvverification())
	{
		if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREISSUER)
			switch(visamsg->cavvverification())
			{
				case isomessage::MATCH:
					message(44,13)="3";
					break;

				case isomessage::NOMATCH:
					message(44,13)="4";
					break;

				default:
					message(44,13)="0";
					break;
			}
		else
			switch(visamsg->cavvverification())
			{
				case isomessage::MATCH:
					message(44,13)="2";
					break;

				case isomessage::NOMATCH:
					message(44,13)="1";
					break;

				default:
					message(44,13)="0";
					break;
			}

		message(44,12)=" ";
		message(44,11)="  ";
		message(44,10)=" ";
		message(44,9)=" ";
		message(44,8)=" ";
		message(44,7)=" ";
		message(44,6)="  ";
		message(44,5)=" ";
		message(44,4)=" ";
		message(44,3)=" ";
		message(44,2)=" ";
		message(44,1)=" ";
	}

	if(!isRequest(visamsg) && visamsg->has_cvv2verification())
	{
		switch(visamsg->cvv2verification())
		{
			case isomessage::MATCH:
				message(44,10)="M";
				break;

			case isomessage::NOMATCH:
				message(44,10)="N";
				break;

			default:
				message(44,10)="P";
				break;
		}

		message(44,9)=" ";
		message(44,8)=" ";
		message(44,7)=" ";
		message(44,6)="  ";
		message(44,5)=" ";
		message(44,4)=" ";
		message(44,3)=" ";
		message(44,2)=" ";
		message(44,1)=" ";
	}

	if(!isRequest(visamsg) && visamsg->has_cardauthenticationresults())
		if(visamsg->cardauthenticationresults()==isomessage::MATCH || visamsg->cardauthenticationresults()==isomessage::NOMATCH)
		{
			if(visamsg->cardauthenticationresults()==isomessage::MATCH)
				message(44,8)="2";
			else
				message(44,8)="1";

			message(44,7)=" ";
			message(44,6)="  ";
			message(44,5)=" ";
			message(44,4)=" ";
			message(44,3)=" ";
			message(44,2)=" ";
			message(44,1)=" ";
		}

	if(!isRequest(visamsg) && visamsg->has_cvvverification())
		if(visamsg->cvvverification()==isomessage::MATCH || visamsg->cvvverification()==isomessage::NOMATCH)
		{
			if(visamsg->cvvverification()==isomessage::MATCH)
				message(44,5)="2";
			else
				message(44,5)="1";

			message(44,4)=" ";
			message(44,3)=" ";
			message(44,2)=" ";
			message(44,1)=" ";
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
							message(44,2)="Y";
						else
							message(44,2)="M";
						break;

					case isomessage::NOMATCH:
					case isomessage::NOTPERFORMED:
						message(44,2)="A";
						break;

					default:
						message(44,2)="B";
						break;
				}
				break;

			case isomessage::NOMATCH:
				if(visamsg->postalcodeverification()==isomessage::MATCH)
					message(44,2)="Z";
				else
					message(44,2)="N";
				break;

			case isomessage::NOTPERFORMED:
				switch(visamsg->postalcodeverification())
				{
					case isomessage::MATCH:
						message(44,2)="Z";
						break;

					case isomessage::NOMATCH:
						message(44,2)="N";
						break;

					default:
						if(isDomestic(visamsg))
							message(44,2)="C";
						else
							message(44,2)="I";
						break;
				}

			default:
				switch(visamsg->postalcodeverification())
				{
					case isomessage::MATCH:
						message(44,2)="P";
						break;

					case isomessage::NOMATCH:
						message(44,2)="N";
						break;

					case isomessage::NOTPERFORMED:
						if(isDomestic(visamsg))
							message(44,2)="N";
						else
							message(44,2)="I";
						break;

					default:
						message(44,2)="C";
						break;
				}
		}

		message(44,1)=" ";
	}

	if(isRequest(visamsg) && visamsg->has_track1() && (visamsg->entrymode()==isomessage::MAGSTRIPE || visamsg->entrymode()==isomessage::EM_UNKNOWN))
		message(45)=visamsg->track1();

	if(visamsg->has_additionaltext())
		message(48)=visamsg->additionaltext();

	if(visamsg->has_currencytransaction())
		message(49).snprintf(4, "%03d", visamsg->currencytransaction());

	if(!isRequest(visamsg) && visamsg->has_currencybilling())
		message(51).snprintf(4, "%03d", visamsg->currencybilling());

	if(isRequest(visamsg) && visamsg->has_pin())
	{
		message(52)=visamsg->pin();

		if(visamsg->pinsecurityformat()!=isomessage::ZONE)
			printf("Warning: Unsupported PIN Security Format, must be Zone encryption.\n");
		message(53,1)="20";

		if(visamsg->pinencryptionalgorithm()!=isomessage::ANSIDES)
			printf("Warning: Unsupported PIN Encryption Algorithm, must be ANSI DES.\n");
		message(53,2)="01";

		if(visamsg->pinblockformat()!=isomessage::ISO0)
			printf("Warning: Unsupported PIN Block Format, must be ISO Format 0.\n");
		message(53,3)="01";

		message(53,4).snprintf(3, "%02d", visamsg->pinkeyindex());

		message(53,5)="00";

		message(53,6)="000000";
	}

	for(i=0; i<visamsg->additionalamount_size() && i<6; i++)
	{
		const isomessage::AddAmnt& addamnt=visamsg->additionalamount(i);

		message(54,i,1).snprintf(3, "%02d", addamnt.accounttype());

		message(54,i,2).snprintf(3, "%02d", addamnt.amounttype());

		message(54,i,3).snprintf(4, "%03d", addamnt.currency());

		if(addamnt.amount() < 0)
		{
			message(54,i,4)="D";
			message(54,i,5).snprintf(13, "%012d", -addamnt.amount());
		}
		else
		{
			message(54,i,4)="C";
			message(54,i,5).snprintf(13, "%012d", addamnt.amount());
		}
	}

	if(visamsg->has_issuerscript1())
		message(55, 1, 0x71)=visamsg->issuerscript1();

	if(visamsg->has_issuerscript2())
		message(55, 1, 0x72)=visamsg->issuerscript2();

	if(visamsg->has_applicationinterchangeprofile())
		message(55, 1, 0x82)=visamsg->applicationinterchangeprofile();

	if(visamsg->has_issuerauthenticationdata())
		message(55, 1, 0x91)=visamsg->issuerauthenticationdata();

	if(visamsg->has_terminalverificationresults())
		message(55, 1, 0x95)=visamsg->terminalverificationresults();

	if(visamsg->has_terminaltransactiondate())
		message(55, 1, 0x9A)=visamsg->terminaltransactiondate();

	if(visamsg->has_cryptogramtransactiontype())
		message(55, 1, 0x9C)=visamsg->cryptogramtransactiontype();

	if(visamsg->has_secondarypinblock())
		message(55, 1, 0xC0)=visamsg->secondarypinblock();

	if(visamsg->has_cryptogramcurrency())
		message(55, 1, 0x5F2A)=visamsg->cryptogramcurrency();

	if(visamsg->has_cryptogramtransactionamount())
		message(55, 1, 0x9F02)=visamsg->cryptogramtransactionamount();

	if(visamsg->has_cryptogramcashbackamount())
		message(55, 1, 0x9F03)=visamsg->cryptogramcashbackamount();

	if(visamsg->has_applicationversionnumber())
		message(55, 1, 0x9F09)=visamsg->applicationversionnumber();

	if(visamsg->has_issuerapplicationdata())
		message(55, 1, 0x9F10)=visamsg->issuerapplicationdata();

	if(visamsg->has_terminalcountry())
		message(55, 1, 0x9F1A)=visamsg->terminalcountry();

	if(visamsg->has_terminalserialnumber())
		message(55, 1, 0x9F1E)=visamsg->terminalserialnumber();

	if(visamsg->has_cryptogram())
		message(55, 1, 0x9F26)=visamsg->cryptogram();

	if(visamsg->has_cryptograminformationdata())
		message(55, 1, 0x9F27)=visamsg->cryptograminformationdata();

	if(visamsg->has_terminalcapabilityprofile())
		message(55, 1, 0x9F33)=visamsg->terminalcapabilityprofile();

	if(visamsg->has_cvmresults())
		message(55, 1, 0x9F34)=visamsg->cvmresults();

	if(visamsg->has_cryptogramterminaltype())
		message(55, 1, 0x9F35)=visamsg->cryptogramterminaltype();

	if(visamsg->has_applicationtransactioncounter())
		message(55, 1, 0x9F36)=visamsg->applicationtransactioncounter();

	if(visamsg->has_unpredictablenumber())
		message(55, 1, 0x9F37)=visamsg->unpredictablenumber();

	if(visamsg->has_issuerscriptresults())
		message(55, 1, 0x9F5B)=visamsg->issuerscriptresults();

	if(visamsg->has_formfactorindicator())
		message(55, 1, 0x9F6E)=visamsg->formfactorindicator();

	if(visamsg->has_customerexclusivedata())
		message(55, 1, 0x9F7C)=visamsg->customerexclusivedata();

	if(visamsg->has_merchantaddress())
		message(59)=visamsg->merchantaddress();

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::PARTIALCAPABLE)
	{
		message(60,10)="1";

		message(60,9)="0";
		message(60,8)="00";
		message(60,7)="0";
		message(60,6)="0";
		message(60,5)="00";
		message(60,4)="0";
		message(60,3)="0";
		message(60,2)="0";
		message(60,1)="0";
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & (isomessage::PHONEORDER | isomessage::RECURRING | isomessage::EM_INSTALLMENT | isomessage::ECOMMERCE))
	{
		if(visamsg->entrymodeflags() & isomessage::RECURRING)
			message(60,8)="02";
		else if(visamsg->entrymodeflags() & isomessage::EM_INSTALLMENT)
			message(60,8)="03";
		else if(visamsg->entrymodeflags() & isomessage::PHONEORDER)
			message(60,8)="01";
		else if(visamsg->entrymodeflags() & isomessage::ECOMMERCE)
		{
			if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREACQUIRER)
				message(60,8)="08";
			else if(visamsg->entrymodeflags() & isomessage::ECOMNOTSECUREISSUER)
				message(60,8)="06";
			else if(visamsg->entrymodeflags() & isomessage::ECOMNOTAUTHENTICATED)
				message(60,8)="07";
			else
				message(60,8)="05";
		}
		else
			message(60,8)="00";

		message(60,7)="0";
		message(60,6)="0";
		message(60,5)="00";
		message(60,4)="0";
		message(60,3)="0";
		message(60,2)="0";
		message(60,1)="0";
	}

	if(isRequest(visamsg) && visamsg->cardauthenticationreliability()==isomessage::ACQUNRELIABLE)
	{
		message(60,7)="1";

		message(60,6)="0";
		message(60,5)="00";
		message(60,4)="0";
		message(60,3)="0";
		message(60,2)="0";
		message(60,1)="0";
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::EXISTINGDEBT)
	{
		message(60,4)="9";

		message(60,3)="0";
		message(60,2)="0";
		message(60,1)="0";
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & isomessage::FALLBACK)
	{
		if(visamsg->entrymodeflags() | isomessage::LASTTERMCHIPREADFAILED)
			message(60,3)="2";
		else
			message(60,3)="1";

		message(60,2)="0";
		message(60,1)="0";
	}

	if(isRequest(visamsg) && visamsg->entrymodeflags() & (isomessage::NOREADCAPABLE | isomessage::TERMNOTPRESENT | isomessage::MAGSTRIPECAPABLE | isomessage::CHIPCAPABLE | isomessage::CONTACTLESSCAPABLE))
	{
		if(visamsg->entrymodeflags() & isomessage::NOREADCAPABLE)
			message(60,2)="9";
		else if(visamsg->entrymodeflags() & isomessage::TERMNOTPRESENT)
			message(60,2)="1";
		else if(visamsg->entrymodeflags() & isomessage::CHIPCAPABLE)
			message(60,2)="5";
		else if(visamsg->entrymodeflags() & isomessage::CONTACTLESSCAPABLE)
			message(60,2)="8";
		else if(visamsg->entrymodeflags() & isomessage::MAGSTRIPECAPABLE)
			message(60,2)="2";
		else
			message(60,2)="0";

		message(60,1)="0";
	}

	if(isRequest(visamsg) && visamsg->has_terminaltype())
		switch(visamsg->terminaltype())
		{
			case isomessage::LIMITEDAMOUNT:
				message(60,1)="1";
				break;

			case isomessage::ATM:
				message(60,1)="2";
				break;

			case isomessage::SELFSERVICE:
				message(60,1)="3";
				break;

			case isomessage::CASHREGISTER:
				message(60,1)="4";
				break;

			case isomessage::PERSONAL:
				message(60,1)="5";
				break;

			case isomessage::PHONE:
				message(60,1)="7";
				break;

			default:
				message(60,1)="0";
				break;
		}

	if(isRequest(visamsg) && visamsg->replacementbillingamount())
	{
		message(61,3).snprintf(13, "%012lld", visamsg->replacementbillingamount());
		message(61,2)="000000000000";
		message(61,1)="000000000000";
	}

	if(isRequest(visamsg) && visamsg->has_cashbackbillingamount())
	{
		message(61,2).snprintf(13, "%012lld", visamsg->cashbackbillingamount());
		message(61,1)="000000000000";
	}

	if(isRequest(visamsg) && visamsg->has_cashbackamount())
		message(61,1).snprintf(13, "%012lld", visamsg->cashbackamount());







	return 1;
}

int isNetMgmt(field &message)
{
	if(message(2,0)[1]=='8')
		return 1;
	else
		return 0;
}

int isNetRequest(field &message)
{
	if(message(2,0)[2]=='0' || message(2,0)[2]=='2')
		return 1;
	else
		return 0;
}

int processNetMgmt(field &message)
{
	if(!message.sfexist(1) || !message.sfexist(2))
		return 0;

	field &header=message(1);
	field &mbody=message(2);

	header(5)=header(6);
	header(6)=stationid;

	if(mbody(0)[2]=='0')
		mbody(0)[2]='1';

	if(mbody(0)[2]=='2')
		mbody(0)[2]='3';

	mbody(39)="00";

	return 1;
}

int declineNetMsg(field &message)
{
	if(!message.sfexist(1) || !message.sfexist(2))
		return 0;

	field &header=message(1);
	field &mbody=message(2);

	header(5)=header(6);
	header(6)=stationid;

	if(mbody(0)[2]=='0')
		mbody(0)[2]='1';

	if(mbody(0)[2]=='2')
		mbody(0)[2]='3';

	header(9)[16]='1';

	mbody(39)="96";
	mbody(44,1)="5";

	mbody(18).clear();
	mbody(22).clear();
	mbody(35).clear();
	mbody(43).clear();
	mbody(60).clear();
	mbody(104).clear();

	return 1;
}
