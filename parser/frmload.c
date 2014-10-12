#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

typedef struct link
{
	char name[256];
	fldformat *frm;
} link;

fldformat *findFrmParent(link**, int*, char*, int*, fldformat *frm=NULL);
//fldformat *newFrmChild(fldformat*, unsigned int);
//void removeFrmChild(fldformat*, unsigned int);
int parseFormat(fldformat*, char*, link**, int*);
int linkFrmChild(fldformat*, unsigned int, fldformat*, link*);
int findLinkNumber(link**, int*, const char*, int maxlen=-1, fldformat *frm=NULL);

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
	link *links=NULL;
	int ln=0;

	if(!frmroot)
		findLinkNumber(&links, &ln, (char*)"message");
	else
	{
		for(; frmroot->altformat!=NULL; )
			frmroot=frmroot->altformat;

		frmroot->altformat=(fldformat*)calloc(1, sizeof(fldformat));
		altfrmpar=&(frmroot->altformat);
		frmroot->altformat->lengthFormat=FRM_UNKNOWN;
		frmroot->altformat->maxLength=1024;
		frmroot->altformat->dataFormat=FRM_SUBFIELDS;
		frmroot->altformat->maxFields=196;
		findLinkNumber(&links, &ln, (char *)"message", -1, frmroot->altformat);
	}

	if((file=fopen(filename, "r"))==NULL)
	{
		if(debug)
			perror("Error: Unable to open file\n");
		freeFormat(links[0].frm);
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

		frmpar=findFrmParent(&links, &ln, number, &j);
		if(!frmpar)
		{
			if(j<0)
			{
				if(debug)
					printf("Error: Unable to parse field number, skipping:\n%s\n", line);
				continue;
			}
		}

		frmtmp=(fldformat*)calloc(1, sizeof(fldformat));

		if (!parseFormat(frmtmp, format, &links, &ln))
		{
			if(debug)
				printf("Error: Unable to parse format, skipping:\n%s\n", line);
			freeFormat(frmtmp);
			continue;
		}

		if(frmpar)
		{
			frmtmp->description=(char*)malloc(strlen(number)+2+strlen(descr)+1);
			strcpy(frmtmp->description, number);
			strcpy(frmtmp->description+strlen(frmtmp->description), ". ");
			strcpy(frmtmp->description+strlen(frmtmp->description), descr);
		}
		else
		{
			frmtmp->description=(char*)malloc(strlen(descr)+1);
			strcpy(frmtmp->description, descr);
		}

		if(!linkFrmChild(frmpar, j, frmtmp, links))
		{
			if(debug)
				printf("Error: Unable to add format, skipping:\n%s\n", line);
			freeFormat(frmtmp);
			continue;
		}

		k++;
	}
	fclose(file);

	for(i=1; i<ln; i++)
	{
		freeFormat(links[i].frm);
	}

	frmtmp=links[0].frm;

	free(links);

	if(!frmtmp->description)
	{
		printf("Warning: No 'message' format, implying default\n");
		frmtmp->description=(char*)malloc(strlen("No description")+1);
		strcpy(frmtmp->description, "No description");
	}

	if(debug)
		printf("Info: Loaded %d fields\n", k);

	if(k==1)
	{
		if(debug)
			printf("Error: No fields loaded\n");
		freeFormat(frmtmp);
		if(altfrmpar)
			*altfrmpar=NULL;
		return NULL;
	}

	return frmtmp;
}

int findLinkNumber(link **links, int *ln, const char *name, int maxlen, fldformat *frm)
{
	int i=0;

	if(maxlen<0)
		maxlen=strlen(name);

	if(maxlen)
	{
		for(; i<*ln; i++)
			if(!strncmp((*links)[i].name, name, maxlen))
				break;
	}

	if(i==*ln)
	{
		*links=(link*)realloc(*links, (*ln+1)*sizeof(link));
		*ln=*ln+1;
		strncpy((*links)[i].name, name, maxlen);
		if(!frm)
		{
			frm=(fldformat*)calloc(1, sizeof(fldformat));
			frm->lengthFormat=FRM_UNKNOWN;
			frm->maxLength=1024;
			frm->dataFormat=FRM_SUBFIELDS;
			frm->maxFields=196;
		}
		(*links)[i].frm=frm;
	}

	return i;
}

fldformat *findFrmParent(link **links, int *ln, char *number, int *position, fldformat *frm)
{
	unsigned int i;
	unsigned int l;
	fldformat *altformat;
	
	if((!frm && !links) || (frm && links))
	{
		if(debug)
			printf("Error: Either one of frm and links must be provided\n");
		*position=-1;
		return NULL;
	}

	if(!ln)
	{
		if(debug)
			printf("Error: Null pointer to 2nd arg\n");
		*position=-1;
		return NULL;
	}

	if(!number)
	{
		if(debug)
			printf("Error: Null pointer to 3rd arg\n");
		*position=-1;
		return NULL;
	}

	if(!position)
        {
		if(debug)
                	printf("Error: Null pointer to 4th arg\n");
		*position=-1;
                return NULL;
        }

	l=strlen(number);

	for(i=0; i<l; i++)
		if((number[i]<'0' || number[i]>'9') && number[i]!='*')
			break;

	if(i==0)
	{
		for(; i<l; i++)
			if(number[i]=='.')
				break;
		if(!links)
		{
			*position=i;
			return NULL;
		}

		*position=findLinkNumber(links, ln, number, i);

		if(i==l || i+1==l)
			return NULL;

		for(altformat=(*links)[*position].frm; altformat->altformat!=NULL; )
			 altformat=altformat->altformat;

		altformat=findFrmParent(NULL, ln, number+i+1, position, altformat);
		if(altformat)
			return altformat;

		*position=findLinkNumber(links, ln, number, *position+i+1);
		return NULL;
	}

	if(links)
		frm=(*links)[findLinkNumber(links, ln, number, 0)].frm;

	if(i==l)
	{
		if(number[i-1]=='*')
			*position=0;
		else	
			*position=atoi(number);

		for(altformat=frm; altformat->altformat!=NULL; )
			altformat=altformat->altformat;

		return altformat;
	}

	if(number[i]!='.')
	{
		for(; i<l; i++)
			if(number[i]=='.')
				break;
		if(links)
			*position=findLinkNumber(links, ln, number, i);
		else
			*position=i;
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
			printf("Warning: Parent format not loaded yet [%s][%d]\n", number, *position);

		if(links)
			*position=findLinkNumber(links, ln, number, *position+i);
		else
			*position=*position+i;
		return NULL;
	}

	for(altformat=frm->fld[*position]; altformat->altformat!=NULL; )
		 altformat=altformat->altformat;

	altformat=findFrmParent(NULL, ln, number+i+1, position, altformat);
	if(altformat)
		return altformat;

	if(links)
		*position=findLinkNumber(links, ln, number, *position+i+1);
	else
		*position=*position+i+1;
	return NULL;
}

int linkFrmChild(fldformat *frm, unsigned int n, fldformat *cld, link *links)
{
	fldformat *altformat;

	if(!frm && !links)
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

	if(!frm)
	{
		if(links[n].frm->description) //already loaded, adding as altformat
		{
			for(altformat=links[n].frm; altformat->altformat!=NULL; )
				 altformat=altformat->altformat;
			altformat->altformat=cld;
		}
		else	//replacing format fields of the link
		{
			if(cld->altformat || cld->fields || cld->fld)
			{
				printf("Error: The child has grown up\n");
				return 0;
			}

			cld->fields=links[n].frm->fields;
			cld->fld=links[n].frm->fld;
			cld->altformat=links[n].frm->altformat;

			mirrorFormat(links[n].frm, cld);
			cld->data=NULL; //transfer pointer ownership to link
			cld->description=NULL;
			free(cld);
		}
		return 1;
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

int parseFormat(fldformat *frm, char *format, link **links, int *ln)
{
	int i, j=0;
	char tmpc;
	char *p;
	fldformat *tmpfrm;

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

		case 'R':
			tmpfrm=findFrmParent(links, ln, format+1, &j);
			if(!tmpfrm)
				copyFormat(frm, (*links)[j].frm);
			else
			{
				if(tmpfrm->fields<j)
				{
					printf("Error: Unable to find referenced format (%s)\n", format+1);
					return 0;
				}
				copyFormat(frm, tmpfrm->fld[j]);
			}

			if(frm->description)
			{
				free(frm->description);
				frm->description=NULL;
			}

			return 1;

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

	if(!frm->maxLength)
		return 1; //allow zero length fields

	p=strstr(format+i, "=");
	if(p)
	{
		if(p[1])
		{
			frm->data=(char *)malloc((strlen(p+1)+1)*sizeof(char));
			strcpy(frm->data, p+1);
		}
		else
		{
			frm->data=(char *)malloc(2*sizeof(char));
			strcpy(frm->data, " "); //replacing empty string with a space character. It is a feature, not bug.
		}
		*p='\0';
	}

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
		if(p)
			*p='=';
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
			if(p)
				*p='=';
			return 0;
		}
	}

	if(p)
		*p='=';

	//printf("Field: %s, Length type: %d, LengthLength: %d, Max Length: %d, Data format: %d, Mandatory data: %s\n", frm->description, frm->lengthFormat, frm->lengthLength, frm->maxLength, frm->dataFormat, frm->data);

	return 1;
}

