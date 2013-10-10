#include <stdio.h>

#include "parser.h"

int main_(int argc, char **argv)
{
	fldformat *frm;
	field *message;
	char format_file[]="formats/fields_mast";
	char msgbuf[1000];
	char msgbuf2[1000];
	unsigned int msglen=0;
	unsigned int msglen2=0;
	FILE *file;
	unsigned int len;

	if(argc!=2)
	{
		printf("Error: No file name\n");
		return 1;
	}

	frm=load_format(format_file);

	if(!frm)
	{
		printf("Error: Can't load format (%s)\n", format_file);
		return 1;
	}

	printf("Message format loaded\n");

	file=fopen(argv[1], "r");
	
	while ((msgbuf[msglen++]=fgetc(file))!=EOF)
		if(msglen>sizeof(msgbuf))
		{
			printf("Message is too big\n");
			fclose(file);
			return 0;
		}

	fclose(file);

	msglen--;

	message=parse_message(msgbuf, msglen, frm);
	if(!message)
	{
		printf("Error: Unable to parse %s\n", frm->description);
		freeFormat(frm);
		return 1;
	}

	printf("%s parsed\n", frm->description);

	print_message(message, frm);

	printf("Building %s, estimated length: %d\n", frm->description, get_length(message, frm));

	msglen2=build_message(msgbuf2, sizeof(msgbuf2), message, frm);

	if(!msglen2)
	{
		printf("Error: Unable to build %s\n", frm->description);
		freeField(message);
		freeFormat(frm);
		return 1;
	}

	printf("%s built. Length: %d\n", frm->description, msglen2);

	file=fopen("message_out", "w");
	for(len=0; len<msglen2; fputc(msgbuf2[len++], file));
	fclose(file);

	if(msglen2!=msglen)
		printf("Warning: Total length don't match (%d)\n", msglen);

	freeField(message);
	freeFormat(frm);
	return 0;
}


int main(int argc, char **argv)
{
	fldformat *frm_header;
	fldformat *frm_message;
	field *message;
	field *header;
	char header_format_file[]="formats/visa_header";
	char message_format_file[]="formats/fields_visa";
	char msgbuf[1000];
	char msgbuf2[1000];
	unsigned int msglen=0;
	unsigned int msglen2=0;
	FILE *file;
	unsigned int len;

	if(argc!=2)
	{
		printf("Error: No file name\n");
		return 1;
	}

	frm_header=load_format(header_format_file);

	if(!frm_header)
	{
		printf("Error: Can't load format (%s)\n", header_format_file);
		return 1;
	}

	printf("Header format loaded\n");

	frm_message=load_format(message_format_file);

	if(!frm_message)
	{
		printf("Error: Can't load format (%s)\n", message_format_file);
		freeFormat(frm_header);
		return 1;
	}

	printf("Message format loaded\n");

	file=fopen(argv[1], "r");
	
	while ((msgbuf[msglen++]=fgetc(file))!=EOF)
		if(msglen>sizeof(msgbuf))
		{
			printf("Message is too big\n");
			fclose(file);
			return 0;
		}

	fclose(file);

	msglen--;

	header=parse_message(msgbuf, msglen, frm_header);
	if(!header)
	{
		printf("Error: Unable to parse the header\n");
		freeFormat(frm_header);
		freeFormat(frm_message);
		return 1;
	}

	printf("%s parsed\n", frm_header->description);

	print_message(header, frm_header);

	message=parse_message(msgbuf+header->length+frm_header->lengthInclusive*frm_header->lengthLength, msglen-header->length-frm_header->lengthInclusive*frm_header->lengthLength, frm_message);
	if(!message)
	{
		printf("Error: Unable to parse the message body\n");
		freeField(header);
		freeFormat(frm_header);
		freeFormat(frm_message);
		return 1;
	}

	printf("%s parsed\n", frm_message->description);

	print_message(message, frm_message);

	sscanf(header->fld[4]->data, "%X", &len);
	
	if(len != header->length + message->length + frm_header->lengthInclusive*frm_header->lengthLength + frm_message->lengthInclusive*frm_message->lengthLength)
	{
		printf("Warning: Total message length doesn't match [%s] %d, %d, %d\n", header->fld[4]->data, len, header->length, message->length);
		freeField(header);
		freeField(message);
		freeFormat(frm_header);
		freeFormat(frm_message);
		return 1;
	}

printf("Building %s, estimated length: %d\n", frm_header->description, get_length(header, frm_header));

	msglen2=build_message(msgbuf2, sizeof(msgbuf2), header, frm_header);

	if(!msglen2)
	{
		printf("Error: Unable to build %s\n", frm_header->description);
		freeField(header);
		freeField(message);
		freeFormat(frm_header);
		freeFormat(frm_message);
		return 1;
	}

	printf("%s built. Length: %d\n", frm_header->description, msglen2);

	file=fopen("message_out", "w");
	for(len=0; len<msglen2; fputc(msgbuf2[len++], file));
	fclose(file);
printf("now building the message\n");
printf("Estimated length: %d\n", get_length(message, frm_message));
	len=build_message(msgbuf2+msglen2, sizeof(msgbuf2)-msglen2, message, frm_message);

	if(!len)
	{
		printf("Error: Unable to build %s\n", frm_message->description);
		freeField(header);
		freeField(message);
		freeFormat(frm_header);
		freeFormat(frm_message);
		return 1;
	}

	printf("%s built. Length: %d\nTotal length: %d\n", frm_message->description, len, msglen2+len);

	msglen2+=len;

	file=fopen("message_out", "w");
	for(len=0; len<msglen2; fputc(msgbuf2[len++], file));
	fclose(file);

	if(msglen2!=msglen)
		printf("Warning: Total length don't match (%d)\n", msglen);

	freeField(header);
	freeField(message);
	freeFormat(frm_header);
	freeFormat(frm_message);
	return 0;
}

