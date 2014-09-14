#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

fldformat *findFrmParent(fldformat*, char*, int*);
//fldformat *newFrmChild(fldformat*, unsigned int);
//void removeFrmChild(fldformat*, unsigned int);
int parseFormat(fldformat*, char*);
int linkFrmChild(fldformat*, unsigned int, fldformat*);

fldformat* load_format(char *filename, fldformat *frmroot)
{
	char line[256];
	char number[sizeof(line)];
	char format[sizeof(line)];
	char descr[sizeof(line)];
	char chr;
	int i=0;
	int j=0;
	int k=0;
	fldformat *frmtmp, *frmpar, **altfrmpar=NULL;
	FILE *file;

	if(!frmroot)
		frmroot=(fldformat*)calloc(1, sizeof(fldformat));
	else
	{
		for(; frmroot->altformat!=NULL; )
			frmroot=frmroot->altformat;

		frmroot->altformat=(fldformat*)calloc(1, sizeof(fldformat));
		altfrmpar=&(frmroot->altformat);
		frmroot=frmroot->altformat;
	}
	
	if((file=fopen(filename, "r"))==NULL)
	{
		if(debug)
			perror("Error: Unable to open file\n");
		freeFormat(frmroot);
		if(altfrmpar)
			*altfrmpar=NULL;
		return NULL;
	}

	while ((chr=fgetc(file))!=EOF)
	{
		if(chr!='\n' && chr!='\r' && i<sizeof(line))
		{
			line[i++]=chr;
			continue;
		}
		
		line[i]='\0';

		for(j=0; j<i; j++)
			if(line[j]=='#')
			{
				line[j]='\0';
				break;
			}

		i=0;

		if(line[0]=='\0')
			continue;

		j=sscanf(line, "%s %s %[][a-zA-Z0-9 .,/;:'\"\\|?!@#$%^&*(){}<>_+=-]", number, format, descr);
		if(j==2)
			strcpy(descr, "No description");
		else if (j!=3)
		{
			if(debug)
				printf("Warning: Wrong format in line, skipping:\n%s\n", line);
			continue;
		}

		//printf("Number[%s] format[%s] descr[%s]\n", number, format, descr);

		if(k==0 && !strcmp(number, "message"))
		{
			frmroot->description=(char*)malloc(strlen(descr)+1);
			strcpy(frmroot->description, descr);

			parseFormat(frmroot, format);

			k++;
			continue;
		}

		if(k==0)
		{
			if(debug)
				printf("Warning: No 'message' format, implying default\n");
			frmroot->description=(char*)malloc(strlen("ISO8583 Message")+1);
			strcpy(frmroot->description, "ISO8583 Message");
			
			parseFormat(frmroot, (char*)"U1024SF");
			
			k++;
		}

		frmpar=findFrmParent(frmroot, number, &j);
		if(!frmpar)
		{
			if(debug)
				printf("Error: Unable to parse field number, skipping:\n%s\n", line);
			continue;
		}

		frmtmp=(fldformat*)calloc(1, sizeof(fldformat));

		frmtmp->description=(char*)malloc(strlen(number)+2+strlen(descr)+1);
		strcpy(frmtmp->description, number);
		strcpy(frmtmp->description+strlen(frmtmp->description), ". ");
		strcpy(frmtmp->description+strlen(frmtmp->description), descr);

		if (!parseFormat(frmtmp, format))
		{
			if(debug)
				printf("Error: Unable to parse format, skipping:\n%s\n", line);
			freeFormat(frmtmp);
			continue;
		}

		if(!linkFrmChild(frmpar, j, frmtmp))
		{
			if(debug)
				printf("Error: Unable to add format, skipping:\n%s\n", line);
			freeFormat(frmtmp);
			continue;
		}

		k++;
	}
	fclose(file);

	if(debug)
		printf("Info: Loaded %d fields\n", k);

	if(k==1)
	{
		if(debug)
			printf("Error: No fields loaded\n");
		freeFormat(frmroot);
		if(altfrmpar)
			*altfrmpar=NULL;
		return NULL;
	}

	return frmroot;
}

fldformat *findFrmParent(fldformat *frm, char *number, int *position)
{
	unsigned int i;
	unsigned int l=strlen(number);
	fldformat *altformat;
	
	if(!frm)
	{
		if(debug)
			printf("Error: Null pointer to 1st arg\n");
		return NULL;
	}

	if(!number)
	{
		if(debug)
			printf("Error: Null pointer to 2nd arg\n");
		return NULL;
	}

	if(!position)
        {
		if(debug)
                	printf("Error: Null pointer to 3rd arg\n");
                return NULL;
        }

	for(i=0; i<l; i++)
		if((number[i]<'0' || number[i]>'9') && number[i]!='*')
			break;
	if(i==0)
	{
		if(debug)
			printf("Error: Empty field number (%s)\n", number);
		return NULL;
	}
	if(i==l)
	{
		if(number[i-1]=='*')
			*position=0;
		else	
			*position=atoi(number);

		if(frm->altformat==NULL)
			return frm;

		//printf("Info: Using alternate format(%s)\n", number);

		for(altformat=frm; altformat->altformat!=NULL; )
			 altformat=altformat->altformat;

		return altformat;
	}

	if(number[i]!='.')
	{
		if(debug)
			printf("Error: Unrecognized field number (%s)\n", number);
		return NULL;
	}

	if(number[i-1]=='*')
		*position=0;
	else
	{
		number[i]='\0';
		*position=atoi(number);
		number[i]='.';
	}

	if(frm->fields < *position + 1)
	{
		if(debug)
			printf("Error: Parent format not loaded yet [%s][%d]\n", number, *position);
		return NULL;
	}

	if(frm->fld[*position]->altformat==NULL)
		return findFrmParent(frm->fld[*position], number+i+1, position);

	//printf("Info: Using alternate format(%s)\n", number);

	for(altformat=frm->fld[*position]; altformat->altformat!=NULL; )
		 altformat=altformat->altformat;

	return findFrmParent(altformat, number+i+1, position);
}

int linkFrmChild(fldformat *frm, unsigned int n, fldformat *cld)
{
	fldformat *altformat;

	if(!frm)
	{
		if(debug)
			printf("Error: Null pointer to first arg\n");
		return 0;
	}

	if(!cld)
	{
		if(debug)
			printf("Error: Null pointer to third arg\n");
		return 0;
	}

	if(n+1 > frm->maxFields)
	{
		if(debug)
			printf("Error: Exceeded max number of fields (required %d, max %d)\n", n, frm->maxFields);
		return 0;
	}

	if(frm->fields==0)
		frm->fld=(fldformat**)calloc(frm->maxFields, sizeof(fldformat*));
	
	if(frm->fields < n+1)
		frm->fields=n+1;

	if(frm->fld[n]==NULL)
		frm->fld[n]=cld;
	else
	{
		//printf("Info: Adding as an alternate format\n");

		for(altformat=frm->fld[n]; altformat->altformat!=NULL; )
			 altformat=altformat->altformat;

		altformat->altformat=cld;
	}

	return 1;
}

int parseFormat(fldformat *frm, char *format)
{
	int i, j=0;
	char tmpc;

	if(!frm)
	{
		if(debug)
			printf("Error: Null pointer to first arg\n");
		return 0;
	}

	if(!format)
	{
		if(debug)
			printf("Error: Null pointer to second arg\n");
		return 0;
	}

	if(!strcmp(format, "ISOBITMAP"))
	{
		frm->dataFormat=FRM_ISOBITMAP;
		frm->maxLength=192;
		return 1;
	}

	switch (format[0])
	{
		case 'F':
			frm->lengthFormat=FRM_FIXED;

			frm->lengthLength=0;

			j=1;

			break;

		case 'L':
			frm->lengthFormat=FRM_ASCII;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='L')
					break;

			frm->lengthLength=j;

			break;

		case 'B':
			frm->lengthFormat=FRM_BIN;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='B')
					break;

			frm->lengthLength=j;

			break;

		case 'C':
			frm->lengthFormat=FRM_BCD;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='C')
					break;

			frm->lengthLength=j;

			break;

		case 'E':
			frm->lengthFormat=FRM_EBCDIC;

			for(j=1; j<strlen(format); j++)
				if(format[j]!='E')
					break;

			frm->lengthLength=j;

			break;

		case 'U':
			frm->lengthFormat=FRM_UNKNOWN;
			frm->lengthLength=0;
			j=1;
			break;

		case 'M':
			frm->lengthFormat=FRM_EMVL;
			frm->lengthLength=1;
			j=1;
			break;

		default:
			if(debug)
				printf("Error: Unrecognized length format (%s)\n", format);
	}

	if(format[j]=='I')
	{
		frm->lengthInclusive=1;
		j++;
	}
	else
		frm->lengthInclusive=0;

	for(i=j; i<strlen(format); i++)
		if (format[i]<'0' || format[i]>'9')
			break;
	if(i==j)
	{
		if(debug)
			printf("Error: Unrecognized max length (%s)\n", format);
		return 0;
	}

	tmpc=format[i];
	format[i]='\0';
	frm->maxLength=atoi(format+j);
	format[i]=tmpc;

	if(!strcmp(format+i, "SF"))
	{
		frm->dataFormat=FRM_SUBFIELDS;
		frm->maxFields=196;
	}
	else if(!strncmp(format+i, "TLV1", 4))
	{
		frm->dataFormat=FRM_TLV1;
		frm->maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV2",4))
	{
		frm->dataFormat=FRM_TLV2;
		frm->maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV3",4))
	{
		frm->dataFormat=FRM_TLV3;
		frm->maxFields=16;
		i+=4;
	}
	else if(!strncmp(format+i, "TLV4",4))
	{
		frm->dataFormat=FRM_TLV4;
		frm->maxFields=16;
		i+=4;
	}
	else if(!strcmp(format+i, "TLVEMV"))
	{
		frm->dataFormat=FRM_TLVEMV;
		frm->tagFormat=FRM_HEX;
		frm->maxFields=16;
	}
	else if(!strncmp(format+i, "TLVDS", 5))
	{
		frm->dataFormat=FRM_TLVDS;
		frm->maxFields=100;
		i+=5;
	}
	else if(!strcmp(format+i, "BCDSF"))
	{
		frm->dataFormat=FRM_BCDSF;
		frm->maxFields=16;
	}
	else if(!strcmp(format+i, "BITMAP"))
		frm->dataFormat=FRM_BITMAP;
	else if(!strcmp(format+i, "BITSTR"))
		frm->dataFormat=FRM_BITSTR;
	else if(!strcmp(format+i, "EBCDIC") || !strcmp(format+i, "EBC"))
		frm->dataFormat=FRM_EBCDIC;
	else if(!strcmp(format+i, "BCD"))
		frm->dataFormat=FRM_BCD;
	else if(!strcmp(format+i, "BIN"))
		frm->dataFormat=FRM_BIN;
	else if(!strcmp(format+i, "HEX"))
		frm->dataFormat=FRM_HEX;
	else if(!strcmp(format+i, "ASCII") || !strcmp(format+i, "ASC"))
		frm->dataFormat=FRM_ASCII;
	else
	{
		if(debug)
			printf("Error: Unrecognized data format (%s)\n", format+i);
		return 0;
	}

	if(frm->dataFormat==FRM_TLV1 || frm->dataFormat==FRM_TLV2 || frm->dataFormat==FRM_TLV3 || frm->dataFormat==FRM_TLV4 || frm->dataFormat==FRM_TLVDS)
	{
		if(!strcmp(format+i, "EBCDIC") || !strcmp(format+i, "EBC"))
			frm->tagFormat=FRM_EBCDIC;
		else if(!strcmp(format+i, "BCD"))
			frm->tagFormat=FRM_BCD;
		else if(!strcmp(format+i, "HEX"))
			frm->tagFormat=FRM_HEX;
		else if(!strcmp(format+i, "ASCII") || !strcmp(format+i, "ASC") || format[i]=='\0')
			frm->tagFormat=FRM_ASCII;
		else
		{
			if(debug)
				printf("Error: Unrecognized TLV tag format (%s)\n", format+i+4);
			return 0;
		}
	}

	//printf("Field: %s, Length type: %d, LengthLength: %d, Max Length: %d, data format: %d\n", frm->description, frm->lengthFormat, frm->lengthLength, frm->maxLength, frm->dataFormat);

	return 1;
}

