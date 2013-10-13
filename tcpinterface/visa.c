#include <stdio.h>
#include "net.h"

fldformat* loadNetFormat(void)
{
	return load_format("../parser/formats/fields_visa");
}

void processNetMsg(unsigned char *buf, unsigned int length, fldformat *frm)
{
	field *message;
	printf("\nMessage received, length %d\n", length);

	//printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	message=parse_message(buf, length, frm);   //TODO: For Visa, parse header (frm->fld[1]) and message(frm->fld[2]) separately to handle reject header properly.

	if(!message)
	{
		printf("Error: Unable to parse the message\n");
		return;
	}

	print_message(message, frm);
}

