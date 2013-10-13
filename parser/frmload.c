#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

fldformat *findFrmParent(fldformat*, char*, int*);
fldformat *newFrmChild(fldformat*, unsigned int);
void removeFrmChild(fldformat*, unsigned int);
int parseFormat(fldformat*, char*);

fldformat* load_format(char *filename)
{
	char line[256];
	char number[sizeof(line)];
	char format[sizeof(line)];
	char descr[sizeof(line)];
	char chr;
	int i=0;
	int j=0;
	int k=0;
	fldformat *frmroot, *frmtmp, *frmpar;
	FILE *file;

	frmroot=calloc(1, sizeof(fldformat));
	
	if((file=fopen(filename, "r"))==NULL)
	{
		printf("Error: Unable to open file\n");
		freeFormat(frmroot);
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
			printf("Warning: Wrong format in line, skipping:\n%s\n", line);
			continue;
		}

		//printf("Number[%s] format[%s] descr[%s]\n", number, format, descr);

		if(k==0 && !strcmp(number, "message"))
		{
			frmroot->description=malloc(strlen(descr)+1);
			strcpy(frmroot->description, descr);

			parseFormat(frmroot, format);

			k++;
			continue;
		}

		if(k==0)
		{
			printf("Warning: No 'message' format, implying default\n");
			frmroot->description=malloc(strlen("ISO8583 Message")+1);
			strcpy(frmroot->description, "ISO8583 Message");
			
			parseFormat(frmroot, "U1024SF");
			
			k++;
		}

		frmpar=findFrmParent(frmroot, number, &j);
		if(!frmpar)
		{
			printf("Error: Unable to parse field number, skipping:\n%s\n", line);
			continue;
		}

		frmtmp=newFrmChild(frmpar, j);
		if(!frmtmp)
		{
			printf("Error: Unable to add format, skipping:\n%s\n", line);
			continue;
		}

		frmtmp->description=malloc(strlen(number)+2+strlen(descr)+1);
		strcpy(frmtmp->description, number);
		strcpy(frmtmp->description+strlen(frmtmp->description), ". ");
		strcpy(frmtmp->description+strlen(frmtmp->description), descr);

		if (!parseFormat(frmtmp, format))
		{
			printf("Error: Unable to parse format, skipping:\n%s\n", line);
			removeFrmChild(frmpar, j);
			continue;
		}

		k++;
	}
	fclose(file);

	printf("Info: Loaded %d fields\n", k);

	if(k==1)
	{
		printf("Error: No fields loaded\n");
		freeFormat(frmroot);
		return NULL;
	}

	return frmroot;
}

fldformat *findFrmParent(fldformat *frm, char *number, int *position)
{
	unsigned int i;
	unsigned int l=strlen(number);
	
	if(!frm)
	{
		printf("Error: Null pointer to 1st arg\n");
		return NULL;
	}

	if(!number)
	{
		printf("Error: Null pointer to 2nd arg\n");
		return NULL;
	}

	if(!position)
        {
                printf("Error: Null pointer to 3rd arg\n");
                return NULL;
        }

	for(i=0; i<l; i++)
		if((number[i]<'0' || number[i]>'9') && number[i]!='*')
			break;
	if(i==0)
	{
		printf("Error: Empty field number (%s)\n", number);
		return NULL;
	}
	if(i==l)
	{
		if(number[i-1]=='*')
			*position=0;
		else	
			*position=atoi(number);
		
		return frm;
	}

	if(number[i]!='.')
	{
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
		printf("Error: Parent format not loaded yet [%s][%d]\n", number, *position);
		return NULL;
	}

	return findFrmParent(frm->fld[*position], number+i+1, position);
}

fldformat *newFrmChild(fldformat *frm, unsigned int n)
{
	if(!frm)
	{
		printf("Error: Null pointer to first arg\n");
		return NULL;
	}

	if(frm->fields > n)
	{
		printf("Error: Format already exists\n");
		return NULL;
	}

	if(n+1 > frm->maxFields)
	{
		printf("Error: Exceeded max number of fields (required %d, max %d)\n", n, frm->maxFields);
		return NULL;
	}

	if(frm->fields==0)
		frm->fld=calloc(frm->maxFields, sizeof(fldformat*));
	
	frm->fields=n+1;

	frm->fld[n]=calloc(1, sizeof(fldformat));

	return frm->fld[n];
}

void removeFrmChild(fldformat *frm, unsigned int n)
{
	unsigned int i;

	if(!frm)
	{
		printf("Error: Null pointer to first arg\n");
		return;
	}

	if(frm->fields < n + 1)
	{
		printf("Warning: No such field in parent format\n");
		return;
	}

	freeFormat(frm->fld[n]);
	
	frm->fld[n]=NULL;

	if(frm->fields == n + 1)
	{
		for(i=frm->fields-1; i!=0; i--)
			if(frm->fld[i]!=NULL)
				break;
	
		if(i==0 && frm->fld[0]==0)
		{
			frm->fields=0;
			free(frm->fld);
		}
		else
			frm->fields=i+1;
	}
}

int parseFormat(fldformat *frm, char *format)
{
	int i, j=0;
	char tmpc;

	if(!frm)
	{
		printf("Error: Null pointer to first arg\n");
		return 0;
	}

	if(!format)
	{
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
			printf("Error: Unrecognized TLV tag format (%s)\n", format+i+4);
			return 0;
		}
	}

	//printf("Length type: %d, LengthLength: %d, Max Length: %d, data format: %d\n", frm->lengthFormat, frm->lengthLength, frm->maxLength, frm->dataFormat);

	return 1;
}

void freeFormat(fldformat *frm)
{
	unsigned int i;

	if(!frm)
	{
		printf("Warning: already freed\n");
		return;
	}

	if(frm->fields!=0)
	{
		for(i=0; i<frm->fields; i++)
			if(frm->fld[i]!=NULL)
				freeFormat(frm->fld[i]);
		free(frm->fld);
	}

	if(frm->description!=NULL)
		free(frm->description);

	free(frm);
}

